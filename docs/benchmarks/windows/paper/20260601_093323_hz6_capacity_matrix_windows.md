# Windows HZ6 Capacity Matrix

Generated: 2026-06-01 09:33:23 +09:00

- artifacts: [out_win_hz6_capacity](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity)
- families: `mixed_ws`
- HZ6 profiles: `rss`
- capacity lanes: `route4k, desc4k-route4k, source512-route4k`
- diagnostic probes: `False`

## mixed_ws / balanced

- Note: larger mixed run for first Windows compare
- Args: `8 250000 4096 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 3.195M | 17,732 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.626 ops/s=3194560.558 alloc_attempts=1757077 alloc_success=247008 alloc_fail=1510069 frees=247008 hz6_route_valid=247008 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1027 hz6_alloc_fail=1510069 hz6_descriptor_exhausted=3391197 hz6_route_register_fail=0 hz6_source_block_exhausted=1139013 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=17732` |
| hz6-rss-desc4k-route4k | 0.469M | 130,248 | `threads=8 iters=250000 ws=4096 size=16..2048 time=4.266 ops/s=468769.608 alloc_attempts=1015808 alloc_success=1004607 alloc_fail=11201 frees=1004607 hz6_route_valid=1004607 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=737575 hz6_alloc_fail=11201 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=22300 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=130248` |
| hz6-rss-source512-route4k | 0.463M | 17,972 | `threads=8 iters=250000 ws=4096 size=16..2048 time=4.316 ops/s=463366.499 alloc_attempts=1757077 alloc_success=247008 alloc_fail=1510069 frees=247008 hz6_route_valid=247008 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1027 hz6_alloc_fail=1510069 hz6_descriptor_exhausted=4530210 hz6_route_register_fail=0 hz6_source_block_exhausted=0 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=17972` |

## mixed_ws / wide_ws

- Note: wider working-set pressure
- Args: `8 200000 16384 16 1024`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 0.388M | 12,172 | `threads=8 iters=200000 ws=16384 size=16..1024 time=4.121 ops/s=388229.126 alloc_attempts=1553929 alloc_success=50125 alloc_fail=1503804 frees=50125 hz6_route_valid=50125 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1299 hz6_alloc_fail=1503804 hz6_descriptor_exhausted=4384482 hz6_route_register_fail=0 hz6_source_block_exhausted=126976 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=12172` |
| hz6-rss-desc4k-route4k | 0.181M | 129,124 | `threads=8 iters=200000 ws=16384 size=16..1024 time=8.825 ops/s=181305.760 alloc_attempts=1304424 alloc_success=322307 alloc_fail=982117 frees=322307 hz6_route_valid=322307 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=257241 hz6_alloc_fail=982117 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=106240 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=129124` |
| hz6-rss-source512-route4k | 0.374M | 12,292 | `threads=8 iters=200000 ws=16384 size=16..1024 time=4.274 ops/s=374314.431 alloc_attempts=1553929 alloc_success=50125 alloc_fail=1503804 frees=50125 hz6_route_valid=50125 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1299 hz6_alloc_fail=1503804 hz6_descriptor_exhausted=4511458 hz6_route_register_fail=0 hz6_source_block_exhausted=0 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=12292` |

## mixed_ws / larger_sizes

- Note: shift toward larger allocations
- Args: `4 150000 4096 256 8192`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 0.747M | 14,560 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.803 ops/s=747292.187 alloc_attempts=530298 alloc_success=71719 alloc_fail=458579 frees=71719 hz6_route_valid=71719 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1014 hz6_alloc_fail=458579 hz6_descriptor_exhausted=1138243 hz6_route_register_fail=0 hz6_source_block_exhausted=0 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=14560` |
| hz6-rss-desc4k-route4k | 0.558M | 70,328 | `threads=4 iters=150000 ws=4096 size=256..8192 time=1.075 ops/s=557922.179 alloc_attempts=308436 alloc_success=305088 alloc_fail=3348 frees=305088 hz6_route_valid=305088 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=261703 hz6_alloc_fail=3348 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=5969 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=70328` |
| hz6-rss-source512-route4k | 0.767M | 14,640 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.782 ops/s=766952.919 alloc_attempts=530298 alloc_success=71719 alloc_fail=458579 frees=71719 hz6_route_valid=71719 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1014 hz6_alloc_fail=458579 hz6_descriptor_exhausted=1138243 hz6_route_register_fail=0 hz6_source_block_exhausted=0 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=14640` |

