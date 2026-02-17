# PHASE S236-O: Medium Slow Refill List Work Order

Date: 2026-02-17
Status: **NO-GO**

## Goal

Reduce fixed cost in `hz3_alloc_slow()` by batching remaining objects push into single `prepend_list` operation.

**Current:** Loop pushes `got-1` objects one by one via `hz3_bin_push()`
**Optimization:** Single `hz3_bin_prepend_list()` call for all remainders

## Constraints

- Do NOT modify hz3_free / remote dispatch
- Only change consumer boundary (alloc_slow)
- Default OFF, opt-in via knob
- Preserve LIFO order (reverse linking)
- Support both SoA and non-SoA modes
- Maintain debug/watch checks

## Implementation

### 1. Config Knob

File: `hakozuna/hz3/include/config/hz3_config_scale_part8_modern.inc`

```c
// S236-O: Medium Slow Refill List (batch remainder prepend)
#ifndef HZ3_S236O_MEDIUM_SLOW_REFILL_LIST
#define HZ3_S236O_MEDIUM_SLOW_REFILL_LIST 0
#endif
```

### 2. S203 Counters

File: `hakozuna/hz3/src/hz3_tcache.c`

- `s236o_remainder_calls`
- `s236o_remainder_objs`

### 3. Helper Function

File: `hakozuna/hz3/src/hz3_tcache_slowpath.inc`

- `hz3_s236o_prepend_batch_remainder_bin()` for Hz3Bin*
- `hz3_s236o_prepend_batch_remainder_binref()` for Hz3BinRef

### 4. Replace Loops

Target locations:
- Line ~252 (mini-refill central hit)
- Line ~488 (S236N bulk refill)
- Line ~586 (S229 central-first)
- Line ~749 (main central hit)

## A/B Testing

**Base:** Current scale default (S236-N ON)
**Var:** Base + `-DHZ3_S236O_MEDIUM_SLOW_REFILL_LIST=1`

**Lanes (RUNS=10, interleaved, pinned 0-15):**
- main: `16 2000000 400 16 32768 90 65536`
- guard: `16 2000000 400 16 2048 90 65536`
- larson_proxy: `4 2500000 400 4096 32768 0 65536`

**Gates:**
- main >= +2.5%
- guard >= -3.0%
- larson_proxy >= -3.0%
- hard-kill: alloc_slow_calls <= +5%

**If pass:** Replay with RUNS=21

## NO-GO Conditions

- Screen gate fail
- guard/larson significantly degraded
- Counter shows mechanism active but no performance gain

If NO-GO: Archive to `hakozuna/hz3/archive/research/s236o_medium_slow_refill_list/`

---

## Execution Result (2026-02-17, RUNS=10)

Run artifacts: `/tmp/s236o_ab_20260217_154826/`

### Benchmark Results

| Lane | BASE (ops/s) | VAR (ops/s) | Change | Gate | Pass |
|------|--------------|-------------|--------|------|------|
| main | 58,871,212 | 58,822,347 | **-0.08%** | >= +2.5% | **FAIL** |
| guard | 98,659,886 | 96,246,678 | -2.44% | >= -3.0% | PASS |
| larson | 148,326,526 | 148,524,070 | +0.13% | >= -3.0% | PASS |

### Decision: S236-O NO-GO

**Reason:** Main gate failed (-0.08% change vs required +2.5%)

**Analysis:**
- The batch prepend optimization had essentially no effect on main lane performance
- The overhead of the reverse linking loop likely offsets any benefit from single prepend_list call
- Guard and larson lanes within acceptable bounds (no significant regression)

**Conclusion:** The optimization does not provide measurable benefit for the main lane. The individual push operations are already well-optimized or the batch remainder case is not frequent enough to matter.
