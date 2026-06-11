# HZ7 v3 Windows Hot Path Microbench

Generated: 2026-06-11 22:12:32 +09:00

- benchmark: `bench_hz7_v3_hotpath`
- allocator: `hz7-v3`
- runs: 1
- iters_per_run: 100000
- note: diagnostic-only; no allocator counters or production hot-path instrumentation

| op | label | size | median rate | rate unit | median peak_kb |
| --- | --- | ---: | ---: | --- | ---: |
| free_batch | direct32k | 32768 | 770.352K | ops/s | 76840 |
| free_batch | small64 | 64 | 72.977M | ops/s | 76536 |
| free_batch | span8k | 8192 | 1.671M | ops/s | 76840 |
| free_retained_loop | direct32k | 32768 | 56.060M | pairs/s | 76840 |
| free_retained_loop | small64 | 64 | 48.070M | pairs/s | 76840 |
| free_retained_loop | span8k | 8192 | 47.110M | pairs/s | 76840 |
| malloc_batch | direct32k | 32768 | 506.894K | ops/s | 76536 |
| malloc_batch | small64 | 64 | 48.008M | ops/s | 12536 |
| malloc_batch | span8k | 8192 | 658.362K | ops/s | 76536 |
| malloc_free | direct32k | 32768 | 56.398M | pairs/s | 3892 |
| malloc_free | small64 | 64 | 49.162M | pairs/s | 3816 |
| malloc_free | span8k | 8192 | 48.082M | pairs/s | 3884 |
| mixed_steady | direct_medium_ws400 | 0 | 27.616M | ops/s | 76840 |
| mixed_steady | medium_ws400 | 0 | 23.568M | ops/s | 76840 |
| mixed_steady | mixed_ws400 | 0 | 23.250M | ops/s | 76840 |
| mixed_steady | small_ws400 | 0 | 47.398M | ops/s | 76840 |
| mixed_steady | span_medium_ws400 | 0 | 25.337M | ops/s | 76840 |
| random_toggle | fresh_medium_ws400 | 0 | 25.332M | ops/s | 9088 |
| random_toggle | fresh_medium_ws400_notouch | 0 | 49.903M | ops/s | 9088 |
| random_toggle | fresh_mixed_ws400 | 0 | 27.370M | ops/s | 9088 |
| random_toggle | fresh_mixed_ws400_notouch | 0 | 51.706M | ops/s | 9088 |
| random_toggle | fresh_small_ws400 | 0 | 71.485M | ops/s | 4668 |
| random_toggle | fresh_small_ws400_notouch | 0 | 83.780M | ops/s | 9088 |
| random_toggle | medium_ws400 | 0 | 27.227M | ops/s | 76840 |
| random_toggle | mixed_ws400 | 0 | 30.823M | ops/s | 76840 |
| random_toggle | small_ws400 | 0 | 77.250M | ops/s | 76840 |
| route_invalid | direct32k | 32768 | 120.555M | ops/s | 3892 |
| route_invalid | small64 | 64 | 121.803M | ops/s | 3892 |
| route_invalid | span8k | 8192 | 121.492M | ops/s | 3892 |
| route_valid | direct32k | 32768 | 122.519M | ops/s | 3892 |
| route_valid | small64 | 64 | 117.897M | ops/s | 3892 |
| route_valid | span8k | 8192 | 118.133M | ops/s | 3892 |

Artifacts: .\docs\benchmarks\windows\hz7_v3_bootstrap_hotpath
