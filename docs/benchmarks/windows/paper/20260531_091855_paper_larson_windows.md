# Paper-Aligned Windows Larson

Generated: 2026-05-31 09:18:55 +09:00

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
| crt | 50.651M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `50.651M` |
| hz3 | 60.735M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `60.735M` |
| hz4 | 54.167M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.167M` |
| hz5-policy | 50.613M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `50.613M` |
| hz6-strict-appcap | 0.007M | 45668 | 45739 | 0 | 0 | NA | NA | NA | NA | 0 | 45739 | 46017 | 0 | 0 | `0.007M` |
| hz6-speed-appcap | 0.004M | 29283 | 29307 | 0 | 0 | NA | NA | NA | NA | 0 | 29307 | 29419 | 0 | 0 | `0.004M` |
| hz6-rss-appcap | 0.004M | 29185 | 29205 | 0 | 0 | NA | NA | NA | NA | 0 | 29205 | 29309 | 0 | 0 | `0.004M` |
| mimalloc | 52.567M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `52.567M` |
| tcmalloc | 51.741M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `51.741M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 50.319M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `50.319M` |
| hz3 | 59.012M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `59.012M` |
| hz4 | 58.981M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `58.981M` |
| hz5-policy | 48.809M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `48.809M` |
| hz6-strict-appcap | 63.127M | 3200 | 4587 | 0 | 0 | NA | NA | NA | NA | 0 | 4587 | 4591 | 0 | 0 | `63.127M` |
| hz6-speed-appcap | 64.313M | 3200 | 4591 | 0 | 0 | NA | NA | NA | NA | 0 | 4591 | 4595 | 0 | 0 | `64.313M` |
| hz6-rss-appcap | 66.289M | 3200 | 4593 | 0 | 0 | NA | NA | NA | NA | 0 | 4593 | 4596 | 0 | 0 | `66.289M` |
| mimalloc | 57.079M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `57.079M` |
| tcmalloc | 52.277M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `52.277M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
