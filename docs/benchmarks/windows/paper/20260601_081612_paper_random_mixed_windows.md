# Paper-Aligned Windows Random Mixed

Generated: 2026-06-01 08:16:12 +09:00

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

## small

- Note: paper SSOT small range
- Params: `iters=20000000 ws=400 size=16..2048`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 45.601M | 4,948 | `45.601M / 4,936 KB, 44.111M / 4,948 KB, 42.646M / 4,912 KB` |
| hz3 | 154.706M | 6,712 | `154.706M / 6,712 KB, 154.422M / 6,708 KB, 153.950M / 6,712 KB` |
| hz4 | 120.920M | 12,104 | `120.786M / 12,104 KB, 120.726M / 12,100 KB, 120.920M / 12,100 KB` |
| hz5-policy | 35.336M | 5,004 | `35.086M / 5,004 KB, 34.732M / 5,004 KB, 35.336M / 5,000 KB` |
| hz6-strict | failed | n/a | `failed:rc1, failed:rc1, failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1, failed:rc1, failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1, failed:rc1, failed:rc1` |
| hz6-strict-broad | 32.955M | 6,748 | `32.955M / 6,740 KB, 32.902M / 6,748 KB, 32.879M / 6,744 KB` |
| hz6-speed-broad | 27.010M | 6,752 | `26.663M / 6,752 KB, 26.720M / 6,748 KB, 27.010M / 6,744 KB` |
| hz6-rss-broad | 29.074M | 6,752 | `29.074M / 6,752 KB, 28.789M / 6,736 KB, 26.312M / 6,744 KB` |
| hz6-strict-control | 29.752M | 4,220 | `27.036M / 4,220 KB, 28.868M / 4,220 KB, 29.752M / 4,216 KB` |
| hz6-speed-control | 23.858M | 4,220 | `23.553M / 4,212 KB, 23.858M / 4,220 KB, 23.758M / 4,216 KB` |
| hz6-rss-control | 24.779M | 4,220 | `24.779M / 4,212 KB, 23.815M / 4,220 KB, 24.767M / 4,208 KB` |
| hz6-strict-route4k | 33.013M | 4,556 | `33.013M / 4,548 KB, 32.238M / 4,556 KB, 32.315M / 4,552 KB` |
| hz6-speed-route4k | 26.879M | 4,556 | `26.879M / 4,556 KB, 26.416M / 4,556 KB, 26.320M / 4,552 KB` |
| hz6-rss-route4k | 28.921M | 4,560 | `28.921M / 4,560 KB, 28.347M / 4,556 KB, 28.789M / 4,556 KB` |
| hz6-strict-appcap | 33.033M | 195,748 | `32.811M / 195,748 KB, 32.658M / 195,744 KB, 33.033M / 195,744 KB` |
| hz6-speed-appcap | 26.718M | 195,748 | `26.010M / 195,740 KB, 26.718M / 195,748 KB, 26.293M / 195,748 KB` |
| hz6-rss-appcap | 28.667M | 195,744 | `28.667M / 195,744 KB, 28.347M / 195,744 KB, 28.453M / 195,744 KB` |
| mimalloc | 101.886M | 6,052 | `100.347M / 6,020 KB, 101.886M / 6,052 KB, 101.134M / 6,032 KB` |
| tcmalloc | 123.127M | 9,000 | `123.127M / 8,992 KB, 121.731M / 9,000 KB, 122.078M / 8,996 KB` |

Artifacts: [out_win_random_mixed_diag](/C:/git/hakozuna-win/out_win_random_mixed_diag)
