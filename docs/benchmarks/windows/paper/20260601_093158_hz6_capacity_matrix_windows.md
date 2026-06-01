# Windows HZ6 Capacity Matrix

Generated: 2026-06-01 09:31:58 +09:00

- artifacts: [out_win_hz6_capacity](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity)
- families: `random_mixed`
- HZ6 profiles: `speed`
- capacity lanes: `route4k, desc4k-route4k, source512-route4k`
- diagnostic probes: `False`

## random_mixed / small

- Note: paper SSOT small range
- Args: `20000000 400 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-speed-route4k | 27.082M | 4,548 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=2048 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.738512 ops/s=27081751.66 [RSS] peak_kb=4548 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=23 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |
| hz6-speed-desc4k-route4k | 27.320M | 5,516 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=2048 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.732079 ops/s=27319734.62 [RSS] peak_kb=5516 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=23 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |
| hz6-speed-source512-route4k | 27.415M | 5,048 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=2048 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.729527 ops/s=27415295.82 [RSS] peak_kb=5048 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=23 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |

