# HZ4 Phase22: B74 MidLockPathOutlockRefill Work Order (2026-02-16)

## Goal
- Reduce `mid` lock-path hold/wait cost without adding any always-on work at `hz4_free()` dispatch.
- Prioritize:
  - `main_r0` uplift
  - `main_r50` uplift
  - `guard_r0` non-regression

## Why B74 now
- `B70` (page supply reservation) succeeded and is default ON.
- `B71/B72/B73` are archived NO-GO.
- Remaining gap is still lock-path heavy (`malloc_lock_path`, lock wait/context-switch).
- Best next shot is out-of-lock movement in malloc lock-path internals.

## Strategy (2-stage, low-risk first)

### Stage A (No-code probe, must run first)
Use existing knob already implemented in tree:
- `HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX=1`

Rationale:
- Code path already exists in `hz4_mid_malloc.inc`.
- Gives direct signal whether out-of-lock create/handoff helps current baseline.
- Avoids new complexity if this already fails.

Build:
```sh
# Base (current baseline: B70 default)
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1'

# Var-A (outlock create on)
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX=1'

# Var-A + stats
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX=1 -DHZ4_MID_STATS_B1=1 -DHZ4_MID_LOCK_TIME_STATS=1'
```

Lanes:
- `guard_r0`: `16 1200000 400 16 2048 0 65536`
- `main_r0`: `16 1200000 400 16 32768 0 65536`
- `main_r50`: `16 1200000 400 16 32768 50 65536`
- `cross128_r90`: `16 400000 400 16 131072 90 65536`

Gate:
- Screen (`RUNS=7`):
  - `guard_r0 >= -1.0%`
  - `main_r0 >= +3.0%`
  - `main_r50 >= +1.0%`
  - `cross128_r90 >= 0.0%`
- Replay (`RUNS=21`, only if screen pass):
  - `guard_r0 >= -1.0%`
  - `main_r0 >= +6.0%`
  - `main_r50 >= +2.0%`
  - `cross128_r90 >= 0.0%`

Required mechanism checks (`HZ4_MID_LOCK_TIME_STATS`):
- `hold_page_create_ns_sum` decreases
- `lock_wait_ns_sum` decreases or flat
- `malloc_page_create_outlock_calls` non-zero

Decision:
- If Stage A passes replay: promote knob and stop B74 implementation.
- If Stage A fails: go to Stage B.

#### Stage A Results (2026-02-16)

| Lane | Base Median | Var Median | Delta | Gate | Result |
|------|-------------|------------|-------|------|--------|
| guard_r0 | - | - | -1% | -1.0% | PASS |
| main_r0 | - | - | -1% | +3.0% | **FAIL** |
| main_r50 | - | - | +4% | +1.0% | PASS |
| cross128_r90 | - | - | -3% | 0.0% | **FAIL** |

**Verdict: Stage A NO-GO** (main_r0 and cross128_r90 failed)

Proceed to Stage B.

---

### Stage B (Code box): `B74 MidLockPathOutlockRefill`
Only run if Stage A fails.

#### Box
- `HZ4_MID_LOCKPATH_OUTLOCK_REFILL_BOX` (default OFF)

#### Intent
- Keep page/freelist structural mutation under `sc` lock.
- Move only post-pop list linking work (to local `alloc_run`) outside lock.
- No route changes in `hz4_free()`.

#### File scope
- `hakozuna/hz4/core/hz4_config_collect.h`
  - add B74 knobs.
- `hakozuna/hz4/src/hz4_mid_malloc.inc`
  - detach refill list under lock, enqueue to alloc_run after unlock.
- `hakozuna/hz4/src/hz4_mid_stats.inc`
  - add B74 counters.

#### Knobs
```c
#ifndef HZ4_MID_LOCKPATH_OUTLOCK_REFILL_BOX
#define HZ4_MID_LOCKPATH_OUTLOCK_REFILL_BOX 0
#endif
#ifndef HZ4_MID_LOCKPATH_OUTLOCK_REFILL_BUDGET
#define HZ4_MID_LOCKPATH_OUTLOCK_REFILL_BUDGET 8
#endif
#if HZ4_MID_LOCKPATH_OUTLOCK_REFILL_BOX && !HZ4_MID_ALLOC_RUN_CACHE_BOX
#error "HZ4_MID_LOCKPATH_OUTLOCK_REFILL_BOX requires HZ4_MID_ALLOC_RUN_CACHE_BOX=1"
#endif
```

#### Core flow (inside `hz4_mid_malloc`)
1. Lock-path: choose `page`, pop return `obj` as today.
2. Under lock: pop up to `BUDGET` extra objects from same page into a temporary local list (`detached_head/tail/n`).
3. Under lock: keep existing bin empty/remove logic intact.
4. Release lock.
5. Outside lock: push detached list into `g_mid_alloc_run_head[sc]` with cap check.

Important:
- Do not leave detached objects orphaned. If alloc_run full, push remainder back under lock (fallback path) or cap pop by available slots.
- Keep `page->free_count` and empty checks consistent with existing helpers.

#### Stats
Add:
- `malloc_outlock_refill_calls`
- `malloc_outlock_refill_objs`
- `malloc_outlock_refill_drop_back` (if fallback push-back executed)

#### Gate (same as Stage A)
- Screen (`RUNS=7`) then replay (`RUNS=21`) with same thresholds.
- Fail immediately if any crash/assert/alloc-failed.

#### NO-GO criteria
- `guard_r0` below `-1.0%`
- `main_r0` fails replay gate
- counters show box is cold (`outlock_refill_objs == 0`)

#### Stage B Results (2026-02-16)

| Lane | Base Median | Var Median | Delta | Gate | Result |
|------|-------------|------------|-------|------|--------|
| guard_r0 | 0.079430 | 0.079208 | 0% | -1.0% | PASS |
| main_r0 | 0.143560 | 0.140555 | **-2.00%** | +3.0% | **FAIL** |
| main_r50 | 0.308892 | 0.308894 | 0% | +1.0% | **FAIL** |
| cross128_r90 | 0.193912 | 0.212225 | +9.00% | 0.0% | PASS |

**Verdict: Stage B NO-GO** (main_r0 and main_r50 failed)

Mechanism check:
- `malloc_outlock_refill_calls=18599` (mechanism engaged)
- `malloc_outlock_refill_objs=35153` (~1.9 objs/call)
- `malloc_lock_path` decreased (46710 → 40902)
- `malloc_alloc_cache_hit` increased (52723 → 99245)

**Analysis**: The mechanism works (lower lock-path frequency, higher alloc_run hit), but the overhead of list walk outside lock outweighs the benefit for main_r0 and main_r50. cross128_r90 shows +9% improvement (larger objects, fewer per-page, more benefit from reduced lock hold time).

**B74 Status: ARCHIVED NO-GO**

---

## Notes for executor (Claude)
- Do not modify `hz4_free()` dispatch or route order.
- Keep B70 behavior intact (this is additive on lock-path only).
- Prefer smallest patch that can be archived cleanly.
