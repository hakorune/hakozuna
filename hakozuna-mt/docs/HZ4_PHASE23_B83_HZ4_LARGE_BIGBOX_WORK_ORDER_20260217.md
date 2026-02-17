# HZ4 Phase23: B83 Large Big-Box Retry Work Order (One-Shot)

Date: 2026-02-17
Scope: `hz4` retry line after `B82` freeze
Policy: `hz4` retry is **big-box only** (no micro-box / no knob sweep loop)
Status: executed (NO-GO at Stage A)

---

## 1) Background Lock

- `hz4 mid` line is frozen (`B82` Stage A failed: lock-path ratio 4%).
- Re-opening `mid` with small knobs is prohibited.
- `hz4` retry is allowed only if:
  - evidence shows non-trivial headroom in `large` path, and
  - implementation is a single large-scope box with clear boundary.

Reference:
- `hakozuna/hz4/docs/HZ4_PHASE23_B82_NO_GO_20260217.md`

---

## 2) Stage A: Observation First (No Code Change)

### Build (baseline)

```sh
BASE_DEFS='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_STATS_B1=1 -DHZ4_MID_LOCK_TIME_STATS=1 -DHZ4_MID_LOCK_TIME_STATS_SAMPLE_SHIFT=7'
make -C hakozuna/hz4 clean all HZ4_DEFS_EXTRA="$BASE_DEFS"
```

### Lanes (RUNS=7 screen)

- `guard_r0`: `16 1200000 400 16 2048 0 65536`
- `main_r0`: `16 1200000 400 16 32768 0 65536`
- `main_r50`: `16 1200000 400 16 32768 50 65536`
- `cross128_r90`: `16 400000 400 16 131072 90 65536`
- `large_r0`: `16 400000 400 16 131072 0 65536`

### Perf (required)

- `main_r0` and `large_r0`, both `hz4` and `tcmalloc`:
  - `cycles,instructions,branch-misses,cache-misses,context-switches,page-faults`
- Collect top stacks (`perf record -g`) for `large_r0`.

---

## 3) Stage A Proceed / Stop Gate

Proceed to Stage B **only if all are true**:

1. `hz4_large_r0 / tcmalloc_large_r0 <= 0.90`
2. `hz4_main_r0 / tcmalloc_main_r0 <= 0.70`
3. `large` symbols are meaningful in `main_r0` profile:
   - combined share of `hz4_large_*` + `hz4_os_large_*` >= `15%`

If any condition fails:

- Stop `hz4` retry immediately.
- Record as `NO-GO (no large headroom)`.
- Move effort to `hz3 aggressive`.

---

## Stage A Execution Result (2026-02-17, RUNS=7)

Run artifacts: `/tmp/b83_stage_a_20260217_133027/`

### Benchmark Results

| Lane | hz4 | tcmalloc | ratio |
|------|-----|----------|-------|
| guard_r0 | 236.2M | 364.6M | 0.65 |
| main_r0 | 51.8M | 220.7M | 0.23 |
| main_r50 | 38.0M | 72.1M | 0.53 |
| cross128_r90 | 33.4M | 7.4M | 4.51 |
| large_r0 | 51.7M | 10.4M | **4.98** |

### Gate Check

| Condition | Required | Actual | Result |
|-----------|----------|--------|--------|
| hz4_large_r0 / tcmalloc_large_r0 | <= 0.90 | 4.98 | **FAIL** |
| hz4_main_r0 / tcmalloc_main_r0 | <= 0.70 | 0.23 | PASS |
| large symbols in main_r0 profile | >= 15% | ~2.86% | **FAIL** |

### Perf Analysis (main_r0)

Top symbols:
- `hz4_mid_malloc`: 37.13%
- `hz4_mid_free`: 25.83%
- `bench_thread`: 7.12%
- `hz4_large_header_valid`: **2.86%** (only large symbol visible)

### Decision: B83 NO-GO

**Reason:** hz4 is already 5x faster than tcmalloc on large path. No headroom for large optimization.

**Analysis:**
- hz4 dominates tcmalloc on large/cross128 lanes (4-5x faster)
- hz4 is slower on main/guard lanes (0.23-0.65x)
- Large symbols account for only ~3% of main_r0 profile
- Optimizing large path won't help main lane significantly

**Next steps:**
- Freeze hz4 retry line
- Return effort to hz3 aggressive line

---

## 4) Stage B: B83 LargeBurstReuseBox (One Big-Box)

### Goal

- Improve `main_r0` fixed cost via large-path OS-call reduction.
- Keep `small/mid` path untouched.

### Boundary Files

- `hakozuna/hz4/src/hz4_large.c`
- `hakozuna/hz4/src/hz4_large_paths.inc`
- `hakozuna/hz4/src/hz4_large_cache_box_paths.inc`
- `hakozuna/hz4/core/hz4_config_collect.h`

### Box Idea

- Add one TLS-local burst reuse tier for `large` blocks:
  - consume exact-page blocks first from TLS burst list,
  - publish overflow to existing global large cache in batch,
  - no changes in `hz4_tcache_free.inc` dispatch path.

### Required Counters

- `large_b83_tls_burst_hit`
- `large_b83_tls_burst_fill`
- `large_b83_tls_burst_spill`
- `large_b83_os_acquire_avoided`
- `large_b83_os_release_avoided`

---

## 5) A/B Gates (Screen then Replay)

### Screen (RUNS=7)

- `guard_r0 >= -1.0%`
- `main_r0 >= +3.0%`
- `main_r50 >= +1.0%`
- `cross128_r90 >= 0.0%`
- `large_r0 >= +8.0%`

### Replay (RUNS=21, only if screen passes)

- `guard_r0 >= -1.0%`
- `main_r0 >= +6.0%`
- `main_r50 >= +2.0%`
- `cross128_r90 >= 0.0%`
- `large_r0 >= +10.0%`

Fail policy:
- Any gate fail => immediate archive (`NO-GO`) and return to `hz3`.

---

## 6) Safety / Non-Regression Lock

- No new always-on work in `hz4_free()` dispatch boundary.
- No changes in frozen `mid` line.
- No parameter sweep loop after failure (single retry only).

