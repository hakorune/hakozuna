# Linux x86_64 HZ6 Backpressure Policy Clock, 2026-06-19

## Box

`BackpressurePolicyClock-L1`

This box separates backpressure policy cadence from diagnostic stats counters.
Before this change, `HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_STRIDE` and
`HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_STRIDE` used
`stats.transfer_reserve_attempt` as their clock. That counter is only advanced
when diagnostic probes are enabled, so production and diagnostic builds could
run different policies.

The new policy clock is:

```text
Hz6Allocator.remote_backpressure_policy_epoch
```

It is an allocator-owned relaxed atomic counter used by origin-transfer and
drain stride decisions.

## Selected Policy

The selected preload lane now uses:

```text
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_L1=1
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_STRIDE=1
```

`STRIDE=1` matches the old production behavior implied by the diagnostic-counter
bug. `STRIDE=2` is now a real policy and measured weaker.

## Integrity Smoke

Command:

```bash
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Result: passed.

Key counters from the selected stride-1 smoke:

```text
remote_free_backpressure_origin_transfer_success=7119
remote_free_backpressure_origin_transfer_fail=26894
remote_free_backpressure_origin_transfer_stride_skip=0
remote_free_returned_backpressure=26894
remote_free_returned_uncommitted=0
remote_free_status_integrity_failure=0
```

## RUNS

True `STRIDE=2`, RUNS=10:

```text
remote50  14,090,990.96 ops/s
remote90   2,134,217.85 ops/s
```

True `STRIDE=1`, RUNS=3:

```text
remote50  15,149,047.54 ops/s
remote90   8,788,405.81 ops/s
```

## Decision

`GO(correctness)` for policy-clock separation.

`GO(default)` for selected `ORIGIN_TRANSFER_STRIDE=1`.

Do not compare old diagnostic-smoke stride counts to production medians before
this box. Follow-up work should use the policy clock and move toward
`OwnerStableRemotePending-L1` rather than tuning stats-driven cadence.
