# Linux x86_64 HZ6 Remote Free Status Dispatch, 2026-06-19

## Box

`RemoteFreeStatusDispatch-L1`

This box routes transfer-based remote frees through the status-returning commit
boundary in the callers that matter for the selected preload lane.  It is
enabled in the selected Ubuntu preload flags with:

```text
HZ6_REMOTE_FREE_STATUS_DISPATCH_L1=1
```

Scope:

- Toy
- MidPage
- Local2P

Large fronts still use the existing bool fallback.

## Integrity Smoke

Command:

```bash
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Result: passed.

Key counters:

```text
remote_free_status_dispatch_transfer=34757
remote_free_status_committed=3227
remote_free_status_backpressure=31530
remote_free_status_stale=0
remote_free_status_integrity_failure=0
transfer_reserve_full_after_state_mutation=0
```

Interpretation: the selected transfer-based foreign remote frees now reach the
caller as status-classified outcomes instead of being collapsed to bool before
policy can inspect them.

## Quick Perf Guard

Command:

```bash
RUNS=3 ./hakozuna-hz6/linux/run_hz6_preload_remote_median.sh
```

Result:

```text
remote50  14,969,255.96 ops/s
remote90   6,625,844.88 ops/s
```

This is a guard for the dispatch boundary, not a new selected RUNS=10 claim.

## Decision

`GO(tooling)`.

The next policy box can now handle `BACKPRESSURE` explicitly in the selected
Toy/MidPage/Local2P remote-free path without reverse-engineering it from a bool
failure.
