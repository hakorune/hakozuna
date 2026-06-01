# Paper-Aligned Windows Larson

Generated: 2026-05-31 15:50:44 +09:00

References:
- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)

Windows native note:
- benchmark: `bench_larson_compare`
- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`
- compact control (optional): `chunks=400`
- worker-warmup control (optional): same chunks, but each worker owns its warmup allocations before the timer starts
- shared route visibility diagnostics: `route_visibility_lookup / hit / miss / probe_total / probe_max`
- thread sweep: `1, 4, 8, 16`
- runs: `1`
- timeout: `60s` per allocator row
- statistic: `median alloc/s` from the benchmark's `Throughput = ...` line
- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`

## Larson stress T=16

| allocator | median ops/s | route_miss | route_vis_lookup | route_vis_hit | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 58.017M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 58.017M |
| hz3 | 57.266M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 57.266M |
| hz4 | 57.432M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 57.432M |
| hz5-policy | 48.469M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 48.469M |
| hz6-strict-appcap | 0.003M | 26710 | 0 | 0 | 0 | 0 | 0 | 26822 | 0 | 0 | 0 | 26822 | 0 | 0 | 26822 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=13256 transfer_reuse=0 prefill_reuse=19442 source_prefill=26822 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 32698 | 0 | 0 | 0 | 492 | 0 | 492 | 0 | 32206 | 492 | 0 | 26822 | 26822 | 0 | 0 | 26822 | 26918 | 0 | 0 | 0.003M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=26822 f=26822 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
| hz6-speed-appcap | 0.001M | 0 | 12050 | 12050 | 0 | 12050 | 1 | 1024 | 0 | 0 | 0 | 1024 | 0 | 0 | 1024 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=1076 transfer_reuse=10951 prefill_reuse=65 source_prefill=1024 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 11221 | 10951 | 1141 | 0 | 10951 | 0 | 0 | 174 | 0 | 174 | 11918 | 0 | 174 | 1024 | 1024 | 0 | 0 | 1024 | 1024 | 0 | 0 | 0.001M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=1024 f=1024 fb=0
  visibility: lookup=12050 hit=12050 miss=0 probe_total=12050 probe_max=1
| hz6-rss-appcap | 0.002M | 0 | 16175 | 16175 | 0 | 16175 | 1 | 1354 | 0 | 0 | 0 | 1354 | 0 | 0 | 1354 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=2089 transfer_reuse=13600 prefill_reuse=614 source_prefill=1354 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 13757 | 13600 | 2703 | 0 | 13600 | 0 | 0 | 14567 | 0 | 14567 | 1736 | 0 | 14567 | 1354 | 1354 | 0 | 0 | 1354 | 1354 | 0 | 0 | 0.002M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=1354 f=1354 fb=0
  visibility: lookup=16175 hit=16175 miss=0 probe_total=16175 probe_max=1
| mimalloc | 56.525M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 56.525M |
| tcmalloc | 56.876M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 56.876M |

## Worker-warmup control note

- Worker-warmup mode allocates the initial live set inside each worker thread after allocator thread setup and before the timer starts.
- This separates cross-owner warmup stress from same-owner small-object source placement.

## Larson worker-warmup stress T=16

| allocator | median ops/s | route_miss | route_vis_lookup | route_vis_hit | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 48.849M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 48.849M |
| hz3 | 54.193M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 54.193M |
| hz4 | 56.327M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 56.327M |
| hz5-policy | 43.016M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 43.016M |
| hz6-strict-appcap | 37.327M | 0 | 0 | 0 | 0 | 0 | 0 | 165566 | 0 | 0 | 0 | 165566 | 0 | 0 | 165566 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=394394207 transfer_reuse=0 prefill_reuse=165566 source_prefill=165566 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 394559773 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 394559773 | 0 | 0 | 165566 | 165566 | 0 | 0 | 165566 | 168951 | 0 | 0 | 37.327M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=165566 f=165566 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
| hz6-speed-appcap | 35.672M | 0 | 0 | 0 | 0 | 0 | 0 | 166048 | 0 | 0 | 0 | 166048 | 0 | 0 | 166048 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=379930010 transfer_reuse=0 prefill_reuse=10378 source_prefill=166048 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 379940388 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 379940388 | 0 | 0 | 166048 | 166048 | 0 | 0 | 166048 | 169384 | 0 | 0 | 35.672M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=166048 f=166048 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
| hz6-rss-appcap | 35.701M | 0 | 0 | 0 | 0 | 0 | 0 | 165648 | 0 | 0 | 0 | 165648 | 0 | 0 | 165648 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=380745809 transfer_reuse=0 prefill_reuse=41412 source_prefill=165648 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 380787221 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 380787221 | 0 | 0 | 165648 | 165648 | 0 | 0 | 165648 | 169083 | 0 | 0 | 35.701M |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=165648 f=165648 fb=0
  visibility: lookup=0 hit=0 miss=0 probe_total=0 probe_max=0
| mimalloc | 45.610M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 45.610M |
| tcmalloc | 45.813M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | 45.813M |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
