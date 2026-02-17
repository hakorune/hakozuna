# HZ4 Phase21: B72 MidSpanLeafRemoteQueueBox Work Order (2026-02-16)

## Goal
- Recover remote-lane regression from B71 while keeping B71's `main_r0` gain.
- Remove span-root remote queue concentration by distributing remote push over leaf pages.
- Keep `hz4_free()` dispatch path unchanged (mid-internal changes only).

## Background
- `B71 LargeMidSpan Lite` passed `guard_r0/main_r0` but failed badly on remote lanes:
  - `main_r50 = -51%`, `cross128_r90 = -69%` (`RUNS=7` screen).
- Root cause:
  - all span-member remote frees were normalized to a single root `remote_head/remote_count`,
  - causing heavy CAS contention and cacheline ping-pong.

## Design summary
- Keep span object ownership/freelist accounting at root.
- For remote push:
  - use leaf page queue (`leaf->remote_head`, `leaf->remote_count`) instead of root queue.
- For owner drain:
  - when owner page is a span root, scan all span pages and splice each leaf queue into root freelist.
- Keep existing non-span behavior unchanged.

## Scope boundary
- `hakozuna/hz4/src/hz4_mid_owner_remote.inc` (primary boundary)
  - split remote push target and span drain aggregation.
- `hakozuna/hz4/src/hz4_mid.c`
  - add helper to map root -> leaf page by index (for drain scan only).
- `hakozuna/hz4/src/hz4_mid_free.inc`
  - do not root-normalize before remote push decision.
- `hakozuna/hz4/src/hz4_mid_batch_cache.inc`
  - do not root-normalize before remote push in flush paths.
- `hakozuna/hz4/src/hz4_mid_stats.inc`
  - add B72 counters.

Non-goals:
- No routing change at `hz4_free()`.
- No allocator-wide lock model change.

## Knobs (`hakozuna/hz4/core/hz4_config_collect.h`)
```c
// B72: MidSpanLeafRemoteQueueBox (opt-in, default OFF)
#ifndef HZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX
#define HZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX 0
#endif
#if HZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX && !HZ4_MID_SPAN_LITE_BOX
#error "HZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX requires HZ4_MID_SPAN_LITE_BOX=1"
#endif
```

## Core implementation

### 1) Push side (`hz4_mid_owner_remote.inc`)
- Current B71 behavior normalizes `page = hz4_mid_page_root(page)` in `hz4_mid_owner_remote_push`.
- Change for B72:
  - if `HZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX=1`, do not root-normalize.
  - keep push CAS and `remote_count++` on the passed page (leaf).
  - this distributes contention naturally across span pages.

Pseudo:
```c
static inline void hz4_mid_owner_remote_push(hz4_mid_page_t* page, void* obj) {
#if !(HZ4_MID_SPAN_LITE_BOX && HZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX)
    page = hz4_mid_page_root(page);
#endif
    ... existing CAS push ...
}
```

### 2) Drain side (`hz4_mid_owner_remote.inc`)
- Keep legacy one-page drain path for non-span pages.
- Add span-aware path when `page` is root and `page->span_pages > 1`:
  - for `i=0..span_pages-1`, pick `leaf` (`i==0 ? root : root + i * HZ4_PAGE_SIZE` via helper),
  - `list = atomic_exchange(leaf->remote_head, NULL)`,
  - validate list (`guard_limit`),
  - splice list to root freelist and increment root count.
  - best-effort decrement leaf `remote_count`.

Important:
- Do not call recursive drain on root/leaf pair to avoid duplicate counting.
- Keep fail-fast for cycle/corruption.

### 3) Owner drain trigger (`hz4_mid_owner_should_drain`)
- For span root in B72 mode:
  - check `remote_count` on all leaves; return drain-needed if any leaf exceeds threshold or periodic tick condition.
- For non-span pages keep existing logic.

### 4) Remove eager root normalization in caller sites
- `hakozuna/hz4/src/hz4_mid_free.inc`:
  - keep local freelist path root-based where needed, but remote push must use original leaf page.
- `hakozuna/hz4/src/hz4_mid_batch_cache.inc`:
  - same rule for flush paths; remote push uses leaf page.

## B72 counters (`HZ4_MID_STATS_B1`)
Add:
- `span_leaf_remote_push_calls`
- `span_leaf_remote_push_cas_retry`
- `span_leaf_remote_drain_calls`
- `span_leaf_remote_drain_pages_scanned`
- `span_leaf_remote_drain_objs`

Expected mechanism evidence:
- push and drain counters are non-zero on `main_r50/cross128_r90`.
- average scanned pages near configured span pages.

## Safety / fail-fast
- Abort on:
  - invalid span root/page mapping,
  - `span_index >= span_pages`,
  - remote list cycle/corruption,
  - root/leaf sc mismatch.

## Build matrix
Base (B71 span enabled, no leaf-queue split):
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_SPAN_LITE_BOX=1 -DHZ4_MID_SPAN_LITE_CAP_MAX=4 -DHZ4_MID_SPAN_LITE_MIN_OBJ_SIZE=8192 -DHZ4_MID_SPAN_LITE_PAGES=4'
```

Var (B72 on):
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_SPAN_LITE_BOX=1 -DHZ4_MID_SPAN_LITE_CAP_MAX=4 -DHZ4_MID_SPAN_LITE_MIN_OBJ_SIZE=8192 -DHZ4_MID_SPAN_LITE_PAGES=4 -DHZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX=1'
```

Var+stats:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_MID_OWNER_REMOTE_QUEUE_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_BOX=1 -DHZ4_MID_OWNER_LOCAL_STACK_SLOTS=8 -DHZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE=1 -DHZ4_MID_SPAN_LITE_BOX=1 -DHZ4_MID_SPAN_LITE_CAP_MAX=4 -DHZ4_MID_SPAN_LITE_MIN_OBJ_SIZE=8192 -DHZ4_MID_SPAN_LITE_PAGES=4 -DHZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX=1 -DHZ4_MID_STATS_B1=1 -DHZ4_MID_LOCK_TIME_STATS=1'
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
- `main_r0 >= +3.0%`
- `main_r50 >= +1.0%`
- `cross128_r90 >= 0.0%`
- `crash/assert/alloc-failed = 0`

## Decision rule
- GO:
  - replay gates pass and B72 counters confirm leaf-distributed remote push/drain.
- NO-GO:
  - remote lanes still fail gate, or counters remain cold, or safety regressions appear.
- If NO-GO:
  - archive B72 and keep B70 baseline; stop span-track and switch back to non-span mid lock-path boxes.
