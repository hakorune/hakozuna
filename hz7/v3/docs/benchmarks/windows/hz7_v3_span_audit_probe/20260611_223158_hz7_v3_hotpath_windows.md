# HZ7 v3 Windows Hot Path Microbench

Generated: 2026-06-11 22:31:58 +09:00

- benchmark: `bench_hz7_v3_hotpath`
- allocator: `hz7-v3`
- runs: 1
- iters_per_run: 1000
- note: diagnostic-only; no allocator counters or production hot-path instrumentation

| op | label | size | median rate | rate unit | median peak_kb |
| --- | --- | ---: | ---: | --- | ---: |
| free_batch | direct32k | 32768 | 898.473K | ops/s | 15080 |
| free_batch | small64 | 64 | 103.093M | ops/s | 14816 |
| free_batch | span16k | 16384 | 1.521M | ops/s | 15080 |
| free_batch | span4k | 4096 | 5.081M | ops/s | 14816 |
| free_batch | span8k | 8192 | 2.327M | ops/s | 14816 |
| free_retained_loop | direct32k | 32768 | 57.804M | pairs/s | 15080 |
| free_retained_loop | small64 | 64 | 50.505M | pairs/s | 15080 |
| free_retained_loop | span16k | 16384 | 49.751M | pairs/s | 15080 |
| free_retained_loop | span4k | 4096 | 49.751M | pairs/s | 15080 |
| free_retained_loop | span8k | 8192 | 49.505M | pairs/s | 15080 |
| malloc_batch | direct32k | 32768 | 545.375K | ops/s | 14816 |
| malloc_batch | small64 | 64 | 116.279M | ops/s | 7200 |
| malloc_batch | span16k | 16384 | 527.537K | ops/s | 14816 |
| malloc_batch | span4k | 4096 | 1.379M | ops/s | 9476 |
| malloc_batch | span8k | 8192 | 717.463K | ops/s | 13868 |
| malloc_free | direct32k | 32768 | 50.505M | pairs/s | 3976 |
| malloc_free | small64 | 64 | 23.641M | pairs/s | 3812 |
| malloc_free | span16k | 16384 | 40.816M | pairs/s | 3968 |
| malloc_free | span4k | 4096 | 30.675M | pairs/s | 3912 |
| malloc_free | span8k | 8192 | 35.971M | pairs/s | 3948 |
| mixed_steady | direct_medium_ws400 | 0 | 3.331M | ops/s | 15080 |
| mixed_steady | medium_ws400 | 0 | 3.700M | ops/s | 15080 |
| mixed_steady | mixed_ws400 | 0 | 4.044M | ops/s | 15080 |
| mixed_steady | small_ws400 | 0 | 17.123M | ops/s | 15080 |
| mixed_steady | span_medium_ws400 | 0 | 6.402M | ops/s | 15080 |
| random_toggle | fresh_medium_ws400 | 0 | 1.847M | ops/s | 7200 |
| random_toggle | fresh_medium_ws400_notouch | 0 | 6.689M | ops/s | 7200 |
| random_toggle | fresh_mixed_ws400 | 0 | 3.569M | ops/s | 7200 |
| random_toggle | fresh_mixed_ws400_notouch | 0 | 8.873M | ops/s | 7200 |
| random_toggle | fresh_small_ws400 | 0 | 6.684M | ops/s | 4624 |
| random_toggle | fresh_small_ws400_notouch | 0 | 79.365M | ops/s | 7200 |
| random_toggle | medium_ws400 | 0 | 3.186M | ops/s | 15080 |
| random_toggle | mixed_ws400 | 0 | 3.660M | ops/s | 15080 |
| random_toggle | small_ws400 | 0 | 41.322M | ops/s | 15080 |
| route_invalid | direct32k | 32768 | 113.636M | ops/s | 3976 |
| route_invalid | small64 | 64 | 121.951M | ops/s | 3976 |
| route_invalid | span16k | 16384 | 120.482M | ops/s | 3976 |
| route_invalid | span8k | 8192 | 121.951M | ops/s | 3976 |
| route_invariant | span16k | 16384 | 27.027M | ops/s | 3976 |
| route_invariant | span4k | 4096 | 27.027M | ops/s | 3976 |
| route_invariant | span8k | 8192 | 27.027M | ops/s | 3976 |
| route_valid | direct32k | 32768 | 121.951M | ops/s | 3976 |
| route_valid | small64 | 64 | 120.482M | ops/s | 3976 |
| route_valid | span16k | 16384 | 120.482M | ops/s | 3976 |
| route_valid | span8k | 8192 | 121.951M | ops/s | 3976 |

Artifacts: .\private\allocators\hakozuna\hz7/v3\docs\benchmarks\windows\hz7_v3_span_audit_probe
