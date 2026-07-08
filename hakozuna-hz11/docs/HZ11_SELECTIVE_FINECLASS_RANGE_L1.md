# HZ11SelectiveFineclassRange-L1

Status: GO for `fine128` as an opt-in candidate only. No macro promotion.

`fine256` and global fineclass remain useful attribution points, but they are
not the selected next general candidate.

## Box

Keep the sh6bench RSS win from finer size classes while avoiding the
remote/mixed regressions caused by the global fineclass map.

Boundary:

```text
size-class policy only
candidate sibling only
no default promotion
```

## Design

The previous global fineclass lane used 16-byte steps through 1024 bytes, then
powers of two:

```text
global fineclass:
  16, 32, 48, ..., 1024
  2048, 4096, ..., 65536
```

This box keeps that lane intact and adds selective prefix variants:

```text
fine128:
  16, 32, 48, ..., 128
  256, 512, ..., 65536

fine256:
  16, 32, 48, ..., 256
  512, 1024, ..., 65536
```

Implementation:

```text
HZ11_FINE_SIZE_CLASSES=1
HZ11_FINE_LINEAR_MAX={128,256,512,1024}
```

The existing `HZ11_FINE_SIZE_CLASSES=1` behavior remains `1024`.

New opt-in siblings:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128.so
libhz11_span_transfer_thread_exit_cap_batch32_fine128_live_diag.so
libhz11_span_transfer_thread_exit_cap_batch32_fine256.so
libhz11_span_transfer_thread_exit_cap_batch32_fine256_live_diag.so
```

## Sh6bench

Run:

```bash
RUNS=3 hakozuna-hz11/scripts/run_hz11_sh6bench_size_class_policy_candidate.sh
```

Output:

```text
bench_results/hz11_sh6bench_size_class_policy_20260708T081029Z/summary.md
```

Medians:

| Allocator | wall sec | max RSS KiB | RSS/tcmalloc | span_create |
|---|---:|---:|---:|---:|
| tcmalloc | 0.358 | 261120 | 1.000 | 0 |
| batch32 | 3.574 | 352000 | 1.348 | 16759 |
| fine128 | 3.523 | 322688 | 1.236 | 15510 |
| fine256 | 3.514 | 315264 | 1.207 | 15290 |
| global fineclass | 3.603 | 301312 | 1.154 | 14633 |

Request-to-slot waste:

| Allocator | slot bytes high | waste bytes | waste ratio |
|---|---:|---:|---:|
| batch32 live-diag | 359339904 | 148231845 | 41.3% |
| fine128 live-diag | 330470256 | 119302668 | 36.1% |
| fine256 live-diag | 323023648 | 111644881 | 34.6% |
| global fineclass live-diag | 306814496 | 95468881 | 31.1% |

Interpretation:

```text
fine128 captures enough of the class-packing improvement to bring sh6bench RSS
inside the 1.25x tcmalloc RSS guard in this focused run.

fine256 captures more RSS improvement, but the next checks show more
remote/mixed side effect.
```

## Python Alloc

Run:

```bash
RUNS=10 BUILD=0 RUN_LARSON=0 RUN_XMALLOC=0 RUN_SH6BENCH=0 \
  RUN_CACHE_SCRATCH=0 RUN_PYTHON_ALLOC=1 RUN_MSTRESS=0 \
  hakozuna-hz11/scripts/run_hz11_macro_speed_lane_gate.sh \
    --allocators tcmalloc,hz11-thread-exit-cap-batch32,hz11-thread-exit-cap-batch32-fine128,hz11-thread-exit-cap-batch32-fine256,hz11-thread-exit-cap-batch32-fineclass \
    --candidate hz11-thread-exit-cap-batch32-fine256 \
    --skip-span-soa-check --runs 10
