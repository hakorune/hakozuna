# Windows HZ6 Capacity Matrix

Generated: 2026-06-01 09:37:35 +09:00

- artifacts: [out_win_hz6_capacity](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity)
- families: `mixed_ws`
- HZ6 profiles: `rss`
- capacity lanes: `desc4k-source512-route4k, front1k-desc4k-source512-route4k, broad`
- diagnostic probes: `False`

## mixed_ws / balanced

- Note: larger mixed run for first Windows compare
- Args: `8 250000 4096 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-desc4k-source512-route4k | 0.564M | 122,112 | `threads=8 iters=250000 ws=4096 size=16..2048 time=3.545 ops/s=564097.050 alloc_attempts=1015808 alloc_success=994668 alloc_fail=21140 frees=994668 hz6_route_valid=994668 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=703243 hz6_alloc_fail=21140 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=7179 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=122112` |
| hz6-rss-front1k-desc4k-source512-route4k | 1.736M | 100,264 | `threads=8 iters=250000 ws=4096 size=16..2048 time=1.152 ops/s=1736462.473 alloc_attempts=1032150 alloc_success=986316 alloc_fail=45834 frees=986316 hz6_route_valid=986316 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=190780 hz6_alloc_fail=45834 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=31948 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=100264` |
| hz6-rss-broad | 1.701M | 99,792 | `threads=8 iters=250000 ws=4096 size=16..2048 time=1.176 ops/s=1701385.421 alloc_attempts=1032150 alloc_success=986316 alloc_fail=45834 frees=986316 hz6_route_valid=986316 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=190780 hz6_alloc_fail=45834 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=31948 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=99792` |

## mixed_ws / wide_ws

- Note: wider working-set pressure
- Args: `8 200000 16384 16 1024`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-desc4k-source512-route4k | 0.179M | 121,908 | `threads=8 iters=200000 ws=16384 size=16..1024 time=8.935 ops/s=179075.163 alloc_attempts=1304907 alloc_success=315098 alloc_fail=989809 frees=315098 hz6_route_valid=315098 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=241482 hz6_alloc_fail=989809 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=105472 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=121908` |
| hz6-rss-front1k-desc4k-source512-route4k | 0.191M | 93,368 | `threads=8 iters=200000 ws=16384 size=16..1024 time=8.382 ops/s=190893.652 alloc_attempts=1302992 alloc_success=302997 alloc_fail=999995 frees=302997 hz6_route_valid=302997 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=134972 hz6_alloc_fail=999995 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=105472 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=93368` |
| hz6-rss-broad | 0.145M | 93,376 | `threads=8 iters=200000 ws=16384 size=16..1024 time=11.072 ops/s=144507.639 alloc_attempts=1302992 alloc_success=302997 alloc_fail=999995 frees=302997 hz6_route_valid=302997 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=134972 hz6_alloc_fail=999995 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=105472 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=93376` |

## mixed_ws / larger_sizes

- Note: shift toward larger allocations
- Args: `4 150000 4096 256 8192`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-desc4k-source512-route4k | 0.556M | 67,396 | `threads=4 iters=150000 ws=4096 size=256..8192 time=1.080 ops/s=555709.559 alloc_attempts=311054 alloc_success=303821 alloc_fail=7233 frees=303821 hz6_route_valid=303821 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=250643 hz6_alloc_fail=7233 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=2909 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=67396` |
| hz6-rss-front1k-desc4k-source512-route4k | 0.728M | 65,260 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.824 ops/s=728071.311 alloc_attempts=311077 alloc_success=298287 alloc_fail=12790 frees=298287 hz6_route_valid=298287 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=172090 hz6_alloc_fail=12790 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=2909 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=65260` |
| hz6-rss-broad | 0.747M | 65,240 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.803 ops/s=746775.703 alloc_attempts=311077 alloc_success=298287 alloc_fail=12790 frees=298287 hz6_route_valid=298287 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=172090 hz6_alloc_fail=12790 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=2909 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=65240` |

