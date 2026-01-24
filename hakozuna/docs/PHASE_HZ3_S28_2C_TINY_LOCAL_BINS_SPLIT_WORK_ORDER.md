# S28-2C: tiny hot path を削る（local bins を bank から分離する）

目的:
- S28-2A（bank row cache）が NO-GO だったため、次は **hot の work を削る**方向で tiny（16–256）を詰める。
- 現状（`HZ3_PTAG_DSTBIN_ENABLE=1`）は local alloc/free も `t_hz3_cache.bank[my_shard][bin]` を使うため、
  - TLS 内の深いオフセット参照
  - `my_shard` 依存の行オフセット計算
  が tiny で重くなりやすい。

狙い:
- local だけは `t_hz3_cache.small_bins[]` / `t_hz3_cache.bins[]` を使い、**浅いTLSオフセット**に寄せる。
- remote は従来通り `bank[dst][bin]`（outbox箱）に積む。

重要（箱理論）:
- hot で増えるのは `dst == my_shard` の 1比較のみ（STでは常にtrueで予測が当たりやすい）。
- remote bank（dst!=my_shard）は **従来通り owner 側へ転送**する（inbox/central へ push）。
  - local bins 分離は「local の置き場所」を変えるだけで、remote の流れは変えない。
- A/B できるように compile-time フラグで完全に戻せること。

---

## 変更方針（A/B）

### フラグ

- `HZ3_LOCAL_BINS_SPLIT=0/1`（default 0）

### 実装の形

1) `hz3_tcache_get_small_bin()` / `hz3_tcache_get_bin()` を A/B で切り替える
- `HZ3_LOCAL_BINS_SPLIT=1` のとき:
  - small: `t_hz3_cache.small_bins[sc]`
  - medium: `t_hz3_cache.bins[sc]`
- `HZ3_LOCAL_BINS_SPLIT=0` のとき:
  - 既存通り `bank[my_shard][bin]`

理由:
- `hz3_small_v2_alloc()` / `hz3_alloc_slow()` の hot 側が getter を通るので、ここを差し替えるのが最小。
- bank row cache（S28-2A）が NO-GO だったため、「余計なTLS load無しで浅いオフセットへ寄せる」のが狙い。

2) `hz3_free(ptr)`（PTAG dst/bin direct 経路）で local を local bins に push する

`hz3_free(ptr)`（PTAG dst/bin direct 経路）:
- `dst == t_hz3_cache.my_shard` のとき
  - small bin index → `t_hz3_cache.small_bins[bin]` に push
  - medium bin index → `t_hz3_cache.bins[sc]` に push
- それ以外（remote）のとき
  - 従来通り `t_hz3_cache.bank[dst][bin]` に push

3) remote bank flush は変更しない（event-only / owner転送のまま）
- `hz3_dstbin_flush_remote_budget()` / `hz3_dstbin_flush_remote_all()` は今のまま:
  - `dst != my_shard` の bins だけ detach し、owner 側の inbox/central に転送する。
  - `dst == my_shard` 行は引き続き scan 対象外（skip）で OK。

---

## 変更箇所（目安）

- `hakmem/hakozuna/hz3/include/hz3_config.h`
  - `HZ3_LOCAL_BINS_SPLIT` を追加
- `hakmem/hakozuna/hz3/src/hz3_hot.c`
  - `hz3_malloc` / `hz3_free` の local/remote 分岐を追加（A/B）
- `hakmem/hakozuna/hz3/src/hz3_tcache.c`
  - 変更不要（remote bank flush は owner転送のまま）

---

## A/B（SSOT）

主評価（dist=app tiny 100%）:
```bash
cd hakmem
make -C hakozuna bench_malloc_dist

SKIP_BUILD=0 RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0" \
  MIMALLOC_SO=./mimalloc-install/lib/libmimalloc.so \
  TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

回帰ガード:
- `dist=app`（`BENCH_EXTRA_ARGS="0x12345678 app"`）
- uniform SSOT（default bench）

---

## GO/NO-GO

GO:
- tiny（16–256）が改善（目安 +3%）し、`dist=app` / uniform で退行なし（±1%）
- `HZ3_DSTBIN_REMOTE_HINT_ENABLE=1` の効果が保持される（ST remote=0 で固定費が増えない）

NO-GO:
- tiny が改善しない、または medium/mixed が落ちる場合は撤退。

---

## 成果物

- 結果は `hakmem/hakozuna/hz3/docs/PHASE_HZ3_S28_TINY_GAP_WORK_ORDER.md` に追記（ログパス付き）。
- NO-GO の場合は `hakmem/hakozuna/hz3/archive/research/s28_2c_local_bins_split/README.md` に固定し、
  `hakmem/hakozuna/hz3/docs/NO_GO_SUMMARY.md` を更新する。

---

## 実測結果（2026-01-02）

判定: **GO**
- tiny が +2.9%（目標 +3% に “ほぼ到達”）
- dist=app / uniform も改善し、退行なし

### SSOTログ

- tiny 100%（trimix 16–256）:
  - baseline: `/tmp/s28_2c_baseline_r10.txt`（median `57.46M` ops/s）
  - split=1: `/tmp/s28_2c_split1_r10.txt`（median `59.12M` ops/s）
- dist=app（16–32768）:
  - baseline: `/tmp/s28_2c_distapp_full_base.txt`（median `50.23M` ops/s）
  - split=1: `/tmp/s28_2c_distapp_full_split1.txt`（median `52.55M` ops/s）
- uniform（16–32768）:
  - baseline: `/tmp/s28_2c_uniform_base.txt`（median `61.17M` ops/s）
  - split=1: `/tmp/s28_2c_uniform_split1.txt`（median `63.64M` ops/s）

### 最終状態

- `hakmem/hakozuna/hz3/include/hz3_config.h` のデフォルトは安全側のまま（`HZ3_LOCAL_BINS_SPLIT=0`）。
- `hakmem/hakozuna/hz3/Makefile` の `HZ3_LDPRELOAD_DEFS` では **SSOT到達点として `-DHZ3_LOCAL_BINS_SPLIT=1` を既定化**。
