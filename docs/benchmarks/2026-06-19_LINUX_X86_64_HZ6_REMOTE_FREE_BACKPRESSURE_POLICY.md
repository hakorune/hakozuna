# Linux x86_64 HZ6 Remote Free Backpressure Policy, 2026-06-19

## Box

`RemoteFreeBackpressurePolicy-L1`

This box separates explicit transfer backpressure from stale or integrity
remote-free failures at the caller policy boundary.

New counters:

```text
remote_free_returned_backpressure
remote_free_returned_stale
remote_free_returned_integrity_failure
```

`remote_free_returned_uncommitted` now represents non-backpressure uncommitted
returns in the selected status-dispatch path.

## Integrity Smoke

Command:

```bash
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Result: passed.

Key counters:

```text
remote_free_status_dispatch_transfer=39715
remote_free_status_backpressure=36383
remote_free_returned_backpressure=36383
remote_free_returned_uncommitted=0
remote_free_returned_stale=0
remote_free_returned_integrity_failure=0
remote_free_status_stale=0
remote_free_status_integrity_failure=0
```

The smoke now zero-gates `remote_free_returned_uncommitted`,
`remote_free_returned_stale`, and `remote_free_returned_integrity_failure`.

## Quick Perf Guard

Command:

```bash
RUNS=3 ./hakozuna-hz6/linux/run_hz6_preload_remote_median.sh
```

Result:

```text
remote50  14,743,023.84 ops/s
remote90   5,656,564.99 ops/s
```

This is a policy/observability box, not a selected performance claim.

## Decision

`GO(tooling)`.

The selected path no longer hides transfer backpressure behind generic
uncommitted or invalid remote-free counters. The next behavior box can target
`remote_free_returned_backpressure` directly.
