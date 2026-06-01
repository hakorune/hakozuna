# Paper-Aligned Windows Larson

Generated: 2026-05-31 08:47:38 +09:00

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
| crt | 51.941M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `51.941M` |
| hz3 | 53.975M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `53.975M` |
| hz4 | 54.986M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.986M` |
| hz5-policy | 54.350M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.350M` |
| hz6-strict-appcap | 0.003M | 24717 | 24737 | 0 | 0 | 0 | 24737 | 24807 | 0 | 0 | `0.003M` |
| hz6-speed-appcap | 0.002M | 20688 | 20706 | 0 | 0 | 0 | 20706 | 20760 | 0 | 0 | `0.002M` |
| hz6-rss-appcap | 0.002M | 19925 | 19932 | 0 | 0 | 0 | 19932 | 19979 | 0 | 0 | `0.002M` |
| mimalloc | 50.348M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `50.348M` |
| tcmalloc | 53.981M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `53.981M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 52.213M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `52.213M` |
| hz3 | 55.397M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.397M` |
| hz4 | 57.549M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `57.549M` |
| hz5-policy | 49.134M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `49.134M` |
| hz6-strict-appcap | 60.345M | 3200 | 4587 | 0 | 0 | 0 | 4587 | 4591 | 0 | 0 | `60.345M` |
| hz6-speed-appcap | 61.487M | 3200 | 4587 | 0 | 0 | 0 | 4587 | 4589 | 0 | 0 | `61.487M` |
| hz6-rss-appcap | 61.091M | 3200 | 4587 | 0 | 0 | 0 | 4587 | 4587 | 0 | 0 | `61.091M` |
| mimalloc | 56.343M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `56.343M` |
| tcmalloc | 56.108M | NA | NA | NA | NA | NA | NA | NA | NA | NA | `56.108M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
