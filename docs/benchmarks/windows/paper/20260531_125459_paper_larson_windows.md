# Paper-Aligned Windows Larson

Generated: 2026-05-31 12:54:59 +09:00

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

| allocator | median ops/s | route_miss | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 53.938M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz3 | 55.150M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz4 | 59.156M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz5-policy | 47.967M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz6-strict-appcap | 0.004M | 31531 | 31641 | 0 | 0 | 0 | 31641 | 0 | 0 | 31641 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=16683 transfer_reuse=0 prefill_reuse=23541 source_prefill=31641 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 40224 | 0 | 0 | 0 | 540 | 0 | 540 | 0 | 23541 | 31641 | 0 | 0 | 31641 | 31762 | 0 | `0` |
| hz6-speed-appcap | 0.003M | 27104 | 27584 | 0 | 0 | 0 | 27584 | 0 | 0 | 27584 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=31560 transfer_reuse=0 prefill_reuse=1708 source_prefill=27584 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 33268 | 0 | 0 | 0 | 16 | 0 | 16 | 0 | 1708 | 27584 | 0 | 0 | 27584 | 27668 | 0 | `0` |
| hz6-rss-appcap | 0.003M | 27776 | 27900 | 0 | 0 | 0 | 27900 | 0 | 0 | 27900 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=27438 transfer_reuse=0 prefill_reuse=6819 source_prefill=27900 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 34257 | 0 | 0 | 0 | 52 | 0 | 52 | 0 | 6819 | 27900 | 0 | 0 | 27900 | 28001 | 0 | `0` |
| mimalloc | 51.564M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| tcmalloc | 55.559M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |

## Compact control note

- HZ6-friendly compact control uses chunks=400 while keeping the same runtime/size/seed settings.

## Larson compact control T=16

| allocator | median ops/s | route_miss | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| crt | 50.523M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz3 | 60.981M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz4 | 59.697M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz5-policy | 50.171M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| hz6-strict-appcap | 62.959M | 3200 | 4590 | 0 | 0 | 0 | 4590 | 0 | 0 | 4590 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=632306460 transfer_reuse=0 prefill_reuse=3795 source_prefill=4590 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 632310255 | 0 | 0 | 0 | 53 | 0 | 53 | 0 | 3795 | 4590 | 0 | 0 | 4590 | 4593 | 0 | `0` |
| hz6-speed-appcap | 63.797M | 3200 | 5120 | 0 | 0 | 0 | 5120 | 0 | 0 | 5120 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=639761082 transfer_reuse=0 prefill_reuse=304 source_prefill=5120 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 639761386 | 0 | 0 | 0 | 16 | 0 | 16 | 0 | 304 | 5120 | 0 | 0 | 5120 | 5125 | 0 | `0` |
| hz6-rss-appcap | 61.321M | 3200 | 4652 | 0 | 0 | 0 | 4652 | 0 | 0 | 4652 | 0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=0 transfer_reuse=0 prefill_reuse=0 source_prefill=0 direct_source=0 released_reuse=0 oom=0 unsupported=0 | local_reuse=615023915 transfer_reuse=0 prefill_reuse=1103 source_prefill=4652 direct_source=0 released_reuse=0 oom=0 unsupported=0 | 0 | 0 | 615025018 | 0 | 0 | 0 | 20 | 0 | 20 | 0 | 1103 | 4652 | 0 | 0 | 4652 | 4653 | 0 | `0` |
| mimalloc | 54.731M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |
| tcmalloc | 55.937M | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA |  |  |  |  | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | `NA` |

Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)
