# Paper-Aligned Windows MT Remote

Generated: 2026-07-14 10:44:30 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L199)
- [private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md](/C:/git/hakozuna-win/private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md)

Windows native note:
- benchmark: `bench_random_mixed_mt_remote_compare`
- params: `threads=16 iters=2000000 ws=400 size=16..2048 remote_pct=90 ring_slots=65536`
- runs: `5`
- timeout_seconds: `900`
- statistic: `median ops/s`
- hz3 profile: `scale + S97-2 direct-map bucketize + skip_tail_null`
- note: paper originally reports `hz3 / mimalloc / tcmalloc`; this runner also records `crt`, `hz4`, `hz5-policy`, `hz7-tinyroute`, and `hz7-v2-remote-natural`
- hz7 note: `hz7-tinyroute` is a direct-API coarse-lock safety baseline here, not a lock-free remote allocator.
- hz7-v2 note: `hz7-v2-remote-natural` enables `H7_REMOTE_NATURAL_PRESET`, widening the route table for bounded cross-thread pressure without owner inboxes or lock-free remote queues.
- hz7 skip note: `hz7-tinyroute` was skipped for this run; use it only as a legacy control row.
- hz6 note: HZ6 rows are skipped by default because this legacy benchmark frees cross-thread pointers through per-thread allocator instances; use `-IncludeHz6Legacy` only for debugging the mismatch, and use the HZ6 standalone remote/reuse runner for HZ6 contract numbers.

| allocator | median ops/s | median actual remote % | median fallback % | median peak_kb | runs |
| --- | ---: | ---: | ---: | ---: | --- |
| hz8 | 131.749M | 79.08 | 12.14 | 19,080 | `136.428M / actual 81.21% / fallback 9.77% / 19080 KB, 131.749M / actual 79.06% / fallback 12.16% / 19508 KB, 129.540M / actual 79.08% / fallback 12.14% / 18376 KB, 121.756M / actual 77.01% / fallback 14.44% / 18404 KB, 132.649M / actual 79.97% / fallback 11.14% / 19692 KB` |
| hz8-small-transition-inventory | 127.065M | 75.42 | 16.20 | 20,360 | `139.431M / actual 74.70% / fallback 17.00% / 19816 KB, 114.958M / actual 75.24% / fallback 16.40% / 21224 KB, 133.826M / actual 75.42% / fallback 16.20% / 20480 KB, 127.065M / actual 76.89% / fallback 14.57% / 20360 KB, 126.808M / actual 80.79% / fallback 10.24% / 19072 KB` |

Artifacts: [out_win_mt_remote](/C:/git/hakozuna-win/out_win_mt_remote)
