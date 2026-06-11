# HZ7 v3 Windows Size Slices

Generated: 2026-06-11 22:42:14 +09:00

- benchmark: `bench_hz7_v3_hotpath`
- allocator: `hz7-v3`
- runs: 1
- iters_per_run: 1000
- note: filtered from the v3 hotpath probe to emphasize 4K/8K/16K span-audit rows

| op | label | size | median rate | rate unit | median peak_kb |
| --- | --- | ---: | ---: | --- | ---: |
| malloc_free | span4k | 4096 | 48.681M | pairs/s | 3568 |
| malloc_free | span8k | 8192 | 47.984M | pairs/s | 3604 |
| malloc_free | span16k | 16384 | 48.239M | pairs/s | 3624 |
| route_invariant | span4k | 4096 | 26.976M | ops/s | 3632 |
| route_invariant | span8k | 8192 | 27.088M | ops/s | 3632 |
| route_invariant | span16k | 16384 | 27.062M | ops/s | 3632 |
| route_valid | span4k | 4096 | 121.726M | ops/s | 3632 |
| route_valid | span8k | 8192 | 121.839M | ops/s | 3632 |
| route_valid | span16k | 16384 | 121.414M | ops/s | 3632 |
| route_invalid | span4k | 4096 | 121.686M | ops/s | 3632 |
| route_invalid | span8k | 8192 | 121.482M | ops/s | 3632 |
| route_invalid | span16k | 16384 | 121.641M | ops/s | 3632 |
| malloc_batch | span4k | 4096 | 17.981K | ops/s | 263636 |
| malloc_batch | span8k | 8192 | 718.389K | ops/s | 263636 |
| malloc_batch | span16k | 16384 | 597.856K | ops/s | 263636 |
| free_batch | span4k | 4096 | 2.923M | ops/s | 264116 |
| free_batch | span8k | 8192 | 2.095M | ops/s | 264116 |
| free_batch | span16k | 16384 | 1.252M | ops/s | 264116 |
| free_retained_loop | span4k | 4096 | 47.027M | pairs/s | 264116 |
| free_retained_loop | span8k | 8192 | 46.974M | pairs/s | 264116 |
| free_retained_loop | span16k | 16384 | 47.845M | pairs/s | 264116 |

Artifacts: .\private\allocators\hakozuna\hz7/v3\docs\benchmarks\windows\hz7_v3_size_slices_probe2
