# Paper-Aligned Windows Larson

Generated: 2026-05-31 09:12:20 +09:00

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

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 28.826M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `28.826M` |
| hz3 | 53.954M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `53.954M` |
| hz4 | 54.857M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.857M` |
| hz5-policy | 40.229M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `40.229M` |
| hz6-strict-appcap | 0.003M | 24796 | 24812 | 0 | 0 | NA | NA | NA | NA | 0 | 24812 | 24871 | 0 | 0 | `0.003M` |
| hz6-speed-appcap | 0.004M | 30928 | 30963 | 0 | 0 | NA | NA | NA | NA | 0 | 30963 | 31086 | 0 | 0 | `0.004M` |
| hz6-rss-appcap | 0.003M | 23098 | 23114 | 0 | 0 | NA | NA | NA | NA | 0 | 23114 | 23165 | 0 | 0 | `0.003M` |
| mimalloc | 52.777M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `52.777M` |
| tcmalloc | 50.188M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `50.188M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 44.975M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `44.975M` |
| hz3 | 55.076M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.076M` |
| hz4 | 58.332M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `58.332M` |
| hz5-policy | 50.738M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `50.738M` |
| hz6-strict-appcap | 61.804M | 3200 | 4587 | 0 | 0 | NA | NA | NA | NA | 0 | 4587 | 4588 | 0 | 0 | `61.804M` |
| hz6-speed-appcap | 61.206M | 3200 | 4592 | 0 | 0 | NA | NA | NA | NA | 0 | 4592 | 4594 | 0 | 0 | `61.206M` |
| hz6-rss-appcap | 59.593M | 3200 | 4587 | 0 | 0 | NA | NA | NA | NA | 0 | 4587 | 4589 | 0 | 0 | `59.593M` |
| mimalloc | 58.274M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `58.274M` |
| tcmalloc | 58.973M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `58.973M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
