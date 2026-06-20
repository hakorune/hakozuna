# Linux x86_64 HZ6 Remote Backpressure Origin Transfer, 2026-06-19

## Box

`RemoteFreeBackpressureOriginTransfer-L1`

Selected flags:

```text
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_L1=1
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_STRIDE=2
```

When the destination transfer cache is full before descriptor mutation, this
box tries to commit the free into the origin allocator's transfer cache without
route rehome. The stride keeps the relief path from doubling every reserve-full
probe.

## Integrity Smoke

Command:

```bash
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Result: passed.

Key selected counters:

```text
remote_free_backpressure_origin_transfer_success=5674
remote_free_backpressure_origin_transfer_fail=10644
remote_free_returned_backpressure=26962
remote_free_returned_uncommitted=0
remote_free_status_integrity_failure=0
transfer_reserve_full_after_state_mutation=0
```

## RUNS=10

Command:

```bash
HZ6_EXTRA_CFLAGS='-DHZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_L1=1 -DHZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_STRIDE=2' \
RUNS=10 ./hakozuna-hz6/linux/run_hz6_preload_remote_median.sh
```

Result:

```text
remote50  14,934,275.07 ops/s
remote90   9,902,231.78 ops/s
```

The same flags are now in the selected Ubuntu preload lane.

## Decision

`GO/default`.

Promote the stride-2 origin-transfer relief box. It preserves the strong
remote50 row and materially improves remote90 versus the previous selected
backpressure-policy lane.
