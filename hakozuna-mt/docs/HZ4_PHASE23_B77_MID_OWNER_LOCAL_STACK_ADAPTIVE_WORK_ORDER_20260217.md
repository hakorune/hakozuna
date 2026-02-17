# HZ4 Phase23: B77 MidOwnerLocalStackAdaptive Work Order (2026-02-17)

## Goal
- Keep `B57` direction (`main_r0` uplift) while removing remote-lane regressions seen in fixed `owner_local_stack` configs.
- Do not add any always-on work at `hz4_free()` dispatch boundary.
- Target lanes:
  - `guard_r0` non-regressive
  - `main_r0` positive (`>= +6%` replay gate)
  - `main_r50` / `cross128_r90` non-regressive to positive

## Background
- `B57` showed strong `main_r0` upside, but fixed policy caused lane split.
- `B58` (remote gate on `remote_count`) improved safety, but mixed/remote lanes still miss gate in some settings.
- `B71/B72/B73/B74/B75/B76` are archived NO-GO. Keep scope narrow and adaptive.

## Box
- `B77`: `HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_BOX` (opt-in, default OFF)
- Core idea:
  - Add per-`sc` TLS adaptive score/cooldown for `owner_local_stack`.
  - Local-friendly classes stay enabled.
  - Remote-heavy / stale-prone classes self-throttle quickly.

## Scope Boundary
- `hakozuna/hz4/core/hz4_config_collect.h`
  - B77 knobs + compile-time guards
- `hakozuna/hz4/src/hz4_mid.c`
  - `hz4_mid_owner_local_stack_push()` / `hz4_mid_owner_local_stack_pop()` adaptive gate
  - per-`sc` TLS state arrays
- `hakozuna/hz4/src/hz4_mid_stats.inc`
  - B1 counters for adaptive engagement

Non-goals:
- no change to `hakozuna/hz4/src/hz4_tcache_free.inc`
- no new route/pagetag work
- no mid span/transfer re-introduction

---

## Stage A: Observe First (No Code Changes)

Use existing counters to confirm B77 is worth implementing.

### Build
```sh
BASE_DEFS='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_STATS_B1=1 -DHZ4_MID_LOCK_TIME_STATS=1 -DHZ4_MID_LOCK_TIME_STATS_SAMPLE_SHIFT=7'
make -C hakozuna/hz4 clean all HZ4_DEFS_EXTRA="$BASE_DEFS"
```

### Lanes (RUNS=7)
- `guard_r0`: `16 1200000 400 16 2048 0 65536`
- `main_r0`: `16 1200000 400 16 32768 0 65536`
- `main_r50`: `16 1200000 400 16 32768 50 65536`
- `cross128_r90`: `16 400000 400 16 131072 90 65536`

### Required counters
From `[HZ4_MID_STATS_B1]`:
- `malloc_owner_local_stack_hit`
- `malloc_owner_local_stack_stale_remote`
- `malloc_owner_local_stack_stale_unowned`
- `malloc_owner_local_stack_pop_gated_remote`
- `free_owner_local_stack_hit`
- `free_owner_local_stack_overflow`
- `free_owner_local_stack_push_gated_remote`
- `malloc_lock_path`
- `free_owner_remote`

From `[HZ4_MID_LOCK_STATS]`:
- `lock_enter`
- `inlock_remote_drain_calls`
- `inlock_remote_drain_objs`

### Proceed / stop criteria
Proceed to Stage B only if both hold:
- `main_r50` or `cross128_r90` has notable stack inefficiency:
  - `(malloc_owner_local_stack_stale_remote + malloc_owner_local_stack_pop_gated_remote) / max(1, malloc_owner_local_stack_hit) >= 0.20`
- same ratio in `main_r0` is less than half of `main_r50` (local vs remote separability exists)

If criteria fail: stop B77 and pivot (no implementation).

---

## Stage B: B77 Implementation

### 1) Knobs (`hakozuna/hz4/core/hz4_config_collect.h`)
Add near B57/B58:

```c
// B77: MidOwnerLocalStackAdaptiveBox (opt-in)
#ifndef HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_BOX
#define HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_BOX 0
#endif
#ifndef HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_PROBE_SHIFT
#define HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_PROBE_SHIFT 3  // cold sc: 1/8 push attempts
#endif
#ifndef HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_COOLDOWN
#define HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_COOLDOWN 64    // skip pushes for N events after remote/stale
#endif

#if HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_BOX && !HZ4_MID_OWNER_LOCAL_STACK_BOX
#error "HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_BOX requires HZ4_MID_OWNER_LOCAL_STACK_BOX=1"
#endif
#if HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_BOX && !HZ4_MID_OWNER_REMOTE_QUEUE_BOX
#error "HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_BOX requires HZ4_MID_OWNER_REMOTE_QUEUE_BOX=1"
#endif
#if HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_PROBE_SHIFT > 7
#error "HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_PROBE_SHIFT must be <= 7"
#endif
```

