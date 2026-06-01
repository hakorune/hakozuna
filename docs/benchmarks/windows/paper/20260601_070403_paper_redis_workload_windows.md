# Paper-Aligned Windows Redis Workload

Generated: 2026-06-01 07:04:03 +09:00

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
| crt | 49.40 | 7,896 | `49.40` |
| hz3 | 55.27 | 14,596 | `55.27` |
| hz4 | 53.90 | 27,772 | `53.90` |
| hz5-policy | 41.64 | 10,676 | `41.64` |
| hz6-strict-appcap | 34.55 | 572,788 | `34.55` |
| hz6-speed-appcap | 36.91 | 572,780 | `36.91` |
| hz6-rss-appcap | 36.48 | 572,836 | `36.48` |
| mimalloc | 74.65 | 9,884 | `74.65` |
| tcmalloc | 77.41 | 12,796 | `77.41` |

## GET

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 262.61 | 7,896 | `262.61` |
| hz3 | 312.72 | 14,596 | `312.72` |
| hz4 | 311.47 | 27,772 | `311.47` |
| hz5-policy | 237.51 | 10,676 | `237.51` |
| hz6-strict-appcap | 114.14 | 572,788 | `114.14` |
| hz6-speed-appcap | 121.20 | 572,780 | `121.20` |
| hz6-rss-appcap | 125.17 | 572,836 | `125.17` |
| mimalloc | 316.16 | 9,884 | `316.16` |
| tcmalloc | 309.97 | 12,796 | `309.97` |

## LPUSH

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 48.88 | 7,896 | `48.88` |
| hz3 | 63.04 | 14,596 | `63.04` |
| hz4 | 59.89 | 27,772 | `59.89` |
| hz5-policy | 40.19 | 10,676 | `40.19` |
| hz6-strict-appcap | 29.62 | 572,788 | `29.62` |
| hz6-speed-appcap | 27.33 | 572,780 | `27.33` |
| hz6-rss-appcap | 30.36 | 572,836 | `30.36` |
| mimalloc | 72.93 | 9,884 | `72.93` |
| tcmalloc | 74.47 | 12,796 | `74.47` |

## LPOP

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 1,328.99 | 7,896 | `1,328.99` |
| hz3 | 826.98 | 14,596 | `826.98` |
| hz4 | 978.07 | 27,772 | `978.07` |
| hz5-policy | 690.94 | 10,676 | `690.94` |
| hz6-strict-appcap | 161.40 | 572,788 | `161.40` |
| hz6-speed-appcap | 165.39 | 572,780 | `165.39` |
| hz6-rss-appcap | 153.56 | 572,836 | `153.56` |
| mimalloc | 1,205.15 | 9,884 | `1,205.15` |
| tcmalloc | 1,047.53 | 12,796 | `1,047.53` |

## RANDOM

| allocator | median M ops/sec | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 143.93 | 7,896 | `143.93` |
| hz3 | 151.16 | 14,596 | `151.16` |
| hz4 | 165.86 | 27,772 | `165.86` |
| hz5-policy | 110.28 | 10,676 | `110.28` |
| hz6-strict-appcap | 66.44 | 572,788 | `66.44` |
| hz6-speed-appcap | 64.91 | 572,780 | `64.91` |
| hz6-rss-appcap | 63.51 | 572,836 | `63.51` |
| mimalloc | 171.90 | 9,884 | `171.90` |
| tcmalloc | 167.51 | 12,796 | `167.51` |

## Failures

- none

Artifacts: [out_win_redis](/C:/git/hakozuna-win/out_win_redis)
