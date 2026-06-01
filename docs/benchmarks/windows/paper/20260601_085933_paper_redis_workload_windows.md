# Paper-Aligned Windows Redis Workload

Generated: 2026-06-01 08:59:33 +09:00

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
| crt | 40.19 | 7,836 | `40.19` |
| hz3 | 56.02 | 13,040 | `56.02` |
| hz4 | 55.51 | 27,712 | `55.51` |
| hz5-policy | 42.32 | 10,604 | `42.32` |
| hz6-strict | NaN | NaN | `` |
| hz6-speed | NaN | NaN | `` |
| hz6-rss | NaN | NaN | `` |
| hz6-strict-broad | NaN | NaN | `` |
| hz6-speed-broad | NaN | NaN | `` |
| hz6-rss-broad | NaN | NaN | `` |
| hz6-strict-route4k | NaN | NaN | `` |
| hz6-speed-route4k | NaN | NaN | `` |
| hz6-rss-route4k | NaN | NaN | `` |
| hz6-strict-appcap | 28.11 | 646,780 | `28.11` |
| hz6-speed-appcap | 21.84 | 584,576 | `21.84` |
| hz6-rss-appcap | 11.06 | 597,008 | `11.06` |
| mimalloc | 78.21 | 9,620 | `78.21` |
| tcmalloc | 81.60 | 12,904 | `81.60` |

## GET

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 232.36 | 7,836 | `232.36` |
| hz3 | 272.10 | 13,040 | `272.10` |
| hz4 | 312.90 | 27,712 | `312.90` |
| hz5-policy | 236.67 | 10,604 | `236.67` |
| hz6-strict | NaN | NaN | `` |
| hz6-speed | NaN | NaN | `` |
| hz6-rss | NaN | NaN | `` |
| hz6-strict-broad | NaN | NaN | `` |
| hz6-speed-broad | NaN | NaN | `` |
| hz6-rss-broad | NaN | NaN | `` |
| hz6-strict-route4k | NaN | NaN | `` |
| hz6-speed-route4k | NaN | NaN | `` |
| hz6-rss-route4k | NaN | NaN | `` |
| hz6-strict-appcap | 71.58 | 646,780 | `71.58` |
| hz6-speed-appcap | 42.85 | 584,576 | `42.85` |
| hz6-rss-appcap | 14.67 | 597,008 | `14.67` |
| mimalloc | 331.71 | 9,620 | `331.71` |
| tcmalloc | 325.10 | 12,904 | `325.10` |

## LPUSH

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 44.70 | 7,836 | `44.70` |
| hz3 | 83.89 | 13,040 | `83.89` |
| hz4 | 73.35 | 27,712 | `73.35` |
| hz5-policy | 36.24 | 10,604 | `36.24` |
| hz6-strict | NaN | NaN | `` |
| hz6-speed | NaN | NaN | `` |
| hz6-rss | NaN | NaN | `` |
| hz6-strict-broad | NaN | NaN | `` |
| hz6-speed-broad | NaN | NaN | `` |
| hz6-rss-broad | NaN | NaN | `` |
| hz6-strict-route4k | NaN | NaN | `` |
| hz6-speed-route4k | NaN | NaN | `` |
| hz6-rss-route4k | NaN | NaN | `` |
| hz6-strict-appcap | 21.67 | 646,780 | `21.67` |
| hz6-speed-appcap | 13.98 | 584,576 | `13.98` |
| hz6-rss-appcap | 6.19 | 597,008 | `6.19` |
| mimalloc | 76.11 | 9,620 | `76.11` |
| tcmalloc | 79.26 | 12,904 | `79.26` |

## LPOP

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 1,010.92 | 7,836 | `1,010.92` |
| hz3 | 848.59 | 13,040 | `848.59` |
| hz4 | 758.09 | 27,712 | `758.09` |
| hz5-policy | 1,280.33 | 10,604 | `1,280.33` |
| hz6-strict | NaN | NaN | `` |
| hz6-speed | NaN | NaN | `` |
| hz6-rss | NaN | NaN | `` |
| hz6-strict-broad | NaN | NaN | `` |
| hz6-speed-broad | NaN | NaN | `` |
| hz6-rss-broad | NaN | NaN | `` |
| hz6-strict-route4k | NaN | NaN | `` |
| hz6-speed-route4k | NaN | NaN | `` |
| hz6-rss-route4k | NaN | NaN | `` |
| hz6-strict-appcap | 91.85 | 646,780 | `91.85` |
| hz6-speed-appcap | 45.11 | 584,576 | `45.11` |
| hz6-rss-appcap | 15.34 | 597,008 | `15.34` |
| mimalloc | 1,024.35 | 9,620 | `1,024.35` |
| tcmalloc | 882.85 | 12,904 | `882.85` |

## RANDOM

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 142.32 | 7,836 | `142.32` |
| hz3 | 184.43 | 13,040 | `184.43` |
| hz4 | 140.15 | 27,712 | `140.15` |
| hz5-policy | 122.09 | 10,604 | `122.09` |
| hz6-strict | NaN | NaN | `` |
| hz6-speed | NaN | NaN | `` |
| hz6-rss | NaN | NaN | `` |
| hz6-strict-broad | NaN | NaN | `` |
| hz6-speed-broad | NaN | NaN | `` |
| hz6-rss-broad | NaN | NaN | `` |
| hz6-strict-route4k | NaN | NaN | `` |
| hz6-speed-route4k | NaN | NaN | `` |
| hz6-rss-route4k | NaN | NaN | `` |
| hz6-strict-appcap | 39.12 | 646,780 | `39.12` |
| hz6-speed-appcap | 21.38 | 584,576 | `21.38` |
| hz6-rss-appcap | 7.47 | 597,008 | `7.47` |
| mimalloc | 175.34 | 9,620 | `175.34` |
| tcmalloc | 172.28 | 12,904 | `172.28` |

## Failures

- Redis workload runner allocator hz6-strict run 1 failed with exit code 124
- Redis workload runner allocator hz6-speed run 1 failed with exit code 124
- Redis workload runner allocator hz6-rss run 1 failed with exit code 124
- Redis workload runner allocator hz6-strict-broad run 1 failed with exit code 124
- Redis workload runner allocator hz6-speed-broad run 1 failed with exit code 124
- Redis workload runner allocator hz6-rss-broad run 1 failed with exit code 124
- Redis workload runner allocator hz6-strict-route4k run 1 failed with exit code 124
- Redis workload runner allocator hz6-speed-route4k run 1 failed with exit code 124
- Redis workload runner allocator hz6-rss-route4k run 1 failed with exit code 124

Artifacts: [out_win_redis](/C:/git/hakozuna-win/out_win_redis)
