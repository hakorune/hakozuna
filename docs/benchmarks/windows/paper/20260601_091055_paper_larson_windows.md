# Paper-Aligned Windows Larson

Generated: 2026-06-01 09:10:55 +09:00

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
- timeout: `60s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=1

| allocator | median ops/s | median peak_kb | route_miss | route_vis_lookup | route_vis_hit | route_vis_hit_local_owner | route_vis_hit_foreign_owner | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | source_owned_prepare | source_owned_route_hit_local_owner | source_owned_visibility_hit_local_owner | source_owned_visibility_hit_foreign_owner | source_owned_remote_free_attempt | source_owned_release | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | remote_free_attempt | remote_free_strict_block | remote_free_transfer_fail | route_rehome_attempt | route_rehome_success | route_rehome_fail | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 22.302M | 11,980 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 22.302M / 11980 KB |
| hz3 | 121.248M | 16,328 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 121.248M / 16328 KB |
| hz4 | 82.221M | 18,084 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 82.221M / 18084 KB |
| hz5-policy | 15.876M | 13,580 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 15.876M / 13580 KB |
| hz6-strict | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-speed | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-rss | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-strict-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-speed-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-rss-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-strict-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-speed-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-rss-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-strict-appcap | 8.607M | 336,616 | 5000 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 338 | 0 | 0 | 0 | 338 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 8.607M / 336616 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-speed-appcap | 4.824M | 300,692 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 25 | 0 | 0 | 0 | 25 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 5000 | 5000 | 0 | 29 | 5000 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 4.824M / 300692 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=29
  remote_free: attempt=5000 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-rss-appcap | 6.470M | 302,948 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 157 | 0 | 0 | 0 | 157 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 4899 | 4899 | 0 | 16 | 5000 | 0 | 101 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 6.470M / 302948 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=16
  remote_free: attempt=5000 strict_block=0 transfer_fail=101 rehome_attempt=0 rehome_success=0 rehome_fail=0
| mimalloc | 102.786M | 15,228 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 102.786M / 15228 KB |
| tcmalloc | 113.871M | 15,292 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 113.871M / 15292 KB |

## Worker-warmup control note

- Worker-warmup mode allocates the initial live set inside each worker thread after allocator thread setup and before the timer starts.
- This separates cross-owner warmup stress from same-owner small-object source placement.

## Larson worker-warmup stress T=1

| allocator | median ops/s | median peak_kb | route_miss | route_vis_lookup | route_vis_hit | route_vis_hit_local_owner | route_vis_hit_foreign_owner | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | source_owned_prepare | source_owned_route_hit_local_owner | source_owned_visibility_hit_local_owner | source_owned_visibility_hit_foreign_owner | source_owned_remote_free_attempt | source_owned_release | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | remote_free_attempt | remote_free_strict_block | remote_free_transfer_fail | route_rehome_attempt | route_rehome_success | route_rehome_fail | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 22.518M | 11,888 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 22.518M / 11888 KB |
| hz3 | 121.685M | 13,368 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 121.685M / 13368 KB |
| hz4 | 81.745M | 13,348 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 81.745M / 13348 KB |
| hz5-policy | 16.970M | 13,508 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 16.970M / 13508 KB |
| hz6-strict | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-speed | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-rss | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-strict-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-speed-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-rss-broad | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-strict-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-speed-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-rss-route4k | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-strict-appcap | 10.445M | 333,816 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 10380 | 0 | 0 | 0 | 10380 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 10.445M / 333816 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-speed-appcap | 9.406M | 300,700 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 651 | 0 | 0 | 0 | 651 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 9.406M / 300700 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-rss-appcap | 12.134M | 302,688 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 2596 | 0 | 0 | 0 | 2596 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 12.134M / 302688 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| mimalloc | 90.924M | 12,308 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 90.924M / 12308 KB |
| tcmalloc | 113.865M | 15,220 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 113.865M / 15220 KB |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=1

| allocator | median ops/s | median peak_kb | route_miss | route_vis_lookup | route_vis_hit | route_vis_hit_local_owner | route_vis_hit_foreign_owner | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | source_owned_prepare | source_owned_route_hit_local_owner | source_owned_visibility_hit_local_owner | source_owned_visibility_hit_foreign_owner | source_owned_remote_free_attempt | source_owned_release | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | remote_free_attempt | remote_free_strict_block | remote_free_transfer_fail | route_rehome_attempt | route_rehome_success | route_rehome_fail | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 29.937M | 5,024 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 29.937M / 5024 KB |
| hz3 | 140.648M | 6,880 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 140.648M / 6880 KB |
| hz4 | 103.224M | 12,604 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 103.224M / 12604 KB |
| hz5-policy | 22.176M | 5,172 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 22.176M / 5172 KB |
| hz6-strict | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-speed | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-rss | failed | n/a | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc1` |
| hz6-strict-broad | 16.354M | 10,460 | 200 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 20 | 0 | 0 | 0 | 20 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 16.354M / 10460 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-speed-broad | 12.731M | 9,056 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 7 | 0 | 0 | 0 | 7 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 200 | 200 | 0 | 7 | 200 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 12.731M / 9056 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=7
  remote_free: attempt=200 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-rss-broad | 14.903M | 9,144 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 24 | 0 | 0 | 0 | 24 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 200 | 200 | 0 | 7 | 200 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 14.903M / 9144 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=7
  remote_free: attempt=200 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-strict-route4k | 16.254M | 7,176 | 200 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 20 | 0 | 0 | 0 | 20 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 16.254M / 7176 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-speed-route4k | 12.908M | 5,748 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 7 | 0 | 0 | 0 | 7 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 200 | 200 | 0 | 7 | 200 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 12.908M / 5748 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=7
  remote_free: attempt=200 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-rss-route4k | 14.969M | 5,884 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 24 | 0 | 0 | 0 | 24 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 200 | 200 | 0 | 7 | 200 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 14.969M / 5884 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=7
  remote_free: attempt=200 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-strict-appcap | 16.179M | 293,980 | 200 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 20 | 0 | 0 | 0 | 20 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 16.179M / 293980 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-speed-appcap | 12.953M | 292,556 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 7 | 0 | 0 | 0 | 7 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 200 | 200 | 0 | 7 | 200 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 12.953M / 292556 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=7
  remote_free: attempt=200 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-rss-appcap | 14.749M | 292,664 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 24 | 0 | 0 | 0 | 24 | 0 | 0 | 0 | 0 | NA | NA | NA | NA | 200 | 200 | 0 | 7 | 200 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 14.749M / 292664 KB |
  prefill: local2p a=NA f=NA fb=NA; midpage a=NA f=NA fb=NA; large a=NA f=NA fb=NA; toy a=NA f=NA fb=NA
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=7
  remote_free: attempt=200 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| mimalloc | 138.300M | 6,204 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 138.300M / 6204 KB |
| tcmalloc | 145.577M | 9,300 | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 145.577M / 9300 KB |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
