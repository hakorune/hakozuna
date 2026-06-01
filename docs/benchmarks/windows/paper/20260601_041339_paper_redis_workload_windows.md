# Paper-Aligned Windows Redis Workload

Generated: 2026-06-01 04:13:39 +09:00

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

| allocator | median M ops/sec | runs |
| --- | ---: | --- |
| crt | 57.23 | `57.23` |
| hz3 | 26.57 | `26.57` |
| hz4 | 51.77 | `51.77` |
| hz5-policy | 31.25 | `31.25` |
| hz6-strict | NaN | `` |
| hz6-speed | NaN | `` |
| hz6-rss | NaN | `` |
| hz6-strict-broad | NaN | `` |
| hz6-speed-broad | NaN | `` |
| hz6-rss-broad | NaN | `` |
| hz6-strict-appcap | 34.95 | `34.95` |
| hz6-speed-appcap | 35.90 | `35.90` |
| hz6-rss-appcap | 36.78 | `36.78` |
| mimalloc | 77.77 | `77.77` |
| tcmalloc | 79.51 | `79.51` |

## GET

| allocator | median M ops/sec | runs |
| --- | ---: | --- |
| crt | 309.70 | `309.70` |
| hz3 | 191.35 | `191.35` |
| hz4 | 339.39 | `339.39` |
| hz5-policy | 143.84 | `143.84` |
| hz6-strict | NaN | `` |
| hz6-speed | NaN | `` |
| hz6-rss | NaN | `` |
| hz6-strict-broad | NaN | `` |
| hz6-speed-broad | NaN | `` |
| hz6-rss-broad | NaN | `` |
| hz6-strict-appcap | 113.90 | `113.90` |
| hz6-speed-appcap | 116.07 | `116.07` |
| hz6-rss-appcap | 126.43 | `126.43` |
| mimalloc | 327.09 | `327.09` |
| tcmalloc | 306.83 | `306.83` |

## LPUSH

| allocator | median M ops/sec | runs |
| --- | ---: | --- |
| crt | 33.16 | `33.16` |
| hz3 | 54.73 | `54.73` |
| hz4 | 74.93 | `74.93` |
| hz5-policy | 20.29 | `20.29` |
| hz6-strict | NaN | `` |
| hz6-speed | NaN | `` |
| hz6-rss | NaN | `` |
| hz6-strict-broad | NaN | `` |
| hz6-speed-broad | NaN | `` |
| hz6-rss-broad | NaN | `` |
| hz6-strict-appcap | 31.02 | `31.02` |
| hz6-speed-appcap | 26.23 | `26.23` |
| hz6-rss-appcap | 31.21 | `31.21` |
| mimalloc | 74.45 | `74.45` |
| tcmalloc | 77.77 | `77.77` |

## LPOP

| allocator | median M ops/sec | runs |
| --- | ---: | --- |
| crt | 864.38 | `864.38` |
| hz3 | 887.37 | `887.37` |
| hz4 | 917.85 | `917.85` |
| hz5-policy | 1,154.23 | `1,154.23` |
| hz6-strict | NaN | `` |
| hz6-speed | NaN | `` |
| hz6-rss | NaN | `` |
| hz6-strict-broad | NaN | `` |
| hz6-speed-broad | NaN | `` |
| hz6-rss-broad | NaN | `` |
| hz6-strict-appcap | 158.43 | `158.43` |
| hz6-speed-appcap | 145.55 | `145.55` |
| hz6-rss-appcap | 144.63 | `144.63` |
| mimalloc | 974.83 | `974.83` |
| tcmalloc | 1,169.15 | `1,169.15` |

## RANDOM

| allocator | median M ops/sec | runs |
| --- | ---: | --- |
| crt | 94.28 | `94.28` |
| hz3 | 92.50 | `92.50` |
| hz4 | 166.90 | `166.90` |
| hz5-policy | 87.08 | `87.08` |
| hz6-strict | NaN | `` |
| hz6-speed | NaN | `` |
| hz6-rss | NaN | `` |
| hz6-strict-broad | NaN | `` |
| hz6-speed-broad | NaN | `` |
| hz6-rss-broad | NaN | `` |
| hz6-strict-appcap | 60.27 | `60.27` |
| hz6-speed-appcap | 63.79 | `63.79` |
| hz6-rss-appcap | 63.81 | `63.81` |
| mimalloc | 173.04 | `173.04` |
| tcmalloc | 171.72 | `171.72` |

## Failures

- Redis workload runner allocator hz6-strict run 1 failed with exit code 124
- Redis workload runner allocator hz6-speed run 1 failed with exit code 124
- Redis workload runner allocator hz6-rss run 1 failed with exit code 124
- Redis workload runner allocator hz6-strict-broad run 1 failed with exit code 124
- Redis workload runner allocator hz6-speed-broad run 1 failed with exit code 124
- Redis workload runner allocator hz6-rss-broad run 1 failed with exit code 124

Artifacts: [out_win_redis](/C:/git/hakozuna-win/out_win_redis)
