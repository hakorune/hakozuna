# Paper-Aligned Windows Random Mixed

Generated: 2026-06-01 07:07:06 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=1`, `ITERS=20,000,000`, `WS=400`

## small

- Note: paper SSOT small range
- Params: `iters=20000000 ws=400 size=16..2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 44.031M | 4,892 | `44.031M / 4,892 KB` |
| hz3 | 154.989M | 6,712 | `154.989M / 6,712 KB` |
| hz4 | 120.299M | 12,104 | `120.299M / 12,104 KB` |
| hz5-policy | 36.207M | 5,340 | `36.207M / 5,340 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 39.494M | 7,444 | `39.494M / 7,444 KB` |
| hz6-speed-broad | 38.451M | 7,448 | `38.451M / 7,448 KB` |
| hz6-rss-broad | 39.564M | 7,440 | `39.564M / 7,440 KB` |
| hz6-strict-appcap | 38.714M | 16,260 | `38.714M / 16,260 KB` |
| hz6-speed-appcap | 39.284M | 16,260 | `39.284M / 16,260 KB` |
| hz6-rss-appcap | 39.292M | 16,256 | `39.292M / 16,256 KB` |
| mimalloc | 102.123M | 6,028 | `102.123M / 6,028 KB` |
| tcmalloc | 123.764M | 8,992 | `123.764M / 8,992 KB` |

## medium

- Note: paper SSOT medium range
- Params: `iters=20000000 ws=400 size=4096..32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 7.365M | 10,072 | `7.365M / 10,072 KB` |
| hz3 | 154.366M | 6,416 | `154.366M / 6,416 KB` |
| hz4 | 83.797M | 9,768 | `83.797M / 9,768 KB` |
| hz5-policy | 6.387M | 9,992 | `6.387M / 9,992 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 36.839M | 6,672 | `36.839M / 6,672 KB` |
| hz6-speed-broad | 31.634M | 6,664 | `31.634M / 6,664 KB` |
| hz6-rss-broad | 34.865M | 6,672 | `34.865M / 6,672 KB` |
| hz6-strict-appcap | 37.860M | 15,480 | `37.860M / 15,480 KB` |
| hz6-speed-appcap | 32.297M | 15,472 | `32.297M / 15,472 KB` |
| hz6-rss-appcap | 35.316M | 15,480 | `35.316M / 15,480 KB` |
| mimalloc | 88.935M | 11,116 | `88.935M / 11,116 KB` |
| tcmalloc | 154.210M | 10,268 | `154.210M / 10,268 KB` |

## mixed

- Note: paper SSOT mixed range
- Params: `iters=20000000 ws=400 size=16..32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 7.584M | 10,428 | `7.584M / 10,428 KB` |
| hz3 | 150.100M | 7,428 | `150.100M / 7,428 KB` |
| hz4 | 79.662M | 18,244 | `79.662M / 18,244 KB` |
| hz5-policy | 6.640M | 11,684 | `6.640M / 11,684 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 35.722M | 6,752 | `35.722M / 6,752 KB` |
| hz6-speed-broad | 31.371M | 6,756 | `31.371M / 6,756 KB` |
| hz6-rss-broad | 33.591M | 6,764 | `33.591M / 6,764 KB` |
| hz6-strict-appcap | 35.439M | 15,568 | `35.439M / 15,568 KB` |
| hz6-speed-appcap | 31.268M | 15,564 | `31.268M / 15,564 KB` |
| hz6-rss-appcap | 33.812M | 15,568 | `33.812M / 15,568 KB` |
| mimalloc | 88.653M | 12,136 | `88.653M / 12,136 KB` |
| tcmalloc | 151.342M | 10,888 | `151.342M / 10,888 KB` |

Artifacts: [out_win_random_mixed](/C:/git/hakozuna-win/out_win_random_mixed)
