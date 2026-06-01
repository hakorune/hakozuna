# Paper-Aligned Windows Larson

Generated: 2026-05-31 11:39:21 +09:00

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

| allocator | median ops/s | route_miss | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 50.261M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `50.261M` |
| hz3 | 54.078M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `54.078M` |
| hz4 | 53.458M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `53.458M` |
| hz5-policy | 46.435M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `46.435M` |
| hz6-strict-appcap | 0.004M | 29499 | 29610 | 0 | 0 | 0 | 29610 | 0 | 0 | 29610 | 0 | 0 | 0 | 37064 | 0 | 0 | 0 | 525 | 0 | 525 | 0 | 21735 | 29610 | 0 | 0 | 29610 | 29707 | 0 | 0 | `0.004M` |
| hz6-speed-appcap | 0.004M | 28929 | 29504 | 0 | 0 | 0 | 29504 | 0 | 0 | 29504 | 0 | 0 | 0 | 36048 | 0 | 0 | 0 | 16 | 0 | 16 | 0 | 1828 | 29504 | 0 | 0 | 29504 | 29615 | 0 | 0 | `0.004M` |
| hz6-rss-appcap | 0.003M | 27152 | 27264 | 0 | 0 | 0 | 27264 | 0 | 0 | 27264 | 0 | 0 | 0 | 33235 | 0 | 0 | 0 | 57 | 0 | 57 | 0 | 6645 | 27264 | 0 | 0 | 27264 | 27355 | 0 | 0 | `0.003M` |
| mimalloc | 55.191M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.191M` |
| tcmalloc | 55.420M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `55.420M` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
