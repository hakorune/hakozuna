# Paper-Aligned Windows Random Mixed

Generated: 2026-03-09 04:11:46 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=10`, `ITERS=20,000,000`, `WS=400`

## small

- Note: paper SSOT small range
- Params: `iters=20000000 ws=400 size=16..2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| crt | 47.333M | 5,936 | `47.333M / 5,936 KB` |
| hz3 | 148.921M | 6,656 | `148.921M / 6,656 KB` |
| hz4 | 133.809M | 13,060 | `133.809M / 13,060 KB` |
| mimalloc | 99.208M | 7,172 | `99.208M / 7,172 KB` |
| tcmalloc | 119.350M | 10,012 | `119.350M / 10,012 KB` |

Artifacts: [out_win_random_mixed](/C:/git/hakozuna-win/out_win_random_mixed)

