# Paper-Aligned Windows Random Mixed

Generated: 2026-07-09 17:26:50 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)

Windows native note:
- benchmark: `bench_random_mixed_compare`
- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`
- throughput statistic: `median ops/s`
- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`
- profiles: `small`, `medium`, `mixed` with `RUNS=3`, `ITERS=20,000,000`, `WS=400`
- selected allocators: `hz11-span, hz11-span-cache256`
- `hz7-tinyroute` is a direct-API TinyRoute row: span classes currently cover `<=16KiB`; `>16KiB` uses direct OS regions with bounded 32K/64K direct retain buckets, and it is not an interposer/general allocator row yet.
- `hz7-v2` is the TinyRoute v2 row: it keeps the direct-API/global-lock safety model while testing the v2 task-track changes such as remote-safe smoke and SlowPathOutsideLock.
- `hz11-token` and `hz11-tlsfast` are Windows L0 standalone public-entry rows: token/front-cache only, no fine128/span-transfer parity claim, and no DLL replacement claim.
- `hz11-span` is a Windows L1 diagnostic row: VirtualAlloc-backed span/classify, no transfer/fine128 parity claim, and no DLL replacement claim.
- `hz11-span-cache256` is a Windows L3 candidate row: `hz11-span` plus `HZ11_CACHE_CAP=256`, added after matrix attribution showed returned-object overflow pressure.
- HZ6 rows now include `broad`, `control`, `route4k`, and `appcap` capacity lanes; `route4k` isolates route-table capacity while keeping the other control capacities.

## small

- Note: paper SSOT small range
- Params: `iters=20000000 ws=400 size=16..2048`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz11-span | 148.208M | 4,632 | `147.094M / 4,632 KB, 146.748M / 4,176 KB, 148.208M / 4,180 KB` |
| hz11-span-cache256 | 156.195M | 4,252 | `155.436M / 4,252 KB, 156.195M / 3,788 KB, 156.039M / 3,784 KB` |

## medium

- Note: paper SSOT medium range
- Params: `iters=20000000 ws=400 size=4096..32768`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz11-span | 146.162M | 4,992 | `146.162M / 4,992 KB, 145.491M / 4,988 KB, 145.861M / 4,988 KB` |
| hz11-span-cache256 | 154.459M | 5,016 | `154.277M / 5,008 KB, 152.762M / 5,016 KB, 154.459M / 5,012 KB` |

## mixed

- Note: paper SSOT mixed range
- Params: `iters=20000000 ws=400 size=16..32768`
- Runs: `3`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz11-span | 148.021M | 5,044 | `147.854M / 5,044 KB, 148.021M / 5,040 KB, 147.899M / 5,036 KB` |
| hz11-span-cache256 | 154.560M | 5,124 | `154.312M / 5,124 KB, 154.560M / 5,120 KB, 154.532M / 5,124 KB` |

Artifacts: [out_win_random_mixed_hz11_cache256](/C:/git/hakozuna-win/out_win_random_mixed_hz11_cache256)
