# PHASE S236-L: MiniRefill-K Tune Work Order (No-Code)

Date: 2026-02-17
Status: executed (NO-GO)

## Goal

- Keep S236-A/B architecture unchanged.
- Test only `HZ3_S236_MINIREFILL_K` to reduce `alloc_slow_calls` and improve `main` lane.
- Avoid new hot-path logic or TLS state.

## Context

- S236-K (SC range/source split) ended NO-GO.
- Best-known default remains:
  - `HZ3_S236_SC_MIN=5`
  - `HZ3_S236_SC_MAX=7`
  - `HZ3_S236_MINIREFILL_TRY_INBOX=1`
  - `HZ3_S236_MINIREFILL_TRY_CENTRAL_FAST=1`
  - `HZ3_S236_MINIREFILL_K=8` (current baseline)

## Build Matrix

Base:
```sh
-DHZ3_S203_COUNTERS=1
```

Variants:
```sh
v1_k4  = -DHZ3_S203_COUNTERS=1 -DHZ3_S236_MINIREFILL_K=4
v2_k6  = -DHZ3_S203_COUNTERS=1 -DHZ3_S236_MINIREFILL_K=6
v3_k10 = -DHZ3_S203_COUNTERS=1 -DHZ3_S236_MINIREFILL_K=10
v4_k12 = -DHZ3_S203_COUNTERS=1 -DHZ3_S236_MINIREFILL_K=12
v5_k16 = -DHZ3_S203_COUNTERS=1 -DHZ3_S236_MINIREFILL_K=16
```

## Lanes and Runs

- Interleaved A/B
- `RUNS=10` (screen)
- pinned cores: `0-15`

Lanes:
- `main`: `16 2000000 400 16 32768 90 65536`
- `guard`: `16 2000000 400 16 2048 90 65536`
- `larson_proxy`: `4 2500000 400 4096 32768 0 65536`

## Gates

- `main >= +3.0%`
- `guard >= -3.0%`
- `larson_proxy >= -3.0%`
- hard kill: `alloc_slow_calls` median must not increase by more than `+5%` vs base

## Required Counters (S203)

- `alloc_slow_calls`
- `from_inbox`
- `from_central`
- `from_segment`
- `medium_dispatch_calls`
- `to_mailbox_calls`

## Decision

- If one variant passes all screen gates: run replay (`RUNS=21`) on winner only.
- Replay gates:
  - `main >= +5.0%`
  - `guard >= -4.0%`
  - `larson_proxy >= -4.0%`
  - crash/abort/calloc-failed = 0

- If all variants fail:
  - mark **S236-L NO-GO**
  - freeze S236 micro-tuning line
  - move to non-S236 line (large path or global allocator-level strategy)

---

## Execution Result (2026-02-17, RUNS=10)

Run artifacts:
- `/tmp/s236l_sweep_safe_20260217_113650/summary.tsv`
- `/tmp/s236l_sweep_safe_20260217_113650/raw.tsv`
- `/tmp/s236l_sweep_safe_20260217_113650/counters.tsv`

Summary (median, vs per-pair base):

| config | main | guard | larson_proxy | verdict |
|---|---:|---:|---:|---|
| v1_k4 | -0.62% | +3.92% | -0.21% | FAIL |
| v2_k6 | -1.04% | -3.68% | +0.41% | FAIL |
| v3_k10 | -1.05% | +2.30% | -0.31% | FAIL |
| v4_k12 | +2.90% | +3.37% | -1.73% | FAIL |
| v5_k16 | +2.55% | -5.26% | -1.33% | FAIL |

Decision:
- **S236-L NO-GO**. No variant satisfies `main >= +3.0%` gate.
- v4_k12 (K=12) is closest at +2.90% but still below gate.
- Keep current default `HZ3_S236_MINIREFILL_K=8`.

Observations:
- Smaller K (4, 6) hurts main lane (more alloc_slow_calls from central)
- Larger K (16) hurts guard lane (-5.26%)
- K=12 shows +2.90% on main but not enough for gate

Next steps:
- Freeze S236 micro-tuning line (K, SC range, source split all NO-GO)
- Move to non-S236 design space (large path, global allocator-level strategy)
