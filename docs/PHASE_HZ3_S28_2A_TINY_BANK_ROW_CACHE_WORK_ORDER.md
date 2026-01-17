# S28-2A: tiny hot path の依存チェーン短縮（bank row base キャッシュ）

目的:
- S27で主犯と判明した `tiny(16–256)` の hz3↔tcmalloc gap を詰める。
- S28-2 の asm/perf 分析で見えた「`my_shard * HZ3_BIN_TOTAL` を含む bin アドレス計算の依存チェーン」を短縮する。
- hot path を太らせない（追加分岐を増やさない）。“計算を load へ置換して短くする”。

背景（S28-2）:
- `hz3_malloc tiny path` の `&t_hz3_cache.bank[my_shard][bin]` 計算が `lea/shl/add` の連鎖になり、IPC が下がっている。
- `hz3_small_sc_from_size()` の命令数削減は NO-GO（S28-A）。

方針:
- TLS init 時に `bank_my = &t_hz3_cache.bank[t_hz3_cache.my_shard][0]` を 1回だけ計算・保持する。
- `hz3_tcache_get_small_bin(sc)` / `hz3_tcache_get_bin(sc)` は `bank_my[...]` を使ってアドレス計算を単純化する。

重要:
- `hz3_free` の `dst/bin` push は **変更しない**（branch を増やさないため）。
- 変更は `hz3_malloc` tiny path 側（bin取得）に効くのが主目的。

---

## 実装（A/B）

### フラグ

- `HZ3_TCACHE_BANK_ROW_CACHE=0/1`（default 0）
  - 1 のときだけ TLS に `Hz3Bin* bank_my` を追加し、getter を切替える。

### 変更点（最小）

1) `hakozuna/hz3/include/hz3_types.h`
- `#if HZ3_PTAG_DSTBIN_ENABLE && HZ3_TCACHE_BANK_ROW_CACHE`
  - `Hz3Bin* bank_my;` を `Hz3TCache` に追加

2) `hakozuna/hz3/src/hz3_tcache.c`
- `hz3_tcache_ensure_init_slow()` の最後（`my_shard` 決定後、`bank` を memset 後）に
  - `t_hz3_cache.bank_my = &t_hz3_cache.bank[t_hz3_cache.my_shard][0];`

3) `hakozuna/hz3/include/hz3_tcache.h`
- `hz3_tcache_get_small_bin(sc)` / `hz3_tcache_get_bin(sc)` を
  - cache ON: `return &t_hz3_cache.bank_my[hz3_bin_index_*...]`
  - cache OFF: 既存の `bank[my_shard][...]`

---

## A/B（SSOT）

主評価（tiny 100%）:
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
- uniform SSOT（`bench_random_mixed_malloc_args`）

---

## GO/NO-GO

GO:
- tiny が改善（目安 +2% 以上）し、dist=app / uniform で退行なし（±1%）。

NO-GO:
- tiny が改善しない、または small/mixed が落ちる（TLSサイズ増の悪影響が勝つ）場合は撤退。

---

## 成果物

- 結果は `hakozuna/hz3/docs/PHASE_HZ3_S28_TINY_GAP_WORK_ORDER.md` に追記（SSOTログパス付き）。
- NO-GO の場合は `hakozuna/hz3/archive/research/s28_2a_bank_row_cache/README.md` に固定し、`NO_GO_SUMMARY.md` を更新。

