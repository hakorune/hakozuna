# HZ11Sh6benchTransferBatchVsCap-L1

## Status

```text
Verdict:
  GO as contribution attribution.
  NO-GO as a macro promotion fix.

Decision:
  HZ11_TRANSFER_BATCH=32 is the sh6bench wall lever.
  HZ11_TRANSFER_CAP=8192 is not the wall lever by itself.
```

## Goal

Separate the sh6bench wall contribution of:

```text
HZ11_TRANSFER_BATCH=32
HZ11_TRANSFER_CAP=8192
```

This box explains the prior xferwide improvement; it is not a final
optimization box.

## Compared Lanes

```text
base:
  libhz11_span_transfer_thread_exit_cap.so
  HZ11_TRANSFER_BATCH=16
  HZ11_TRANSFER_CAP=1024

batch-only:
  libhz11_span_transfer_thread_exit_cap_batch32.so
  HZ11_TRANSFER_BATCH=32
  HZ11_TRANSFER_CAP=1024

cap-only:
  libhz11_span_transfer_thread_exit_cap_cap8192.so
  HZ11_TRANSFER_BATCH=16
  HZ11_TRANSFER_CAP=8192

xferwide:
  libhz11_span_transfer_thread_exit_cap_xferwide.so
  HZ11_TRANSFER_BATCH=32
  HZ11_TRANSFER_CAP=8192
```

## Evidence

```text
Command:
  RUNS=3 hakozuna-hz11/scripts/run_hz11_sh6bench_transfer_batch_vs_cap.sh

Output:
  bench_results/hz11_sh6bench_transfer_batch_vs_cap_20260708T050133Z/summary.md
```

| Allocator | wall sec | wall/tcmalloc | max RSS KiB | current RSS KiB | xfer_insert | xfer_spill | central_insert | central high-water max | span_create |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| tcmalloc | 0.373 | 1.000 | 267520 | 266112 | 0 | 0 | 0 | 0 | 0 |
| hz11-thread-exit-cap | 4.552 | 12.204 | 351744 | 351488 | 502065287 | 25525970 | 25525970 | 3296 | 16695 |
| hz11-thread-exit-cap-batch32 | 3.505 | 9.397 | 350848 | 351232 | 521386264 | 26501252 | 26501252 | 3200 | 16758 |
| hz11-thread-exit-cap-cap8192 | 4.660 | 12.493 | 351616 | 352000 | 527592073 | 0 | 0 | 0 | 16680 |
| hz11-thread-exit-cap-xferwide | 3.602 | 9.657 | 351488 | 351232 | 547886803 | 0 | 0 | 0 | 16761 |

## Interpretation

`HZ11_TRANSFER_BATCH=32` is sufficient to recover almost all of the prior
xferwide wall improvement:

```text
base      4.552s
batch32   3.505s
xferwide  3.602s
```

`HZ11_TRANSFER_CAP=8192` by itself eliminates spill and central insert traffic,
but does not improve wall:

```text
base:
  xfer_spill      25525970
  central_insert  25525970
  wall            4.552s

cap8192:
  xfer_spill      0
  central_insert  0
  wall            4.660s
```

The xferwide result is therefore not proof that central spill elimination is the
main win. The measured wall lever is larger transfer batch/refill granularity.

RSS and span creation do not materially change across HZ11 lanes. The remaining
RSS/page-footprint blocker is outside this box.

## Decision

The next wall-time box should tune transfer batch width or refill granularity,
not transfer capacity/spill policy.

Do not promote `batch32`, `cap8192`, or `xferwide` as macro defaults from this
result. Even the best lane remains about `9.397x` tcmalloc wall and about
`1.311x` max RSS on this sh6bench gate.
