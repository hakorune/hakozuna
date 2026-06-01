# Paper-Aligned Windows MT Remote

Generated: 2026-06-01 04:13:39 +09:00

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
- note: paper originally reports `hz3 / mimalloc / tcmalloc`; this runner also records `crt` and `hz4`

| allocator | median ops/s | median actual remote % | median fallback % | runs |
| --- | ---: | ---: | ---: | --- |
| crt | 74.105M | 82.66 | 8.16 | `74.105M / actual 82.66% / fallback 8.16%` |
| hz3 | 125.610M | 82.44 | 8.41 | `125.610M / actual 82.44% / fallback 8.41%` |
| hz4 | 131.889M | 76.93 | 14.52 | `131.889M / actual 76.93% / fallback 14.52%` |
| hz5-policy | 70.344M | 84.35 | 6.28 | `70.344M / actual 84.35% / fallback 6.28%` |
| mimalloc | 109.976M | 82.35 | 8.51 | `109.976M / actual 82.35% / fallback 8.51%` |
| tcmalloc | 122.211M | 74.69 | 17.01 | `122.211M / actual 74.69% / fallback 17.01%` |
| hz6-strict | failed | n/a | n/a | `failed:timeout900` |
| hz6-speed | failed | n/a | n/a | `failed:timeout900` |
| hz6-rss | failed | n/a | n/a | `failed:timeout900` |
| hz6-strict-broad | failed | n/a | n/a | `failed:timeout900` |
| hz6-speed-broad | failed | n/a | n/a | `failed:timeout900` |
| hz6-rss-broad | failed | n/a | n/a | `failed:timeout900` |
| hz6-strict-appcap | 0.328M | 6.57 | 92.70 | `0.328M / actual 6.57% / fallback 92.70%` |
| hz6-speed-appcap | 0.274M | 6.57 | 92.70 | `0.274M / actual 6.57% / fallback 92.70%` |
| hz6-rss-appcap | 0.288M | 6.63 | 92.63 | `0.288M / actual 6.63% / fallback 92.63%` |

Artifacts: [out_win_mt_remote](/C:/git/hakozuna-win/out_win_mt_remote)
