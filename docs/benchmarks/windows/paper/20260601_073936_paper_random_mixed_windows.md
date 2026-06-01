# Paper-Aligned Windows Random Mixed

Generated: 2026-06-01 07:39:36 +09:00

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
| crt | 7.501M | 11,212 | `7.501M / 11,212 KB` |
| hz3 | 147.951M | 7,872 | `147.951M / 7,872 KB` |
| hz4 | 78.750M | 18,696 | `78.750M / 18,696 KB` |
| hz5-policy | 7.359M | 11,472 | `7.359M / 11,472 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 32.472M | 7,200 | `32.472M / 7,200 KB` |
| hz6-speed-broad | 28.054M | 7,212 | `28.054M / 7,212 KB` |
| hz6-rss-broad | 30.231M | 7,204 | `30.231M / 7,204 KB` |
| hz6-strict-control | 28.315M | 4,668 | `28.315M / 4,668 KB` |
| hz6-speed-control | 23.346M | 4,676 | `23.346M / 4,676 KB` |
| hz6-rss-control | 24.744M | 4,676 | `24.744M / 4,676 KB` |
| hz6-strict-appcap | 31.934M | 196,200 | `31.934M / 196,200 KB` |
| hz6-speed-appcap | 27.807M | 196,200 | `27.807M / 196,200 KB` |
| hz6-rss-appcap | 29.385M | 196,208 | `29.385M / 196,208 KB` |
| mimalloc | 88.107M | 12,588 | `88.107M / 12,588 KB` |
| tcmalloc | 149.253M | 11,376 | `149.253M / 11,376 KB` |

Artifacts: [out_win_random_mixed_diag](/C:/git/hakozuna-win/out_win_random_mixed_diag)
