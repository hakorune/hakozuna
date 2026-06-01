# Paper-Aligned Windows Larson

Generated: 2026-05-31 07:53:05 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)

Windows native note:
- benchmark: `bench_larson_compare`
- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`
- compact control (optional): `chunks=400`
- thread sweep: `1, 4, 8, 16`
- runs: `1`
- timeout: `120s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 57.420M | `57.420M` |
| hz3 | 55.662M | `55.662M` |
| hz4 | 57.059M | `57.059M` |
| hz5-policy | 55.383M | `55.383M` |
| hz6-strict-appcap | 0.100M | `0.100M` |
| hz6-speed-appcap | 0.007M | `0.007M` |
| hz6-rss-appcap | 0.004M | `0.004M` |
| mimalloc | 56.061M | `56.061M` |
| tcmalloc | 57.384M | `57.384M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 53.186M | `53.186M` |
| hz3 | 59.093M | `59.093M` |
| hz4 | 59.067M | `59.067M` |
| hz5-policy | 49.852M | `49.852M` |
| hz6-strict-appcap | 62.185M | `62.185M` |
| hz6-speed-appcap | 67.268M | `67.268M` |
| hz6-rss-appcap | 67.237M | `67.237M` |
| mimalloc | 53.496M | `53.496M` |
| tcmalloc | 54.097M | `54.097M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
