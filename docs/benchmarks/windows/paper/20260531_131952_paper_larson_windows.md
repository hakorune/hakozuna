# Paper-Aligned Windows Larson

Generated: 2026-05-31 13:19:52 +09:00

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

| allocator | median ops/s | route_miss | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 58.230M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz3 | 61.286M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz4 | 60.897M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz5-policy | 53.087M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz6-strict-appcap | 0.005M | 37716 | 37812 | 0 | 0 | 0 | 37812 | 0 | 0 | 37812 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=22751 transfer_reuse=0 prefill_reuse=28827 source_prefill=37812 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 51578 | 0 | 0 | 0 | 599 | 0 | 599 | 0 | 50979 | 599 | 0 | 28827 | 37812 | 0 | 0 | 37812 | 37974 | 0 | `0` |
| hz6-speed-appcap | 0.007M | 46117 | 46592 | 0 | 0 | 0 | 46592 | 0 | 0 | 46592 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=67321 transfer_reuse=0 prefill_reuse=2896 source_prefill=46592 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 70217 | 0 | 0 | 0 | 16 | 0 | 16 | 0 | 70201 | 16 | 0 | 2896 | 46592 | 0 | 0 | 46592 | 46870 | 0 | `0` |
| hz6-rss-appcap | 0.003M | 25875 | 25988 | 0 | 0 | 0 | 25988 | 0 | 0 | 25988 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=25013 transfer_reuse=0 prefill_reuse=6347 source_prefill=25988 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 31360 | 0 | 0 | 0 | 50 | 0 | 50 | 0 | 31310 | 50 | 0 | 6347 | 25988 | 0 | 0 | 25988 | 26078 | 0 | `0` |
| mimalloc | 53.404M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| tcmalloc | 53.885M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 52.744M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz3 | 60.098M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz4 | 60.497M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz5-policy | 49.475M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz6-strict-appcap | 62.118M | 3200 | 4587 | 0 | 0 | 0 | 4587 | 0 | 0 | 4587 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=623358883 transfer_reuse=0 prefill_reuse=3792 source_prefill=4587 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 623362675 | 0 | 0 | 0 | 53 | 0 | 53 | 0 | 623362622 | 53 | 0 | 3792 | 4587 | 0 | 0 | 4587 | 4591 | 0 | `0` |
| hz6-speed-appcap | 58.548M | 3200 | 5120 | 0 | 0 | 0 | 5120 | 0 | 0 | 5120 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=587854644 transfer_reuse=0 prefill_reuse=304 source_prefill=5120 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 587854948 | 0 | 0 | 0 | 16 | 0 | 16 | 0 | 587854932 | 16 | 0 | 304 | 5120 | 0 | 0 | 5120 | 5121 | 0 | `0` |
| hz6-rss-appcap | 60.723M | 3200 | 4652 | 0 | 0 | 0 | 4652 | 0 | 0 | 4652 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=609077876 transfer_reuse=0 prefill_reuse=1103 source_prefill=4652 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 609078979 | 0 | 0 | 0 | 20 | 0 | 20 | 0 | 609078959 | 20 | 0 | 1103 | 4652 | 0 | 0 | 4652 | 4657 | 0 | `0` |
| mimalloc | 69.288M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| tcmalloc | 55.323M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
