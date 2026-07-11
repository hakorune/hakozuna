# HZ8 / HZ11 / HZ12 Linux Allocator Comparison (2026-07-11)

## Conditions

- benchmark: `win/bench_allocator_compare.c` portable pthread path
- 8 threads, 500,000 operations per thread
- working set: 1,024 pointers per thread
- random sizes: 16 through 4,096 bytes
- fresh process, repeat-5; table reports medians
- peak RSS: `/usr/bin/time -v` maximum resident set
- all rows completed with zero allocation failures

## Generic Variable-Size Churn

| Allocator | Median ops/s | Median peak RSS |
|---|---:|---:|
| HZ8 public preload | 4.032 M | 4,924.38 MiB |
| HZ11 fine128 transfer | 148.039 M | 22.75 MiB |
| HZ12 core | 37.447 M | 28.62 MiB |
| mimalloc 2.0.5 | 22.960 M | 30.75 MiB |
| tcmalloc 2.9.1 minimal | 268.342 M | 30.88 MiB |

This shape keeps worker owners alive while repeatedly allocating uniformly
distributed 16–4,096 byte objects. It is an adversarial peak-retention probe
for HZ8's medium path, not the HZ8 public balanced-RSS matrix.

## HZ8 Public RSS Audit

The established HZ8 same-run harness was rerun on the same host with 8
threads, 50,000 iterations, and repeat-3. HZ8 reports RSS after worker join and
owner-exit cleanup as well as peak RSS.

| HZ8 row | Median ops/s | Median peak RSS | Median post RSS |
|---|---:|---:|---:|
| small interleaved remote90 | 11.296 M | 23.38 MiB | 2.05 MiB |
| main interleaved remote90 | 4.702 M | 61.12 MiB | 3.54 MiB |
| medium interleaved remote50 | 7.279 M | 33.25 MiB | 3.02 MiB |
| main local0 | 104.273 M | 2.88 MiB | 2.75 MiB |

The low post-workload RSS claim is reproduced. In the three remote-heavy
rows, HZ8 post RSS is lower than mimalloc and tcmalloc in the same run matrix.

## Interpretation

HZ12 exceeds mimalloc in this bounded random-mixed shape while keeping peak
RSS slightly lower. HZ11 and tcmalloc remain substantially faster. This is a
common malloc/free throughput comparison; retirement latency and reclaim
authority are measured separately in the HZ12 turnover gate.

The 4.9 GiB HZ8 peak is real for the generic long-lived-owner churn shape, but
it is not caused by the allocator's 64 GiB virtual arena reservation becoming
resident. An attribution run reported only 256 KiB committed in the small
arena and about 1.3 million minor faults. Size slices locate the growth above
the 128-byte boundary: empty medium backing is retained while the worker owner
remains alive, then the public lifecycle harness recovers RSS at owner exit.

Therefore the corrected reading is: HZ8 remains strong at post-workload RSS,
while long-lived mixed-size medium churn exposes a separate peak-RSS weakness.

Raw logs are kept under
`private/raw-results/linux/hz12_common_20260711/` in the local workspace.
