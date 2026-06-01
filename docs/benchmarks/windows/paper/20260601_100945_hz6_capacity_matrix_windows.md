# Windows HZ6 Capacity Matrix

Generated: 2026-06-01 10:09:45 +09:00

- artifacts: [out_win_hz6_capacity](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity)
- families: `mixed_ws`
- HZ6 profiles: `rss`
- capacity lanes: `route4k, front1k-desc4k-source512-route4k, broad`
- diagnostic probes: `False`

## mixed_ws / balanced

- Note: larger mixed run for first Windows compare
- Args: `8 250000 4096 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 0.461M | 25,164 | `threads=8 iters=250000 ws=4096 size=16..2048 time=4.341 ops/s=460704.848 alloc_attempts=1778816 alloc_success=225280 alloc_fail=1553536 frees=225280 hz6_route_valid=225280 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=110132 hz6_alloc_fail=1553536 hz6_descriptor_exhausted=4593951 hz6_route_register_fail=0 hz6_source_block_exhausted=82646 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=25164` |
| hz6-rss-front1k-desc4k-source512-route4k | 1.733M | 99,812 | `threads=8 iters=250000 ws=4096 size=16..2048 time=1.154 ops/s=1732940.822 alloc_attempts=1032150 alloc_success=986316 alloc_fail=45834 frees=986316 hz6_route_valid=986316 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=190780 hz6_alloc_fail=45834 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=31948 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=99812` |
| hz6-rss-broad | 1.740M | 99,928 | `threads=8 iters=250000 ws=4096 size=16..2048 time=1.150 ops/s=1739502.083 alloc_attempts=1032150 alloc_success=986316 alloc_fail=45834 frees=986316 hz6_route_valid=986316 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=190780 hz6_alloc_fail=45834 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=31948 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=99928` |

## mixed_ws / wide_ws

- Note: wider working-set pressure
- Args: `8 200000 16384 16 1024`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 0.470M | 13,808 | `threads=8 iters=200000 ws=16384 size=16..1024 time=3.404 ops/s=470028.376 alloc_attempts=1554944 alloc_success=49152 alloc_fail=1505792 frees=49152 hz6_route_valid=49152 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=5591 hz6_alloc_fail=1505792 hz6_descriptor_exhausted=4391912 hz6_route_register_fail=0 hz6_source_block_exhausted=126993 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=13808` |
| hz6-rss-front1k-desc4k-source512-route4k | 0.190M | 93,380 | `threads=8 iters=200000 ws=16384 size=16..1024 time=8.438 ops/s=189628.697 alloc_attempts=1302992 alloc_success=302997 alloc_fail=999995 frees=302997 hz6_route_valid=302997 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=134972 hz6_alloc_fail=999995 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=105472 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=93380` |
| hz6-rss-broad | 0.201M | 93,376 | `threads=8 iters=200000 ws=16384 size=16..1024 time=7.968 ops/s=200799.120 alloc_attempts=1302992 alloc_success=302997 alloc_fail=999995 frees=302997 hz6_route_valid=302997 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=134972 hz6_alloc_fail=999995 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=105472 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=93376` |

## mixed_ws / larger_sizes

- Note: shift toward larger allocations
- Args: `4 150000 4096 256 8192`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 0.749M | 13,756 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.801 ops/s=749135.965 alloc_attempts=534464 alloc_success=67584 alloc_fail=466880 frees=67584 hz6_route_valid=67584 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=22429 hz6_alloc_fail=466880 hz6_descriptor_exhausted=1160740 hz6_route_register_fail=0 hz6_source_block_exhausted=0 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=13756` |
| hz6-rss-front1k-desc4k-source512-route4k | 0.816M | 65,256 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.736 ops/s=815573.096 alloc_attempts=311077 alloc_success=298287 alloc_fail=12790 frees=298287 hz6_route_valid=298287 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=172090 hz6_alloc_fail=12790 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=2909 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=65256` |
| hz6-rss-broad | 0.800M | 65,248 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.750 ops/s=800068.699 alloc_attempts=311077 alloc_success=298287 alloc_fail=12790 frees=298287 hz6_route_valid=298287 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=172090 hz6_alloc_fail=12790 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=2909 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=65248` |
