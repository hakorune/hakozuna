# Paper-Aligned Windows Larson

Generated: 2026-05-31 07:39:19 +09:00

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
| crt | 51.294M | `51.294M` |
| hz3 | 62.915M | `62.915M` |
| hz4 | 57.025M | `57.025M` |
| hz5-policy | 47.832M | `47.832M` |
| hz6-strict-appcap | 0.003M | `0.003M` |
| hz6-speed-appcap | 0.004M | `0.004M` |
| hz6-rss-appcap | 0.004M | `0.004M` |
| mimalloc | 55.551M | `55.551M` |
| tcmalloc | 71.162M | `71.162M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 65.888M | `65.888M` |
| hz3 | 57.764M | `57.764M` |
| hz4 | 59.868M | `59.868M` |
| hz5-policy | 50.483M | `50.483M` |
| hz6-strict-appcap | 62.412M | `62.412M` |
| hz6-speed-appcap | 61.648M | `61.648M` |
| hz6-rss-appcap | 63.976M | `63.976M` |
| mimalloc | 58.982M | `58.982M` |
| tcmalloc | 60.829M | `60.829M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
