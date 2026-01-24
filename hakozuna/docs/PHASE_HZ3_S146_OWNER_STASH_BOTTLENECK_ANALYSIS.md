# S146: OwnerStash T=32/R=90 Bottleneck Analysis Results

## Date: 2026-01-19

## Background

perf結果より、T=32/R=90で `owner_stash` が42%のCPU時間を占有:
- `hz3_owner_stash_pop_batch`: 25.73%
- `hz3_owner_stash_push_one`: 16.28%

ring_slots拡張で+18%改善済み（164M→194M）だが、競合の調査を実施。

## Methodology

### Statistics Enabled
- `HZ3_S96_OWNER_STASH_PUSH_STATS=1` - Push CAS retry tracking (added to push_one)
- `HZ3_S44_4_STATS=1` - Pop observation stats
- `HZ3_S67_STATS=1` - Spill stats

### Test Configuration
| Item | Value |
|------|-------|
| ring_slots | 262144 |
| RUNS | 10 |
| ITERS | 2,500,000 |
| Threads | 32 |
| Remote % | 90 |

## Results

### S96 Push Statistics (per run average)

| Metric | Value |
|--------|-------|
| push_calls | ~36,000,000 |
| cas_retry_total | ~730 |
| cas_retry_max | 1 |
| **CAS retry rate** | **0.002%** |

### S44-4 Pop Statistics

| Metric | Value |
|--------|-------|
| pop_calls | ~14,500,000 |
| want_avg | 32.0 |
| drained_avg | 0.0 (S112 mode) |
| spill_hit_pct | 0.1-0.5% |

### Benchmark Stats

| Metric | Value | Note |
|--------|-------|------|
| remote_sent | 35,995,286 | Bench ring buffer sends |
| overflow_sent | 9-13M | Ring overflow fallback to owner_stash |
| throughput (baseline) | ~226M ops/s | RUNS=10 median |

## Analysis

### Key Finding: CAS Contention is NOT the Bottleneck

CAS retry rate of 0.002% is far below the 10% threshold specified in the plan.
This means:
- Push operations succeed on first CAS attempt 99.998% of the time
- The bottleneck is NOT spin-waiting on CAS failures
- The bottleneck is the **volume** of atomic operations, not contention

### Why drained_avg = 0?

S112 (Full Drain Exchange) is enabled by default for scale lane:
- Uses `atomic_exchange` instead of bounded CAS drain
- The drained_sum/drained_max stats are only tracked in bounded drain mode
- Cannot evaluate Step C criteria (`drained_avg > 32`)

### High overflow_sent Analysis

The benchmark's ring buffer (262144 slots) is saturating:
- `overflow_sent` = 9-13M per run
- This is above the <1M target
- Each overflow falls back to hz3's owner_stash push_one
- This creates the 36M push_one calls observed

## S146-B OverflowBatchBox A/B

Conditions: T=32 / R=90, ring_slots=262144, RUNS=10, ITERS=2.5M.

| Config | Median ops/s | vs baseline |
|--------|--------------|-------------|
| baseline | ~226M | - |
| S146-B N=8 | ~205M | -9% |
| S146-B N=16 | ~141M | -38% |

R=0 check:
- N=8: ~-4.5% (noise range)
- N=16: ~+1.6%

Notes:
- The earlier “9x improvement” was caused by an invalid baseline run (~20M ops/s).
- With correct baseline, S146-B is a regression.

## Conclusion

### Plan Criteria Evaluation

| Criterion | Threshold | Actual | Status |
|-----------|-----------|--------|--------|
| CAS retry rate (Step B) | >10% | 0.002% | **NOT MET** |
| drained_avg (Step C) | >32 | 0 (N/A) | **Cannot evaluate** |
| overflow_sent | <1M | 9-13M | **EXCEEDS** |

### Recommendations

1. **Step B (OverflowBatchBox): NO-GO**
   - CAS retry rate is too low to benefit from batching
   - A/B shows regression vs baseline

2. **Step C (Sharding): NOT EVALUABLE**
   - S112 mode doesn't track drained_avg
   - Could try with S112=0 if needed

3. **Alternative Optimization Targets:**
   - Reduce number of push_one calls (batch at caller level)
   - Increase benchmark ring_slots further (beyond 262144)
   - Investigate overflow path optimization

## Code Changes Made

### Added S96 stats to push_one

File: `hakozuna/hz3/src/owner_stash/hz3_owner_stash_push.inc`

Previously, S96 push stats were only tracked in `hz3_owner_stash_push_list`.
Added equivalent tracking to `hz3_owner_stash_push_one` for accurate measurement.

```c
#if HZ3_S96_OWNER_STASH_PUSH_STATS
    S96_STAT_REGISTER();
    S96_STAT_INC(g_s96_push_calls);
    S96_STAT_ADD(g_s96_push_objs_total, 1);
    // ... CAS retry tracking in loop
#endif
```

## Log Files

- `/tmp/mt_r90_t32_stats_v2_20260119_014604.log` - Full 10-run benchmark with S96 stats

## Future Work

If further optimization is needed for owner_stash:
1. Profile with `perf record` to identify exact hot spots within the function
2. Consider S121 (Page-Local Remote) which uses per-page remote lists
3. Investigate caller-side batching to reduce push_one call volume
