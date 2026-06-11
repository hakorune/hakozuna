# Paper-Aligned Windows MT Remote

Generated: 2026-06-11 21:18:11 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L199)
- [private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md](/C:/git/hakozuna-win/private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md)

Windows native note:
- benchmark: `bench_random_mixed_mt_remote_compare`
- params: `threads=16 iters=2000000 ws=400 size=16..2048 remote_pct=90 ring_slots=65536`
- runs: `1`
- timeout_seconds: `60`
- statistic: `median ops/s`
- hz3 profile: `scale + S97-2 direct-map bucketize + skip_tail_null`
- note: paper originally reports `hz3 / mimalloc / tcmalloc`; this runner also records `crt`, `hz4`, `hz5-policy`, `hz7-tinyroute`, and `hz7-v2-remote-natural`
- hz7 note: `hz7-tinyroute` is a direct-API coarse-lock safety baseline here, not a lock-free remote allocator.
- hz7-v2 note: `hz7-v2-remote-natural` enables `H7_REMOTE_NATURAL_PRESET`, widening the route table for bounded cross-thread pressure without owner inboxes or lock-free remote queues.
- hz7 skip note: `hz7-tinyroute` was skipped for this run; use it only as a legacy control row.
- hz6 note: HZ6 rows are skipped by default because this legacy benchmark frees cross-thread pointers through per-thread allocator instances; use `-IncludeHz6Legacy` only for debugging the mismatch, and use the HZ6 standalone remote/reuse runner for HZ6 contract numbers.

| allocator | median ops/s | median actual remote % | median fallback % | median peak_kb | runs |
| --- | ---: | ---: | ---: | ---: | --- |
| crt | 75.102M | 85.11 | 5.44 | 457,688 | `75.102M / actual 85.11% / fallback 5.44% / 457688 KB` |
| hz3 | 136.692M | 81.91 | 9.00 | 542,288 | `136.692M / actual 81.91% / fallback 9.00% / 542288 KB` |
| hz4 | 119.554M | 79.84 | 11.29 | 548,604 | `119.554M / actual 79.84% / fallback 11.29% / 548604 KB` |
| hz5-policy | 69.215M | 87.66 | 2.61 | 549,356 | `69.215M / actual 87.66% / fallback 2.61% / 549356 KB` |
| hz7-v2-remote-natural | 4.942M | 87.48 | 2.80 | 520,412 | `4.942M / actual 87.48% / fallback 2.80% / 520412 KB` |
| mimalloc | 113.033M | 81.49 | 9.46 | 450,240 | `113.033M / actual 81.49% / fallback 9.46% / 450240 KB` |
| tcmalloc | 113.045M | 71.42 | 20.65 | 793,820 | `113.045M / actual 71.42% / fallback 20.65% / 793820 KB` |

Artifacts: [out_win_mt_remote](/C:/git/hakozuna-win/out_win_mt_remote)
