# Paper-Aligned Windows MT Remote

Generated: 2026-07-14 16:26:55 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L199)
- [private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md](/C:/git/hakozuna-win/private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md)

Windows native note:
- benchmark: `bench_random_mixed_mt_remote_compare`
- params: `threads=16 iters=2000000 ws=400 size=16..2048 remote_pct=90 ring_slots=65536`
- runs: `1`
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
| hz3 | 131.193M | 79.38 | 11.80 | 641,084 | `131.193M / actual 79.38% / fallback 11.80% / 641084 KB` |
| hz4 | 120.994M | 77.27 | 14.15 | 604,132 | `120.994M / actual 77.27% / fallback 14.15% / 604132 KB` |
| hz8 | 66.275M | 77.79 | 13.57 | 19,908 | `66.275M / actual 77.79% / fallback 13.57% / 19908 KB` |
| mimalloc | 107.927M | 79.21 | 12.00 | 536,036 | `107.927M / actual 79.21% / fallback 12.00% / 536036 KB` |
| tcmalloc | 123.337M | 69.36 | 22.93 | 815,540 | `123.337M / actual 69.36% / fallback 22.93% / 815540 KB` |

Artifacts: [out_win_mt_remote](/C:/git/hakozuna-win/out_win_mt_remote)
