# HZ4 Phase18: SmallFree Fixed-Cost Attack (B33 First)

Date: 2026-02-14  
Status: B33 GO(default)

## Goal
- Recover `guard_r0` by reducing small local free fixed cost.
- Preserve MT wins (`main_r50`, `cross128_r90`) while improving small-only lane.

## Why this order
- B26/B27 observation lock:
  - `large_validate` calls can be reduced strongly, but guard uplift stays limited.
  - next hotspot is inside small local free path, not route-order switching.
- B30 (mid lock bypass) was hard-NO-GO with large regressions.
- Therefore first attack is B33 (`useddec` fixed-cost), not new route branches.

## Box boundary
- Code boundary: `hakozuna/hz4/src/hz4_tcache_free.inc`
  - local owner free path `used_count` decrement only.
- Knob boundary: `hakozuna/hz4/core/hz4_config_collect.h`
  - `HZ4_ST_FREE_USEDDEC_RELAXED`

## Safety rules
- Relaxed useddec path is allowed only when all are false:
  - `HZ4_PAGE_DECOMMIT`
  - `HZ4_SEG_RELEASE_EMPTY`
  - `HZ4_CENTRAL_PAGEHEAP`
- Otherwise automatically fallback to strict `hz4_page_used_dec_meta()`.

## A/B gate (screen)
- Runs: `RUNS=10`, interleaved, pinned CPU
- Lanes:
  - `guard_r0` (`16..2048`, `R=0`)
  - `main_r50` (`16..32768`, `R=50`)
  - `cross128_r90` (`16..131072`, `R=90`)
- Pass:
  - `guard_r0 >= -0.5%` (non-regressive lock)
  - `main_r50 >= 0%`
  - `cross128_r90 >= 0%`
  - crash/abort/assert/alloc-failed = 0

## Promotion rule
- If screen passes:
  - make B33 default in config
  - run `larson` quick gate (`rc=0`)
  - update `CURRENT_TASK.md` and `HZ4_GO_BOXES.md`

## B33 execution result (2026-02-14)
- Compare:
  - base: `-DHZ4_ST_FREE_USEDDEC_RELAXED=0`
  - var: default (`HZ4_ST_FREE_USEDDEC_RELAXED=1`)
- Screen A/B (`RUNS=10`, interleaved, pinned `0-15`):
  - artifact: `/tmp/hz4_b33_default_ab_20260214_123959/summary.tsv`
  - replay: `/tmp/hz4_b33_default_ab_rerun_20260214_124046/`
  - screen-1:
    - `guard_r0`: `-0.38%`
    - `main_r50`: `+0.92%`
    - `cross128_r90`: `+6.62%`
  - replay:
    - `guard_r0`: `-0.11%`
    - `main_r50`: `+2.61%`
    - `cross128_r90`: `+4.45%`
- larson quick:
  - artifact: `/tmp/hz4_b33_larson_20260214_124126/summary.tsv`
  - throughput median: `-0.21%` (`rc=0` all runs)
- Decision:
  - `B33` promoted to default (`HZ4_ST_FREE_USEDDEC_RELAXED=1`).
  - Interpretation lock:
    - not a direct guard-recovery box.
    - stable gain source is `main_r50/cross128_r90` uplift with guard neutral.
