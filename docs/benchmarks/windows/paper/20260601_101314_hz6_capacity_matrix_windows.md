# Windows HZ6 Capacity Matrix

Generated: 2026-06-01 10:13:14 +09:00

- artifacts: [out_win_hz6_capacity_diag](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity_diag)
- families: `mixed_ws`
- HZ6 profiles: `rss`
- capacity lanes: `route4k, front1k-desc4k-source512-route4k, broad`
- diagnostic probes: `True`

## mixed_ws / balanced

- Note: larger mixed run for first Windows compare
- Args: `8 250000 4096 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 3.223M | 17,736 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.621 ops/s=3223204.494 alloc_attempts=1757077 alloc_success=247008 alloc_fail=1510069 frees=247008 hz6_route_valid=247008 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1027 hz6_alloc_fail=1510069 hz6_descriptor_exhausted=3391197 hz6_route_register_fail=0 hz6_source_block_exhausted=1139013 hz6_descriptor_probe_total=1736298491 hz6_descriptor_probe_max=512 hz6_route_lookup_probe_total=342021 hz6_route_lookup_probe_max=8 hz6_route_register_probe_total=4213318 hz6_route_register_probe_max=4101 hz6_route_unregister_probe_total=44 hz6_route_unregister_probe_max=3 hz6_source_block_probe_total=193162877 hz6_source_block_probe_max=128 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=17736` |
| hz6-rss-front1k-desc4k-source512-route4k | 1.698M | 100,456 | `threads=8 iters=250000 ws=4096 size=16..2048 time=1.178 ops/s=1697673.542 alloc_attempts=1032150 alloc_success=986316 alloc_fail=45834 frees=986316 hz6_route_valid=986316 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=190780 hz6_alloc_fail=45834 hz6_descriptor_exhausted=0 hz6_route_register_fail=91893 hz6_source_block_exhausted=31948 hz6_descriptor_probe_total=1312233 hz6_descriptor_probe_max=3585 hz6_route_lookup_probe_total=58065797 hz6_route_lookup_probe_max=4088 hz6_route_register_probe_total=1108686088 hz6_route_register_probe_max=4133 hz6_route_unregister_probe_total=25330360 hz6_route_unregister_probe_max=4088 hz6_source_block_probe_total=31398235 hz6_source_block_probe_max=512 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=100456` |
| hz6-rss-broad | 1.665M | 100,340 | `threads=8 iters=250000 ws=4096 size=16..2048 time=1.201 ops/s=1664909.909 alloc_attempts=1032150 alloc_success=986316 alloc_fail=45834 frees=986316 hz6_route_valid=986316 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=190780 hz6_alloc_fail=45834 hz6_descriptor_exhausted=0 hz6_route_register_fail=91893 hz6_source_block_exhausted=31948 hz6_descriptor_probe_total=1312233 hz6_descriptor_probe_max=3585 hz6_route_lookup_probe_total=60154541 hz6_route_lookup_probe_max=4084 hz6_route_register_probe_total=1108922495 hz6_route_register_probe_max=4135 hz6_route_unregister_probe_total=26712756 hz6_route_unregister_probe_max=4077 hz6_source_block_probe_total=31398235 hz6_source_block_probe_max=512 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=100340` |

## mixed_ws / larger_sizes

- Note: shift toward larger allocations
- Args: `4 150000 4096 256 8192`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 0.754M | 14,560 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.795 ops/s=754253.993 alloc_attempts=530298 alloc_success=71719 alloc_fail=458579 frees=71719 hz6_route_valid=71719 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1014 hz6_alloc_fail=458579 hz6_descriptor_exhausted=1138243 hz6_route_register_fail=0 hz6_source_block_exhausted=0 hz6_descriptor_probe_total=582813666 hz6_descriptor_probe_max=512 hz6_route_lookup_probe_total=444892 hz6_route_lookup_probe_max=304 hz6_route_register_probe_total=4240371 hz6_route_register_probe_max=4249 hz6_route_unregister_probe_total=69210 hz6_route_unregister_probe_max=301 hz6_source_block_probe_total=35400501 hz6_source_block_probe_max=92 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=14560` |
| hz6-rss-front1k-desc4k-source512-route4k | 0.797M | 65,260 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.753 ops/s=796836.347 alloc_attempts=311077 alloc_success=298287 alloc_fail=12790 frees=298287 hz6_route_valid=298287 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=172090 hz6_alloc_fail=12790 hz6_descriptor_exhausted=0 hz6_route_register_fail=18512 hz6_source_block_exhausted=2909 hz6_descriptor_probe_total=829151 hz6_descriptor_probe_max=3585 hz6_route_lookup_probe_total=13352980 hz6_route_lookup_probe_max=4066 hz6_route_register_probe_total=773069168 hz6_route_register_probe_max=4336 hz6_route_unregister_probe_total=10541741 hz6_route_unregister_probe_max=4066 hz6_source_block_probe_total=8430417 hz6_source_block_probe_max=512 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=65260` |
| hz6-rss-broad | 0.779M | 65,264 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.770 ops/s=778836.722 alloc_attempts=311077 alloc_success=298287 alloc_fail=12790 frees=298287 hz6_route_valid=298287 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=172090 hz6_alloc_fail=12790 hz6_descriptor_exhausted=0 hz6_route_register_fail=18512 hz6_source_block_exhausted=2909 hz6_descriptor_probe_total=829151 hz6_descriptor_probe_max=3585 hz6_route_lookup_probe_total=15660793 hz6_route_lookup_probe_max=4067 hz6_route_register_probe_total=773197617 hz6_route_register_probe_max=4336 hz6_route_unregister_probe_total=12181111 hz6_route_unregister_probe_max=4067 hz6_source_block_probe_total=8430417 hz6_source_block_probe_max=512 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=65264` |
