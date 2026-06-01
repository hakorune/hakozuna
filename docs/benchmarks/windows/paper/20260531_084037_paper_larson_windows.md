# Paper-Aligned Windows Larson

Generated: 2026-05-31 08:40:37 +09:00

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

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 51.990M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `51.990M` |
| hz3 | 56.075M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `56.075M` |
| hz4 | 55.811M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.811M` |
| hz5-policy | 46.778M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `46.778M` |
| hz6-strict-appcap | 0.004M | 28978 | 28998 | 0 | 0 | 0 | NA | NA | NA | NA | `0.004M` |
| hz6-speed-appcap | 0.003M | 28358 | 28376 | 0 | 0 | 0 | NA | NA | NA | NA | `0.003M` |
| hz6-rss-appcap | 0.003M | 27353 | 27366 | 0 | 0 | 0 | NA | NA | NA | NA | `0.003M` |
| mimalloc | 52.314M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `52.314M` |
| tcmalloc | 53.383M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `53.383M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 50.555M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `50.555M` |
| hz3 | 55.610M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.610M` |
| hz4 | 57.247M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `57.247M` |
| hz5-policy | 47.492M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `47.492M` |
| hz6-strict-appcap | 66.273M | 3200 | 4591 | 0 | 0 | 0 | NA | NA | NA | NA | `66.273M` |
| hz6-speed-appcap | 65.318M | 3200 | 4591 | 0 | 0 | 0 | NA | NA | NA | NA | `65.318M` |
| hz6-rss-appcap | 65.663M | 3200 | 4591 | 0 | 0 | 0 | NA | NA | NA | NA | `65.663M` |
| mimalloc | 54.171M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.171M` |
| tcmalloc | 56.269M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `56.269M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
