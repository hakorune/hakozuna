# Paper-Aligned Windows Redis Workload

Generated: 2026-03-09 14:04:18 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L86)
- [private/hakmem/benchmarks/redis/workload_bench_fixed.c](/C:/git/hakozuna-win/private/hakmem/benchmarks/redis/workload_bench_fixed.c)

Windows native note:
- benchmark: `bench_redis_workload_compare`
- params: `threads=4 cycles=500 ops=2000 size=16..256`
- runs: `5`
- statistic: `median M ops/sec` per pattern
- note: paper originally compares `hz3` and `tcmalloc`; this runner also records `crt`, `hz4`, and `mimalloc`

## SET

| allocator | median M ops/sec | runs |
| --- | ---: | --- |
| crt | 41.37 | `40.12, 44.98, 43.49, 34.38, 41.37` |
| hz3 | 81.69 | `81.69, 80.63, 81.84, 82.18, 81.60` |
| hz4 | 74.81 | `75.91, 76.05, 74.81, 69.48, 69.68` |
| mimalloc | 73.26 | `77.73, 60.71, 72.85, 75.39, 73.26` |
| tcmalloc | 77.75 | `69.65, 76.23, 77.75, 77.88, 80.93` |

## GET

| allocator | median M ops/sec | runs |
| --- | ---: | --- |
| crt | 268.47 | `273.68, 234.84, 306.59, 268.47, 234.76` |
| hz3 | 269.59 | `266.16, 284.23, 326.43, 269.55, 269.59` |
| hz4 | 309.41 | `320.33, 242.88, 274.65, 309.41, 325.66` |
| mimalloc | 329.92 | `340.55, 329.92, 317.43, 330.96, 292.68` |
| tcmalloc | 327.58 | `299.65, 323.76, 327.58, 329.53, 328.52` |

## LPUSH

| allocator | median M ops/sec | runs |
| --- | ---: | --- |
| crt | 46.44 | `43.40, 46.44, 46.71, 48.97, 40.94` |
| hz3 | 75.50 | `60.71, 75.50, 82.36, 59.60, 82.58` |
| hz4 | 71.62 | `59.55, 55.70, 75.37, 71.62, 73.58` |
| mimalloc | 73.42 | `76.47, 73.42, 70.20, 72.42, 75.10` |
| tcmalloc | 76.59 | `76.22, 77.56, 76.59, 77.40, 74.64` |

## LPOP

| allocator | median M ops/sec | runs |
| --- | ---: | --- |
| crt | 1,316.92 | `1,383.27, 1,186.73, 1,173.36, 1,316.92, 1,366.21` |
| hz3 | 1,103.57 | `950.53, 1,045.40, 1,103.57, 1,214.14, 1,203.95` |
| hz4 | 1,068.66 | `1,040.64, 1,109.79, 726.88, 1,418.64, 1,068.66` |
| mimalloc | 1,013.12 | `952.52, 1,108.56, 857.38, 1,013.12, 1,337.57` |
| tcmalloc | 1,036.30 | `1,084.04, 1,036.30, 881.66, 1,170.00, 1,018.98` |

## RANDOM

| allocator | median M ops/sec | runs |
| --- | ---: | --- |
| crt | 125.52 | `128.60, 117.50, 138.14, 125.52, 117.90` |
| hz3 | 141.57 | `131.59, 174.74, 137.37, 143.15, 141.57` |
| hz4 | 141.57 | `139.10, 141.57, 135.12, 144.78, 144.75` |
| mimalloc | 170.69 | `151.53, 173.76, 168.25, 170.69, 171.62` |
| tcmalloc | 163.92 | `166.94, 154.87, 163.92, 159.74, 170.37` |

Artifacts: [out_win_redis](/C:/git/hakozuna-win/out_win_redis)

