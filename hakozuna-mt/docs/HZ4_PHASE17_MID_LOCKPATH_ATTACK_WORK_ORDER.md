# HZ4 Phase17 Mid LockPath Attack Work Order (2026-02-14)

## Goal
- Reflect the latest perf lock in one place.
- Freeze the next attack order after B27.
- Record the first mid lock-path implementation (`B30`) and its quick verdict.

## Perf Lock (source of truth)
- Artifact:
  - `/tmp/hz4_b27_probe_fixfull_20260214_110931/`
- Observed:
  - `guard_r0`: `large_validate` calls can be reduced, but throughput gain is limited.
  - `main_r50` / `cross128_r90`: hot remains `hz4_mid_malloc` lock boundary.
  - `cross128_r90`: large path is still relevant, but second priority after mid lock-path.
- Decision:
  - Next optimization must attack `hz4_mid_malloc` lock path first.
  - Avoid adding always-on free-route branches in `hz4_free`.

## B30: MidFreeBatchLockBypassBox
- Boundary:
  - `hakozuna/hz4/core/hz4_config_archived.h` (guarded)
  - `hakozuna/hz4/src/hz4_mid_batch_cache.inc`
  - `hakozuna/hz4/src/hz4_mid_malloc.inc`
- Knobs:
  - `HZ4_MID_FREE_BATCH_LOCK_BYPASS_BOX` (hard-archived)
  - `HZ4_MID_FREE_BATCH_LOCK_BYPASS_MIN` (default `1`)
- Mechanism:
  - Only when malloc is about to enter lock-path, permit low-threshold
    `free-batch -> alloc-run` refill once.
  - Historical one-shot counter (used during lock only):
    - `malloc_free_batch_lock_bypass_hit`

## B30 quick verdict
- Smoke (counters):
  - `/tmp/hz4_b30_smoke_20260214_111803/summary.txt`
  - `main_r50/cross128_r90` show non-zero bypass hits (mechanism engaged).
- Quick A/B (`RUNS=1`, release):
  - `/tmp/hz4_b30_quick_ab_20260214_111830/summary.csv`
  - `guard_r0 +0.73%`
  - `main_r50 -46.11%`
  - `cross128_r90 -54.65%`
- Decision:
  - Current shape is NO-GO and hard-archived.
  - Runtime cleanup completed:
    - removed B30 branch from `hz4_mid_malloc` hot path
    - removed B30 helper/counter additions
  - archive note:
    - `hakozuna/archive/research/hz4_mid_free_batch_lock_bypass_box/README.md`

## Next Attack Order (fixed)
1. Mid lock boundary: reduce lock hold/acquire cost without route-condition overhead.
2. Re-check large path for `cross128` only after 1) is stable.
3. Keep B27/B30 as opt-in references (no default impact).
