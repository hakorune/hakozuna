# Paper-Aligned Windows Random Mixed

Generated: 2026-06-01 07:26:59 +09:00

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

## small

- Note: paper SSOT small range
- Params: `iters=20000000 ws=400 size=16..2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 42.767M | 5,496 | `42.767M / 5,496 KB` |
| hz3 | 153.762M | 7,288 | `153.762M / 7,288 KB` |
| hz4 | 116.307M | 12,684 | `116.307M / 12,684 KB` |
| hz5-policy | 33.378M | 5,616 | `33.378M / 5,616 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 34.026M | 7,332 | `34.026M / 7,332 KB` |
| hz6-speed-broad | 27.498M | 7,332 | `27.498M / 7,332 KB` |
| hz6-rss-broad | 29.095M | 7,328 | `29.095M / 7,328 KB` |
| hz6-strict-control | 30.782M | 4,800 | `30.782M / 4,800 KB` |
| hz6-speed-control | 24.520M | 4,804 | `24.520M / 4,804 KB` |
| hz6-rss-control | 24.722M | 4,808 | `24.722M / 4,808 KB` |
| hz6-strict-appcap | 34.871M | 196,332 | `34.871M / 196,332 KB` |
| hz6-speed-appcap | 23.764M | 196,336 | `23.764M / 196,336 KB` |
| hz6-rss-appcap | 29.589M | 196,332 | `29.589M / 196,332 KB` |
| mimalloc | 101.338M | 6,628 | `101.338M / 6,628 KB` |
| tcmalloc | 123.028M | 9,588 | `123.028M / 9,588 KB` |

## medium

- Note: paper SSOT medium range
- Params: `iters=20000000 ws=400 size=4096..32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 7.597M | 10,812 | `7.597M / 10,812 KB` |
| hz3 | 152.735M | 6,996 | `152.735M / 6,996 KB` |
| hz4 | 83.701M | 10,356 | `83.701M / 10,356 KB` |
| hz5-policy | 6.811M | 10,292 | `6.811M / 10,292 KB` |
| hz6-strict | failed | n/a | `failed:rc1` |
| hz6-speed | failed | n/a | `failed:rc1` |
| hz6-rss | failed | n/a | `failed:rc1` |
| hz6-strict-broad | 36.204M | 7,332 | `36.204M / 7,332 KB` |
| hz6-speed-broad | 30.387M | 7,336 | `30.387M / 7,336 KB` |
| hz6-rss-broad | 33.040M | 7,332 | `33.040M / 7,332 KB` |
| hz6-strict-control | 29.950M | 4,804 | `29.950M / 4,804 KB` |
| hz6-speed-control | 26.154M | 4,800 | `26.154M / 4,800 KB` |
| hz6-rss-control | 27.707M | 4,804 | `27.707M / 4,804 KB` |
| hz6-strict-appcap | 35.052M | 196,332 | `35.052M / 196,332 KB` |
| hz6-speed-appcap | 25.813M | 196,332 | `25.813M / 196,332 KB` |
| hz6-rss-appcap | 32.455M | 196,332 | `32.455M / 196,332 KB` |
| mimalloc | 88.983M | 11,964 | `88.983M / 11,964 KB` |
| tcmalloc | 152.582M | 10,848 | `152.582M / 10,848 KB` |

Artifacts: [out_win_random_mixed](/C:/git/hakozuna-win/out_win_random_mixed)
