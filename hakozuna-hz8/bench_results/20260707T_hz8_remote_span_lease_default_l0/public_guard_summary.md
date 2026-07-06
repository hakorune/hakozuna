# HZ8 remote span lease default public guard

Date: 2026-07-07
Box: HZ8RemoteSpanLeasePublish-L0 default enablement guard
Allocator: libhakozuna_hz8_preload.so (default flags include H8_REMOTE_SPAN_LEASE_PUBLISH_L1 and H8_REMOTE_TRANSITION_BACKOFF_L1)
Bench: hakozuna-hz8/bench/out/bench_matrix_malloc
Conditions: RUNS=3, THREADS=16, ITERS=50000

| row | median ops/s | median post RSS | median peak RSS |
| --- | ---: | ---: | ---: |
| small_interleaved_remote90 | 14733601.023 | 3031040 B | 31064064 B |
| main_interleaved_r90 | 6835922.864 | 4784128 B | 68550656 B |
| medium_interleaved_r50 | 9592334.190 | 4104192 B | 62914560 B |
| main_local0 | 116138353.298 | 3670016 B | 3801088 B |
| medium_local0 | 109886608.009 | 3014656 B | 3014656 B |

Verdict: PASS. Default preload preserves the low post-RSS public guard band while fixing the xmalloc freeze.
