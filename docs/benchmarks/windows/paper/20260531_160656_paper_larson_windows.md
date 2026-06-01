# Paper-Aligned Windows Larson

Generated: 2026-05-31 16:06:56 +09:00

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
- remote free diagnostics: `remote_free_attempt / strict_owner_block / transfer_fail`
- thread sweep: `1, 4, 8, 16`
- runs: `1`
- timeout: `60s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=16

| allocator | median ops/s | route_miss | route_vis_lookup | route_vis_hit | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 55.584M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 55.584M |
| hz3 | 56.805M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 56.805M |
| hz4 | 57.404M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 57.404M |
| hz5-policy | 48.688M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 48.688M |
| hz6-strict-appcap | 0.005M | 39339 | 0 | 0 | 0 | 0 | 0 | 39456 | 0 | 0 | 0 | 39456 | 0 | 0 | 39456 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=24317 transfer_reuse=0 prefill_reuse=30246 source_prefill=39456 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 54563 | 0 | 0 | 0 | 614 | 0 | 614 | 0 | 53949 | 614 | 0 | 39456 | 39456 | 0 | 0 | 39456 | 39648 | 0 | 0 | 0.005M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=39456 f=39456 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0
| hz6-speed-appcap | 0.002M | 0 | 15955 | 15955 | 0 | 15955 | 1 | 1120 | 0 | 0 | 0 | 1120 | 0 | 0 | 1120 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=1742 transfer_reuse=14203 prefill_reuse=72 source_prefill=1120 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 14465 | 14203 | 262 | 54 | 14465 | 0 | 0 | 1814 | 0 | 14203 | 0 | 0 | 577 | 0 | 577 | 15440 | 0 | 577 | 1120 | 1120 | 0 | 0 | 1120 | 1120 | 0 | 0 | 0.002M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=1120 f=1120 fb=0
  visibility: lookup=15955 hit=15955 miss=0 probe_total=15955 probe_max=1
  transfer: current=262 max=54
  remote_free: attempt=14465 strict_block=0 transfer_fail=0
| hz6-rss-appcap | 0.002M | 0 | 15636 | 15636 | 0 | 15636 | 1 | 1366 | 0 | 0 | 0 | 1366 | 0 | 0 | 1366 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=1970 transfer_reuse=13163 prefill_reuse=621 source_prefill=1366 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 13337 | 13163 | 174 | 16 | 14248 | 0 | 911 | 2591 | 0 | 13163 | 0 | 0 | 13961 | 0 | 13961 | 1793 | 0 | 13961 | 1366 | 1366 | 0 | 0 | 1366 | 1366 | 0 | 0 | 0.002M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=1366 f=1366 fb=0
  visibility: lookup=15636 hit=15636 miss=0 probe_total=15636 probe_max=1
  transfer: current=174 max=16
  remote_free: attempt=14248 strict_block=0 transfer_fail=911
| mimalloc | 50.562M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 50.562M |
| tcmalloc | 55.132M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 55.132M |

## Worker-warmup control note

- Worker-warmup mode allocates the initial live set inside each worker thread after allocator thread setup and before the timer starts.
- This separates cross-owner warmup stress from same-owner small-object source placement.

## Larson worker-warmup stress T=16

| allocator | median ops/s | route_miss | route_vis_lookup | route_vis_hit | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 53.644M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 53.644M |
| hz3 | 57.436M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 57.436M |
| hz4 | 58.551M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 58.551M |
| hz5-policy | 47.260M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 47.260M |
| hz6-strict-appcap | 38.415M | 0 | 0 | 0 | 0 | 0 | 0 | 165566 | 0 | 0 | 0 | 165566 | 0 | 0 | 165566 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=401597382 transfer_reuse=0 prefill_reuse=165566 source_prefill=165566 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 401762948 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 401762948 | 0 | 0 | 165566 | 165566 | 0 | 0 | 165566 | 168935 | 0 | 0 | 38.415M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=165566 f=165566 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0
| hz6-speed-appcap | 38.809M | 0 | 0 | 0 | 0 | 0 | 0 | 166048 | 0 | 0 | 0 | 166048 | 0 | 0 | 166048 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=405024020 transfer_reuse=0 prefill_reuse=10378 source_prefill=166048 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 405034398 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 405034398 | 0 | 0 | 166048 | 166048 | 0 | 0 | 166048 | 169481 | 0 | 0 | 38.809M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=166048 f=166048 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0
| hz6-rss-appcap | 38.684M | 0 | 0 | 0 | 0 | 0 | 0 | 165652 | 0 | 0 | 0 | 165652 | 0 | 0 | 165652 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=402973168 transfer_reuse=0 prefill_reuse=41413 source_prefill=165652 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 403014581 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 403014581 | 0 | 0 | 165652 | 165652 | 0 | 0 | 165652 | 168996 | 0 | 0 | 38.684M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=165652 f=165652 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
  transfer: current=0 max=0
  remote_free: attempt=0 strict_block=0 transfer_fail=0
| mimalloc | 55.664M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 55.664M |
| tcmalloc | 57.802M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 57.802M |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
