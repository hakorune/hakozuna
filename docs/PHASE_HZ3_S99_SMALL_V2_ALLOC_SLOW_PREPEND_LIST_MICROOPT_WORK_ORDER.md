# PHASE HZ3 S99: small_v2_alloc_slow RefillPrependList MicroOptBox — Work Order

目的:
- `hz3_small_v2_alloc_slow()` の “refill したバッチの残り” を TLS bin へ押し戻す処理で、
  per-object push ループを減らして固定費を削る。
- r90/r50/dist のいずれも退行させない（±1% 以内）範囲で改善を狙う（scale lane 既定候補）。

背景（観測）:
- `perf annotate` で `hz3_small_v2_alloc_slow` 内の “push back loop” が目立つ。
- 現状は `for (i=1..got-1) hz3_binref_push(...)` で head 更新 + count 更新を繰り返している。

箱の境界（1箇所）:
- `hakozuna/hz3/src/hz3_small_v2.c` の `hz3_small_v2_alloc_slow()` 内、refill 後の “残りを TLS bin に戻す” 部分。

方針（意味は同じ）:
- pop されたオブジェクト列は “元の intrusive list の順序” を保持しているので、
  tail の next を 1 回書き換えるだけで `head..tail` を丸ごと prepend できる。
- SoA: `hz3_binref_prepend_list(ref, head, tail, n)`
- 非SoA: `hz3_small_bin_prepend_list(bin, head, tail, n)`

フラグ（compile-time only, default OFF）:
- `HZ3_S99_ALLOC_SLOW_PREPEND_LIST=0/1`

実装場所:
- config: `hakozuna/hz3/include/hz3_config_scale.h`
- code: `hakozuna/hz3/src/hz3_small_v2.c`

---

## A/B 手順（推奨）

ビルド:
- baseline: scale 既定
- treat: `-DHZ3_S99_ALLOC_SLOW_PREPEND_LIST=1`

スイート（10–20 runs、interleave）:
- `RUNS=10 ./hakozuna/hz3/scripts/run_ab_s99_alloc_slow_prepend_list_10runs_suite.sh`

GO/NO-GO:
- GO: r50 が ±1% 以内で、r90 か dist が改善（または R=0 が改善）
- NO-GO: r50 が -1% 超の退行、または dist が -1% 超の退行

---

## 結果（2026-01-14）

20runs, interleaved, warmup:
- mt_remote_r50_small: **+4.63%**
- mt_remote_r90_small: **+8.29%**
- mt_remote_r0_small: **-0.92%（±1%以内）**
- dist_app: **-0.44%（±1%以内）**
- dist_uniform: **-0.90%（±1%以内）**
- malloc_flood_32t: **+0.20%**

判定:
- **GO**（ただし S112 が scale lane 既定になったため、scale 既定では “条件付き”）

補足（S112 との互換）:
- `HZ3_S112_FULL_DRAIN_EXCHANGE=1` のときは、`spill_array` により batch が “linked list” ではないため、
  `HZ3_S99_ALLOC_SLOW_PREPEND_LIST` は **自動で 0**（`hakozuna/hz3/include/hz3_config_scale.h` でガード）。
  - S99 を使う場合は `HZ3_S112_FULL_DRAIN_EXCHANGE=0` の lane/条件で A/B する。
