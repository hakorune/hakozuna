# HZ4 Phase21: B70 MidPageSupplyReservation Work Order (2026-02-16)

## Goal
- Reduce `mid` lock-path cost without adding any always-on work in `hz4_free()` dispatch.
- Target lanes:
  - improve `main_r0` and `main_r50`
  - keep `guard_r0` non-regressive
  - keep `cross128_r90` non-regressive

## Background
- `B60/B61/B62/B63` are archived NO-GO.
- Current snapshot:
  - `main_r0` and `main_r50` still have non-trivial `malloc_lock_path`.
  - `HZ4_MID_LOCK_TIME_STATS` shows non-negligible page-create/lock waiting cost.
  - `cross128_r90` already has large headroom, so local/mixed fixed-cost reduction is priority.

## Box
- `B70`: `HZ4_MID_PAGE_SUPPLY_RESV_BOX` (promoted default ON)
- Core idea:
  - replace "1-page per seg-lock" supply with "N-page reservation per seg-lock".
  - amortize segment lock traffic during `hz4_mid_page_create()`.

## Scope Boundary
- Primary boundary:
  - `hakozuna/hz4/src/hz4_mid_page_supply.inc`
    - page supply inside `hz4_mid_page_create()`.
- Optional observability boundary:
  - `hakozuna/hz4/src/hz4_mid_stats.inc` (B1 one-shot counters).
- No changes to:
  - `hakozuna/hz4/src/hz4_tcache_free.inc`
  - `hz4_free()` route/order logic.

## Implementation Plan

### 1) Config knobs (`hakozuna/hz4/core/hz4_config_collect.h`)
Add near existing mid box knobs:

```c
// B70: MidPageSupplyReservationBox (default ON)
#ifndef HZ4_MID_PAGE_SUPPLY_RESV_BOX
#define HZ4_MID_PAGE_SUPPLY_RESV_BOX 1
#endif
#ifndef HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES
#define HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES 8
#endif
#if HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES < 2
#error "HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES must be >= 2"
#endif
#if HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES > HZ4_PAGES_PER_SEG
#error "HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES must be <= HZ4_PAGES_PER_SEG"
#endif
```

### 2) TLS reservation state (`hakozuna/hz4/src/hz4_mid_page_supply.inc`)
Add lightweight TLS state:

```c
#if HZ4_MID_PAGE_SUPPLY_RESV_BOX
typedef struct hz4_mid_page_resv {
    hz4_mid_seg_t* seg;
    uint32_t next_page;
    uint32_t end_page;
} hz4_mid_page_resv_t;
static __thread hz4_mid_page_resv_t g_mid_page_resv;
#endif
```

### 3) Supply helper (`hakozuna/hz4/src/hz4_mid_page_supply.inc`)
Introduce a helper used by `hz4_mid_page_create()`:

- fast path:
  - if `g_mid_page_resv.seg` valid and `next_page < end_page`, return next page without seg lock.
- refill path:
  - acquire seg lock
  - allocate a reservation range `[begin, end)` of `chunk_pages`
  - update global `seg->next_page = end`
  - release seg lock
  - return first page; keep remaining range in TLS reservation.

Important:
- clamp `end` to `HZ4_PAGES_PER_SEG`
- if current seg has no room, create a new seg exactly as current code does.
- preserve existing page init/tag registration logic after page address is chosen.

### 4) Keep semantics unchanged
- `page->magic/sc/obj_size/capacity/free/free_count` init stays exactly same.
- `HZ4_PAGE_TAG_TABLE` registration behavior stays same.
- no changes to owner/remote logic.

### 5) Optional counters (`hakozuna/hz4/src/hz4_mid_stats.inc`)
Under `HZ4_MID_STATS_B1`, add:
- `mid_supply_resv_hit`
- `mid_supply_resv_refill`
- `mid_supply_resv_pages`
- `mid_supply_resv_new_seg`

Emit them in `hz4_mid_stats_dump_atexit()`.

## Fail-fast / Safety Rules
- If reservation metadata is invalid (`next_page > end_page`, `end_page > HZ4_PAGES_PER_SEG`), abort.
- If computed page address crosses segment bounds, abort.
- Keep all existing `HZ4_FAIL` checks for page init and freelist integrity.

## A/B Protocol

### Build
Base:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1'
```

Var:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_PAGE_SUPPLY_RESV_BOX=1 -DHZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES=8'
```

Var+stats:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_PAGE_SUPPLY_RESV_BOX=1 -DHZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES=8 -DHZ4_MID_STATS_B1=1 -DHZ4_MID_LOCK_TIME_STATS=1'
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
- `cross128_r90 >= -1.0%`

Replay (`RUNS=21`):
- `guard_r0 >= -1.0%`
- `main_r0 >= +6.0%`
- `main_r50 >= +2.0%`
- `cross128_r90 >= 0.0%`

Stability:
- `crash/assert/alloc-failed = 0`

### Mechanism checks (required for GO)
- `mid_supply_resv_hit > 0`
- `mid_supply_resv_refill > 0`
- `mid_supply_resv_pages / mid_supply_resv_refill` close to chunk size
- `HZ4_MID_LOCK_TIME_STATS` direction (var vs base):
  - `hold_page_create_ns_sum` down
  - `lock_wait_ns_sum` down or flat

## Execution Result (2026-02-16)
- Mechanism:
  - `supply_resv_hit=4538`
  - `supply_resv_refill=709`
  - `supply_resv_pages/supply_resv_refill ~= 7.5` (chunk `8`)
  - lock traffic reduction: about `7.4x` (`5247` page creates / `709` refills)
- Replay (`RUNS=21`, interleaved base/var):
  - `guard_r0`: `-0.35%`
  - `main_r0`: `+4.20%`
  - `main_r50`: `+3.73%`
  - `cross128_r90`: `+3.59%`

## Decision
- **GO / promoted to default**
  - keep `HZ4_MID_PAGE_SUPPLY_RESV_BOX=1`
  - keep `HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES=8`
- Next:
  - proceed to `B71 lite` work order (upper-mid only spanization).
