# Paper-Aligned Windows MT Remote

Generated: 2026-03-09 18:50:15 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L199)
- [private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md](/C:/git/hakozuna-win/private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md)

Windows native note:
- benchmark: `bench_random_mixed_mt_remote_compare`
- params: `threads=16 iters=2000000 ws=400 size=16..2048 remote_pct=90 ring_slots=65536`
- runs: `10`
- statistic: `median ops/s`
- hz3 profile: `scale + S97-2 direct-map bucketize + skip_tail_null`
- note: paper originally reports `hz3 / mimalloc / tcmalloc`; this runner also records `crt` and `hz4`

| allocator | median ops/s | median actual remote % | median fallback % | runs |
| --- | ---: | ---: | ---: | --- |
| crt | 75.303M | 82.50 | 8.34 | `75.174M / actual 82.54% / fallback 8.29%, 76.691M / actual 80.36% / fallback 10.71%, 72.086M / actual 84.81% / fallback 5.78%, 76.768M / actual 82.45% / fallback 8.39%, 77.744M / actual 84.00% / fallback 6.67%, 72.352M / actual 82.31% / fallback 8.55%, 74.398M / actual 83.35% / fallback 7.39%, 75.433M / actual 79.34% / fallback 11.85%, 73.407M / actual 82.68% / fallback 8.14%, 75.839M / actual 80.66% / fallback 10.38%` |
| hz3 | 135.963M | 79.71 | 11.45 | `133.800M / actual 83.17% / fallback 7.59%, 140.910M / actual 81.75% / fallback 9.17%, 136.729M / actual 77.98% / fallback 13.36%, 132.219M / actual 81.51% / fallback 9.44%, 126.085M / actual 80.31% / fallback 10.77%, 135.198M / actual 78.15% / fallback 13.18%, 131.077M / actual 79.10% / fallback 12.12%, 138.157M / actual 81.09% / fallback 9.90%, 138.742M / actual 78.22% / fallback 13.10%, 137.598M / actual 77.61% / fallback 13.77%` |
| hz4 | 124.155M | 78.21 | 13.11 | `124.039M / actual 76.97% / fallback 14.48%, 122.469M / actual 77.26% / fallback 14.15%, 124.270M / actual 79.83% / fallback 11.30%, 126.022M / actual 78.62% / fallback 12.65%, 134.364M / actual 81.01% / fallback 9.99%, 130.559M / actual 77.80% / fallback 13.56%, 122.767M / actual 77.07% / fallback 14.37%, 122.092M / actual 83.09% / fallback 7.68%, 119.585M / actual 81.01% / fallback 9.99%, 127.538M / actual 76.70% / fallback 14.78%` |
| mimalloc | 111.701M | 80.85 | 10.18 | `112.325M / actual 80.86% / fallback 10.17%, 118.017M / actual 80.84% / fallback 10.18%, 108.001M / actual 82.81% / fallback 8.00%, 101.817M / actual 84.11% / fallback 6.54%, 113.603M / actual 78.42% / fallback 12.87%, 115.844M / actual 78.83% / fallback 12.42%, 112.651M / actual 82.54% / fallback 8.30%, 107.595M / actual 80.62% / fallback 10.43%, 111.077M / actual 82.81% / fallback 7.99%, 109.268M / actual 80.53% / fallback 10.52%` |
| tcmalloc | 123.360M | 71.41 | 20.67 | `129.773M / actual 73.08% / fallback 18.81%, 122.949M / actual 73.41% / fallback 18.43%, 120.475M / actual 70.21% / fallback 21.99%, 117.493M / actual 70.11% / fallback 22.10%, 125.118M / actual 71.22% / fallback 20.87%, 123.771M / actual 71.59% / fallback 20.46%, 126.216M / actual 70.10% / fallback 22.11%, 117.735M / actual 71.06% / fallback 21.05%, 119.915M / actual 74.97% / fallback 16.71%, 124.655M / actual 73.66% / fallback 18.16%` |

Artifacts: [out_win_mt_remote](/C:/git/hakozuna-win/out_win_mt_remote)

