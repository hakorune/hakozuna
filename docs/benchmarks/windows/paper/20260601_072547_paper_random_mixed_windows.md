# Paper-Aligned Windows Random Mixed

Generated: 2026-06-01 07:25:47 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=1`, `ITERS=20,000,000`, `WS=400`
- HZ6 rows now include `broad`, `control`, and `appcap` capacity lanes; `control` is the new diagnostic bridge between default capacity controls and appcap.

## mixed

- Note: paper SSOT mixed range
- Params: `iters=20000000 ws=400 size=16..32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 7.322M | 11,344 | `7.322M / 11,344 KB` |
| hz3 | 149.995M | 7,424 | `149.995M / 7,424 KB` |
| hz4 | 79.671M | 18,232 | `79.671M / 18,232 KB` |
| hz5-policy | 7.123M | 10,800 | `7.123M / 10,800 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 34.079M | 6,744 | `34.079M / 6,744 KB` |
| hz6-speed-broad | 28.964M | 6,752 | `28.964M / 6,752 KB` |
| hz6-rss-broad | 31.339M | 6,744 | `31.339M / 6,744 KB` |
| hz6-strict-control | 29.469M | 4,680 | `29.469M / 4,680 KB` |
| hz6-speed-control | 24.390M | 4,676 | `24.390M / 4,676 KB` |
| hz6-rss-control | 27.199M | 4,676 | `27.199M / 4,676 KB` |
| hz6-strict-appcap | 33.415M | 195,748 | `33.415M / 195,748 KB` |
| hz6-speed-appcap | 28.473M | 195,744 | `28.473M / 195,744 KB` |
| hz6-rss-appcap | 30.786M | 195,744 | `30.786M / 195,744 KB` |
| mimalloc | 88.425M | 11,916 | `88.425M / 11,916 KB` |
| tcmalloc | 148.176M | 10,884 | `148.176M / 10,884 KB` |

Artifacts: [out_win_random_mixed](/C:/git/hakozuna-win/out_win_random_mixed)
