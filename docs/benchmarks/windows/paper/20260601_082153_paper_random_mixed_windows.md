# Paper-Aligned Windows Random Mixed

Generated: 2026-06-01 08:21:53 +09:00

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

## mixed

- Note: paper SSOT mixed range
- Params: `iters=20000000 ws=400 size=16..32768`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 7.008M | 11,096 | `7.008M / 11,084 KB, 6.902M / 11,096 KB, 6.982M / 11,092 KB` |
| hz3 | 147.753M | 7,424 | `147.753M / 7,424 KB, 147.528M / 7,424 KB, 146.659M / 7,424 KB` |
| hz4 | 78.302M | 18,248 | `78.124M / 18,228 KB, 78.168M / 18,248 KB, 78.302M / 18,244 KB` |
| hz5-policy | 6.719M | 11,460 | `6.635M / 11,456 KB, 6.706M / 11,460 KB, 6.719M / 11,460 KB` |
| hz6-strict | failed | n/a | `failed:rc1, failed:rc1, failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1, failed:rc1, failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1, failed:rc1, failed:rc1` |
| hz6-strict-broad | 33.132M | 6,748 | `33.011M / 6,740 KB, 33.132M / 6,748 KB, 33.020M / 6,744 KB` |
| hz6-speed-broad | 28.377M | 6,752 | `28.377M / 6,744 KB, 28.092M / 6,744 KB, 28.147M / 6,752 KB` |
| hz6-rss-broad | 30.398M | 6,748 | `30.398M / 6,748 KB, 30.151M / 6,740 KB, 30.249M / 6,744 KB` |
| hz6-strict-control | 27.857M | 4,220 | `26.950M / 4,216 KB, 27.574M / 4,216 KB, 27.857M / 4,220 KB` |
| hz6-speed-control | 23.089M | 4,220 | `23.089M / 4,220 KB, 22.629M / 4,220 KB, 23.076M / 4,212 KB` |
| hz6-rss-control | 26.122M | 4,224 | `26.122M / 4,220 KB, 24.762M / 4,216 KB, 25.740M / 4,224 KB` |
| hz6-strict-route4k | 32.851M | 4,556 | `32.851M / 4,556 KB, 32.531M / 4,552 KB, 32.727M / 4,548 KB` |
| hz6-speed-route4k | 28.399M | 4,552 | `28.399M / 4,552 KB, 28.394M / 4,548 KB, 28.238M / 4,548 KB` |
| hz6-rss-route4k | 30.424M | 4,556 | `30.200M / 4,552 KB, 30.424M / 4,548 KB, 30.272M / 4,556 KB` |
| hz6-strict-appcap | 31.993M | 195,748 | `31.364M / 195,744 KB, 31.988M / 195,748 KB, 31.993M / 195,744 KB` |
| hz6-speed-appcap | 27.558M | 195,748 | `27.228M / 195,740 KB, 27.230M / 195,748 KB, 27.558M / 195,740 KB` |
| hz6-rss-appcap | 29.705M | 195,748 | `29.576M / 195,732 KB, 29.705M / 195,748 KB, 28.726M / 195,740 KB` |
| mimalloc | 87.032M | 12,160 | `86.700M / 11,992 KB, 87.032M / 11,880 KB, 85.673M / 12,160 KB` |
| tcmalloc | 148.453M | 10,888 | `147.774M / 10,888 KB, 145.588M / 10,884 KB, 148.453M / 10,876 KB` |

Artifacts: [out_win_random_mixed_diag](/C:/git/hakozuna-win/out_win_random_mixed_diag)
