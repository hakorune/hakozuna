# HZ8 Matrix Snapshot 20260623T141023Z

HEAD: ab15455
Build: bench-release
Command shape: RUNS=10, T=16, iters/thread=100000, size=16..2048, class_map_id=p2-v0, bench_attribution=0

| Lane | Shape | Median ops/s | p25 | min | steady median | post RSS median | peak RSS median | Minor faults median | Phase |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|
| guard/local0 | remote_pct=0 interleaved=0 | 406.43M | 379.82M | 359.90M | 469.56M | 4.00MiB | 4.00MiB | 153 | alloc 3.474ms |
| small_interleaved_remote90 | remote_pct=90 interleaved=1 live_window=4096 | 58.74M | 57.35M | 53.16M | 60.71M | 3.42MiB | 25.86MiB | 4,900 | work 26.456ms / tail 14.048ms |
| small_phase_remote90 | remote_pct=90 interleaved=0 | 6.69M | 6.54M | 6.51M | 9.23M | 28.93MiB | 1917.07MiB | 486,031 | alloc 173.570ms / remote 10.622ms |

Raw logs:

- bench_results/20260623T141023Z_matrix_local_r10.log
- bench_results/20260623T141023Z_matrix_interleaved_r10.log
- bench_results/20260623T141023Z_matrix_phase_r10.log

Interpretation:

- local0 and interleaved remote90 clear current HZ8 v0 bring-up gates in this single R10 matrix snapshot.
- interleaved remote90 is the primary steady-state remote lane; phase remote90 remains peak-live / first-touch / lifecycle stress.
- phase remote90 peak RSS is expectedly high because 90% of objects remain live until the remote phase barrier; post-purge RSS remains low.
- HZ8 rows are from the HZ8 local harness and are not a same-run cross-allocator claim.
