# HZ8 / HZ11 / HZ12 Linux Allocator Comparison (2026-07-11)

## Conditions

- benchmark: `win/bench_allocator_compare.c` portable pthread path
- 8 threads, 500,000 operations per thread
- working set: 1,024 pointers per thread
- random sizes: 16 through 4,096 bytes
- fresh process, repeat-5; table reports medians
- peak RSS: `/usr/bin/time -v` maximum resident set
- all rows completed with zero allocation failures

## Results

| Allocator | Median ops/s | Median peak RSS |
|---|---:|---:|
| HZ8 public preload | 4.032 M | 4,924.38 MiB |
| HZ11 fine128 transfer | 148.039 M | 22.75 MiB |
| HZ12 core | 37.447 M | 28.62 MiB |
| mimalloc 2.0.5 | 22.960 M | 30.75 MiB |
| tcmalloc 2.9.1 minimal | 268.342 M | 30.88 MiB |

## Interpretation

HZ12 exceeds mimalloc in this bounded random-mixed shape while keeping peak
RSS slightly lower. HZ11 and tcmalloc remain substantially faster. This is a
common malloc/free throughput comparison; retirement latency and reclaim
authority are measured separately in the HZ12 turnover gate.

The HZ8 public preload row maps its configured large arena into resident
memory on this host and reports about 4.9 GiB peak RSS. That row is retained as
measured, but it must not be used as evidence for the intended balanced
low-RSS HZ8 positioning until the Linux public profile/lane is audited.

Raw logs are kept under
`private/raw-results/linux/hz12_common_20260711/` in the local workspace.
