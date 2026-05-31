# Paper-Aligned Windows Random Mixed

Generated: 2026-06-01 07:42:25 +09:00

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
| crt | 7.474M | 11,088 | `7.474M / 11,088 KB` |
| hz3 | 149.082M | 7,428 | `149.082M / 7,428 KB` |
| hz4 | 79.333M | 18,220 | `79.333M / 18,220 KB` |
| hz5-policy | 6.834M | 11,460 | `6.834M / 11,460 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 33.101M | 6,748 | `33.101M / 6,748 KB` |
| hz6-speed-broad | 28.435M | 6,740 | `28.435M / 6,740 KB` |
| hz6-rss-broad | 30.851M | 6,752 | `30.851M / 6,752 KB` |
| hz6-strict-control | 27.128M | 4,216 | `27.128M / 4,216 KB` |
| hz6-speed-control | 24.045M | 4,220 | `24.045M / 4,220 KB` |
| hz6-rss-control | 26.442M | 4,220 | `26.442M / 4,220 KB` |
| hz6-strict-appcap | 32.157M | 195,748 | `32.157M / 195,748 KB` |
| hz6-speed-appcap | 27.451M | 195,748 | `27.451M / 195,748 KB` |
| hz6-rss-appcap | 29.657M | 195,744 | `29.657M / 195,744 KB` |
| mimalloc | 87.678M | 11,572 | `87.678M / 11,572 KB` |
| tcmalloc | 151.110M | 10,872 | `151.110M / 10,872 KB` |

Artifacts: [out_win_random_mixed_diag](/C:/git/hakozuna-win/out_win_random_mixed_diag)
