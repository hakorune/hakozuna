# Paper-Aligned Windows Larson

Generated: 2026-05-31 16:51:09 +09:00

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
- timeout: `40s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=16

| allocator | median ops/s | route_miss | route_vis_lookup | route_vis_hit | route_vis_hit_local_owner | route_vis_hit_foreign_owner | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | remote_free_attempt | remote_free_strict_block | remote_free_transfer_fail | route_rehome_attempt | route_rehome_success | route_rehome_fail | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 48.726M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 48.726M |
| hz3 | 53.041M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 53.041M |
| hz4 | 54.254M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 54.254M |
| hz5-policy | 44.400M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 44.400M |
| hz6-strict-appcap | failed | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `failed:rc124` |
| hz6-speed-appcap | 0.001M | 0 | 13575 | 13575 | 1055 | 12520 | 0 | 13575 | 1 | 1080 | 0 | 0 | 0 | 1080 | 0 | 0 | 1080 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=1313 transfer_reuse=12250 prefill_reuse=69 source_prefill=1080 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 12520 | 12250 | 270 | 49 | 12520 | 0 | 0 | 12520 | 12520 | 0 | 1382 | 0 | 12250 | 0 | 0 | 479 | 0 | 479 | 13153 | 0 | 479 | 1080 | 1080 | 0 | 0 | 1080 | 1080 | 0 | 0 | 0.001M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=1080 f=1080 fb=0
  visibility: lookup=13575 hit=13575 miss=0 probe_total=13575 probe_max=1
  visibility owner: local=1055 foreign=12520
  transfer: current=270 max=49
  remote_free: attempt=12520 strict_block=0 transfer_fail=0 rehome_attempt=12520 rehome_success=12520 rehome_fail=0
| hz6-rss-appcap | 0.001M | 0 | 13828 | 13828 | 1072 | 12756 | 0 | 13828 | 1 | 1218 | 0 | 0 | 0 | 1218 | 0 | 0 | 1218 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=1596 transfer_reuse=11772 prefill_reuse=551 source_prefill=1218 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 11953 | 11772 | 181 | 16 | 12756 | 0 | 803 | 12756 | 11953 | 803 | 2147 | 0 | 11772 | 0 | 0 | 12611 | 0 | 12611 | 1308 | 0 | 12611 | 1218 | 1218 | 0 | 0 | 1218 | 1218 | 0 | 0 | 0.001M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=1218 f=1218 fb=0
  visibility: lookup=13828 hit=13828 miss=0 probe_total=13828 probe_max=1
  visibility owner: local=1072 foreign=12756
  transfer: current=181 max=16
  remote_free: attempt=12756 strict_block=0 transfer_fail=803 rehome_attempt=12756 rehome_success=11953 rehome_fail=803
| mimalloc | 50.685M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 50.685M |
| tcmalloc | 50.975M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 50.975M |

## Worker-warmup control note

- Worker-warmup mode allocates the initial live set inside each worker thread after allocator thread setup and before the timer starts.
- This separates cross-owner warmup stress from same-owner small-object source placement.

## Larson worker-warmup stress T=16

| allocator | median ops/s | route_miss | route_vis_lookup | route_vis_hit | route_vis_hit_local_owner | route_vis_hit_foreign_owner | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | remote_free_attempt | remote_free_strict_block | remote_free_transfer_fail | route_rehome_attempt | route_rehome_success | route_rehome_fail | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 47.993M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 47.993M |
| hz3 | 51.058M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 51.058M |
| hz4 | 52.950M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 52.950M |
| hz5-policy | 40.813M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 40.813M |
| hz6-strict-appcap | 35.373M | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 165563 | 0 | 0 | 0 | 165563 | 0 | 0 | 165563 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=372461791 transfer_reuse=0 prefill_reuse=165563 source_prefill=165563 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 372627354 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 372627354 | 0 | 0 | 165563 | 165563 | 0 | 0 | 165563 | 168968 | 0 | 0 | 35.373M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=165563 f=165563 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-speed-appcap | 40.485M | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 166064 | 0 | 0 | 0 | 166064 | 0 | 0 | 166064 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=426602664 transfer_reuse=0 prefill_reuse=10379 source_prefill=166064 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 426613043 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 426613043 | 0 | 0 | 166064 | 166064 | 0 | 0 | 166064 | 169428 | 0 | 0 | 40.485M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=166064 f=166064 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| hz6-rss-appcap | 40.080M | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 165660 | 0 | 0 | 0 | 165660 | 0 | 0 | 165660 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=422392106 transfer_reuse=0 prefill_reuse=41415 source_prefill=165660 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 422433521 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 422433521 | 0 | 0 | 165660 | 165660 | 0 | 0 | 165660 | 169059 | 0 | 0 | 40.080M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=165660 f=165660 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  visibility owner: local=0 foreign=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0 rehome_attempt=0 rehome_success=0 rehome_fail=0
| mimalloc | 50.829M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 50.829M |
| tcmalloc | 50.943M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 50.943M |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
