# Windows HZ6 Capacity Matrix

Generated: 2026-06-04 15:46:49 +09:00

- artifacts: [out_win_hz6_capacity](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity)
- families: `random_mixed`
- HZ6 profiles: `strict`
- capacity lanes: `sameownerfast-descavail-noboost-route4k`
- diagnostic probes: `False`

## random_mixed / small

- Note: paper SSOT small range
- Args: `20000000 400 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-strict-sameownerfast-descavail-noboost-route4k | 45.485M | 4,628 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=2048 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.439712 ops/s=45484791.29 [RSS] peak_kb=4628 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=321 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |

## random_mixed / medium

- Note: paper SSOT medium range
- Args: `20000000 400 4096 32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-strict-sameownerfast-descavail-noboost-route4k | 43.310M | 4,636 | `[BENCH_ARGS] iters=20000000 ws=400 min=4096 max=32768 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.461793 ops/s=43309908.69 [RSS] peak_kb=4636 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=118 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |

## random_mixed / mixed

- Note: paper SSOT mixed range
- Args: `20000000 400 16 32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-strict-sameownerfast-descavail-noboost-route4k | 41.706M | 4,640 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=32768 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.479552 ops/s=41706017.28 [RSS] peak_kb=4640 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=153 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |

