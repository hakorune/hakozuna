# HZ8 V0 Freeze Safety Batch 20260623T160850Z

HEAD: 2d5073a
Builds: clean smoke, safety-stress, preload-smoke, bench-release

Functional gates:

- h8_smoke: pass
- h8_safety_stress: pass
- preload smoke: pass
- timeout / abort: 0

Performance gates:

| Lane | Shape | Batch | Median ops/s | p25 | min | steady median | post RSS median | peak RSS median | Result |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|
| guard/local0 | 16..2048 remote=0 | 1 | 443.73M | 408.03M | 373.00M | 507.81M | 3.77MiB | 3.88MiB | pass |
| guard/local0 | 16..2048 remote=0 | 2 | 440.38M | 426.26M | 364.56M | 509.00M | 3.82MiB | 3.88MiB | pass |
| small_interleaved_remote90 | 16..4096 interleaved=1 | 1 | 55.25M | 54.41M | 52.09M | 56.92M | 3.34MiB | 22.29MiB | pass |
| small_interleaved_remote90 | 16..4096 interleaved=1 | 2 | 55.49M | 54.88M | 54.73M | 57.12M | 3.31MiB | 17.89MiB | pass |

Stress row:

| Lane | Shape | Median ops/s | p25 | min | steady median | post RSS median | peak RSS median | Minor faults median | Phase |
|---|---|---:|---:|---:|---:|---:|---:|---:|---|
| small_phase_remote90 | 16..4096 interleaved=0 | 3.52M | 3.48M | 3.48M | 4.85M | 38.57MiB | 3803.02MiB | 966,176 | alloc 332.171ms / remote 10.550ms |

Zero-gate evidence from release snapshots:

- remote validate_fail = 0
- duplicate_claim = 0
- quiescent_pending bitmap_nonzero = 0
- quiescent_pending repair = 0
- local_zero_gates alloc/free/live = 0
- slot_shadow mismatch counters = 0

Interpretation:

- V0FreezeSafetyBatch-L1 passes.
- small v0 can be soft-frozen for same-run allocator matrix work.
- phase-separated 16..4096 remote90 is not a primary throughput gate; it is a peak-live / first-touch / lifecycle / RSS stress row.
- post RSS recovers after phase stress; high peak RSS matches the barrier live-set shape.

Raw logs:

- bench_results/20260623T160850Z_v0_freeze_smoke.log
- bench_results/20260623T160850Z_v0_freeze_safety.log
- bench_results/20260623T160850Z_v0_freeze_preload.log
- bench_results/20260623T160850Z_v0_freeze_local_b1_r10.log
- bench_results/20260623T160850Z_v0_freeze_local_b2_r10.log
- bench_results/20260623T160850Z_v0_freeze_interleaved4096_b1_r10.log
- bench_results/20260623T160850Z_v0_freeze_interleaved4096_b2_r10.log
- bench_results/20260623T160850Z_v0_freeze_phase4096_r10.log
