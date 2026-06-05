# Windows HZ6 Capacity Matrix

Generated: 2026-06-06 00:59:25 +09:00

- artifacts: [out_win_hz6_capacity](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity)
- families: `random_mixed`
- HZ6 profiles: `speed`
- capacity lanes: `ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-budget2048-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096`
- diagnostic probes: `False`

## random_mixed / small

- Note: paper SSOT small range
- Args: `20000000 400 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-speed-ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-budget2048-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096 | 26.253M | 50,508 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=2048 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.761823 ops/s=26253091.93 [RSS] peak_kb=50508 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=23 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |

## random_mixed / medium

- Note: paper SSOT medium range
- Args: `20000000 400 4096 32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-speed-ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-budget2048-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096 | 28.993M | 12,468 | `[BENCH_ARGS] iters=20000000 ws=400 min=4096 max=32768 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.689826 ops/s=28993101.60 [RSS] peak_kb=12468 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=118 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |

## random_mixed / mixed

- Note: paper SSOT mixed range
- Args: `20000000 400 16 32768`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-speed-ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-budget2048-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096 | 27.576M | 12,988 | `[BENCH_ARGS] iters=20000000 ws=400 min=16 max=32768 seed=305419896 bench_random_mixed_compare: ops=20000204 time=0.725276 ops/s=27575990.38 [RSS] peak_kb=12988 [HZ6_STATS] label=random_mixed_final route_valid=10000102 route_invalid=0 route_miss=0 route_visibility_lookup=0 route_visibility_hit=0 route_visibility_hit_local_owner=0 route_visibility_hit_foreign_owner=0 route_visibility_miss=0 route_visibility_probe_total=0 route_visibility_probe_max=0 transfer_push=0 transfer_pop=0 transfer_current=0 transfer_current_max=0 remote_free_attempt=0 remote_free_strict_owner_block=0 remote_free_transfer_fail=0 route_rehome_attempt=0 route_rehome_success=0 route_rehome_fail=0 source_owned_prepare=0 source_owned_route_hit_local_owner=0 source_owned_visibility_hit_local_owner=0 source_owned_visibility_hit_foreign_owner=0 source_owned_remote_free_attempt=0 source_owned_release=0 source_alloc=112 alloc_fail=0 descriptor_exhausted=0 route_register_fail=0 source_block_exhausted=0 large_span_central_push=0 large_span_central_pop=0 large_span_source_alloc=0` |

