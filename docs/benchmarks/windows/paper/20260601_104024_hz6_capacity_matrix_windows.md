# Windows HZ6 Capacity Matrix

Generated: 2026-06-01 10:40:24 +09:00

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
| hz6-rss-route4k | 2.625M | 17,296 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.762 ops/s=2624789.377 alloc_attempts=1757077 alloc_success=247008 alloc_fail=1510069 frees=247008 hz6_route_valid=247008 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1027 hz6_alloc_fail=1510069 hz6_descriptor_exhausted=3391197 hz6_route_register_fail=0 hz6_source_block_exhausted=1139013 hz6_descriptor_probe_total=1736298491 hz6_descriptor_probe_max=512 hz6_descriptor_fail_active_max=512 hz6_descriptor_fail_local_free_max=457 hz6_descriptor_fail_transfer_free_max=0 hz6_descriptor_fail_remote_pending_max=0 hz6_descriptor_fail_central_free_max=0 hz6_descriptor_fail_released_max=0 hz6_descriptor_fail_orphan_max=0 hz6_descriptor_fail_dead_with_ptr_max=0 hz6_descriptor_fail_frontcache_total_max=457 hz6_descriptor_fail_frontcache_largest_bin_max=236 hz6_descriptor_fail_frontcache_nonempty_bins_max=5 hz6_frontcache_spill_dryrun_calls=3391197 hz6_frontcache_spill_dryrun_requested_empty=3391194 hz6_frontcache_spill_dryrun_candidate_calls=834864 hz6_frontcache_spill_dryrun_reclaimable_total=3979102 hz6_frontcache_spill_dryrun_largest_donor_max=235 hz6_frontcache_spill_dryrun_donor_bins_max=4 hz6_frontcache_spill_attempt=3391197 hz6_frontcache_spill_success=0 hz6_frontcache_spill_no_candidate=3391197 hz6_frontcache_spill_invalid=0 hz6_frontcache_spill_retry_success=0 hz6_frontcache_borrow_dryrun_calls=3391197 hz6_frontcache_borrow_dryrun_candidate_calls=213353 hz6_frontcache_borrow_dryrun_candidate_total=413696 hz6_frontcache_borrow_dryrun_largest_candidate_max=236 hz6_frontcache_borrow_attempt=4531234 hz6_frontcache_borrow_success=0 hz6_frontcache_borrow_no_candidate=4531234 hz6_frontcache_borrow_invalid=0 hz6_frontcache_cap_dryrun_push=251126 hz6_frontcache_cap_dryrun_over_cap=33019 hz6_frontcache_cap_dryrun_would_release=33019 hz6_frontcache_cap_dryrun_soft_cap_max=32 hz6_frontcache_cap_dryrun_bin_count_max=256 hz6_route_lookup_probe_total=921745 hz6_route_lookup_probe_max=8 hz6_route_register_probe_total=4213429 hz6_route_register_probe_max=4102 hz6_route_unregister_probe_total=44 hz6_route_unregister_probe_max=5 hz6_source_block_probe_total=193162877 hz6_source_block_probe_max=128 hz6_source_block_fail_active_max=128 hz6_source_block_fail_registered_max=128 hz6_source_block_fail_ref_nonzero_max=128 hz6_source_block_fail_ref_zero_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=17296` |

## mixed_ws / larger_sizes

- Note: shift toward larger allocations
- Args: `4 150000 4096 256 8192`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 0.827M | 14,572 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.726 ops/s=826803.424 alloc_attempts=530298 alloc_success=71719 alloc_fail=458579 frees=71719 hz6_route_valid=71719 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1014 hz6_alloc_fail=458579 hz6_descriptor_exhausted=1138243 hz6_route_register_fail=0 hz6_source_block_exhausted=0 hz6_descriptor_probe_total=582813666 hz6_descriptor_probe_max=512 hz6_descriptor_fail_active_max=512 hz6_descriptor_fail_local_free_max=84 hz6_descriptor_fail_transfer_free_max=0 hz6_descriptor_fail_remote_pending_max=0 hz6_descriptor_fail_central_free_max=0 hz6_descriptor_fail_released_max=0 hz6_descriptor_fail_orphan_max=0 hz6_descriptor_fail_dead_with_ptr_max=0 hz6_descriptor_fail_frontcache_total_max=84 hz6_descriptor_fail_frontcache_largest_bin_max=69 hz6_descriptor_fail_frontcache_nonempty_bins_max=3 hz6_frontcache_spill_dryrun_calls=1138243 hz6_frontcache_spill_dryrun_requested_empty=1138186 hz6_frontcache_spill_dryrun_candidate_calls=390798 hz6_frontcache_spill_dryrun_reclaimable_total=5658951 hz6_frontcache_spill_dryrun_largest_donor_max=68 hz6_frontcache_spill_dryrun_donor_bins_max=2 hz6_frontcache_spill_attempt=1138243 hz6_frontcache_spill_success=0 hz6_frontcache_spill_no_candidate=1138243 hz6_frontcache_spill_invalid=0 hz6_frontcache_spill_retry_success=0 hz6_frontcache_borrow_dryrun_calls=1138243 hz6_frontcache_borrow_dryrun_candidate_calls=3310 hz6_frontcache_borrow_dryrun_candidate_total=4173 hz6_frontcache_borrow_dryrun_largest_candidate_max=33 hz6_frontcache_borrow_attempt=901120 hz6_frontcache_borrow_success=0 hz6_frontcache_borrow_no_candidate=901120 hz6_frontcache_borrow_invalid=0 hz6_frontcache_cap_dryrun_push=81084 hz6_frontcache_cap_dryrun_over_cap=32555 hz6_frontcache_cap_dryrun_would_release=32555 hz6_frontcache_cap_dryrun_soft_cap_max=32 hz6_frontcache_cap_dryrun_bin_count_max=256 hz6_route_lookup_probe_total=453201 hz6_route_lookup_probe_max=167 hz6_route_register_probe_total=4224937 hz6_route_register_probe_max=4232 hz6_route_unregister_probe_total=55523 hz6_route_unregister_probe_max=167 hz6_source_block_probe_total=35400501 hz6_source_block_probe_max=92 hz6_source_block_fail_active_max=0 hz6_source_block_fail_registered_max=0 hz6_source_block_fail_ref_nonzero_max=0 hz6_source_block_fail_ref_zero_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=14572` |
