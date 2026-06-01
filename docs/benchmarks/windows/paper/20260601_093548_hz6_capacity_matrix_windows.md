# Windows HZ6 Capacity Matrix

Generated: 2026-06-01 09:35:48 +09:00

- artifacts: [out_win_hz6_capacity](Z:\TextureVoice_local\git\allocator-bench-lab\private\allocators\hakozuna\out_win_hz6_capacity)
- families: `mixed_ws`
- HZ6 profiles: `rss`
- capacity lanes: `route4k, desc4k-source512-route4k, broad`
- diagnostic probes: `False`

## mixed_ws / balanced

- Note: larger mixed run for first Windows compare
- Args: `8 250000 4096 16 2048`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 3.249M | 17,304 | `threads=8 iters=250000 ws=4096 size=16..2048 time=0.616 ops/s=3249316.831 alloc_attempts=1757077 alloc_success=247008 alloc_fail=1510069 frees=247008 hz6_route_valid=247008 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1027 hz6_alloc_fail=1510069 hz6_descriptor_exhausted=3391197 hz6_route_register_fail=0 hz6_source_block_exhausted=1139013 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=17304` |
| hz6-rss-desc4k-source512-route4k | 0.502M | 122,516 | `threads=8 iters=250000 ws=4096 size=16..2048 time=3.981 ops/s=502325.402 alloc_attempts=1015808 alloc_success=994668 alloc_fail=21140 frees=994668 hz6_route_valid=994668 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=703243 hz6_alloc_fail=21140 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=7179 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=122516` |
| hz6-rss-broad | 1.647M | 100,220 | `threads=8 iters=250000 ws=4096 size=16..2048 time=1.214 ops/s=1647457.314 alloc_attempts=1032150 alloc_success=986316 alloc_fail=45834 frees=986316 hz6_route_valid=986316 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=190780 hz6_alloc_fail=45834 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=31948 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=100220` |

## mixed_ws / wide_ws

- Note: wider working-set pressure
- Args: `8 200000 16384 16 1024`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 0.393M | 12,120 | `threads=8 iters=200000 ws=16384 size=16..1024 time=4.076 ops/s=392513.222 alloc_attempts=1553929 alloc_success=50125 alloc_fail=1503804 frees=50125 hz6_route_valid=50125 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1299 hz6_alloc_fail=1503804 hz6_descriptor_exhausted=4384482 hz6_route_register_fail=0 hz6_source_block_exhausted=126976 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=12120` |
| hz6-rss-desc4k-source512-route4k | 0.195M | 121,908 | `threads=8 iters=200000 ws=16384 size=16..1024 time=8.226 ops/s=194503.856 alloc_attempts=1304907 alloc_success=315098 alloc_fail=989809 frees=315098 hz6_route_valid=315098 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=241482 hz6_alloc_fail=989809 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=105472 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=121908` |
| hz6-rss-broad | 0.198M | 93,380 | `threads=8 iters=200000 ws=16384 size=16..1024 time=8.091 ops/s=197746.745 alloc_attempts=1302992 alloc_success=302997 alloc_fail=999995 frees=302997 hz6_route_valid=302997 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=134972 hz6_alloc_fail=999995 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=105472 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=93380` |

## mixed_ws / larger_sizes

- Note: shift toward larger allocations
- Args: `4 150000 4096 256 8192`
- Runs: `1`

| allocator | median ops/s | median peak_kb | runs |
| --- | ---: | ---: | --- |
| hz6-rss-route4k | 0.748M | 14,548 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.802 ops/s=748083.037 alloc_attempts=530298 alloc_success=71719 alloc_fail=458579 frees=71719 hz6_route_valid=71719 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=1014 hz6_alloc_fail=458579 hz6_descriptor_exhausted=1138243 hz6_route_register_fail=0 hz6_source_block_exhausted=0 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=14548` |
| hz6-rss-desc4k-source512-route4k | 0.575M | 66,940 | `threads=4 iters=150000 ws=4096 size=256..8192 time=1.044 ops/s=574609.170 alloc_attempts=311054 alloc_success=303821 alloc_fail=7233 frees=303821 hz6_route_valid=303821 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=250643 hz6_alloc_fail=7233 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=2909 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=66940` |
| hz6-rss-broad | 0.784M | 65,252 | `threads=4 iters=150000 ws=4096 size=256..8192 time=0.765 ops/s=784024.508 alloc_attempts=311077 alloc_success=298287 alloc_fail=12790 frees=298287 hz6_route_valid=298287 hz6_route_invalid=0 hz6_route_miss=0 hz6_transfer_push=0 hz6_transfer_pop=0 hz6_source_alloc=172090 hz6_alloc_fail=12790 hz6_descriptor_exhausted=0 hz6_route_register_fail=0 hz6_source_block_exhausted=2909 hz6_descriptor_probe_total=0 hz6_descriptor_probe_max=0 hz6_route_lookup_probe_total=0 hz6_route_lookup_probe_max=0 hz6_route_register_probe_total=0 hz6_route_register_probe_max=0 hz6_route_unregister_probe_total=0 hz6_route_unregister_probe_max=0 hz6_source_block_probe_total=0 hz6_source_block_probe_max=0 hz6_large_central_push=0 hz6_large_central_pop=0 hz6_large_source_alloc=0 peak_kb=65252` |

