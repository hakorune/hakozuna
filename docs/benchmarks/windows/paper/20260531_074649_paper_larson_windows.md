# Paper-Aligned Windows Larson

Generated: 2026-05-31 07:46:49 +09:00

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
| crt | 49.510M | `49.510M` |
| hz3 | 55.307M | `55.307M` |
| hz4 | 59.396M | `59.396M` |
| hz5-policy | 51.242M | `51.242M` |
| hz6-strict-appcap | 0.004M | `0.004M` |
| hz6-speed-appcap | 0.006M | `0.006M` |
| hz6-rss-appcap | 0.004M | `0.004M` |
| mimalloc | 50.291M | `50.291M` |
| tcmalloc | 50.569M | `50.569M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | runs |
| --- | ---: | --- |
| crt | 49.269M | `49.269M` |
| hz3 | 59.560M | `59.560M` |
| hz4 | 59.216M | `59.216M` |
| hz5-policy | 50.718M | `50.718M` |
| hz6-strict-appcap | 62.267M | `62.267M` |
| hz6-speed-appcap | 62.635M | `62.635M` |
| hz6-rss-appcap | 61.127M | `61.127M` |
| mimalloc | 58.177M | `58.177M` |
| tcmalloc | 57.118M | `57.118M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
