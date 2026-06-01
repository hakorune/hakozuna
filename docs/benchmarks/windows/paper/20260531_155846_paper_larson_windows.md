# Paper-Aligned Windows Larson

Generated: 2026-05-31 15:58:46 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)

Windows native note:
- benchmark: `bench_larson_compare`
- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`
- compact control (optional): `chunks=400`
- worker-warmup control (optional): same chunks, but each worker owns its warmup allocations before the timer starts
- shared route visibility diagnostics: `route_visibility_lookup / hit / miss / probe_total / probe_max`
- transfer backlog diagnostics: `transfer_current / transfer_current_max`
- thread sweep: `1, 4, 8, 16`
- runs: `1`
- timeout: `60s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=16

| allocator | median ops/s | route_miss | route_vis_lookup | route_vis_hit | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 47.194M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 47.194M |
| hz3 | 52.880M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 52.880M |
| hz4 | 59.970M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 59.970M |
| hz5-policy | 52.243M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 52.243M |
| hz6-strict-appcap | 0.006M | 40253 | 0 | 0 | 0 | 0 | 0 | 40383 | 0 | 0 | 0 | 40383 | 0 | 0 | 40383 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=25202 transfer_reuse=0 prefill_reuse=31158 source_prefill=40383 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 56360 | 0 | 0 | 0 | 615 | 0 | 615 | 0 | 55745 | 615 | 0 | 40383 | 40383 | 0 | 0 | 40383 | 40588 | 0 | 0 | 0.006M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=40383 f=40383 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  transfer: current=0 max=0
| hz6-speed-appcap | 0.002M | 0 | 16490 | 16490 | 0 | 16490 | 1 | 1168 | 0 | 0 | 0 | 1168 | 0 | 0 | 1168 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=1814 transfer_reuse=14669 prefill_reuse=75 source_prefill=1168 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 14934 | 14669 | 265 | 53 | 1889 | 0 | 14669 | 0 | 0 | 519 | 0 | 519 | 16039 | 0 | 519 | 1168 | 1168 | 0 | 0 | 1168 | 1168 | 0 | 0 | 0.002M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=1168 f=1168 fb=0
  visibility: lookup=16490 hit=16490 miss=0 probe_total=16490 probe_max=1
  transfer: current=265 max=53
| hz6-rss-appcap | 0.002M | 0 | 15441 | 15441 | 0 | 15441 | 1 | 1312 | 0 | 0 | 0 | 1312 | 0 | 0 | 1312 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=1897 transfer_reuse=13058 prefill_reuse=597 source_prefill=1312 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 13228 | 13058 | 170 | 16 | 2494 | 0 | 13058 | 0 | 0 | 14101 | 0 | 14101 | 1451 | 0 | 14101 | 1312 | 1312 | 0 | 0 | 1312 | 1312 | 0 | 0 | 0.002M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=1312 f=1312 fb=0
  visibility: lookup=15441 hit=15441 miss=0 probe_total=15441 probe_max=1
  transfer: current=170 max=16
| mimalloc | 55.975M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 55.975M |
| tcmalloc | 56.613M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 56.613M |

## Worker-warmup control note

- Worker-warmup mode allocates the initial live set inside each worker thread after allocator thread setup and before the timer starts.
- This separates cross-owner warmup stress from same-owner small-object source placement.

## Larson worker-warmup stress T=16

| allocator | median ops/s | route_miss | route_vis_lookup | route_vis_hit | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 53.787M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 53.787M |
| hz3 | 57.912M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 57.912M |
| hz4 | 60.546M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 60.546M |
| hz5-policy | 46.362M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 46.362M |
| hz6-strict-appcap | 37.264M | 0 | 0 | 0 | 0 | 0 | 0 | 165566 | 0 | 0 | 0 | 165566 | 0 | 0 | 165566 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=396438604 transfer_reuse=0 prefill_reuse=165566 source_prefill=165566 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 396604170 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 396604170 | 0 | 0 | 165566 | 165566 | 0 | 0 | 165566 | 168939 | 0 | 0 | 37.264M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=165566 f=165566 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  transfer: current=0 max=0
| hz6-speed-appcap | 39.082M | 0 | 0 | 0 | 0 | 0 | 0 | 166048 | 0 | 0 | 0 | 166048 | 0 | 0 | 166048 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=415841840 transfer_reuse=0 prefill_reuse=10378 source_prefill=166048 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 415852218 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 415852218 | 0 | 0 | 166048 | 166048 | 0 | 0 | 166048 | 169394 | 0 | 0 | 39.082M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=166048 f=166048 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  transfer: current=0 max=0
| hz6-rss-appcap | 36.773M | 0 | 0 | 0 | 0 | 0 | 0 | 165652 | 0 | 0 | 0 | 165652 | 0 | 0 | 165652 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=390221617 transfer_reuse=0 prefill_reuse=41413 source_prefill=165652 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 390263030 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 390263030 | 0 | 0 | 165652 | 165652 | 0 | 0 | 165652 | 169039 | 0 | 0 | 36.773M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=165652 f=165652 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  transfer: current=0 max=0
| mimalloc | 49.233M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 49.233M |
| tcmalloc | 49.080M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 49.080M |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
