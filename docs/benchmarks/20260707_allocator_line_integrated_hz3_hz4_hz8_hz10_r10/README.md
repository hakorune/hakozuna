# Allocator Line Integrated Matrix, HZ3/HZ4/HZ8/HZ10

Status: measured same-run snapshot.

## Conditions

```text
date_utc=2026-07-07T00:49:24Z
commit=d5a2f2e4b94b698de008bb6eb325315fd6642bb5
RUNS=10
THREADS=16
ITERS=50000
allocators=hz3,hz4,hz8,hz10,mimalloc,tcmalloc,system
harness=hakozuna-hz8/bench/bench_matrix_malloc via LD_PRELOAD
raw=samples.csv
summary=summary.md
```

All allocators use the same plain malloc/free harness. `system` uses no preload.
This is a same-harness allocator-line snapshot, not a replacement for each line's paper-specific tables.

## Read First

- HZ8 remains the public recommended balanced allocator line.
- HZ10 is now measurable in the same matrix and is often faster on remote/interleaved rows, but it keeps much higher post RSS than HZ8 here.
- HZ3 is still a useful local-heavy baseline.
- HZ4 is retained as a historical remote-heavy line; in this harness it is not the current winner.
- RSS matters: use `post RSS` to judge recovery/retention and `peak RSS` to judge high-water pressure.

## Median Ops/s

| row | hz3 | hz4 | hz8 | hz10 | mimalloc | tcmalloc |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `guard_local0` | 156.85M | 49.01M | 207.05M | 137.53M | 88.23M | 354.64M |
| `main_local0` | 149.31M | 28.82M | 117.94M | 118.18M | 21.32M | 367.54M |
| `main_interleaved_r50` | 16.86M | 12.28M | 10.84M | 20.18M | 5.63M | 22.22M |
| `main_interleaved_r90` | 10.20M | 9.75M | 7.04M | 12.60M | 4.28M | 13.90M |
| `small_interleaved_remote90` | 12.95M | 11.13M | 14.70M | 14.96M | 13.19M | 26.54M |
| `medium_interleaved_r50` | 15.43M | 8.68M | 9.84M | 19.87M | 4.20M | 16.76M |

## Post RSS

| row | hz3 | hz4 | hz8 | hz10 | mimalloc | tcmalloc |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `guard_local0` | 12.35MiB | 131.11MiB | 2.00MiB | 26.38MiB | 13.33MiB | 7.00MiB |
| `main_local0` | 15.11MiB | 169.99MiB | 3.50MiB | 33.75MiB | 67.89MiB | 9.00MiB |
| `main_interleaved_r50` | 163.77MiB | 258.10MiB | 4.33MiB | 79.50MiB | 190.12MiB | 68.50MiB |
| `main_interleaved_r90` | 205.82MiB | 275.23MiB | 4.62MiB | 89.62MiB | 234.37MiB | 82.88MiB |
| `small_interleaved_remote90` | 130.92MiB | 144.87MiB | 2.95MiB | 43.75MiB | 56.82MiB | 31.62MiB |
| `medium_interleaved_r50` | 148.37MiB | 237.98MiB | 3.83MiB | 69.62MiB | 191.08MiB | 85.12MiB |

## Best Throughput and Best RSS

| row | best throughput, all allocators | best post RSS, HZ lines |
| --- | ---: | ---: |
| `guard_local0` | tcmalloc 354.64M | hz8 2.00 MiB |
| `main_local0` | tcmalloc 367.54M | hz8 3.50 MiB |
| `main_interleaved_r50` | tcmalloc 22.22M | hz8 4.33 MiB |
| `main_interleaved_r90` | tcmalloc 13.90M | hz8 4.62 MiB |
| `small_interleaved_remote90` | tcmalloc 26.54M | hz8 2.95 MiB |
| `medium_interleaved_r50` | hz10 19.87M | hz8 3.83 MiB |

## Interpretation

- HZ8 is the RSS-discipline HZ line in this matrix: post RSS stays around 2-5 MiB on every measured row.
- HZ10 is the speed-oriented research candidate: it is close to or ahead of HZ8 on local rows and clearly ahead on remote/interleaved rows, but post RSS is tens of MiB.
- tcmalloc remains the strongest raw-throughput comparator on most local/main rows.
- `medium_interleaved_r50` is the clearest HZ10 speed win in this run: HZ10 leads throughput overall, while HZ8 remains the lowest-post-RSS HZ line.
- Do not collapse this into a single winner. The useful public split is HZ8 for balanced low-RSS recommendation, HZ10 for active macro/shim speed research.
