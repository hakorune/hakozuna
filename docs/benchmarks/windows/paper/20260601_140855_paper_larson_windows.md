# Paper-Aligned Windows Larson

Generated: 2026-06-01 14:08:55 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)

Windows native note:
- benchmark: `bench_larson_compare`
- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`
- compact control (optional): `chunks=400`
- worker-warmup control (optional): same chunks, but each worker owns its warmup allocations before the timer starts
- shared route visibility diagnostics: `route_visibility_lookup / hit / miss / probe_total / probe_max`
- shared route owner diagnostics: `route_visibility_hit_local_owner / route_visibility_hit_foreign_owner`
- transfer backlog diagnostics: `transfer_current / transfer_current_max`
- remote free diagnostics: `remote_free_attempt / strict_owner_block / transfer_fail / route_rehome_attempt / route_rehome_success / route_rehome_fail`
- thread sweep: `1, 4, 8, 16`
- runs: `1`
- timeout: `120s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=16

| allocator | median ops/s | median peak_kb | route_miss | route_vis_lookup | route_vis_hit | route_vis_hit_local_owner | route_vis_hit_foreign_owner | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | source_owned_prepare | source_owned_route_hit_local_owner | source_owned_visibility_hit_local_owner | source_owned_visibility_hit_foreign_owner | source_owned_remote_free_attempt | source_owned_release | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | remote_free_attempt | remote_free_strict_block | remote_free_transfer_fail | route_rehome_attempt | route_rehome_success | route_rehome_fail | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 49.662M | 158,204 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 49.662M / 158204 KB |
| hz3 | 53.435M | 175,420 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 53.435M / 175420 KB |
| hz4 | 54.546M | 192,044 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 54.546M / 192044 KB |
| hz5-policy | 47.485M | 183,180 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 47.485M / 183180 KB |
| hz6-strict | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-speed | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-rss | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-strict-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-speed-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-rss-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-strict-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-speed-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-rss-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-strict-appcap | 0.003M | 2,397,268 | 27018 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1715 | 0 | 0 | 0 | 1715 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0.003M / 2397268 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-speed-appcap | 0.001M | 1,866,232 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 69 | 0 | 0 | 0 | 69 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 12718 | 12465 | 253 | 46 | 12718 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0.001M / 1866232 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=253 max=46
  remote_free: attempt=12718 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-rss-appcap | 0.001M | 1,984,192 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 576 | 0 | 0 | 0 | 576 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 12351 | 12177 | 174 | 16 | 13198 | 0 | 847 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0.001M / 1984192 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=174 max=16
  remote_free: attempt=13198 strict_block=0 transfer_fail=847 rehome_attempt=0 rehome_success=0 rehome_fail=0
| mimalloc | 55.396M | 155,148 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 55.396M / 155148 KB |
| tcmalloc | 52.887M | 112,320 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 52.887M / 112320 KB |

## Worker-warmup control note

- Worker-warmup mode allocates the initial live set inside each worker thread after allocator thread setup and before the timer starts.
- This separates cross-owner warmup stress from same-owner small-object source placement.

## Larson worker-warmup stress T=16

| allocator | median ops/s | median peak_kb | route_miss | route_vis_lookup | route_vis_hit | route_vis_hit_local_owner | route_vis_hit_foreign_owner | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | source_owned_prepare | source_owned_route_hit_local_owner | source_owned_visibility_hit_local_owner | source_owned_visibility_hit_foreign_owner | source_owned_remote_free_attempt | source_owned_release | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | remote_free_attempt | remote_free_strict_block | remote_free_transfer_fail | route_rehome_attempt | route_rehome_success | route_rehome_fail | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 53.443M | 134,660 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 53.443M / 134660 KB |
| hz3 | 54.121M | 129,036 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 54.121M / 129036 KB |
| hz4 | 55.259M | 146,260 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 55.259M / 146260 KB |
| hz5-policy | 42.987M | 155,640 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 42.987M / 155640 KB |
| hz6-strict | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-speed | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-rss | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-strict-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-speed-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-rss-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-strict-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-speed-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-rss-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-strict-appcap | 37.475M | 2,397,364 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 165566 | 0 | 0 | 0 | 165566 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 37.475M / 2397364 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-speed-appcap | 47.763M | 1,868,684 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 10380 | 0 | 0 | 0 | 10380 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 47.763M / 1868684 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-rss-appcap | 41.689M | 1,900,744 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 41413 | 0 | 0 | 0 | 41413 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 41.689M / 1900744 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| mimalloc | 56.856M | 111,760 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 56.856M / 111760 KB |
| tcmalloc | 57.529M | 112,092 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 57.529M / 112092 KB |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
