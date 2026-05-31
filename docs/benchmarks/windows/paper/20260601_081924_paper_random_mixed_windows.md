# Paper-Aligned Windows Random Mixed

Generated: 2026-06-01 08:19:24 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=3`, `ITERS=20,000,000`, `WS=400`
- HZ6 rows now include `broad`, `control`, `route4k`, and `appcap` capacity lanes; `route4k` isolates route-table capacity while keeping the other control capacities.

## medium

- Note: paper SSOT medium range
- Params: `iters=20000000 ws=400 size=4096..32768`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 8.121M | 10,248 | `7.302M / 10,028 KB, 7.297M / 10,028 KB, 8.121M / 10,248 KB` |
| hz3 | 154.658M | 6,412 | `154.658M / 6,412 KB, 153.909M / 6,412 KB, 152.951M / 6,412 KB` |
| hz4 | 84.596M | 9,784 | `83.584M / 9,784 KB, 84.596M / 9,780 KB, 83.513M / 9,776 KB` |
| hz5-policy | 6.612M | 10,184 | `6.415M / 10,176 KB, 6.551M / 10,172 KB, 6.612M / 10,184 KB` |
| hz6-strict | failed | n/a | `failed:rc1, failed:rc1, failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1, failed:rc1, failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1, failed:rc1, failed:rc1` |
| hz6-strict-broad | 34.774M | 6,748 | `34.350M / 6,748 KB, 34.774M / 6,744 KB, 34.690M / 6,744 KB` |
| hz6-speed-broad | 29.989M | 6,748 | `29.989M / 6,740 KB, 29.920M / 6,748 KB, 29.914M / 6,748 KB` |
| hz6-rss-broad | 32.515M | 6,748 | `32.515M / 6,748 KB, 32.491M / 6,744 KB, 32.331M / 6,740 KB` |
| hz6-strict-control | 30.150M | 4,220 | `30.150M / 4,212 KB, 28.801M / 4,216 KB, 29.152M / 4,220 KB` |
| hz6-speed-control | 26.624M | 4,220 | `26.624M / 4,220 KB, 25.918M / 4,216 KB, 25.210M / 4,212 KB` |
| hz6-rss-control | 27.769M | 4,220 | `27.769M / 4,216 KB, 27.393M / 4,220 KB, 27.530M / 4,220 KB` |
| hz6-strict-route4k | 35.254M | 4,556 | `34.954M / 4,544 KB, 35.254M / 4,556 KB, 34.576M / 4,552 KB` |
| hz6-speed-route4k | 30.282M | 4,556 | `30.282M / 4,556 KB, 29.776M / 4,556 KB, 30.186M / 4,556 KB` |
| hz6-rss-route4k | 32.581M | 4,560 | `32.544M / 4,548 KB, 32.581M / 4,556 KB, 32.308M / 4,560 KB` |
| hz6-strict-appcap | 33.520M | 195,752 | `33.467M / 195,748 KB, 33.507M / 195,748 KB, 33.520M / 195,752 KB` |
| hz6-speed-appcap | 29.320M | 195,748 | `28.923M / 195,744 KB, 29.320M / 195,748 KB, 28.807M / 195,748 KB` |
| hz6-rss-appcap | 31.643M | 195,744 | `31.095M / 195,744 KB, 30.859M / 195,740 KB, 31.643M / 195,744 KB` |
| mimalloc | 88.670M | 11,948 | `88.187M / 11,948 KB, 88.643M / 11,688 KB, 88.670M / 11,360 KB` |
| tcmalloc | 154.566M | 10,276 | `154.566M / 10,268 KB, 154.070M / 10,268 KB, 153.629M / 10,276 KB` |

Artifacts: [out_win_random_mixed_diag](/C:/git/hakozuna-win/out_win_random_mixed_diag)
