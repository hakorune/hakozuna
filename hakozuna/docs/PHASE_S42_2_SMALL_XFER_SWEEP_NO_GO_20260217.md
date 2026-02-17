# PHASE S42-2: SmallTransferCache slots sweep (NO-GO)

Date: 2026-02-17
Status: executed (NO-GO)

## Goal

- Re-check `HZ3_S42_SMALL_XFER` with a small, safe knob sweep.
- Find a lane-wide pass point for:
  - `main >= +3.0%`
  - `guard >= -3.0%`
  - `larson_proxy >= -3.0%`

## Setup

- Interleaved A/B, `RUNS=10`, pinned `0-15`
- Base defs:
  - `-DHZ3_S203_COUNTERS=1 -DHZ3_S44_4_STATS=1`
- Variants:
  - `v1_xfer64`: `-DHZ3_S42_SMALL_XFER=1 -DHZ3_S42_SMALL_XFER_SLOTS=64`
  - `v2_xfer32`: `-DHZ3_S42_SMALL_XFER=1 -DHZ3_S42_SMALL_XFER_SLOTS=32`
  - `v3_xfer128`: `-DHZ3_S42_SMALL_XFER=1 -DHZ3_S42_SMALL_XFER_SLOTS=128`

Run artifacts:
- `/tmp/s42_sweep_safe_20260217_125548/summary.tsv`
- `/tmp/s42_sweep_safe_20260217_125548/raw.tsv`
- `/tmp/s42_sweep_safe_20260217_125548/counter_main_delta.tsv`

## Results (median)

| config | main | guard | larson_proxy | verdict |
|---|---:|---:|---:|---|
| v1_xfer64 | -0.50% | -1.63% | -0.24% | FAIL |
| v2_xfer32 | +0.95% | +2.26% | -0.33% | FAIL |
| v3_xfer128 | +1.11% | -1.77% | +0.41% | FAIL |

## Notes

- Hard-kill (`alloc_slow_calls > +5%`) was not triggered.
- `main` best value is `+1.11%` (`v3_xfer128`), below promotion gate `+3.0%`.
- This sweep does not justify changing current default.

## Decision

- **NO-GO** for S42-2 parameter-only tuning.
- Keep current S42 default.
- Do not continue micro-sweeps on this line without new boundary-level mechanism.
