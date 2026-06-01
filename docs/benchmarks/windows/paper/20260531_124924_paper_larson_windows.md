# Paper-Aligned Windows Larson

Generated: 2026-05-31 12:49:24 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)

Windows native note:
- benchmark: `bench_larson_compare`
- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`
- compact control (optional): `chunks=400`
- thread sweep: `1, 4, 8, 16`
- runs: `1`
- timeout: `30s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=16

| allocator | median ops/s | route_miss | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 54.688M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz3 | 60.804M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz4 | 51.829M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz5-policy | 43.810M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz6-strict-appcap | failed | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-speed-appcap | failed | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-rss-appcap | failed | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| mimalloc | 51.781M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| tcmalloc | 54.500M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 50.918M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz3 | 57.338M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz4 | 57.966M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz5-policy | 43.824M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz6-strict-appcap | 61.791M | 3200 | 4590 | 0 | 0 | 0 | 4590 | 0 | 0 | 4590 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=620206824 transfer_reuse=0 prefill_reuse=3795 source_prefill=4590 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 620210619 | 0 | 0 | 0 | 53 | 0 | 53 | 0 | 3795 | 4590 | 0 | 0 | 4590 | 4592 | 0 | `0` |
| hz6-speed-appcap | 62.475M | 3200 | 5120 | 0 | 0 | 0 | 5120 | 0 | 0 | 5120 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=627255970 transfer_reuse=0 prefill_reuse=304 source_prefill=5120 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 627256274 | 0 | 0 | 0 | 16 | 0 | 16 | 0 | 304 | 5120 | 0 | 0 | 5120 | 5121 | 0 | `0` |
| hz6-rss-appcap | 59.647M | 3200 | 4652 | 0 | 0 | 0 | 4652 | 0 | 0 | 4652 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=598644241 transfer_reuse=0 prefill_reuse=1103 source_prefill=4652 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 598645344 | 0 | 0 | 0 | 20 | 0 | 20 | 0 | 1103 | 4652 | 0 | 0 | 4652 | 4656 | 0 | `0` |
| mimalloc | 64.322M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| tcmalloc | 62.768M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
