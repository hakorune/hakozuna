# Linux x86_64 HZ6 Preload Malloc Transfer Retry, 2026-06-19

## Box

`PreloadMallocTransferRetry-L1`

Selected baseline remains:

```text
HZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1=0
```

Opt-in candidate:

```text
HZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1=1
```

The selected preload Toy/MidPage malloc fast paths normally skip transfer-first
probing to preserve local hot-path speed. This box keeps that fast path, but
after local frontcache reuse misses it tries one transfer reuse when transfer
backlog is present.

## Integrity Smoke

Command:

```bash
HZ6_EXTRA_CFLAGS='-UHZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1 -DHZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1=1' \
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Result: passed.

Key counters:

```text
preload_malloc_transfer_retry_eligible=3040
preload_malloc_transfer_retry_attempt=3040
preload_malloc_transfer_retry_hit=2690
remote_free_status_committed=6434
remote_free_status_backpressure=40992
remote_free_returned_backpressure=25816
remote_free_returned_uncommitted=0
remote_free_status_integrity_failure=0
transfer_reserve_full=40992
```

The box successfully consumes transfer objects from owner-local malloc and
reduces remote backpressure versus the selected smoke shape, without integrity
failures.

## RUNS=3

Command:

```bash
HZ6_EXTRA_CFLAGS='-UHZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1 -DHZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1=1' \
RUNS=3 ./hakozuna-hz6/linux/run_hz6_preload_remote_median.sh
```

Result:

```text
remote50  14,613,277.36 ops/s
remote90   1,077,952.16 ops/s
```

## Decision

`NO-GO(default)`.

Keep the implementation as an opt-in research box only. It proves that the
preload skip-transfer malloc paths can consume transfer backlog, but the
high-remote row is still too slow. The next selected candidate needs to avoid
turning remote90 into repeated malloc-side transfer activation work.