### 2) TLS adaptive state (`hakozuna/hz4/src/hz4_mid.c`)
Under `#if HZ4_MID_OWNER_LOCAL_STACK_BOX && HZ4_MID_OWNER_REMOTE_QUEUE_BOX`:

```c
#if HZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_BOX
static __thread uint8_t g_mid_owner_local_stack_hot[HZ4_MID_SC_COUNT];
static __thread uint8_t g_mid_owner_local_stack_probe_tick[HZ4_MID_SC_COUNT];
static __thread uint8_t g_mid_owner_local_stack_cooldown[HZ4_MID_SC_COUNT];
#endif
```

### 3) Adaptive gate in push (`hz4_mid_owner_local_stack_push`)
Before normal push body:

- If `cooldown[sc] > 0`: decrement and skip push.
- If `hot[sc] == 0`: allow only sampled attempts (`probe_tick` by `PROBE_SHIFT`), otherwise skip.
- Existing B58 `remote_count > 0` gate remains; on this event set `cooldown[sc]=ADAPTIVE_COOLDOWN`.
- On successful push: if `hot[sc] < 255` increment `hot[sc]` by 1.

### 4) Feedback in pop (`hz4_mid_owner_local_stack_pop`)
- On owner-hit return: boost `hot[sc]` (`+2`, saturating).
- On `stale_remote`, `stale_unowned`, or `pop_gated_remote`: set `hot[sc]=0`, set `cooldown[sc]=ADAPTIVE_COOLDOWN`.

This makes remote-heavy classes cool down quickly while preserving local hot classes.

### 5) Stats (`hakozuna/hz4/src/hz4_mid_stats.inc`)
Under `HZ4_MID_STATS_B1` and B77 box:
- `malloc_owner_local_stack_adaptive_push_skip_cold`
- `malloc_owner_local_stack_adaptive_push_skip_cooldown`
- `malloc_owner_local_stack_adaptive_hot_promote`
- `malloc_owner_local_stack_adaptive_hot_reset`
- `free_owner_local_stack_adaptive_push_skip_cold`
- `free_owner_local_stack_adaptive_push_skip_cooldown`

Emit one-line summary in `hz4_mid_stats_dump_atexit()`.

---

## A/B Protocol

### Build
Base (B57+B58 fixed):
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1'
```

Var (B77):
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_PROBE_SHIFT=3 -DHZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_COOLDOWN=64'
```

Var+stats:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_PROBE_SHIFT=3 -DHZ4_MID_OWNER_LOCAL_STACK_ADAPTIVE_COOLDOWN=64 -DHZ4_MID_STATS_B1=1 -DHZ4_MID_LOCK_TIME_STATS=1'
```

### Lanes
- `guard_r0`: `16 1200000 400 16 2048 0 65536`
- `main_r0`: `16 1200000 400 16 32768 0 65536`
- `main_r50`: `16 1200000 400 16 32768 50 65536`
- `cross128_r90`: `16 400000 400 16 131072 90 65536`

### Gates
Screen (`RUNS=7`):
- `guard_r0 >= -1.0%`
- `main_r0 >= +3.0%`
- `main_r50 >= +1.0%`
- `cross128_r90 >= +1.0%`

Replay (`RUNS=21`):
- `guard_r0 >= -1.0%`
- `main_r0 >= +6.0%`
- `main_r50 >= +1.0%`
- `cross128_r90 >= +1.0%`

Stability:
- `crash/assert/alloc-failed = 0`

### Mechanism GO checks
- `adaptive_*_skip_cold` and `adaptive_*_skip_cooldown` must be non-zero in `main_r50/cross128_r90`.
- `adaptive_hot_promote` must stay high in `main_r0`.
- `adaptive_hot_reset` must be materially higher in `main_r50/cross128_r90` than `main_r0`.

---

## NO-GO Conditions
- Any replay gate failure.
- Adaptive counters indicate no lane separation (all lanes show similar skip/reset profile).
- Significant `guard_r0` regression (< -1.0%).

If NO-GO: archive under `hakozuna/archive/research/hz4_mid_owner_local_stack_adaptive_box/README.md` and keep baseline unchanged.
