# HZ4 Phase23: B82 Final One-Shot Work Order (2026-02-17)

## Goal
- Run exactly one final `hz4 mid` attempt focused on fixed-cost root causes.
- Do **not** add always-on work to `hz4_free()` route boundary.
- If B82 fails, freeze `hz4 mid` optimization line.

## Why this box
- `hz4` vs `tcmalloc` (`main_r0`) still shows large fixed-cost gap in:
  - `context-switches`
  - `page-faults`
- B80/B81 knob sweeps did not produce lane-wide pass.

---

## B82 Scope (Observation-first, then minimal change)

### Stage A: Observation lock (no code change)

Build:
```sh
BASE_DEFS='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_STATS_B1=1 -DHZ4_MID_LOCK_TIME_STATS=1 -DHZ4_MID_LOCK_TIME_STATS_SAMPLE_SHIFT=7'
make -C hakozuna/hz4 clean all HZ4_DEFS_EXTRA="$BASE_DEFS"
```

Perf:
```sh
BENCH=./hakozuna/out/bench_random_mixed_mt_remote_malloc
ARGS_MAIN='16 1200000 400 16 32768 0 65536'
perf stat -r 7 -e cycles,instructions,branch-misses,cache-misses,context-switches,page-faults -- \
  taskset -c 0-15 env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so $BENCH $ARGS_MAIN
perf record -F 999 -g -- \
  taskset -c 0-15 env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so $BENCH $ARGS_MAIN
```

Proceed to Stage B only if both hold:
1. `malloc_lock_path / malloc_calls >= 0.10`
2. lock-time sample indicates lock-path subwork concentration:
   - `hold_page_create_ns_sum` or `inlock_remote_drain_calls` remains non-trivial.

If not, stop and freeze.

---

### Stage B: Minimal one-box candidate

Box:
- `HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX=1` plus **single guard fixup only**:
  - no extra list walks
  - no new TLS arrays
  - no adaptive state

Rationale:
- Target lock wait/hold without introducing hot-path instruction growth.
- Keep change radius limited to `hz4_mid_malloc.inc` lock boundary.

Files (if code edit is needed):
- `hakozuna/hz4/src/hz4_mid_malloc.inc`
- `hakozuna/hz4/src/hz4_mid_stats.inc` (counter only, optional)

Non-goals:
- no new fast-path conditionals in `hz4_mid_free.inc`
- no changes in `hz4_tcache_free.inc`

---

## A/B Protocol

Baseline:
```sh
BASE='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1'
```

Variant:
```sh
VAR="$BASE -DHZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX=1"
```

Lanes:
- `guard_r0`: `16 1200000 400 16 2048 0 65536`
- `main_r0`: `16 1200000 400 16 32768 0 65536`
- `main_r50`: `16 1200000 400 16 32768 50 65536`
- `cross128_r90`: `16 400000 400 16 131072 90 65536`

Screen (`RUNS=7`) gate:
- `guard_r0 >= -1.0%`
- `main_r0 >= +3.0%`
- `main_r50 >= +1.0%`
- `cross128_r90 >= 0.0%`

Replay (`RUNS=21`) gate:
- `guard_r0 >= -1.0%`
- `main_r0 >= +6.0%`
- `main_r50 >= +2.0%`
- `cross128_r90 >= 0.0%`

Additional perf gate (required):
- `context-switches/op` improves vs baseline
  or
- `page-faults/op` improves vs baseline

---

## One-shot rule (hard)
- B82 gets **one implementation cycle only**.
- If screen or replay fails, archive immediately and freeze `hz4 mid` line.
- After freeze, prioritize non-mid tracks (`hz4 large` or `hz3 aggressive lane`).