```

Output:

```text
bench_results/hz11_macro_speed_lane_20260708T081413Z/summary.md
```

Medians:

| Allocator | wall sec | max RSS KiB | current RSS KiB |
|---|---:|---:|---:|
| tcmalloc | 0.033 | 8320 | 1792 |
| batch32 | 0.033 | 6464 | 2816 |
| fine128 | 0.037 | 6336 | 2752 |
| fine256 | 0.042 | 6464 | 3072 |
| global fineclass | 0.042 | 6912 | 2432 |

The tiny-denominator current-RSS artifact from
`HZ11FineclassPythonAllocCurrentRSS-L1` still applies. The useful signal here is
that fine128 keeps max RSS below batch32 and avoids the larger wall movement
seen in fine256/global fineclass.

## Remote/Mixed

Run:

```bash
RUNS=5 THREADS=16 ITERS=100000 \
  hakozuna-hz11/scripts/run_hz11_transfer_promotion_matrix.sh \
    --allocators tcmalloc,hz11-span-transfer,hz11-thread-exit-cap-batch32,hz11-thread-exit-cap-batch32-fine128,hz11-thread-exit-cap-batch32-fine256,hz11-thread-exit-cap-batch32-fineclass
```

Output:

```text
bench_results/hz11_transfer_promotion_20260708T081413Z/summary.md
```

Fine128 versus span-transfer:

| Row | ops ratio | post RSS ratio | span_create ratio |
|---|---:|---:|---:|
| main_local0 | 0.819 | 1.200 | 1.814 |
| main_r50 | 1.009 | 1.140 | 1.277 |
| main_r90 | 0.890 | 1.178 | 1.223 |
| small_remote90 | 1.018 | 0.831 | 1.034 |
| medium_r50 | 0.972 | 1.048 | 0.963 |
| medium_r90 | 0.918 | 1.187 | 1.255 |

Fine256 versus span-transfer:

| Row | ops ratio | post RSS ratio | span_create ratio |
|---|---:|---:|---:|
| main_local0 | 0.661 | 2.067 | 2.116 |
| main_r50 | 0.975 | 1.115 | 1.356 |
| main_r90 | 0.920 | 1.286 | 1.303 |
| small_remote90 | 1.004 | 0.916 | 1.331 |
| medium_r50 | 0.884 | 1.239 | 1.243 |
| medium_r90 | 0.958 | 1.101 | 1.162 |

Global fineclass versus span-transfer:

| Row | ops ratio | post RSS ratio | span_create ratio |
|---|---:|---:|---:|
| main_local0 | 0.601 | 3.800 | 4.105 |
| main_r50 | 0.943 | 1.269 | 1.812 |
| main_r90 | 0.921 | 1.239 | 1.567 |
| small_remote90 | 0.985 | 1.976 | 3.522 |
| medium_r50 | 0.931 | 1.225 | 1.081 |
| medium_r90 | 0.957 | 1.039 | 1.035 |

Interpretation:

```text
fine128 removes the worst global-fineclass remote/mixed RSS expansion,
especially small_remote90.

fine256 preserves more sh6bench RSS, but it reintroduces larger main_local0 and
medium_r50 side effects.

fine128 still has side effects in this RUNS=5 matrix, especially main_local0
throughput and medium_r90. It is a candidate for the next gate, not a promotion.
```

## Fixed Sanity

Output:

```text
bench_results/hz11_selective_fineclass_fixed_20260708T081439Z/summary.md
```

Medians:

| slot | batch32 | fine128 | fine256 | global fineclass |
|---:|---:|---:|---:|---:|
| 64 | 167.06M | 166.52M | 167.68M | 166.75M |
| 256 | 163.36M | 166.05M | 165.82M | 167.12M |

Fixed64/fixed256 sanity does not show a blocking local fixed-size regression.

## Decision

Select:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128.so
```

as the next opt-in selective fineclass candidate.

Do not select `fine256` as the next general candidate. It improves sh6bench RSS
more than fine128, but its remote/mixed side effects are closer to the rejected
global fineclass lane.

No macro promotion is made here. The next box should run the full macro gate
with fine128 and keep the prior python_alloc current-RSS sampling note in view.
