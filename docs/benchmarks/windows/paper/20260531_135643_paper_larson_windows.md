# Paper-Aligned Windows Larson

Generated: 2026-05-31 13:56:43 +09:00

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
| crt | 52.337M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz3 | 56.359M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz4 | 56.161M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz5-policy | 47.042M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz6-strict-appcap | 0.003M | 22828 | 22924 | 0 | 0 | 0 | 22924 | 0 | 0 | 22924 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=10670 transfer_reuse=0 prefill_reuse=16294 source_prefill=22924 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 26964 | 0 | 0 | 0 | 442 | 0 | 442 | 0 | 26522 | 442 | 0 | 16294 | 22924 | 0 | 0 | 22924 | 22987 | 0 | `0` |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=0 f=0 fb=0
| hz6-speed-appcap | 0.004M | 28889 | 29312 | 0 | 0 | 0 | 29312 | 0 | 0 | 29312 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=34296 transfer_reuse=0 prefill_reuse=1816 source_prefill=29312 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 36112 | 0 | 0 | 0 | 16 | 0 | 16 | 0 | 36096 | 16 | 0 | 1816 | 29312 | 0 | 0 | 29312 | 29416 | 0 | `0` |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=0 f=0 fb=0
| hz6-rss-appcap | 0.002M | 21209 | 21312 | 0 | 0 | 0 | 21312 | 0 | 0 | 21312 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=19576 transfer_reuse=0 prefill_reuse=5184 source_prefill=21312 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 24760 | 0 | 0 | 0 | 48 | 0 | 48 | 0 | 24712 | 48 | 0 | 5184 | 21312 | 0 | 0 | 21312 | 21370 | 0 | `0` |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=0 f=0 fb=0
| mimalloc | 51.722M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| tcmalloc | 52.835M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 50.332M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz3 | 54.386M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz4 | 57.066M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz5-policy | 48.635M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz6-strict-appcap | 65.878M | 3200 | 4590 | 0 | 0 | 0 | 4590 | 0 | 0 | 4590 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=660890415 transfer_reuse=0 prefill_reuse=3795 source_prefill=4590 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 660894210 | 0 | 0 | 0 | 53 | 0 | 53 | 0 | 660894157 | 53 | 0 | 3795 | 4590 | 0 | 0 | 4590 | 4592 | 0 | `0` |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=0 f=0 fb=0
| hz6-speed-appcap | 59.227M | 3200 | 5120 | 0 | 0 | 0 | 5120 | 0 | 0 | 5120 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=594487716 transfer_reuse=0 prefill_reuse=304 source_prefill=5120 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 594488020 | 0 | 0 | 0 | 16 | 0 | 16 | 0 | 594488004 | 16 | 0 | 304 | 5120 | 0 | 0 | 5120 | 5120 | 0 | `0` |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=0 f=0 fb=0
| hz6-rss-appcap | 63.868M | 3200 | 4660 | 0 | 0 | 0 | 4660 | 0 | 0 | 4660 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=640994258 transfer_reuse=0 prefill_reuse=1105 source_prefill=4660 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 640995363 | 0 | 0 | 0 | 20 | 0 | 20 | 0 | 640995343 | 20 | 0 | 1105 | 4660 | 0 | 0 | 4660 | 4661 | 0 | `0` |
  prefill: local2p a=0 f=0 fb=0; midpage a=0 f=0 fb=0; large a=0 f=0 fb=0; toy a=0 f=0 fb=0
| mimalloc | 55.183M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| tcmalloc | 56.105M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
