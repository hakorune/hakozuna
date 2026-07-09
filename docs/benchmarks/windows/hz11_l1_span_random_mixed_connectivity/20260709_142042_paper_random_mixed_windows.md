# Paper-Aligned Windows Random Mixed

Generated: 2026-07-09 14:20:42 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=3`, `ITERS=20,000,000`, `WS=400`
- selected allocators: `hz11-span`
- `hz7-tinyroute` is a direct-API TinyRoute row: span classes currently cover `<=16KiB`; `>16KiB` uses direct OS regions with bounded 32K/64K direct retain buckets, and it is not an interposer/general allocator row yet.
- `hz7-v2` is the TinyRoute v2 row: it keeps the direct-API/global-lock safety model while testing the v2 task-track changes such as remote-safe smoke and SlowPathOutsideLock.
- `hz11-token` and `hz11-tlsfast` are Windows L0 standalone public-entry rows: token/front-cache only, no fine128/span-transfer parity claim, and no DLL replacement claim.
- `hz11-span` is a Windows L1 diagnostic row: VirtualAlloc-backed span/classify, no transfer/fine128 parity claim, and no DLL replacement claim.
- HZ6 rows now include `broad`, `control`, `route4k`, and `appcap` capacity lanes; `route4k` isolates route-table capacity while keeping the other control capacities.

## small

- Note: paper SSOT small range
- Params: `iters=20000000 ws=400 size=16..2048`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz11-span | 149.284M | 4,180 | `148.847M / 4,180 KB, 148.898M / 4,176 KB, 149.284M / 4,172 KB` |

## medium

- Note: paper SSOT medium range
- Params: `iters=20000000 ws=400 size=4096..32768`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz11-span | 147.110M | 4,988 | `147.110M / 4,988 KB, 146.480M / 4,984 KB, 146.695M / 4,988 KB` |

## mixed

- Note: paper SSOT mixed range
- Params: `iters=20000000 ws=400 size=16..32768`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz11-span | 148.464M | 5,040 | `148.193M / 5,036 KB, 147.702M / 5,040 KB, 148.464M / 5,036 KB` |

Artifacts: [out_win_random_mixed_hz11_l1](/C:/git/hakozuna-win/out_win_random_mixed_hz11_l1)
