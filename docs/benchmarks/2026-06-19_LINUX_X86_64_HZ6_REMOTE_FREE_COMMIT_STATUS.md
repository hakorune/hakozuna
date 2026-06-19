# Linux x86_64 HZ6 Remote Free Commit Status, 2026-06-19

## Box

`RemoteFreeCommitStatus-L1`

This is a behavior-neutral boundary box. It adds
`hz6_allocator_remote_free_active_descriptor_status()` and keeps the existing
`hz6_allocator_remote_free_active_descriptor()` bool API as a wrapper.

The status boundary is:

```text
COMMITTED
BACKPRESSURE
STALE
INTEGRITY_FAILURE
```

Current front dispatch behavior is unchanged. The point of this box is to stop
treating all false returns as the same condition before the next policy change.

## Integrity Smoke

Command:

```bash
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Result: passed.

Key counters:

```text
remote_free_status_committed=3404
remote_free_status_backpressure=36311
remote_free_status_stale=0
remote_free_status_integrity_failure=0
transfer_reserve_full=36311
remote_free_returned_uncommitted=36311
transfer_reserve_full_after_state_mutation=0
```

Interpretation: the uncommitted remote free returns in the selected smoke are
classified as transfer backpressure, not stale route/descriptors or integrity
failure. `remote_free_status_integrity_failure` is now an integrity smoke
zero-gate.

## Quick Perf Guard

Command:

```bash
RUNS=3 ./hakozuna-hz6/linux/run_hz6_preload_remote_median.sh
```

Result:

```text
remote50  14,214,014.66 ops/s
remote90   6,654,842.21 ops/s
```

This is a guard for the diagnostic boundary, not a new selected performance
claim.

## Decision

`GO(tooling)`.

Use this status API for the next remote-free policy box. The next behavior
change should handle `BACKPRESSURE` explicitly instead of silently merging it
with stale or invalid remote-free outcomes.
