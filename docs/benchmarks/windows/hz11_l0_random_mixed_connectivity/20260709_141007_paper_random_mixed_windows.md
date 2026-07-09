# Paper-Aligned Windows Random Mixed

Generated: 2026-07-09 14:10:07 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=3`, `ITERS=20,000,000`, `WS=400`
- selected allocators: `hz11-token, hz11-tlsfast`
- `hz7-tinyroute` is a direct-API TinyRoute row: span classes currently cover `<=16KiB`; `>16KiB` uses direct OS regions with bounded 32K/64K direct retain buckets, and it is not an interposer/general allocator row yet.
- `hz7-v2` is the TinyRoute v2 row: it keeps the direct-API/global-lock safety model while testing the v2 task-track changes such as remote-safe smoke and SlowPathOutsideLock.
- `hz11-token` and `hz11-tlsfast` are Windows L0 standalone public-entry rows: token/front-cache only, no fine128/span-transfer parity claim, and no DLL replacement claim.
- HZ6 rows now include `broad`, `control`, `route4k`, and `appcap` capacity lanes; `route4k` isolates route-table capacity while keeping the other control capacities.

## small

- Note: paper SSOT small range
- Params: `iters=20000000 ws=400 size=16..2048`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz11-token | 143.192M | 4,340 | `143.192M / 4,336 KB, 140.901M / 4,340 KB, 130.507M / 4,308 KB` |
| hz11-tlsfast | 141.421M | 4,308 | `139.375M / 4,304 KB, 141.421M / 4,272 KB, 139.244M / 4,308 KB` |

## medium

- Note: paper SSOT medium range
- Params: `iters=20000000 ws=400 size=4096..32768`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz11-token | 112.123M | 5,508 | `109.758M / 5,428 KB, 110.147M / 5,508 KB, 112.123M / 5,496 KB` |
| hz11-tlsfast | 114.059M | 5,500 | `98.414M / 5,492 KB, 114.059M / 5,396 KB, 114.057M / 5,500 KB` |

## mixed

- Note: paper SSOT mixed range
- Params: `iters=20000000 ws=400 size=16..32768`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz11-token | 113.706M | 5,568 | `113.706M / 5,440 KB, 106.440M / 5,424 KB, 113.173M / 5,568 KB` |
| hz11-tlsfast | 123.160M | 5,516 | `114.273M / 5,516 KB, 121.497M / 5,444 KB, 123.160M / 5,416 KB` |

Artifacts: [out_win_random_mixed_hz11_l0](/C:/git/hakozuna-win/out_win_random_mixed_hz11_l0)
