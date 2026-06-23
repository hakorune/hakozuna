# HZ8 Remeasure Summary 20260623T145238Z

HEAD: 9127838
Build: bench-release
Command shape: RUNS=10, T=16, iters/thread=100000, size=16..2048, class_map_id=p2-v0, bench_attribution=0

| Lane | Shape | Median ops/s | p25 | min | steady median | post RSS median | peak RSS median | Minor faults median | Phase |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|
| guard/local0 | remote_pct=0 interleaved=0 | 409.27M | 380.63M | 360.34M | 465.98M | 3.88MiB | 3.88MiB | 153 | alloc 3.555ms |
| small_interleaved_remote90 | remote_pct=90 interleaved=1 live_window=4096 | 57.01M | 56.59M | 54.11M | 59.12M | 3.31MiB | 25.28MiB | 5,156 | work 27.067ms / tail 13.821ms |
| small_phase_remote90 | remote_pct=90 interleaved=0 | 6.68M | 6.62M | 6.44M | 9.20M | 28.93MiB | 1917.37MiB | 486,030 | alloc 174.490ms / remote 10.516ms |

Previous matrix reference: bench_results/20260623T141023Z_hz8_matrix_snapshot.md

Delta vs previous R10 matrix:

| Lane | Previous median | Current median | Delta |
|---|---:|---:|---:|
| guard/local0 | 406.43M | 409.27M | +0.7% |
| small_interleaved_remote90 | 58.74M | 57.01M | -2.9% |
| small_phase_remote90 | 6.69M | 6.68M | -0.1% |

Interpretation:

- Re-measure confirms the post-evidence-knob release shape stayed in the same performance band.
- local0 and interleaved remote90 remain above v0 bring-up gates.
- phase remote90 remains a peak-live / first-touch / lifecycle stress row, not the primary steady remote performance row.
- Latest data supports freezing the current small remote protocol unless a paired R10x2 regression appears.

Raw logs:

- bench_results/20260623T145238Z_remeasure_local_r10.log
- bench_results/20260623T145238Z_remeasure_interleaved_r10.log
- bench_results/20260623T145238Z_remeasure_phase_r10.log
