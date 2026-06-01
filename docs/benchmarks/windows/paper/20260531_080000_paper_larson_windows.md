# Paper-Aligned Windows Larson

Generated: 2026-05-31 08:00:00 +09:00

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
| crt | 50.281M | `50.281M` |
| hz3 | 52.456M | `52.456M` |
| hz4 | 55.126M | `55.126M` |
| hz5-policy | 45.861M | `45.861M` |
| hz6-strict-appcap | 0.004M | `0.004M` |
| hz6-speed-appcap | 0.004M | `0.004M` |
| hz6-rss-appcap | 0.004M | `0.004M` |
| mimalloc | 51.842M | `51.842M` |
| tcmalloc | 51.665M | `51.665M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 48.555M | `48.555M` |
| hz3 | 54.551M | `54.551M` |
| hz4 | 55.930M | `55.930M` |
| hz5-policy | 45.323M | `45.323M` |
| hz6-strict-appcap | 66.930M | `66.930M` |
| hz6-speed-appcap | 68.277M | `68.277M` |
| hz6-rss-appcap | 68.015M | `68.015M` |
| mimalloc | 53.443M | `53.443M` |
| tcmalloc | 53.498M | `53.498M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
