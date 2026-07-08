# HZ11Sh6benchTransferBatchGranularity-L1

## Status

```text
Verdict:
  GO as transfer-batch granularity attribution.
  NO-GO as a macro promotion fix.

Decision:
  Pick batch32 as the smallest useful sh6bench wall candidate.
  Do not widen to batch64 or batch128.
```

## Goal

Find the transfer batch/refill granularity that improves sh6bench wall without
increasing RSS or hiding the remaining macro blockers.

This is a candidate-tuning box, not a macro-default promotion box.

## Compared Lanes

```text
batch16:
  libhz11_span_transfer_thread_exit_cap.so
  HZ11_TRANSFER_BATCH=16
  HZ11_TRANSFER_CAP=1024

batch32:
  libhz11_span_transfer_thread_exit_cap_batch32.so
  HZ11_TRANSFER_BATCH=32
  HZ11_TRANSFER_CAP=1024

batch64:
  libhz11_span_transfer_thread_exit_cap_batch64.so
  HZ11_TRANSFER_BATCH=64
  HZ11_TRANSFER_CAP=1024

batch128:
  libhz11_span_transfer_thread_exit_cap_batch128.so
  HZ11_TRANSFER_BATCH=128
  HZ11_TRANSFER_CAP=1024
```

## Evidence

```text
Command:
  RUNS=3 hakozuna-hz11/scripts/run_hz11_sh6bench_transfer_batch_granularity.sh

Output:
  bench_results/hz11_sh6bench_transfer_batch_granularity_20260708T060304Z/summary.md
```

| Allocator | wall sec | wall/tcmalloc | max RSS KiB | current RSS KiB | xfer_insert | xfer_spill | central_insert | central high-water max | span_create |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| tcmalloc | 0.354 | 1.000 | 262016 | 258432 | 0 | 0 | 0 | 0 | 0 |
| batch16 | 4.539 | 12.822 | 352128 | 352000 | 502153255 | 25439525 | 25439525 | 3520 | 16692 |
| batch32 | 3.506 | 9.904 | 350336 | 350592 | 520999790 | 26892960 | 26892960 | 3392 | 16747 |
| batch64 | 5.501 | 15.540 | 352128 | 352128 | 1057071437 | 35262083 | 35262083 | 3456 | 16715 |
| batch128 | 9.312 | 26.305 | 352128 | 352000 | 2096521636 | 51954787 | 51954787 | 4032 | 16720 |

## Interpretation

`batch32` captures the useful wall improvement:

```text
batch16  4.539s
batch32  3.506s
batch64  5.501s
batch128 9.312s
```

Wider batches are not monotonic wins. They reduce refill miss counts, but the
larger returned-object batches sharply increase insert work and spill/central
insert traffic:

```text
xfer_insert:
  batch16   502153255
  batch32   520999790
  batch64  1057071437
  batch128 2096521636

central_insert:
  batch16  25439525
  batch32  26892960
  batch64  35262083
  batch128 51954787
```

RSS and span creation do not materially improve. The page-footprint blocker is
still unresolved.

## Decision

Use batch32 as the smallest useful transfer-batch candidate for any focused
follow-up gate. Do not tune by widening to batch64 or batch128.

Do not promote batch32 to a macro default from this result. It remains about
`9.904x` tcmalloc wall on this sh6bench run, and the full macro gate has not
been rerun.
