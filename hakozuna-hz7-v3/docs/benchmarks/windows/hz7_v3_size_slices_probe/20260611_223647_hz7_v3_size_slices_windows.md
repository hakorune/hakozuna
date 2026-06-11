# HZ7 v3 Windows Size Slices

Generated: 2026-06-11 22:37:32 +09:00

- benchmark: `bench_hz7_v3_hotpath`
- allocator: `hz7-v3`
- runs: 1
- iters_per_run: 1000
- note: filtered from the v3 hotpath probe to emphasize 4K/8K/16K span-audit rows

| op | label | size | median rate | rate unit | median peak_kb |
| --- | --- | ---: | ---: | --- | ---: |
| malloc_free | span4k | 4096 | 48.766M | pairs/s | 3920 |
| malloc_free | span8k | 8192 | 48.498M | pairs/s | 3964 |
| malloc_free | span16k | 16384 | 48.705M | pairs/s | 3984 |
| route_invariant | span4k | 4096 | 27.277M | ops/s | 3992 |
| route_invariant | span8k | 8192 | 27.245M | ops/s | 3992 |
| route_invariant | span16k | 16384 | 27.123M | ops/s | 3992 |
| route_valid | span4k | 4096 | 121.445M | ops/s | 3992 |
| route_valid | span8k | 8192 | 121.407M | ops/s | 3992 |
| route_valid | span16k | 16384 | 122.046M | ops/s | 3992 |
| route_invalid | span4k | 4096 | 121.285M | ops/s | 3992 |
| route_invalid | span8k | 8192 | 121.331M | ops/s | 3992 |
| route_invalid | span16k | 16384 | 121.572M | ops/s | 3992 |
| malloc_batch | span4k | 4096 | 17.577K | ops/s | 263992 |
| malloc_batch | span8k | 8192 | 702.440K | ops/s | 263992 |
| malloc_batch | span16k | 16384 | 527.258K | ops/s | 263992 |
| free_batch | span4k | 4096 | 2.844M | ops/s | 264476 |
| free_batch | span8k | 8192 | 1.108M | ops/s | 264476 |
| free_batch | span16k | 16384 | 818.488K | ops/s | 264476 |
| free_retained_loop | span4k | 4096 | 47.259M | pairs/s | 264476 |
| free_retained_loop | span8k | 8192 | 47.393M | pairs/s | 264476 |
| free_retained_loop | span16k | 16384 | 47.494M | pairs/s | 264476 |

Artifacts: .\private\allocators\hakozuna\hakozuna-hz7-v3\docs\benchmarks\windows\hz7_v3_size_slices_probe
