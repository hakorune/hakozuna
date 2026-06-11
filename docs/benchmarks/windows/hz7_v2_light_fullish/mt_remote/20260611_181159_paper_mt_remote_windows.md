# Paper-Aligned Windows MT Remote

Generated: 2026-06-11 18:11:59 +09:00

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
- note: paper originally reports `hz3 / mimalloc / tcmalloc`; this runner also records `crt`, `hz4`, `hz5-policy`, and `hz7-tinyroute`
- hz7 note: `hz7-tinyroute` is a direct-API coarse-lock safety baseline here, not a lock-free remote allocator.
- hz6 note: HZ6 rows are skipped by default because this legacy benchmark frees cross-thread pointers through per-thread allocator instances; use `-IncludeHz6Legacy` only for debugging the mismatch, and use the HZ6 standalone remote/reuse runner for HZ6 contract numbers.

| allocator | median ops/s | median actual remote % | median fallback % | median peak_kb | runs |
| --- | ---: | ---: | ---: | ---: | --- |
| crt | 70.410M | 83.72 | 6.98 | 386,188 | `70.410M / actual 83.72% / fallback 6.98% / 386188 KB` |
| hz3 | 131.155M | 77.89 | 13.46 | 553,364 | `131.155M / actual 77.89% / fallback 13.46% / 553364 KB` |
| hz4 | 116.669M | 79.87 | 11.26 | 479,380 | `116.669M / actual 79.87% / fallback 11.26% / 479380 KB` |
| hz5-policy | 70.112M | 84.63 | 5.97 | 506,340 | `70.112M / actual 84.63% / fallback 5.97% / 506340 KB` |
| hz7-tinyroute | failed | n/a | n/a | n/a | `failed:rc-1` |
| mimalloc | 112.669M | 83.96 | 6.72 | 459,776 | `112.669M / actual 83.96% / fallback 6.72% / 459776 KB` |
| tcmalloc | 120.786M | 70.94 | 21.18 | 749,100 | `120.786M / actual 70.94% / fallback 21.18% / 749100 KB` |

Artifacts: [out_win_mt_remote](/C:/git/hakozuna-win/out_win_mt_remote)
