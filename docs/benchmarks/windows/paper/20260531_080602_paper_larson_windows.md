# Paper-Aligned Windows Larson

Generated: 2026-05-31 08:06:02 +09:00

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

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | alloc_fail | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 50.579M | NA | NA | NA | NA | NA | `50.579M` |
| hz3 | 53.427M | NA | NA | NA | NA | NA | `53.427M` |
| hz4 | 54.828M | NA | NA | NA | NA | NA | `54.828M` |
| hz5-policy | 45.383M | NA | NA | NA | NA | NA | `45.383M` |
| hz6-strict-appcap | 0.004M | NA | NA | NA | NA | NA | `0.004M` |
| hz6-speed-appcap | 0.004M | NA | NA | NA | NA | NA | `0.004M` |
| hz6-rss-appcap | 0.005M | NA | NA | NA | NA | NA | `0.005M` |
| mimalloc | 51.562M | NA | NA | NA | NA | NA | `51.562M` |
| tcmalloc | 50.997M | NA | NA | NA | NA | NA | `50.997M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | alloc_fail | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 48.674M | NA | NA | NA | NA | NA | `48.674M` |
| hz3 | 57.119M | NA | NA | NA | NA | NA | `57.119M` |
| hz4 | 60.414M | NA | NA | NA | NA | NA | `60.414M` |
| hz5-policy | 53.386M | NA | NA | NA | NA | NA | `53.386M` |
| hz6-strict-appcap | 60.243M | NA | NA | NA | NA | NA | `60.243M` |
| hz6-speed-appcap | 61.411M | NA | NA | NA | NA | NA | `61.411M` |
| hz6-rss-appcap | 62.011M | NA | NA | NA | NA | NA | `62.011M` |
| mimalloc | 55.755M | NA | NA | NA | NA | NA | `55.755M` |
| tcmalloc | 55.825M | NA | NA | NA | NA | NA | `55.825M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
