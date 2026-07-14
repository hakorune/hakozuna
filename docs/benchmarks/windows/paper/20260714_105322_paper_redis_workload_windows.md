# Paper-Aligned Windows Redis Workload

Generated: 2026-07-14 10:53:22 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L86)
- [private/hakmem/benchmarks/redis/workload_bench_fixed.c](/C:/git/hakozuna-win/private/hakmem/benchmarks/redis/workload_bench_fixed.c)

Windows native note:
- benchmark: `bench_redis_workload_compare`
- params: `threads=4 cycles=500 ops=2000 size=16..256`
- runs: `20`
- per-run timeout: `60s`
- statistic: `median M ops/sec` per pattern
- note: paper originally compares `hz3` and `tcmalloc`; this runner also records `crt`, `hz4`, and `mimalloc`

## SET

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz8 | 56.64 | 16,932 | `54.67, 70.88, 69.58, 54.54, 56.75, 56.52, 68.07, 55.81, 61.92, 56.03, 54.43, 66.95, 55.22, 56.09, 72.43, 56.48, 71.52, 55.19, 71.80, 56.77` |
| hz8-small-transition-inventory | 55.88 | 16,718 | `62.37, 68.93, 55.85, 55.34, 55.62, 73.01, 72.29, 55.91, 57.58, 55.66, 55.44, 56.58, 71.90, 55.70, 54.90, 55.50, 55.48, 66.64, 48.38, 56.24` |

## GET

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz8 | 285.37 | 16,932 | `276.42, 317.73, 317.78, 236.86, 264.48, 331.29, 313.06, 239.38, 274.12, 277.44, 265.23, 299.07, 255.46, 322.47, 317.50, 244.16, 304.79, 320.16, 293.30, 246.05` |
| hz8-small-transition-inventory | 298.69 | 16,718 | `297.38, 300.22, 286.56, 302.46, 275.09, 314.42, 316.74, 294.94, 241.78, 320.66, 299.11, 261.79, 240.86, 232.23, 318.61, 327.54, 310.47, 296.97, 306.49, 298.28` |

## LPUSH

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz8 | 60.83 | 16,932 | `66.75, 56.71, 58.96, 65.78, 54.90, 55.30, 55.04, 56.04, 68.16, 56.16, 62.70, 64.25, 66.42, 56.89, 67.50, 68.09, 64.77, 55.62, 65.90, 54.76` |
| hz8-small-transition-inventory | 66.33 | 16,718 | `66.92, 66.25, 56.52, 57.66, 57.23, 69.11, 68.79, 69.00, 69.28, 56.19, 56.00, 54.03, 69.54, 66.41, 55.65, 57.16, 68.25, 68.41, 56.40, 69.15` |

## LPOP

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz8 | 921.87 | 16,932 | `1,122.49, 1,036.00, 1,184.59, 851.77, 901.51, 895.98, 883.47, 822.52, 907.65, 966.42, 831.32, 1,137.07, 936.09, 996.44, 975.75, 641.85, 866.29, 1,025.40, 1,016.98, 900.05` |
| hz8-small-transition-inventory | 1,013.70 | 16,718 | `1,229.07, 937.78, 1,002.76, 954.36, 1,064.06, 843.99, 1,024.64, 1,174.85, 858.33, 1,192.68, 1,234.03, 967.14, 974.37, 911.91, 785.30, 1,051.80, 1,093.70, 1,067.58, 993.54, 1,752.93` |

## RANDOM

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz8 | 157.82 | 16,932 | `155.34, 168.10, 160.45, 159.18, 165.68, 157.75, 138.33, 165.52, 167.71, 155.95, 127.91, 160.48, 138.92, 136.22, 133.43, 138.44, 157.90, 161.64, 162.76, 154.89` |
| hz8-small-transition-inventory | 158.34 | 16,718 | `116.06, 137.97, 163.17, 164.95, 134.64, 161.82, 156.34, 160.49, 133.24, 135.56, 133.99, 140.49, 163.17, 160.53, 164.02, 133.39, 159.20, 167.15, 162.28, 157.49` |

## Failures

- none

Artifacts: [out_win_redis](/C:/git/hakozuna-win/out_win_redis)
