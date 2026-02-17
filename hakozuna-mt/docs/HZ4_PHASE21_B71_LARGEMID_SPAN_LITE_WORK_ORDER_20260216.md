# HZ4 Phase21: B71 LargeMidSpan Lite Work Order (2026-02-16)

## Goal
- Improve local/mixed lanes by reducing repeated `mid` page-create pressure on low-capacity upper-mid classes.
- Keep `hz4_free()` dispatch unchanged (no new always-on route work).
- Build on `B70` default baseline (`HZ4_MID_PAGE_SUPPLY_RESV_BOX=1`).

## Why this box
- `B70` reduced seg-lock traffic and improved `main_r0/main_r50`, but `main_r0` gap vs `tcmalloc` remains large.
- Upper-mid classes with low per-page capacity still force frequent lock-path `page_create`.
- Spanizing only low-capacity upper-mid classes should reduce create frequency without touching guard/small fast path.

## Scope boundary
- `hakozuna/hz4/include/hz4_mid.h`
  - `hz4_mid_page_t` span metadata fields (opt-in).
- `hakozuna/hz4/core/hz4_config_collect.h`
  - `B71` knobs.
- `hakozuna/hz4/src/hz4_mid_page_supply.inc`
  - span create and page supply integration.
- `hakozuna/hz4/src/hz4_mid_malloc.inc`
  - bin-miss branch: page-create vs span-create.
- `hakozuna/hz4/src/hz4_mid_free.inc`
  - ptr -> root-page normalization in mid path only.
- `hakozuna/hz4/src/hz4_mid_owner_remote.inc`
  - ensure remote push/drain operates on root page.
- `hakozuna/hz4/src/hz4_mid_batch_cache.inc`
  - root normalization in free-batch/alloc-run flush paths.
- `hakozuna/hz4/src/hz4_mid_stats.inc`
  - B71 counters (debug/one-shot).

Non-goals:
- No changes in `hakozuna/hz4/src/hz4_tcache_free.inc`.
- No change to `small` allocator routing.

## Knobs (add to `hakozuna/hz4/core/hz4_config_collect.h`)
```c
// B71: LargeMidSpanLiteBox (opt-in, default OFF)
#ifndef HZ4_MID_SPAN_LITE_BOX
#define HZ4_MID_SPAN_LITE_BOX 0
#endif
// Enable span mode only when 1-page capacity <= this threshold.
#ifndef HZ4_MID_SPAN_LITE_CAP_MAX
#define HZ4_MID_SPAN_LITE_CAP_MAX 4
#endif
// Apply only from this minimum object size (upper-mid only).
#ifndef HZ4_MID_SPAN_LITE_MIN_OBJ_SIZE
#define HZ4_MID_SPAN_LITE_MIN_OBJ_SIZE (8u * 1024u)
#endif
// Fixed span pages in lite mode.
#ifndef HZ4_MID_SPAN_LITE_PAGES
#define HZ4_MID_SPAN_LITE_PAGES 4
#endif
#if HZ4_MID_SPAN_LITE_PAGES < 2
#error "HZ4_MID_SPAN_LITE_PAGES must be >= 2"
#endif
#if HZ4_MID_SPAN_LITE_PAGES > HZ4_PAGES_PER_SEG
#error "HZ4_MID_SPAN_LITE_PAGES must be <= HZ4_PAGES_PER_SEG"
#endif
```

## Data model
Add span metadata to `hz4_mid_page_t` (`hakozuna/hz4/include/hz4_mid.h`) under `#if HZ4_MID_SPAN_LITE_BOX`:
```c
    struct hz4_mid_page* span_root;  // NULL for normal page, root ptr for span members
    uint16_t span_pages;             // valid on root
    uint16_t span_index;             // 0..span_pages-1
```

Rules:
- Normal page: `span_root=NULL`, `span_pages=1`, `span_index=0`.
- Span root: `span_root=NULL`, `span_pages=N`, `span_index=0`.
- Span member page: `span_root=root`, `span_pages=N`, `span_index=i`.

