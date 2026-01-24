# S143: PGO (scale lane) Results

## Summary
PGO improves r0/r50 but regresses r90. Default adoption is **NO-GO**.

## Build / Profile
- Instrumented build: `-fprofile-generate -fprofile-dir=/tmp/hz3_pgo_prof`
- Profile workloads: r90 + mixed uniform + dist_app
- Optimized build: `-fprofile-use -fprofile-dir=/tmp/hz3_pgo_prof`

## Throughput (RUNS=5, median)

**Corrected measurement (P95条件 / LD_PRELOAD / max_size=2048):**

| Workload | Baseline | PGO | Change | Status |
| --- | --- | --- | --- | --- |
| r0 (0% remote) | 238.94M | 237.39M | -0.65% | OK |
| r50 (50% remote) | 69.13M | 68.92M | -0.30% | OK |
| r90 (90% remote) | 64.85M | 65.32M | +0.74% | **NO-GO** |

Primary criterion: r90 must be +1% or better → **failed**.

Note: earlier measurements used mismatched max_size (32768), which explains the 20–30M range. The table above is the SSOT for S143.

## Artifacts
- PGO binary: `libhakozuna_hz3_scale_pgo.so`
- Baseline binary: `libhakozuna_hz3_scale_baseline_s143.so`
- Measurement script: `hakozuna/scripts/s143_measure_pgo_ab.sh`
- Profile data: `/tmp/hz3_pgo_prof/` (32 `.gcda`)
- Logs: `/tmp/s143_runs5/`
- Analysis: `/tmp/s143_analyze_results.py`

## Interpretation
Mixed-profile PGO likely optimized for r0/r50 patterns at the expense of r90.
Code layout and branch weighting can hurt remote-heavy control flow.

## Next if revisiting
- Re-run r90 with RUNS=20 to confirm (noise check).
- Try workload-specific PGO (separate profiles for r90 only).
- Consider post-link layout tools (BOLT) rather than mixed PGO.
