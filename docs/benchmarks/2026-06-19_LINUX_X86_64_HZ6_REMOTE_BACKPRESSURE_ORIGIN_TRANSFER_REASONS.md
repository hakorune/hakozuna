# Linux x86_64 HZ6 Remote Backpressure Origin Transfer Reasons, 2026-06-19

## Box

`RemoteFreeBackpressureOriginTransferReasonObserve-L1`

This is a behavior-neutral observe box for the selected
`RemoteFreeBackpressureOriginTransfer-L1` lane. It splits origin-transfer
non-commits into:

```text
remote_free_backpressure_origin_transfer_stride_skip
remote_free_backpressure_origin_transfer_validation_fail
remote_free_backpressure_origin_transfer_full
```

The existing aggregate success/fail counters remain in place.

## Integrity Smoke

Command:

```bash
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Result: passed.

Key selected counters:

```text
remote_free_foreign_candidate=34665
remote_free_status_committed=7059
remote_free_status_backpressure=43902
transfer_reserve_success=7059
transfer_reserve_full=43902
remote_free_returned_backpressure=27606
remote_free_returned_uncommitted=0
remote_free_backpressure_origin_transfer_success=4985
remote_free_backpressure_origin_transfer_fail=11311
remote_free_backpressure_origin_transfer_stride_skip=16295
remote_free_backpressure_origin_transfer_validation_fail=0
remote_free_backpressure_origin_transfer_full=11311
```

The split shows that origin-transfer attempts are not failing ownership
validation. Every attempted non-commit in this smoke is origin transfer-cache
full.

## RUNS=3 Guard

Command:

```bash
RUNS=3 ./hakozuna-hz6/linux/run_hz6_preload_remote_median.sh
```

Result:

```text
remote50  14,823,669.53 ops/s
remote90   1,689,959.38 ops/s
```

`remote50` stayed aligned with the selected origin-transfer RUNS=10 result.
`remote90` fell sharply in this short guard run, so the current selected
origin-transfer lane remains useful but not closed. Treat the next behavior box
as origin transfer-cache saturation or stride/capacity policy work, not
ownership-validation work.

## Decision

`GO(tooling)`.

Keep the counters. They identify the current high-remote gap as transfer-cache
backpressure after the destination cache and origin fallback cache are both
full.
