# Windows HZ6 Capacity Matrix

Generated: 2026-06-01 10:25:58 +09:00

- artifacts: [out_win_hz6_capacity_diag](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity_diag)
- families: `mixed_ws`
- HZ6 profiles: `rss`
- capacity lanes: `route4k`
- diagnostic probes: `True`

## mixed_ws / balanced

- Note: larger mixed run for first Windows compare
- Args: `8 250000 4096 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 2.509M | 17,300 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.797 ops/s=2508889.623 alloc_attempts=1757077 alloc_success=247008 alloc_fail=1510069 frees=247008 hz6_route_valid=247008 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1027 hz6_alloc_fail=1510069 hz6_descriptor_exhausted=3391197 hz6_route_register_fail=0 hz6_source_block_exhausted=1139013 hz6_descriptor_probe_total=1736298491 hz6_descriptor_probe_max=512 hz6_descriptor_fail_active_max=512 hz6_descriptor_fail_local_free_max=457 hz6_descriptor_fail_transfer_free_max=0 hz6_descriptor_fail_remote_pending_max=0 hz6_descriptor_fail_central_free_max=0 hz6_descriptor_fail_released_max=0 hz6_descriptor_fail_orphan_max=0 hz6_descriptor_fail_dead_with_ptr_max=0 hz6_descriptor_fail_frontcache_total_max=457 hz6_descriptor_fail_frontcache_largest_bin_max=236 hz6_descriptor_fail_frontcache_nonempty_bins_max=5 hz6_frontcache_spill_dryrun_calls=3391197 hz6_frontcache_spill_dryrun_requested_empty=3391194 hz6_frontcache_spill_dryrun_candidate_calls=834864 hz6_frontcache_spill_dryrun_reclaimable_total=3979102 hz6_frontcache_spill_dryrun_largest_donor_max=235 hz6_frontcache_spill_dryrun_donor_bins_max=4 hz6_route_lookup_probe_total=349421 hz6_route_lookup_probe_max=7 hz6_route_register_probe_total=4213494 hz6_route_register_probe_max=4102 hz6_route_unregister_probe_total=43 hz6_route_unregister_probe_max=4 hz6_source_block_probe_total=193162877 hz6_source_block_probe_max=128 hz6_source_block_fail_active_max=128 hz6_source_block_fail_registered_max=128 hz6_source_block_fail_ref_nonzero_max=128 hz6_source_block_fail_ref_zero_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=17300` |
