# HZ7 v2 Windows Hot Path Microbench

Generated: 2026-06-11 18:11:47 +09:00

- benchmark: `bench_hz7_v2_hotpath`
- allocator: `hz7-v2`
- runs: 1
- iters_per_run: 10000000
- note: diagnostic-only; no allocator counters or production hot-path instrumentation

| op | label | size | median rate | rate unit | median peak_kb |
| --- | --- | ---: | ---: | --- | ---: |
| free_batch | direct32k | 32768 | 755.986K | ops/s | 76008 |
| free_batch | small64 | 64 | 60.680M | ops/s | 75696 |
| free_batch | span8k | 8192 | 1.695M | ops/s | 76008 |
| free_retained_loop | direct32k | 32768 | 53.360M | pairs/s | 76008 |
| free_retained_loop | small64 | 64 | 47.597M | pairs/s | 76008 |
| free_retained_loop | span8k | 8192 | 46.909M | pairs/s | 76008 |
| malloc_batch | direct32k | 32768 | 457.720K | ops/s | 75696 |
| malloc_batch | small64 | 64 | 40.691M | ops/s | 23808 |
| malloc_batch | span8k | 8192 | 593.778K | ops/s | 75696 |
| malloc_free | direct32k | 32768 | 54.148M | pairs/s | 3980 |
| malloc_free | small64 | 64 | 46.487M | pairs/s | 3900 |
| malloc_free | span8k | 8192 | 45.540M | pairs/s | 3968 |
| mixed_steady | direct_medium_ws400 | 0 | 48.728M | ops/s | 76008 |
| mixed_steady | medium_ws400 | 0 | 28.978M | ops/s | 76008 |
| mixed_steady | mixed_ws400 | 0 | 25.258M | ops/s | 76008 |
| mixed_steady | small_ws400 | 0 | 45.681M | ops/s | 76008 |
| mixed_steady | span_medium_ws400 | 0 | 31.514M | ops/s | 76008 |
| random_toggle | fresh_medium_ws400 | 0 | 34.641M | ops/s | 9908 |
| random_toggle | fresh_medium_ws400_notouch | 0 | 54.807M | ops/s | 9908 |
| random_toggle | fresh_mixed_ws400 | 0 | 34.493M | ops/s | 9908 |
| random_toggle | fresh_mixed_ws400_notouch | 0 | 52.850M | ops/s | 9908 |
| random_toggle | fresh_small_ws400 | 0 | 83.204M | ops/s | 4756 |
| random_toggle | fresh_small_ws400_notouch | 0 | 83.923M | ops/s | 9908 |
| random_toggle | medium_ws400 | 0 | 35.418M | ops/s | 76008 |
| random_toggle | mixed_ws400 | 0 | 32.783M | ops/s | 76008 |
| random_toggle | small_ws400 | 0 | 74.499M | ops/s | 76008 |
| route_invalid | direct32k | 32768 | 116.132M | ops/s | 3980 |
| route_invalid | small64 | 64 | 116.074M | ops/s | 3980 |
| route_invalid | span8k | 8192 | 116.237M | ops/s | 3980 |
| route_valid | direct32k | 32768 | 117.206M | ops/s | 3980 |
| route_valid | small64 | 64 | 111.234M | ops/s | 3980 |
| route_valid | span8k | 8192 | 111.522M | ops/s | 3980 |

Artifacts: .\docs\benchmarks\windows\hz7_v2_light_fullish\hotpath
