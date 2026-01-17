# PHASE_HZ3_S12-4: PageTagMap A/B Results

## Summary

S12-4 PageTagMap implementation achieves significant performance improvement over Task 3 v2 baseline.

**Key Achievement**: small workload regression reduced from -16.3% to -3.2%, medium now exceeds v1 baseline.

## Context

- Git: `0d2b5db85`（work-order doc merge point; code changes for S12-4 are in the same working tree phase）
- Lane: SSOT-HZ3（`hakozuna/hz3/scripts/run_bench_hz3_ssot.sh` 相当の条件）
- Note: `PTAG OFF` は “v2 経路は有効だが PageTagMap を使わない” 比較（`HZ3_SMALL_V2_PTAG_ENABLE=0`）

## A/B Results (RUNS=10, median ops/s)

| Workload | PTAG OFF | PTAG ON | A/B Δ | vs v1 |
|----------|----------|---------|-------|-------|
| small | 68.85M | **78.19M** | **+13.6%** | **-3.2%** |
| medium | 81.53M | **88.47M** | **+8.5%** | **+1.9%** |
| mixed | 69.28M | **75.34M** | **+8.7%** | -9.4% |

## GO/NO-GO Assessment

**Criteria from plan:**
- small: target -7% (was -16%) → actual **-3.2%** ✅
- medium: target ±0% → actual **+1.9%** ✅
- mixed: target ±0% → actual -9.4% ⚠️

**Result: PARTIAL GO**

- small/medium meet criteria
- mixed still has regression but improved by 4.1pp (13.5% → 9.4%)

## What Changed (S12-4 PageTagMap)

### Hot Path (before: used[] + page_hdr->magic)
```
hz3_arena_contains_fast(ptr) → reads used[]
page_hdr->magic check → reads page_hdr
```

### Hot Path (after: range check + 1 load)
```
hz3_arena_page_index_fast(ptr) → range compare only, no used[] read
hz3_pagetag_load(page_idx) → single atomic load from dense array
```

## Implementation Details

- `g_page_tag[]`: 2MB array (4GB arena / 4KB pages × 2 bytes)
- Tag encoding: `(sc+1) | ((owner+1) << 8) | (1 << 15)`
- Event-only writes: set on page allocation, clear on segment free
- Miss path: range check + tag load (0) → immediate fallback

## Files Modified

- `hz3/include/hz3_config.h`: HZ3_SMALL_V2_PTAG_ENABLE flag
- `hz3/include/hz3_arena.h`: hz3_arena_page_index_fast(), pagetag helpers
- `hz3/src/hz3_arena.c`: g_page_tag array, atomic base/end
- `hz3/src/hz3_small_v2.c`: tag set on alloc_page, hz3_small_v2_free_by_tag()
- `hz3/src/hz3_hot.c`: PageTagMap hot path for free/realloc/usable_size

## Build Commands

```bash
# PTAG ON (recommended)
CFLAGS="-O3 -fPIC -Wall -Wextra -Werror -std=c11 \
  -DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
  -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 \
  -DHZ3_SMALL_V2_PTAG_ENABLE=1" \
  make -C hakozuna/hz3 all_ldpreload

# PTAG OFF (comparison)
... -DHZ3_SMALL_V2_PTAG_ENABLE=0 ...
```

## Benchmark Logs

- `/tmp/s12_4_ptag_ab/ptag_on_small.log`
- `/tmp/s12_4_ptag_ab/ptag_on_medium.log`
- `/tmp/s12_4_ptag_ab/ptag_on_mixed.log`
- `/tmp/s12_4_ptag_ab/ptag_off_small.log`
- `/tmp/s12_4_ptag_ab/ptag_off_medium.log`
- `/tmp/s12_4_ptag_ab/ptag_off_mixed.log`

## Next Steps

1. Investigate mixed workload regression (may be v1 fallback path overhead)
2. Consider enabling PTAG as default for v2
3. Profile mixed workload to identify remaining bottlenecks
