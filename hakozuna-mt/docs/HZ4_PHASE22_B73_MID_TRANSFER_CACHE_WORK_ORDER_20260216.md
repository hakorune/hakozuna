# HZ4 Phase22: B73 MidTransferCache Work Order (2026-02-16)

## Goal
- Reduce `mid` lock-path entry rate by adding a shared transfer tier between TLS `alloc_run` and locked bin/page-create path.
- Keep `hz4_free()` dispatch unchanged (no new always-on route work).
- Primary targets:
  - `main_r0 >= +6.0%` (replay gate)
  - `main_r50 >= +2.0%` (replay gate)
  - `guard_r0` non-regressive (`>= -1.0%`)

## Background
- `B70` is default ON and gave stable gain (`main_r0/main_r50` positive).
- Span track (`B71/B72`) is archived NO-GO.
- Observation lock:
  - `malloc_lock_path` is still non-trivial in `main_r0/main_r50`.
  - `hz4` context-switch count is still much higher than `tcmalloc` in local/mixed lanes.
- Therefore the next box should cut lock entries directly, not add new per-free routing work.

## Box
- `B73`: `HZ4_MID_TRANSFER_CACHE_BOX` (default OFF)
- Core idea:
  - Introduce per-sizeclass shared transfer cache (`SC -> global object list`).
  - Producer: lock-path refill can publish overflow objects from local `alloc_run` to transfer.
  - Consumer: pre-lock `malloc` miss can pop from transfer and satisfy allocation without taking `sc` lock.

## Scope Boundary
- `hakozuna/hz4/core/hz4_config_collect.h`
  - B73 knobs and compile-time constraints.
- `hakozuna/hz4/src/hz4_mid.c`
  - shared transfer state and helper declarations.
- `hakozuna/hz4/src/hz4_mid_batch_cache.inc`
  - transfer push/pop helpers (single-linked list CAS stack).
- `hakozuna/hz4/src/hz4_mid_malloc.inc`
  - pre-lock consume path and in-lock publish hook.
- `hakozuna/hz4/src/hz4_mid_stats.inc`
  - one-shot counters for mechanism verification.

Non-goals:
- No changes in `hakozuna/hz4/src/hz4_tcache_free.inc`.
- No new route/order logic at `hz4_free()` boundary.
- No span metadata extension.

## Knobs (`hakozuna/hz4/core/hz4_config_collect.h`)
Add near other mid boxes:

```c
// B73: MidTransferCacheBox (opt-in)
#ifndef HZ4_MID_TRANSFER_CACHE_BOX
#define HZ4_MID_TRANSFER_CACHE_BOX 0
#endif
#ifndef HZ4_MID_TRANSFER_CACHE_BATCH_MAX
#define HZ4_MID_TRANSFER_CACHE_BATCH_MAX 16
#endif
#ifndef HZ4_MID_TRANSFER_CACHE_PUBLISH_MIN
#define HZ4_MID_TRANSFER_CACHE_PUBLISH_MIN 12
#endif
#ifndef HZ4_MID_TRANSFER_CACHE_TARGET_LOCAL
#define HZ4_MID_TRANSFER_CACHE_TARGET_LOCAL 8
#endif
#ifndef HZ4_MID_TRANSFER_CACHE_GLOBAL_SOFT_LIMIT
#define HZ4_MID_TRANSFER_CACHE_GLOBAL_SOFT_LIMIT 512
#endif

#if HZ4_MID_TRANSFER_CACHE_BOX && !HZ4_MID_ALLOC_RUN_CACHE_BOX
#error "HZ4_MID_TRANSFER_CACHE_BOX requires HZ4_MID_ALLOC_RUN_CACHE_BOX=1"
#endif
#if HZ4_MID_TRANSFER_CACHE_BATCH_MAX < 4
#error "HZ4_MID_TRANSFER_CACHE_BATCH_MAX must be >= 4"
#endif
#if HZ4_MID_TRANSFER_CACHE_TARGET_LOCAL >= HZ4_MID_ALLOC_RUN_CACHE_LIMIT
#error "HZ4_MID_TRANSFER_CACHE_TARGET_LOCAL must be < HZ4_MID_ALLOC_RUN_CACHE_LIMIT"
#endif
```

## Data model
Add shared transfer state in `hakozuna/hz4/src/hz4_mid.c`:

```c
#if HZ4_MID_TRANSFER_CACHE_BOX
static _Atomic(void*) g_mid_transfer_head[HZ4_MID_SC_COUNT];
static _Atomic(uint32_t) g_mid_transfer_approx_n[HZ4_MID_SC_COUNT];
#endif
```

Notes:
- `head` is a lock-free stack of objects (`hz4_obj_get_next/set_next`).
- `approx_n` is advisory only (soft-limit gate and observability).

## Implementation Plan

### 1) Transfer helpers (`hakozuna/hz4/src/hz4_mid_batch_cache.inc`)
Add:
- `hz4_mid_transfer_push_list(sc, head, tail, n)`
- `hz4_mid_transfer_pop_burst(sc, max_n)`
- `hz4_mid_transfer_refill_alloc_run(sc)` (pop burst then prepend into local alloc_run)
- `hz4_mid_transfer_publish_from_alloc_run(sc)` (publish overflow from local alloc_run)

