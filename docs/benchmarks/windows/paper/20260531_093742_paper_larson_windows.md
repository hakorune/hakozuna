# Paper-Aligned Windows Larson

Generated: 2026-05-31 09:37:42 +09:00

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

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 52.648M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `52.648M` |
| hz3 | 55.772M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.772M` |
| hz4 | 57.040M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `57.040M` |
| hz5-policy | 49.111M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `49.111M` |
| hz6-strict-appcap | 0.004M | 33879 | 33910 | 0 | 0 | 10392 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 33910 | 34057 | 0 | 0 | `0.004M` |
| hz6-speed-appcap | 0.003M | 26176 | 26196 | 0 | 0 | 5575 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 26196 | 26299 | 0 | 0 | `0.003M` |
| hz6-rss-appcap | 0.004M | 33862 | 33899 | 0 | 0 | 10304 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 33899 | 34039 | 0 | 0 | `0.004M` |
| mimalloc | 57.149M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `57.149M` |
| tcmalloc | 58.697M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `58.697M` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 58.875M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `58.875M` |
| hz3 | 64.177M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `64.177M` |
| hz4 | 59.293M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `59.293M` |
| hz5-policy | 47.765M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `47.765M` |
| hz6-strict-appcap | 59.368M | 3200 | 4587 | 0 | 0 | 594879365 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 4587 | 4588 | 0 | 0 | `59.368M` |
| hz6-speed-appcap | 60.058M | 3200 | 4587 | 0 | 0 | 602735474 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 4587 | 4593 | 0 | 0 | `60.058M` |
| hz6-rss-appcap | 60.716M | 3200 | 4587 | 0 | 0 | 609288410 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 4587 | 4590 | 0 | 0 | `60.716M` |
| mimalloc | 56.356M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `56.356M` |
| tcmalloc | 57.328M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `57.328M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
