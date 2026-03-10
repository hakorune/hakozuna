# Paper-Aligned Windows MT Remote

Generated: 2026-03-09 06:12:17 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L199)
- [private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md](/C:/git/hakozuna-win/private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md)

Windows native note:
- benchmark: `bench_random_mixed_mt_remote_compare`
- params: `threads=16 iters=2000000 ws=400 size=16..2048 remote_pct=90 ring_slots=65536`
- runs: `10`
- statistic: `median ops/s`
- note: paper originally reports `hz3 / mimalloc / tcmalloc`; this runner also records `crt` and `hz4`

| allocator | median ops/s | median actual remote % | median fallback % | runs |
| --- | ---: | ---: | ---: | --- |
| crt | 77.504M | 82.25 | 8.62 | `81.476M / actual 81.22% / fallback 9.76%, 76.421M / actual 82.46% / fallback 8.38%, 76.582M / actual 81.59% / fallback 9.35%, 79.770M / actual 82.57% / fallback 8.26%, 78.814M / actual 84.25% / fallback 6.39%, 76.459M / actual 82.72% / fallback 8.09%, 77.886M / actual 82.63% / fallback 8.19%, 77.122M / actual 79.94% / fallback 11.18%, 76.376M / actual 82.03% / fallback 8.86%, 79.138M / actual 80.59% / fallback 10.46%` |
| hz3 | 2.298M | 89.37 | 0.71 | `2.114M / actual 89.20% / fallback 0.90%, 2.249M / actual 90.00% / fallback 0.00%, 2.232M / actual 89.80% / fallback 0.23%, 2.367M / actual 88.75% / fallback 1.40%, 2.311M / actual 89.78% / fallback 0.25%, 2.273M / actual 89.64% / fallback 0.40%, 2.286M / actual 89.25% / fallback 0.84%, 2.331M / actual 89.48% / fallback 0.58%, 2.348M / actual 88.92% / fallback 1.20%, 2.427M / actual 88.47% / fallback 1.70%` |
| hz4 | 129.264M | 77.86 | 13.50 | `137.129M / actual 77.66% / fallback 13.71%, 124.291M / actual 78.06% / fallback 13.28%, 143.172M / actual 76.14% / fallback 15.41%, 127.696M / actual 74.86% / fallback 16.82%, 124.532M / actual 80.11% / fallback 11.00%, 129.268M / actual 76.18% / fallback 15.36%, 134.354M / actual 83.50% / fallback 7.23%, 128.255M / actual 82.23% / fallback 8.63%, 138.627M / actual 71.89% / fallback 20.12%, 129.261M / actual 78.06% / fallback 13.27%` |
| mimalloc | 109.682M | 80.10 | 11.02 | `119.454M / actual 78.28% / fallback 13.03%, 108.260M / actual 77.41% / fallback 13.99%, 108.145M / actual 83.79% / fallback 6.91%, 104.612M / actual 77.06% / fallback 14.39%, 112.793M / actual 81.06% / fallback 9.94%, 107.749M / actual 81.80% / fallback 9.12%, 109.666M / actual 81.06% / fallback 9.94%, 110.036M / actual 77.77% / fallback 13.59%, 109.697M / actual 79.13% / fallback 12.09%, 110.055M / actual 84.21% / fallback 6.43%` |
| tcmalloc | 124.014M | 71.97 | 20.04 | `121.260M / actual 70.86% / fallback 21.27%, 119.041M / actual 72.11% / fallback 19.88%, 120.662M / actual 74.16% / fallback 17.60%, 119.424M / actual 69.21% / fallback 23.10%, 126.643M / actual 71.25% / fallback 20.84%, 128.247M / actual 74.38% / fallback 17.36%, 126.180M / actual 72.52% / fallback 19.43%, 128.644M / actual 71.83% / fallback 20.20%, 129.849M / actual 75.08% / fallback 16.58%, 121.847M / actual 70.93% / fallback 21.19%` |

Artifacts: [out_win_mt_remote](/C:/git/hakozuna-win/out_win_mt_remote)