Push-list pseudocode:

```c
old = atomic_load(&g_mid_transfer_head[sc]);
do {
  hz4_obj_set_next(tail, old);
} while (!atomic_compare_exchange_weak(&g_mid_transfer_head[sc], &old, head));
atomic_fetch_add(&g_mid_transfer_approx_n[sc], n);
```

Pop-burst pseudocode:

```c
old = atomic_load(&g_mid_transfer_head[sc]);
if (!old) return 0;
// detach up to max_n nodes from old list
// CAS head -> rest
atomic_fetch_sub(&g_mid_transfer_approx_n[sc], detached_n_clamped);
```

Safety:
- list cycle guard (`detached_n <= max_n + 8`).
- `page->magic` and `page->sc` validation per popped object.
- on mismatch/corruption: `HZ4_FAIL + abort`.

### 2) Pre-lock consume (`hakozuna/hz4/src/hz4_mid_malloc.inc`)
Insert after local `alloc_run` miss and before lock-path:

```c
#if HZ4_MID_TRANSFER_CACHE_BOX
if (hz4_mid_transfer_refill_alloc_run(sc) != 0) {
    obj = hz4_mid_alloc_run_pop(sc);
    if (obj) return obj;
}
#endif
```

This keeps free-route untouched and targets lock-path miss only.

### 3) In-lock publish (`hakozuna/hz4/src/hz4_mid_malloc.inc`)
After `hz4_mid_alloc_run_refill_from_page_locked()` (and optional burst refill), publish overflow:

```c
#if HZ4_MID_TRANSFER_CACHE_BOX
hz4_mid_transfer_publish_from_alloc_run(sc);
#endif
```

Publish gate:
- local `alloc_run_n[sc] >= HZ4_MID_TRANSFER_CACHE_PUBLISH_MIN`
- global soft-limit not exceeded (`approx_n < GLOBAL_SOFT_LIMIT`)
- publish only until local cache reaches `TARGET_LOCAL`.

This converts one lock-path refill into multi-thread reusable supply.

### 4) Optional cleanup hook
In alloc-run TLS destructor path (`hz4_mid_alloc_run_flush_all()`), do not force transfer flush.
Reason:
- transfer is shared process lifetime cache.
- keep behavior simple for first box; cleanup remains via normal consume path.

## Stats (`hakozuna/hz4/src/hz4_mid_stats.inc`)
Under `HZ4_MID_STATS_B1` add:
- `malloc_transfer_hit`
- `malloc_transfer_refill_calls`
- `malloc_transfer_refill_objs`
- `malloc_transfer_refill_fail_sc_mismatch`
- `malloc_transfer_refill_fail_invalid_page`
- `malloc_transfer_publish_calls`
- `malloc_transfer_publish_objs`
- `malloc_transfer_publish_skip_softlimit`

Mechanism GO checks require non-zero transfer push/pop on `main_r0/main_r50`.

## Fail-fast / Safety Rules
- Abort on:
  - transfer popped object with `page->magic != HZ4_MID_MAGIC`
  - `page->sc != sc`
  - cycle/corruption in transfer list
- Keep existing `alloc_run` and page freelist guards unchanged.

## A/B Protocol

### Build
Base (B70 default baseline):
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1'
```

Var (B73 ON):
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_TRANSFER_CACHE_BOX=1 -DHZ4_MID_TRANSFER_CACHE_BATCH_MAX=16 -DHZ4_MID_TRANSFER_CACHE_PUBLISH_MIN=12 -DHZ4_MID_TRANSFER_CACHE_TARGET_LOCAL=8'
```

Var+stats:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_TRANSFER_CACHE_BOX=1 -DHZ4_MID_TRANSFER_CACHE_BATCH_MAX=16 -DHZ4_MID_TRANSFER_CACHE_PUBLISH_MIN=12 -DHZ4_MID_TRANSFER_CACHE_TARGET_LOCAL=8 -DHZ4_MID_STATS_B1=1 -DHZ4_MID_LOCK_TIME_STATS=1'
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
- `cross128_r90 >= 0.0%`

Replay (`RUNS=21`):
- `guard_r0 >= -1.0%`
- `main_r0 >= +6.0%`
- `main_r50 >= +2.0%`
- `cross128_r90 >= 0.0%`

Stability:
- `crash/assert/alloc-failed = 0`

Mechanism:
- `malloc_transfer_publish_objs > 0`
- `malloc_transfer_refill_objs > 0`
- direction check under `HZ4_MID_LOCK_TIME_STATS`:
  - `malloc_lock_path` down
  - `lock_wait_ns_sum` down or flat

## NO-GO conditions
- mechanism counters are near zero in target lanes (cold box).
- `main_r0` or `main_r50` regresses after replay.
- lock-path ratio does not decrease despite non-zero transfer traffic.

## Next fallback (only if B73 NO-GO)
- `B74 MidLockPathOutlockRefill`:
  - move selected refill work outside `sc` lock,
  - keep same free-route rule (no `hz4_free` boundary additions).
