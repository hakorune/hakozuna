# S28-5: PTAG32 lookup を “hit最適化” する（in_range out-param を hot から外す）

Note: `HZ3_V2_ONLY` は 2026-01-10 の掃除で削除済み（本ドキュメントは履歴用）。

目的:
- S28-2C（local bins split）+ S28-4（LTO=conditional keep）後、tiny で tcmalloc に ~6% 残差が残ることがある。
- perf で `hz3_free` のホットに
  - PTAG32 lookup（range check + tag load）
  - local/remote 分岐（`dst==my_shard`）
  が出ている。
- ここでは “箱を増やさず” に、**PTAG32 lookup の hit path を短く**する。

背景（現状の形）:
- `hz3_free` は `hz3_pagetag32_lookup_fast(ptr, &tag32, &in_range)` を呼び、
  - hit → dst/bin で push
  - miss → `in_range` を見て arena外/内（tag==0）を切る
- しかし hit path でも `*in_range_out = 1` の store が必ず発生する。
  - hit が支配的なワークロードでは、この store は純粋な固定費になりうる。

狙い:
- hit path から `in_range` の書き込みを消す（= hot store を1つ減らす）。
- miss 時だけ、別の range check で arena外/内を判定する（miss は基本レア想定）。

---

## 実装（A/B）

### フラグ

- `HZ3_PTAG32_NOINRANGE=0/1`（default 0）
  - 1 のとき、free hot path は “hit最適化” 版の lookup を使う。

### 変更点（最小）

1) `hz3/include/hz3_arena.h`
- `hz3_pagetag32_lookup_hit_fast(const void* ptr, uint32_t* tag_out)` を追加
  - `in_range_out` を持たない（storeしない）
  - 返り値: 1=hit, 0=miss（arena外/内は区別しない）

2) `hz3/src/hz3_hot.c`（PTAG dst/bin + fastlookup 経路）

before:
```c
uint32_t tag32 = 0;
int in_range = 0;
if (hz3_pagetag32_lookup_fast(ptr, &tag32, &in_range)) { ... return; }
if (!in_range) { ... next_free ...; return; }
... failfast/no-op ...
```

after（NOINRANGE=1）:
```c
uint32_t tag32 = 0;
if (hz3_pagetag32_lookup_hit_fast(ptr, &tag32)) { ... return; }

// miss 時だけ range check（遅くてOK）
if (!hz3_arena_page_index_fast(ptr, NULL)) { ... next_free ...; return; }
... failfast/no-op ...
```

注意:
- `hz3_arena_page_index_fast` は base/end を読むが、miss がレアなら許容。
- `HZ3_V2_ONLY` の “in arena but tag==0” の扱い（failfast/no-op）は現状維持する。

---

## A/B（SSOT）

主評価（dist=app tiny 100% / RUNS=30 推奨）:
```bash
cd hakmem
make -C hakozuna bench_malloc_dist
make -C hakozuna/hz3 clean all_ldpreload \
  HZ3_LDPRELOAD_DEFS="${HZ3_LDPRELOAD_DEFS} -DHZ3_PTAG32_NOINRANGE=1"

SKIP_BUILD=1 RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

回帰ガード:
- dist=app（`BENCH_EXTRA_ARGS="0x12345678 app"`）
- uniform（default）

---

## perf（任意）

狙い: hit path の “store減” が `instructions` / `cycles` に反映されるかを見る。

最低限:
- `instructions,cycles,branches,branch-misses`

---

## GO/NO-GO

GO:
- tiny 100% が +1% 以上改善（RUNS=30 median）
- dist=app / uniform の退行なし（±1%）

NO-GO:
- tiny が動かない（±1%）または回帰
- dist=app / uniform が落ちる

NO-GO の場合:
- `hakmem/hakozuna/hz3/archive/research/s28_5_ptag32_noinrange/README.md` に固定
- `hakmem/hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

---

## 結果（S28-5）

結論:
- tiny 目的では **NO-GO**（ほぼ動かない）
- dist=app / uniform の固定費削減としては **KEEP**（退行なしで +1〜2% 改善）

SSOT（RUNS=30 の例）:
- tiny: +0.07%（ほぼ不変）
- dist=app: +1.32%
- uniform: +1.84%

運用上の決定:
- `hakozuna/hz3/include/hz3_config.h` の安全デフォルトは `HZ3_PTAG32_NOINRANGE=0` のまま維持する。
- ただし hz3 lane（`make -C hakmem/hakozuna/hz3 all_ldpreload`）の既定プロファイルでは
  `hakozuna/hz3/Makefile` の `HZ3_LDPRELOAD_DEFS` に `-DHZ3_PTAG32_NOINRANGE=1` を含めて採用する。
