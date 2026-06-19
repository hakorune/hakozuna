# Linux x86_64 HZ6 Remote Backpressure Origin Drain, 2026-06-19

## Box

`RemoteFreeBackpressureOriginDrain-L1`

Selected baseline remains:

```text
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_L1=1
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_STRIDE=2
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_DRAIN_L1=0
```

Opt-in candidate:

```text
HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_DRAIN_L1=1
```

When destination transfer reserve fails and origin-transfer fallback also sees
the origin transfer cache as full, this box drains one same-class transfer
object from the origin cache into the origin frontcache and retries the origin
commit once.

## Integrity Smoke

Command:

```bash
HZ6_EXTRA_CFLAGS='-DHZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_DRAIN_L1=1' \
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Result: passed.

Key selected counters:

```text
remote_free_status_committed=21358
remote_free_status_backpressure=48831
transfer_reserve_success=21358
transfer_reserve_full=48831
remote_free_returned_backpressure=18353
remote_free_returned_uncommitted=0
remote_free_backpressure_origin_transfer_success=18286
remote_free_backpressure_origin_transfer_fail=32
remote_free_backpressure_origin_transfer_full=32
remote_free_backpressure_origin_drain_attempt=12192
remote_free_backpressure_origin_drain_success=12160
remote_free_backpressure_origin_drain_retry_success=12160
remote_free_backpressure_drain_attempt=0
remote_free_backpressure_drain_success=0
remote_free_backpressure_requeue_fail=0
remote_free_status_integrity_failure=0
```

The candidate proves that the remaining origin-transfer full path can be
converted into commits, but it does so by adding hot remote-thread drain work.

## RUNS=3

Command:

```bash
HZ6_EXTRA_CFLAGS='-DHZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_DRAIN_L1=1' \
RUNS=3 ./hakozuna-hz6/linux/run_hz6_preload_remote_median.sh
```

Result:

```text
remote50  13,935,320.47 ops/s
remote90   1,287,581.08 ops/s
```

## Decision

`NO-GO(default)`.

Keep the implementation as an opt-in research box only. It reduces origin
transfer full counters, but it adds enough drain/frontcache work on the remote
path to regress both the stable `remote50` row and the already-unstable
`remote90` row.
