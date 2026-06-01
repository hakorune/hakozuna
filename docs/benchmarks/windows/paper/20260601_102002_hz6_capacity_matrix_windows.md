# Windows HZ6 Capacity Matrix

Generated: 2026-06-01 10:20:02 +09:00

- artifacts: [out_win_hz6_capacity_diag](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity_diag)
- families: `mixed_ws`
- HZ6 profiles: `rss`
- capacity lanes: `route4k, front1k-desc4k-source512-route4k`
- diagnostic probes: `True`

## mixed_ws / balanced

- Note: larger mixed run for first Windows compare
- Args: `8 250000 4096 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 2.534M | 17,316 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.789 ops/s=2533833.968 alloc_attempts=1757077 alloc_success=247008 alloc_fail=1510069 frees=247008 hz6_route_valid=247008 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1027 hz6_alloc_fail=1510069 hz6_descriptor_exhausted=3391197 hz6_route_register_fail=0 hz6_source_block_exhausted=1139013 hz6_descriptor_probe_total=1736298491 hz6_descriptor_probe_max=512 hz6_descriptor_fail_active_max=512 hz6_descriptor_fail_local_free_max=457 hz6_descriptor_fail_transfer_free_max=0 hz6_descriptor_fail_remote_pending_max=0 hz6_descriptor_fail_central_free_max=0 hz6_descriptor_fail_released_max=0 hz6_descriptor_fail_orphan_max=0 hz6_descriptor_fail_dead_with_ptr_max=0 hz6_route_lookup_probe_total=347492 hz6_route_lookup_probe_max=8 hz6_route_register_probe_total=4213430 hz6_route_register_probe_max=4101 hz6_route_unregister_probe_total=47 hz6_route_unregister_probe_max=4 hz6_source_block_probe_total=193162877 hz6_source_block_probe_max=128 hz6_source_block_fail_active_max=128 hz6_source_block_fail_registered_max=128 hz6_source_block_fail_ref_nonzero_max=128 hz6_source_block_fail_ref_zero_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=17316` |
| hz6-rss-front1k-desc4k-source512-route4k | 1.489M | 99,848 | `threads=8 iters=250000 ws=4096 size=16..2048 time=1.344 ops/s=1488613.151 alloc_attempts=1032150 alloc_success=986316 alloc_fail=45834 frees=986316 hz6_route_valid=986316 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=190780 hz6_alloc_fail=45834 hz6_descriptor_exhausted=0 hz6_route_register_fail=91893 hz6_source_block_exhausted=31948 hz6_descriptor_probe_total=1312233 hz6_descriptor_probe_max=3585 hz6_descriptor_fail_active_max=0 hz6_descriptor_fail_local_free_max=0 hz6_descriptor_fail_transfer_free_max=0 hz6_descriptor_fail_remote_pending_max=0 hz6_descriptor_fail_central_free_max=0 hz6_descriptor_fail_released_max=0 hz6_descriptor_fail_orphan_max=0 hz6_descriptor_fail_dead_with_ptr_max=0 hz6_route_lookup_probe_total=65406238 hz6_route_lookup_probe_max=4089 hz6_route_register_probe_total=1109090137 hz6_route_register_probe_max=4142 hz6_route_unregister_probe_total=28971088 hz6_route_unregister_probe_max=4089 hz6_source_block_probe_total=31398235 hz6_source_block_probe_max=512 hz6_source_block_fail_active_max=512 hz6_source_block_fail_registered_max=512 hz6_source_block_fail_ref_nonzero_max=512 hz6_source_block_fail_ref_zero_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=99848` |

## mixed_ws / larger_sizes

- Note: shift toward larger allocations
- Args: `4 150000 4096 256 8192`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 0.795M | 14,552 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.755 ops/s=795097.745 alloc_attempts=530298 alloc_success=71719 alloc_fail=458579 frees=71719 hz6_route_valid=71719 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1014 hz6_alloc_fail=458579 hz6_descriptor_exhausted=1138243 hz6_route_register_fail=0 hz6_source_block_exhausted=0 hz6_descriptor_probe_total=582813666 hz6_descriptor_probe_max=512 hz6_descriptor_fail_active_max=512 hz6_descriptor_fail_local_free_max=84 hz6_descriptor_fail_transfer_free_max=0 hz6_descriptor_fail_remote_pending_max=0 hz6_descriptor_fail_central_free_max=0 hz6_descriptor_fail_released_max=0 hz6_descriptor_fail_orphan_max=0 hz6_descriptor_fail_dead_with_ptr_max=0 hz6_route_lookup_probe_total=325199 hz6_route_lookup_probe_max=147 hz6_route_register_probe_total=4212556 hz6_route_register_probe_max=4242 hz6_route_unregister_probe_total=45518 hz6_route_unregister_probe_max=144 hz6_source_block_probe_total=35400501 hz6_source_block_probe_max=92 hz6_source_block_fail_active_max=0 hz6_source_block_fail_registered_max=0 hz6_source_block_fail_ref_nonzero_max=0 hz6_source_block_fail_ref_zero_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=14552` |
| hz6-rss-front1k-desc4k-source512-route4k | 0.792M | 65,264 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.757 ops/s=792468.904 alloc_attempts=311077 alloc_success=298287 alloc_fail=12790 frees=298287 hz6_route_valid=298287 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=172090 hz6_alloc_fail=12790 hz6_descriptor_exhausted=0 hz6_route_register_fail=18512 hz6_source_block_exhausted=2909 hz6_descriptor_probe_total=829151 hz6_descriptor_probe_max=3585 hz6_descriptor_fail_active_max=0 hz6_descriptor_fail_local_free_max=0 hz6_descriptor_fail_transfer_free_max=0 hz6_descriptor_fail_remote_pending_max=0 hz6_descriptor_fail_central_free_max=0 hz6_descriptor_fail_released_max=0 hz6_descriptor_fail_orphan_max=0 hz6_descriptor_fail_dead_with_ptr_max=0 hz6_route_lookup_probe_total=13994090 hz6_route_lookup_probe_max=4088 hz6_route_register_probe_total=773284359 hz6_route_register_probe_max=4365 hz6_route_unregister_probe_total=10653448 hz6_route_unregister_probe_max=4088 hz6_source_block_probe_total=8430417 hz6_source_block_probe_max=512 hz6_source_block_fail_active_max=512 hz6_source_block_fail_registered_max=512 hz6_source_block_fail_ref_nonzero_max=512 hz6_source_block_fail_ref_zero_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=65264` |
