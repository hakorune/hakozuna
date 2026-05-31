# Paper-Aligned Windows Random Mixed

Generated: 2026-06-01 07:50:23 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=1`, `ITERS=20,000,000`, `WS=400`
- HZ6 rows now include `broad`, `control`, `route4k`, and `appcap` capacity lanes; `route4k` isolates route-table capacity while keeping the other control capacities.

## mixed

- Note: paper SSOT mixed range
- Params: `iters=20000000 ws=400 size=16..32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 7.509M | 11,068 | `7.509M / 11,068 KB` |
| hz3 | 151.727M | 7,420 | `151.727M / 7,420 KB` |
| hz4 | 79.812M | 18,244 | `79.812M / 18,244 KB` |
| hz5-policy | 7.065M | 11,460 | `7.065M / 11,460 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 32.952M | 6,740 | `32.952M / 6,740 KB` |
| hz6-speed-broad | 28.592M | 6,744 | `28.592M / 6,744 KB` |
| hz6-rss-broad | 30.575M | 6,744 | `30.575M / 6,744 KB` |
| hz6-strict-control | 27.794M | 4,220 | `27.794M / 4,220 KB` |
| hz6-speed-control | 23.134M | 4,216 | `23.134M / 4,216 KB` |
| hz6-rss-control | 25.396M | 4,212 | `25.396M / 4,212 KB` |
| hz6-strict-route4k | 32.752M | 5,008 | `32.752M / 5,008 KB` |
| hz6-speed-route4k | 28.102M | 5,016 | `28.102M / 5,016 KB` |
| hz6-rss-route4k | 30.577M | 5,008 | `30.577M / 5,008 KB` |
| hz6-strict-appcap | 31.762M | 195,744 | `31.762M / 195,744 KB` |
| hz6-speed-appcap | 27.440M | 195,752 | `27.440M / 195,752 KB` |
| hz6-rss-appcap | 29.921M | 195,748 | `29.921M / 195,748 KB` |
| mimalloc | 88.574M | 11,900 | `88.574M / 11,900 KB` |
| tcmalloc | 150.924M | 10,880 | `150.924M / 10,880 KB` |

Artifacts: [out_win_random_mixed_diag](/C:/git/hakozuna-win/out_win_random_mixed_diag)
