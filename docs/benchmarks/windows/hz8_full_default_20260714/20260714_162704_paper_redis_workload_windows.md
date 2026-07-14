# Paper-Aligned Windows Redis Workload

Generated: 2026-07-14 16:27:04 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L86)
- [private/hakmem/benchmarks/redis/workload_bench_fixed.c](/C:/git/hakozuna-win/private/hakmem/benchmarks/redis/workload_bench_fixed.c)

Windows native note:
- benchmark: `bench_redis_workload_compare`
- params: `threads=4 cycles=500 ops=2000 size=16..256`
- runs: `1`
- per-run timeout: `60s`
- statistic: `median M ops/sec` per pattern
- note: paper originally compares `hz3` and `tcmalloc`; this runner also records `crt`, `hz4`, and `mimalloc`

## SET

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz3 | 59.79 | 17,920 | `59.79` |
| hz4 | 76.56 | 28,368 | `76.56` |
| hz8 | 56.45 | 16,824 | `56.45` |
| mimalloc | 78.39 | 10,700 | `78.39` |
| tcmalloc | 80.45 | 13,520 | `80.45` |

## GET

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz3 | 250.80 | 17,920 | `250.80` |
| hz4 | 258.17 | 28,368 | `258.17` |
| hz8 | 262.52 | 16,824 | `262.52` |
| mimalloc | 331.18 | 10,700 | `331.18` |
| tcmalloc | 322.07 | 13,520 | `322.07` |

## LPUSH

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz3 | 61.42 | 17,920 | `61.42` |
| hz4 | 74.68 | 28,368 | `74.68` |
| hz8 | 55.90 | 16,824 | `55.90` |
| mimalloc | 65.23 | 10,700 | `65.23` |
| tcmalloc | 76.37 | 13,520 | `76.37` |

## LPOP

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz3 | 914.60 | 17,920 | `914.60` |
| hz4 | 960.64 | 28,368 | `960.64` |
| hz8 | 814.32 | 16,824 | `814.32` |
| mimalloc | 941.40 | 10,700 | `941.40` |
| tcmalloc | 999.23 | 13,520 | `999.23` |

## RANDOM

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz3 | 153.92 | 17,920 | `153.92` |
| hz4 | 139.70 | 28,368 | `139.70` |
| hz8 | 165.93 | 16,824 | `165.93` |
| mimalloc | 171.39 | 10,700 | `171.39` |
| tcmalloc | 172.27 | 13,520 | `172.27` |

## Failures

- none

Artifacts: [out_win_redis](/C:/git/hakozuna-win/out_win_redis)
