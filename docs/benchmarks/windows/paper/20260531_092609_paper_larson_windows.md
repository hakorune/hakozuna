# Paper-Aligned Windows Larson

Generated: 2026-05-31 09:26:09 +09:00

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
| crt | 55.574M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.574M` |
| hz3 | 58.708M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `58.708M` |
| hz4 | 68.578M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `68.578M` |
| hz5-policy | 56.225M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `56.225M` |
| hz6-strict-appcap | 0.005M | 36921 | 36966 | 0 | 0 | 12769 | 0 | 0 | 0 | 0 | 36966 | 37132 | 0 | 0 | `0.005M` |
| hz6-speed-appcap | 0.004M | 28769 | 28786 | 0 | 0 | 7027 | 0 | 0 | 0 | 0 | 28786 | 28877 | 0 | 0 | `0.004M` |
| hz6-rss-appcap | 0.004M | 32946 | 32974 | 0 | 0 | 9844 | 0 | 0 | 0 | 0 | 32974 | 33118 | 0 | 0 | `0.004M` |
| mimalloc | 54.286M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.286M` |
| tcmalloc | 50.073M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `50.073M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 47.868M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `47.868M` |
| hz3 | 54.122M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.122M` |
| hz4 | 54.672M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.672M` |
| hz5-policy | 44.915M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `44.915M` |
| hz6-strict-appcap | 62.438M | 3200 | 4587 | 0 | 0 | 626232070 | 0 | 0 | 0 | 0 | 4587 | 4591 | 0 | 0 | `62.438M` |
| hz6-speed-appcap | 65.440M | 3200 | 4591 | 0 | 0 | 656354453 | 0 | 0 | 0 | 0 | 4591 | 4593 | 0 | 0 | `65.440M` |
| hz6-rss-appcap | 64.859M | 3200 | 4593 | 0 | 0 | 650650311 | 0 | 0 | 0 | 0 | 4593 | 4597 | 0 | 0 | `64.859M` |
| mimalloc | 60.840M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `60.840M` |
| tcmalloc | 54.154M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.154M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
