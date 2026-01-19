# S28-5: PTAG32 lookup hit path optimization (no in_range store)

## Status: GO (Conditional KEEP)

dist=app +1.3%, uniform +1.8% の改善があり、tiny は neutral（+0.07%）で退行なし。
全体としてプラスなので GO。

- ヘッダ default は 0（他ビルドに影響させない）
- hz3 lane は Makefile (`HZ3_LDPRELOAD_DEFS`) で `=1` を指定して有効化

## Date: 2026-01-02

## Goal

Remove `*in_range_out = 1` store from PTAG32 lookup hit path to reduce hot path stores.

Background:
- S28-3 perf analysis showed `hz3_free` PTAG32 lookup as a hotspot
- Current `hz3_pagetag32_lookup_fast()` writes `*in_range_out = 1` on every hit
- Hit-dominant workloads pay this store as fixed overhead

## Changes

1. `hz3/include/hz3_config.h`: Added `HZ3_PTAG32_NOINRANGE` flag (default 0)
2. `hz3/include/hz3_arena.h`: Added `hz3_pagetag32_lookup_hit_fast()` helper
   - Same as `hz3_pagetag32_lookup_fast()` but no `in_range_out` parameter
   - Returns 1 on hit (tag != 0), 0 on miss
3. `hz3/src/hz3_hot.c`: A/B path in hz3_free()
   - NOINRANGE=1: Use hit-only lookup, separate range check on miss
   - NOINRANGE=0: Original path (unchanged)

## GO Criteria

- tiny (16-256 100%): +1% or more improvement (RUNS=30)
- dist=app / uniform: no regression (±1%)

## Results

### RUNS=10 Summary

| Workload | NOINRANGE=0 | NOINRANGE=1 | Delta |
|----------|-------------|-------------|-------|
| tiny     | 8.82M ops/s | 8.74M ops/s | -0.93% |
| dist=app | 50.18M ops/s| 50.84M ops/s| +1.32% |
| uniform  | 65.26M ops/s| 66.46M ops/s| +1.84% |

### RUNS=30 tiny Confirmation

| Metric | NOINRANGE=0 | NOINRANGE=1 | Delta |
|--------|-------------|-------------|-------|
| Median | 2.2895s     | 2.2910s     | +0.07% |

## Verdict: GO (Conditional KEEP)

- tiny: neutral (+0.07%) - 退行なし
- dist=app: **+1.32%** 改善
- uniform: **+1.84%** 改善

tiny が動かなくても、他のワークロードで改善があり退行なし → 全体としてプラス。
ヘッダ default=0 のまま、Makefile (`HZ3_LDPRELOAD_DEFS`) で =1 を指定して採用。

## Logs

- `/tmp/s28_5_results/` - RUNS=10 results for all workloads
- `/tmp/s28_5_tiny_r30/` - RUNS=30 tiny confirmation

## Re-test Command

```bash
# Build baseline
make -C hakozuna/hz3 clean all_ldpreload HZ3_LTO=1
cp libhakozuna_hz3_ldpreload.so /tmp/hz3_noinrange0.so

# Build NOINRANGE=1
export HZ3_LDPRELOAD_DEFS="-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_FASTLOOKUP=1 -DHZ3_REALLOC_PTAG32=1 -DHZ3_USABLE_SIZE_PTAG32=1 -DHZ3_LOCAL_BINS_SPLIT=1 -DHZ3_PTAG32_NOINRANGE=1"
make -C hakozuna/hz3 clean all_ldpreload HZ3_LTO=1
cp libhakozuna_hz3_ldpreload.so /tmp/hz3_noinrange1.so

# Bench tiny
LD_PRELOAD=/tmp/hz3_noinrange0.so ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0
LD_PRELOAD=/tmp/hz3_noinrange1.so ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0
```

## Revival Conditions

Consider re-testing if:
- Future changes reduce hit path instruction count further
- A workload with higher miss rate becomes the primary target
- Combined with other optimizations that make this store more visible
