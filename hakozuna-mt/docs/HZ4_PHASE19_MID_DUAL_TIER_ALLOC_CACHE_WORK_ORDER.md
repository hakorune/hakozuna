# HZ4 Phase19: MidDualTierAllocCacheBox Work Order (2026-02-15)

## Status
- Result: **NO-GO** (2026-02-15), hard-archived.
- Archive: `hakozuna/archive/research/hz4_mid_dual_tier_alloc_box/README.md`
- Mainline state: B46 ON-path code removed.
- Final artifact: `/tmp/hz4_b46_ab_20260215_083933/REPORT.md`
- This document is kept as execution record only.

## Goal
Reduce `main_r0` gap by lowering mid lock-path pressure without touching owner/large routes.

- Primary KPI:
  - lower `malloc_lock_path` ratio in `main_r0`
  - lower `context-switches` in `main_r0`
- Keep gates:
  - no regression in `guard_r0`, `main_r50`, `cross128_r90`

## Locked Evidence
- Small/local observation:
  - `/tmp/hz4_small_local_obs_20260215_080601/REPORT.md`
- Key facts:
  - `main_r0`: `hz4` vs `tcmalloc` is `-44.79%`
  - `main_r0` context-switches: `14140` vs `1333` (`+960%`)
  - `main_r0` midstats: `malloc_lock_path=2032016 / malloc_calls=15008441` (`~13.5%`)
- Phase1 sweep (alloc-run limit only) hit headroom:
  - `/tmp/hz4_mid_ar_sweep_20260215_090000/summary.tsv`
  - `ALLOC_RUN in {8,10,12,14,16}` did not achieve `main_r0 >= +3%`

## Box Definition (B46)
Name: `MidDualTierAllocCacheBox`

Concept:
- Keep current alloc-run cache as L1.
- Add a tiny L0 lockless cache per-sizeclass in TLS.
- Malloc order: `L0 -> L1 -> existing path`.
- L0 refill is only from L1 (no lock, no new global structure).
- No lock-path work increase, no mutex policy changes.

## Boundaries (strict)
Edit only these files:
- `hakozuna/hz4/core/hz4_config_collect.h`
- `hakozuna/hz4/src/hz4_mid_batch_cache.inc`
- `hakozuna/hz4/src/hz4_mid_malloc.inc`
- `hakozuna/hz4/src/hz4_mid_stats.inc` (counter only)

Do not edit:
- owner box (`hz4_mid_owner_remote.inc`)
- large box (`hz4_large.c`)
- lock implementation (`pthread_mutex_*`)

## Knobs (opt-in first)
Add to `hz4_config_collect.h`:

```c
#ifndef HZ4_MID_DUAL_TIER_ALLOC_BOX
#define HZ4_MID_DUAL_TIER_ALLOC_BOX 0
#endif

#ifndef HZ4_MID_DUAL_TIER_L0_LIMIT
#define HZ4_MID_DUAL_TIER_L0_LIMIT 4
#endif

#ifndef HZ4_MID_DUAL_TIER_L0_REFILL_BATCH
#define HZ4_MID_DUAL_TIER_L0_REFILL_BATCH 2
#endif

#if HZ4_MID_DUAL_TIER_ALLOC_BOX && !HZ4_MID_ALLOC_RUN_CACHE_BOX
#error "HZ4_MID_DUAL_TIER_ALLOC_BOX requires HZ4_MID_ALLOC_RUN_CACHE_BOX=1"
#endif
#if HZ4_MID_DUAL_TIER_L0_LIMIT < 1 || HZ4_MID_DUAL_TIER_L0_LIMIT > 16
#error "HZ4_MID_DUAL_TIER_L0_LIMIT must be in [1,16]"
#endif
#if HZ4_MID_DUAL_TIER_L0_REFILL_BATCH < 1 || HZ4_MID_DUAL_TIER_L0_REFILL_BATCH > HZ4_MID_DUAL_TIER_L0_LIMIT
#error "HZ4_MID_DUAL_TIER_L0_REFILL_BATCH must be in [1,L0_LIMIT]"
#endif
```

## Implementation Steps
1. Add L0 TLS storage and helpers in `hz4_mid_batch_cache.inc`.
2. Add `hz4_mid_alloc_l0_pop(sc)` and `hz4_mid_alloc_l0_refill_from_l1(sc)`.
3. Integrate in `hz4_mid_malloc.inc` before L1 pop:
   - try L0 pop
   - if miss, refill L0 from L1 then pop once
   - then proceed to existing L1/free-batch/lock path
4. Add stats (only when `HZ4_MID_STATS_B1=1`):
   - `malloc_l0_hit`
   - `malloc_l0_refill_calls`
   - `malloc_l0_refill_objs`
5. Keep fail-fast behavior unchanged.

## Required Invariants
- L0 stores only valid objects belonging to target `sc` page.
- No object duplication between L0 and L1 at the same time.
- Lock-path code path remains functionally identical when box is OFF.
- OFF path is byte-for-byte equivalent in behavior.

## Build Matrix
Base (current default):
```bash
make -C hakozuna/hz4 clean libhakozuna_hz4.so
cp -f hakozuna/hz4/libhakozuna_hz4.so /tmp/hz4_b46_base.so
```

Var (B46 ON):
```bash
make -C hakozuna/hz4 clean libhakozuna_hz4.so \
  HZ4_DEFS_EXTRA='-DHZ4_MID_DUAL_TIER_ALLOC_BOX=1'
cp -f hakozuna/hz4/libhakozuna_hz4.so /tmp/hz4_b46_var.so
```

Var+stats (mechanism check):
```bash
make -C hakozuna/hz4 clean libhakozuna_hz4.so \
  HZ4_DEFS_EXTRA='-DHZ4_MID_DUAL_TIER_ALLOC_BOX=1 -DHZ4_MID_STATS_B1=1'
cp -f hakozuna/hz4/libhakozuna_hz4.so /tmp/hz4_b46_var_stats.so
```

## A/B Protocol
Bench: `./hakozuna/out/bench_random_mixed_mt_remote_malloc`

Lanes:
- `guard_r0`: `16 2000000 400 16 2048 0 65536`
- `main_r0`: `16 2000000 400 16 32768 0 65536`
- `main_r50`: `16 2000000 400 16 32768 50 65536`
- `cross128_r90`: `16 600000 400 16 131072 90 65536`

Screen gate (`RUNS=7`, interleaved):
- `main_r0 >= +3.0%`
- `guard_r0 >= -1.0%`
- `main_r50 >= -1.0%`
- `cross128_r90 >= -1.0%`
- crash/abort/assert/alloc-failed = `0`

Replay gate (`RUNS=11`):
- keep screen results direction
- `main_r0` remains positive
- no regression worse than `-1.0%` in other 3 lanes

## Mechanism Check (1-shot)
Run `main_r0` once with stats build and confirm:
- `malloc_l0_hit > 0`
- `malloc_lock_path` lower vs base stats run

## NO-GO Conditions
- `main_r0 < +3.0%` on screen
- any lane below `-1.0%`
- `malloc_lock_path` not improving while L0 hits are present
- crashes or correctness failures

## Archive Rule
If NO-GO, archive as:
- `hakozuna/archive/research/hz4_mid_dual_tier_alloc_box/README.md`
and fully remove ON-path code from mainline.

## Deliverables
- `/tmp/hz4_b46_ab_*/raw.tsv`
- `/tmp/hz4_b46_ab_*/summary.tsv`
- `/tmp/hz4_b46_ab_*/REPORT.md`
- optional perf bundle if borderline
