# PHASE S236-M: Mini-Refill Source Arbiter Work Order

Date: 2026-02-17
Status: executed (NO-GO at Stage A)

## Goal

- Improve main lane (16..32768 r90) by >= +3.0%
- Don't break guard/larson_proxy (>= -3.0%)
- Per-SC source arbiter that adapts inbox/central order based on recent hit history

## Context

- S236-K, S236-L, S44-2 all NO-GO
- Fixed inbox→central order may be suboptimal
- Hypothesis: Some SCs benefit from central-first order

## Constraints

- Do NOT modify hz3_free / remote dispatch
- Do NOT reuse archived S236E/F/G/I
- Only change mini-refill boundary
- Fallback only once (no extra retries)

---

## Stage A: Observation (No Code Change)

Build: `-DHZ3_S203_COUNTERS=1`

Check counters (main lane only):
- `s236_mini_miss / s236_mini_calls >= 0.10`
- `from_inbox / alloc_slow_calls >= 0.20`
- `from_central / alloc_slow_calls >= 0.20`

Proceed to Stage B only if ALL three conditions met.

---

## Stage A Observation Result (2026-02-17)

Build: `-DHZ3_S203_COUNTERS=1`

Main lane samples (16 2000000 400 16 32768 90 65536):

| Sample | mini_miss | mini_calls | miss_rate | from_inbox | from_central | alloc_slow |
|--------|-----------|------------|-----------|------------|--------------|------------|
| 1 | 22901 | 691605 | 3.31% | 617268 (64.6%) | 293760 (30.8%) | 954945 |
| 2 | 28375 | 701914 | 4.04% | 601418 (62.5%) | 306337 (31.8%) | 962365 |
| 3 | 31256 | 713813 | 4.38% | 597084 (61.3%) | 316758 (32.5%) | 973815 |
| 4 | 24790 | 686483 | 3.61% | 613299 (64.6%) | 288194 (30.4%) | 949189 |

**Condition check:**
| Condition | Required | Actual | Result |
|-----------|----------|--------|--------|
| mini_miss / mini_calls | >= 10% | 3.31% - 4.38% | **FAIL** |
| from_inbox / alloc_slow_calls | >= 20% | 61.3% - 64.6% | PASS |
| from_central / alloc_slow_calls | >= 20% | 30.4% - 32.5% | PASS |

**Decision: Stage A FAIL → S236-M NO-GO**

The mini-refill miss rate is too low (~3-4%) to justify source arbiter implementation.
The current inbox→central order already achieves ~96% hit rate on mini-refill.

**Next steps:**
- Do not implement S236-M (no code change needed)
- Explore different optimization avenues
- Archive result:
  - `hakozuna/hz3/archive/research/s236m_minirefill_source_arbiter/README.md`

---

## Stage B: Code Implementation

### Files to modify

1. `hakozuna/hz3/include/config/hz3_config_scale_part8_modern.inc` (~line 847)
   - Add new knobs:
     - `HZ3_S236M_MINI_SOURCE_ARBITER` (default 0)
     - `HZ3_S236M_INBOX_EMPTY_STREAK_MIN` (default 3)
     - `HZ3_S236M_STICKY_EPOCH` (default 8)

2. `hakozuna/hz3/src/hz3_tcache_slowpath.inc` (~line 196)
   - Add per-SC sticky source preference
   - Switch order when inbox miss streak + central hit streak thresholds met
   - Fallback only once (no extra retries)

3. `hakozuna/hz3/src/hz3_tcache.c` (~line 66, S203 dump)
   - Add counters:
     - `s236m_pref_inbox`
     - `s236m_pref_central`
     - `s236m_hit_pref`
     - `s236m_hit_fallback`
     - `s236m_switch_to_central`
     - `s236m_switch_to_inbox`

### Algorithm

```
// Per-SC state (TLS)
sticky_source[sc]        // 0=inbox first, 1=central first
inbox_miss_streak[sc]    // consecutive inbox misses
central_hit_streak[sc]   // consecutive central hits (on fallback)

// In mini-refill:
if (HZ3_S236M_MINI_SOURCE_ARBITER) {
    if (sticky_source[sc] == 0) {
        // inbox first
        ptr = try_inbox();
        if (ptr) {
            inbox_miss_streak[sc] = 0;
            central_hit_streak[sc] = 0;  // reset on inbox hit
            hit_pref++;
        } else {
            inbox_miss_streak[sc]++;
            ptr = try_central();
            if (ptr) {
                hit_fallback++;
                central_hit_streak[sc]++;
                if (inbox_miss_streak[sc] >= STREAK_MIN && central_hit_streak[sc] >= EPOCH) {
                    sticky_source[sc] = 1;  // switch to central first
                    switch_to_central++;
                }
            }
        }
    } else {
        // central first
        ptr = try_central();
        if (ptr) {
            hit_pref++;
        } else {
            ptr = try_inbox();
            if (ptr) {
                hit_fallback++;
                sticky_source[sc] = 0;  // switch back to inbox first
                central_hit_streak[sc] = 0;
                switch_to_inbox++;
            }
        }
    }
} else {
    // legacy path (inbox first)
}
```

### Compile Guards

```c
#if HZ3_S236M_MINI_SOURCE_ARBITER
#  if !HZ3_S236_MINIREFILL
#    error "HZ3_S236M_MINI_SOURCE_ARBITER=1 requires HZ3_S236_MINIREFILL=1"
#  endif
#  if !HZ3_S236_MINIREFILL_TRY_INBOX
#    error "HZ3_S236M_MINI_SOURCE_ARBITER=1 requires HZ3_S236_MINIREFILL_TRY_INBOX=1"
#  endif
#  if !HZ3_S236_MINIREFILL_TRY_CENTRAL_FAST
#    error "HZ3_S236M_MINI_SOURCE_ARBITER=1 requires HZ3_S236_MINIREFILL_TRY_CENTRAL_FAST=1"
#  endif
#endif
```

---

## Stage C: A/B Testing

Base: `-DHZ3_S203_COUNTERS=1`
Var: `-DHZ3_S203_COUNTERS=1 -DHZ3_S236M_MINI_SOURCE_ARBITER=1`

Lanes (RUNS=10, interleaved, pinned 0-15):
- main: `16 2000000 400 16 32768 90 65536`
- guard: `16 2000000 400 16 2048 90 65536`
- larson_proxy: `4 2500000 400 4096 32768 0 65536`

Screen gates:
- `main >= +3.0%`
- `guard >= -3.0%`
- `larson_proxy >= -3.0%`
- hard-kill: `alloc_slow_calls` median (main lane) increase > +5% vs base

If pass: Replay with RUNS=21

If fail:
- Revert implementation
- Archive to `hakozuna/hz3/archive/research/s236m_minirefill_source_arbiter/`
- Update `hakozuna/hz3/docs/NO_GO_SUMMARY.md`
