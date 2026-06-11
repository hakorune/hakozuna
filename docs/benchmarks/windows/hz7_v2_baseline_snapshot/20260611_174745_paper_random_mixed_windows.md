# Paper-Aligned Windows Random Mixed

Generated: 2026-06-11 17:47:45 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=3`, `ITERS=20,000,000`, `WS=400`
- selected allocators: `hz3, hz4, hz7-v2, mimalloc, tcmalloc`
- `hz7-tinyroute` is a direct-API TinyRoute row: span classes currently cover `<=16KiB`; `>16KiB` uses direct OS regions with bounded 32K/64K direct retain buckets, and it is not an interposer/general allocator row yet.
- `hz7-v2` is the TinyRoute v2 row: it keeps the direct-API/global-lock safety model while testing the v2 task-track changes such as remote-safe smoke and SlowPathOutsideLock.
- HZ6 rows now include `broad`, `control`, `route4k`, and `appcap` capacity lanes; `route4k` isolates route-table capacity while keeping the other control capacities.

## small

- Note: paper SSOT small range
- Params: `iters=20000000 ws=400 size=16..2048`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz3 | 156.548M | 6,728 | `155.790M / 6,728 KB, 156.548M / 6,728 KB, 155.526M / 6,728 KB` |
| hz4 | 120.952M | 12,124 | `119.522M / 12,124 KB, 119.431M / 12,120 KB, 120.952M / 12,124 KB` |
| hz7-v2 | 79.741M | 4,576 | `79.170M / 4,572 KB, 79.660M / 4,576 KB, 79.741M / 4,572 KB` |
| mimalloc | 101.425M | 6,056 | `101.425M / 6,044 KB, 101.390M / 6,056 KB, 100.665M / 6,036 KB` |
| tcmalloc | 122.852M | 9,016 | `122.333M / 9,012 KB, 122.852M / 9,004 KB, 122.620M / 9,016 KB` |

## medium

- Note: paper SSOT medium range
- Params: `iters=20000000 ws=400 size=4096..32768`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz3 | 154.632M | 6,436 | `154.632M / 6,436 KB, 154.260M / 6,432 KB, 153.588M / 6,436 KB` |
| hz4 | 84.350M | 9,800 | `83.368M / 9,800 KB, 84.350M / 9,784 KB, 84.029M / 9,800 KB` |
| hz7-v2 | 53.353M | 5,140 | `52.643M / 5,140 KB, 53.353M / 5,140 KB, 53.116M / 5,140 KB` |
| mimalloc | 88.337M | 11,468 | `88.337M / 11,424 KB, 88.149M / 11,468 KB, 88.174M / 11,368 KB` |
| tcmalloc | 154.623M | 10,292 | `153.767M / 10,292 KB, 153.751M / 10,292 KB, 154.623M / 10,276 KB` |

## mixed

- Note: paper SSOT mixed range
- Params: `iters=20000000 ws=400 size=16..32768`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz3 | 151.243M | 7,444 | `150.865M / 7,444 KB, 149.615M / 7,440 KB, 151.243M / 7,444 KB` |
| hz4 | 80.019M | 18,260 | `79.491M / 18,248 KB, 80.019M / 18,260 KB, 79.642M / 18,260 KB` |
| hz7-v2 | 52.911M | 5,664 | `52.669M / 5,664 KB, 52.819M / 5,664 KB, 52.911M / 5,660 KB` |
| mimalloc | 88.414M | 12,124 | `88.386M / 11,952 KB, 88.211M / 12,024 KB, 88.414M / 12,124 KB` |
| tcmalloc | 151.538M | 10,900 | `150.072M / 10,892 KB, 151.362M / 10,900 KB, 151.538M / 10,892 KB` |

Artifacts: [out_win_random_mixed](/C:/git/hakozuna-win/out_win_random_mixed)
