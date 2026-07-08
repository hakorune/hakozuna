# HZ11 Transfer Promotion Matrix L1

Status: gate script added and default gate measured.
Verdict: GO for `libhz11_span_transfer.so` as the recommended HZ11 speed lane
candidate. This is not a default allocator decision.

## Purpose

Decide whether `libhz11_span_transfer.so` is a real HZ11 span-lane promotion
candidate rather than a one-off remote-row win.

This is a promotion gate only. A GO result may make `libhz11_span_transfer.so`
the recommended HZ11 speed lane, but it does not make HZ11 the default allocator.

## Script

```bash
RUNS=10 THREADS=16 ITERS=100000 \
  ./hakozuna-hz11/scripts/run_hz11_transfer_promotion_matrix.sh
```

The script builds `libhz11_span_soa.so`, `libhz11_span_transfer.so`, and
`bench/out/bench_matrix_malloc` unless `--skip-build` is passed.
It also accepts optional candidate aliases such as
`hz11-thread-exit-cap-batch32` and
`hz11-thread-exit-cap-batch32-fine128`,
`hz11-thread-exit-cap-batch32-fine256`, and
`hz11-thread-exit-cap-batch32-fineclass` for tradeoff runs.

Output goes under:

```text
bench_results/hz11_transfer_promotion_<timestamp>/
  README.log
  samples.csv
  summary.md
  *_run*_*.log
```

## Rows

```text
main_local0
main_r50
main_r90
small_remote90
medium_r50
medium_r90
```

## Allocators

```text
tcmalloc
hz11-span-soa
hz11-span-transfer
```

## Reported Fields

```text
median ops/s
p25 ops/s
p75 ops/s
post RSS
peak RSS
HZ11 transfer counters:
  xfer_hit
  xfer_miss
  xfer_insert
  xfer_spill
  central_hit
  central_miss
  central_insert
  refill_xfer
  refill_central
  refill_span
  span_create
```

## GO Conditions

```text
fixed/local hit path no regression:
  main_local0 transfer/span-soa >= 0.97

main remote rows clearly improve over span-soa:
  main_r50 transfer/span-soa >= 1.10
  main_r90 transfer/span-soa >= 1.10

tcmalloc parity on main remote rows:
  main_r50 transfer/tcmalloc >= 1.00
  main_r90 transfer/tcmalloc >= 1.00

RSS guard:
  transfer post RSS <= tcmalloc * 1.25 on every matrix row

transfer cache is actually used:
  main_r50/main_r90 aggregate xfer_hit > 0
  main_r50/main_r90 aggregate xfer_insert > 0
```

## NO-GO Conditions

```text
medium/small remote throughput regresses materially:
  small_remote90, medium_r50, medium_r90 transfer/span-soa < 0.90

RSS gets materially worse:
  transfer post RSS > tcmalloc * 1.25 or span-soa * 1.25

integrity failure:
  central stack overflow abort
  benchmark aborts

transfer cache is not used:
  xfer_hit == 0 or xfer_insert == 0 on main remote rows
```

## Next Step After GO

```text
Recommended speed lane:
  libhz11_span_transfer.so

Not yet:
  default allocator

Next boxes:
  transfer cap/batch size tuning
  macro workload gate
```

## Default Gate Result

Run:

```bash
RUNS=10 THREADS=16 ITERS=100000 \
TCMALLOC_SO=/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
  ./hakozuna-hz11/scripts/run_hz11_transfer_promotion_matrix.sh
```

Output:

```text
bench_results/hz11_transfer_promotion_20260707T220243Z/
```

Conditions:

```text
RUNS=10
THREADS=16
ITERS=100000
LIVE_WINDOW=1024
allocators=tcmalloc,hz11-span-soa,hz11-span-transfer
```

Summary:

| Row | tcmalloc ops/s | span-soa ops/s | span-transfer ops/s | transfer/tcmalloc | transfer/span-soa | transfer post RSS |
|---|---:|---:|---:|---:|---:|---:|
| main_local0 | 608.60M | 509.88M | 498.88M | 0.820 | 0.978 | 1.88MiB |
| main_r50 | 44.68M | 21.22M | 83.95M | 1.879 | 3.956 | 29.81MiB |
| main_r90 | 24.29M | 10.70M | 59.24M | 2.439 | 5.536 | 35.50MiB |
| small_remote90 | 57.99M | 12.29M | 65.39M | 1.128 | 5.320 | 7.25MiB |
| medium_r50 | 15.52M | 22.76M | 76.74M | 4.945 | 3.372 | 45.44MiB |
| medium_r90 | 8.56M | 11.17M | 53.80M | 6.285 | 4.815 | 53.56MiB |

Gate result:

```text
Verdict: GO
all checks PASS
```

Important counters:

```text
main remote aggregate:
  xfer_hit=445486
  xfer_insert=7168688

small/medium remote rows also exercised the transfer cache:
  small_remote90 xfer_hit=388644, xfer_insert=6241520
  medium_r50     xfer_hit=123061, xfer_insert=1982448
  medium_r90     xfer_hit=324444, xfer_insert=5208064
```

Interpretation:

```text
The transfer lane is no longer a one-row artifact.
It wins every remote/mixed row in this matrix and keeps post RSS below tcmalloc.
The only non-win is main_local0, where transfer is intentionally not the lever;
the gate allows this as long as transfer/span-soa >= 0.97, and the measured
ratio was 0.978.
```
