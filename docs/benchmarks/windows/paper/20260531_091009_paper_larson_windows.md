# Paper-Aligned Windows Larson

Generated: 2026-05-31 09:10:09 +09:00

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
| crt | 52.510M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `52.510M` |
| hz3 | 59.063M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `59.063M` |
| hz4 | 54.793M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.793M` |
| hz5-policy | 44.757M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `44.757M` |
| hz6-strict-appcap | 0.003M | 27462 | 27482 | 0 | 0 | NA | NA | NA | NA | 0 | 27482 | 27567 | 0 | 0 | `0.003M` |
| hz6-speed-appcap | 0.003M | 26461 | 26483 | 0 | 0 | NA | NA | NA | NA | 0 | 26483 | 26574 | 0 | 0 | `0.003M` |
| hz6-rss-appcap | 0.005M | 34749 | 34782 | 0 | 0 | NA | NA | NA | NA | 0 | 34782 | 34963 | 0 | 0 | `0.005M` |
| mimalloc | 51.878M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `51.878M` |
| tcmalloc | 53.265M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `53.265M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 37.669M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `37.669M` |
| hz3 | 55.866M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.866M` |
| hz4 | 56.347M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `56.347M` |
| hz5-policy | 48.626M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `48.626M` |
| hz6-strict-appcap | 61.219M | 3200 | 4587 | 0 | 0 | NA | NA | NA | NA | 0 | 4587 | 4587 | 0 | 0 | `61.219M` |
| hz6-speed-appcap | 55.589M | 3200 | 4586 | 0 | 0 | NA | NA | NA | NA | 0 | 4586 | 4588 | 0 | 0 | `55.589M` |
| hz6-rss-appcap | 60.095M | 3200 | 4586 | 0 | 0 | NA | NA | NA | NA | 0 | 4586 | 4588 | 0 | 0 | `60.095M` |
| mimalloc | 51.427M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `51.427M` |
| tcmalloc | 51.035M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `51.035M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
