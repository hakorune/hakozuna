# Windows HZ6 Capacity Matrix

Generated: 2026-06-08 01:34:28 +09:00

- artifacts: [out_win_hz6_capacity](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity)
- families: `random_mixed`
- HZ6 profiles: `strict`
- capacity lanes: `sameownerfast-descavail-noboost-route4k, sameownertrustedfree-descavail-noboost-route4k`
- diagnostic probes: `False`

## random_mixed / small

- Note: paper SSOT small range
- Args: `20000000 400 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-strict-sameownerfast-descavail-noboost-route4k | 43.367M | 4,632 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=2048 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.461184 ops/s=43367118.87 [RSS] peak_kb=4632 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=321 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |
| hz6-strict-sameownertrustedfree-descavail-noboost-route4k | 44.661M | 5,076 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=2048 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.447824 ops/s=44660827.92 [RSS] peak_kb=5076 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=321 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |

## random_mixed / medium

- Note: paper SSOT medium range
- Args: `20000000 400 4096 32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-strict-sameownerfast-descavail-noboost-route4k | 41.838M | 4,632 | `[BENCH_ARGS] iters=20000000 ws=400 min=4096 max=32768 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.478039 ops/s=41838052.41 [RSS] peak_kb=4632 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=118 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |
| hz6-strict-sameownertrustedfree-descavail-noboost-route4k | 44.354M | 4,640 | `[BENCH_ARGS] iters=20000000 ws=400 min=4096 max=32768 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.450918 ops/s=44354454.12 [RSS] peak_kb=4640 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=118 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |

## random_mixed / mixed

- Note: paper SSOT mixed range
- Args: `20000000 400 16 32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-strict-sameownerfast-descavail-noboost-route4k | 40.211M | 4,628 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=32768 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.497385 ops/s=40210693.84 [RSS] peak_kb=4628 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=153 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |
| hz6-strict-sameownertrustedfree-descavail-noboost-route4k | 41.813M | 4,628 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=32768 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.478324 ops/s=41813045.33 [RSS] peak_kb=4628 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=153 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |

