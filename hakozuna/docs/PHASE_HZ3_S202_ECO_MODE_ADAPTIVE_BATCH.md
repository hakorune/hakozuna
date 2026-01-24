# S202: Eco Mode - Adaptive Batch Sizing

## Background

### S200 CPU Efficiency Baseline (2026-01-20)

| Bench | ops/s | CPU% | ops/s per CPU% |
|-------|-------|------|----------------|
| random_mixed T=1 | 168.04M | 93% | 1.81M |
| mt_remote T=8 R90 | 128.23M | 369% | 0.35M |
| redis T=4 | 4.50M | 284% | 0.016M |

### S201 Static Batch 2x Results (2026-01-20)

| Bench | Base eff | Eco eff | Diff |
|-------|----------|---------|------|
| random_mixed T=1 | 1.75M/CPU% | 1.76M/CPU% | +0.6% |
| mt_remote T=8 | 0.27M/CPU% | 0.42M/CPU% | **+55.6%** |
| redis T=4 | 0.015M/CPU% | 0.006M/CPU% | **-60.0%** |

**Conclusion**: Static batch 2x is **CONDITIONAL GO**
- WIN for high-rate MT workloads (mt_remote)
- LOSS for low-rate I/O-bound workloads (redis)

## S202 Design

### Goal
Implement adaptive batch sizing that increases batch size only under high allocation rate.

### Two-Stage Batch Sizing

| Stage | S74_REFILL_BURST | S118_SMALL_V2_REFILL_BATCH | When |
|-------|------------------|----------------------------|------|
| SMALL (default) | 8 | 32 | alloc_rate < 10M/sec |
| LARGE (eco) | 16 | 64 | alloc_rate >= 10M/sec |

### Sampling Strategy

- Sample at **refill boundary** (not per-alloc) to minimize hot path overhead
- Trigger on **time OR count** (whichever first)
  - Time: 1 second interval
  - Count: 1M ops
- Low-rate workloads (redis ~4M ops/s) won't reach 1M ops, so time trigger is essential

### Control

- Compile-time: `-DHZ3_ECO_MODE=1` (default OFF)
- Runtime: `HZ3_ECO_ENABLED=1` env var
- Threshold: `HZ3_ECO_RATE_THRESH=10000000` (optional)

## Files

- `hakozuna/hz3/src/hz3_eco_mode.c` (new)
- `hakozuna/hz3/include/hz3_eco_mode.h` (new)
- Integration: `hz3_shim.c`, `hz3_tcache.c`, `hz3_small_v2.c`

## Expected Results

| Bench | Expected Stage | Expected Efficiency |
|-------|----------------|---------------------|
| random_mixed | SMALL | ~neutral (±5%) |
| mt_remote | LARGE | +40-55% |
| redis | SMALL | ~neutral (no -60% regression) |

## Results (2026-01-20)

| Bench | Stage | Baseline | Eco Mode | Diff |
|-------|-------|----------|----------|------|
| random_mixed T=1 | SMALL | 1.81M/CPU% | 1.77M/CPU% | ~neutral |
| mt_remote T=8 R90 | LARGE | 0.329M/CPU% | 0.373M/CPU% | **+13.4%** |
| redis T=4 | SMALL | 0.015M/CPU% | 0.016M/CPU% | **+7%** |

補足:
- mt_remote は長時間ベンチ（10M iters）で **throughput +59.5%** を確認。
- 短時間ベンチ（<1s）では warm-up が足りず、stage 切替が効かないことがある。

## Status

- [x] S200: CPU efficiency baseline
- [x] S201: Static batch 2x (CONDITIONAL GO)
- [x] S202: Adaptive batch (GO, redis regression avoided)