## Core algorithm
### A) root normalization helper
Add helper in `hz4_mid.c` (before `*.inc` includes):
```c
static inline hz4_mid_page_t* hz4_mid_page_root(hz4_mid_page_t* page) {
#if HZ4_MID_SPAN_LITE_BOX
    return page->span_root ? page->span_root : page;
#else
    return page;
#endif
}
```

### B) span create in `hz4_mid_page_supply.inc`
1. Refactor page supply into a raw one-page supplier (reuse B70 reservation path).
2. Keep existing `hz4_mid_page_create()` for normal one-page creation.
3. Add `hz4_mid_span_create_lite(sc, obj_size)`:
   - reserve `HZ4_MID_SPAN_LITE_PAGES` pages from the same segment path.
   - initialize metadata for all pages.
   - build one freelist across all pages, but skip each page header region:
     - per page payload range: `page + align_up(sizeof(hz4_mid_page_t), HZ4_SIZE_ALIGN)` .. page end.
   - root holds combined `capacity/free_count/free`.
   - all pages use `magic=HZ4_MID_MAGIC`, `sc` identical.
   - register pagetag for every page as mid.

### C) create policy in `hz4_mid_malloc.inc` (bin miss branch)
On `!page`:
- compute one-page capacity estimate:
  - `payload = HZ4_PAGE_SIZE - align_up(sizeof(hz4_mid_page_t), HZ4_SIZE_ALIGN)`
  - `cap1 = payload / obj_size`
- choose span create only if:
  - `obj_size >= HZ4_MID_SPAN_LITE_MIN_OBJ_SIZE`
  - `cap1 <= HZ4_MID_SPAN_LITE_CAP_MAX`
- otherwise keep existing one-page create.

### D) root normalization call sites
Normalize to root before sc/owner/freelist operations in:
- `hz4_mid_free.inc`: right after page-base extraction and magic check.
- `hz4_mid_owner_remote.inc`: at start of push/drain entry points.
- `hz4_mid_batch_cache.inc`: when deriving `page` from object in flush/refill paths.

Do not normalize in `hz4_free()` dispatch path; only inside mid box.

## B71 counters (`HZ4_MID_STATS_B1`)
Add:
- `mid_span_create_calls`
- `mid_span_pages_total`
- `mid_span_capacity_total`
- `mid_span_root_fixups` (leaf ptr normalized to root)
- `mid_span_create_fallbacks` (span path requested but not possible)

Expected evidence:
- non-zero `mid_span_create_calls/pages_total/root_fixups`
- `malloc_page_create` decreases in upper-mid workloads

## Safety / fail-fast
- Abort on:
  - invalid root pointer or root/page sc mismatch
  - `span_index >= span_pages`
  - freelist corruption/cycle
  - capacity/free_count overflow/underflow
- Keep existing remote drain guard-limit checks.

## Build matrix
Base (current promoted baseline):
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1'
```

Var (B71 lite):
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_SPAN_LITE_BOX=1 -DHZ4_MID_SPAN_LITE_CAP_MAX=4 -DHZ4_MID_SPAN_LITE_MIN_OBJ_SIZE=8192 -DHZ4_MID_SPAN_LITE_PAGES=4'
```

Var+stats:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_SPAN_LITE_BOX=1 -DHZ4_MID_SPAN_LITE_CAP_MAX=4 -DHZ4_MID_SPAN_LITE_MIN_OBJ_SIZE=8192 -DHZ4_MID_SPAN_LITE_PAGES=4 -DHZ4_MID_STATS_B1=1 -DHZ4_MID_LOCK_TIME_STATS=1'
```

## A/B protocol
Lanes:
- `guard_r0`: `16 1200000 400 16 2048 0 65536`
- `main_r0`: `16 1200000 400 16 32768 0 65536`
- `main_r50`: `16 1200000 400 16 32768 50 65536`
- `cross128_r90`: `16 400000 400 16 131072 90 65536`

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
- `crash/assert/alloc-failed = 0`

## Decision rule
- GO:
  - replay gates pass and B71 counters show mechanism engagement.
- NO-GO:
  - replay gate miss, or mechanism remains cold, or fail-fast/safety regression.
- If NO-GO:
  - archive as research box and keep B70 default as stable baseline.
