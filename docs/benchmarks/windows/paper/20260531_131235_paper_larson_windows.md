# Paper-Aligned Windows Larson

Generated: 2026-05-31 13:12:35 +09:00

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
| crt | 60.017M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz3 | 61.009M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz4 | 60.096M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz5-policy | 50.458M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz6-strict-appcap | 0.003M | 26666 | 26744 | 0 | 0 | 0 | 26744 | 0 | 0 | 26744 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=13044 transfer_reuse=0 prefill_reuse=19484 source_prefill=26744 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 32528 | 0 | 0 | 0 | 484 | 0 | 484 | 0 | NA | NA | NA | 19484 | 26744 | 0 | 0 | 26744 | 26835 | 0 | `0` |
| hz6-speed-appcap | 0.007M | 46417 | 46976 | 0 | 0 | 0 | 46976 | 0 | 0 | 46976 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=68532 transfer_reuse=0 prefill_reuse=2920 source_prefill=46976 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 71452 | 0 | 0 | 0 | 16 | 0 | 16 | 0 | NA | NA | NA | 2920 | 46976 | 0 | 0 | 46976 | 47263 | 0 | `0` |
| hz6-rss-appcap | 0.006M | 43273 | 43416 | 0 | 0 | 0 | 43416 | 0 | 0 | 43416 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=52579 transfer_reuse=0 prefill_reuse=10653 source_prefill=43416 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 63232 | 0 | 0 | 0 | 67 | 0 | 67 | 0 | NA | NA | NA | 10653 | 43416 | 0 | 0 | 43416 | 43629 | 0 | `0` |
| mimalloc | 53.085M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| tcmalloc | 55.587M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 57.927M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz3 | 61.734M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz4 | 63.246M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz5-policy | 55.764M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz6-strict-appcap | 63.158M | 3200 | 4587 | 0 | 0 | 0 | 4587 | 0 | 0 | 4587 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=633590058 transfer_reuse=0 prefill_reuse=3792 source_prefill=4587 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 633593850 | 0 | 0 | 0 | 53 | 0 | 53 | 0 | NA | NA | NA | 3792 | 4587 | 0 | 0 | 4587 | 4592 | 0 | `0` |
| hz6-speed-appcap | 60.073M | 3200 | 5120 | 0 | 0 | 0 | 5120 | 0 | 0 | 5120 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=603307929 transfer_reuse=0 prefill_reuse=304 source_prefill=5120 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 603308233 | 0 | 0 | 0 | 16 | 0 | 16 | 0 | NA | NA | NA | 304 | 5120 | 0 | 0 | 5120 | 5121 | 0 | `0` |
| hz6-rss-appcap | 62.687M | 3200 | 4656 | 0 | 0 | 0 | 4656 | 0 | 0 | 4656 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=629057760 transfer_reuse=0 prefill_reuse=1104 source_prefill=4656 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 629058864 | 0 | 0 | 0 | 20 | 0 | 20 | 0 | NA | NA | NA | 1104 | 4656 | 0 | 0 | 4656 | 4658 | 0 | `0` |
| mimalloc | 63.611M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| tcmalloc | 63.508M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
