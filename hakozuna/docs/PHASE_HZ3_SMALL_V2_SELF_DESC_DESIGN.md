# hz3 Small v2 (self-describing) design

## Goal
Build a native hz3 small allocator (16B-2048B) that is self-describing
and does not rely on global segmap for free/ptr classification.
Keep hot path minimal and isolate learning/epoch to event-only boxes.

## Non-goals (v2)
- No perf tuning yet (correctness first).
- No env knobs in hot path (compile-time only).
- No hybrid routing (hz3 standalone only).

## Assumptions
- Segments are 2MB aligned (HZ3_SEG_SIZE).
- We can reserve a dedicated VA arena for hz3 segments (recommended).
- Pointer classification is via self-describing SegHdr (no segmap).

## Box decomposition

| Box | Role | Boundary API (single entry) |
|-----|------|-----------------------------|
| SegProviderBox | Reserve VA arena + mmap 2MB segments | hz3_seg_alloc_selfdesc() |
| SegHdrBox | ptr -> SegHdr validation | hz3_seg_from_ptr(ptr) |
| SmallPageBox | page meta + free list | hz3_small_page_alloc(), hz3_small_page_free() |
| TCacheBox | TLS bins (push/pop only) | hz3_small_alloc(), hz3_small_free() |
| CentralBox | per-shard batch transfer (objects) | hz3_small_central_pop_batch(), hz3_small_central_push_list() |
| RemoteBox | cross-thread free batching | hz3_small_outbox_push(), hz3_small_inbox_drain() |

## Data layout

### Reserved arena
- A single reserved VA range dedicated to hz3 segments.
- All hz3 segments are mapped inside this range.
- ptr outside range is never hz3 (fast reject).

Optional upgrade (later):
- Split arena into `SmallArena` / `LargeArena` to reduce classification work:
  - range check alone determines kind (no SegHdr load required for kind).

### Segment header (self-describing)
At segment base (2MB aligned):
- magic
- kind (small/large)
- owner shard
- (optional) pointer to page_meta area
- (optional) segment base pointer (redundancy)

### Page metadata
Per page:
- size class (sc)
- (optional) owner override (default uses SegHdr.owner)
- (optional) free_head pointer (avoid if TLS bins own free list)

Page meta location:
- Stored in a fixed area in the segment (e.g., first pages),
  or in a separate mmap region; SegHdr always points to it.

Recommendation for “hot stays hot”:
- Page meta is stored at a fixed offset inside the segment (no extra pointer chase).
- The free hot path needs only `sc` (and owner via SegHdr.owner).

## Pointer classification (SegHdrBox)

```
hz3_seg_from_ptr(ptr):
  if ptr outside reserved arena -> NULL
  base = ptr & ~(2MB-1)
  hdr = (SegHdr*)base
  if hdr.magic mismatch -> NULL
  return hdr
```

Fail-fast: in debug, abort on magic mismatch when ptr is inside arena.

## Small alloc flow (TCacheBox)

Fast path:
1. size -> small sc
2. pop from TLS bin

Slow path:
1. refill from TLS current page
2. if empty, pop a page from CentralBox
3. if Central empty, allocate a new page from segment
4. split page into objects, push to bin, return one

## Small free flow

1. SegHdr = hz3_seg_from_ptr(ptr)
2. if NULL -> fallback (non-hz3)
3. page meta lookup -> sc
4. if owner == local: push to TLS bin
5. else: RemoteBox outbox push

## Central + Remote (event-only)

- CentralBox stores object batches (chains), not whole pages.
- RemoteBox batches frees by owner×sc (outbox/inbox list transfer).
- Inbox drain occurs in slow path and epoch only.

Rationale:
- Hot free should avoid page-meta writes when possible.
- Shared structures should be touched in batch only.

## A/B flags (compile-time)

- HZ3_SMALL_V2_ENABLE (default 0)
- HZ3_SEG_SELF_DESC_ENABLE (default 0)
- HZ3_SMALL_V2_FAILFAST (debug only)

## Observability (minimal)

- Counters (event-only):
  - small_refill_pages
  - small_central_pop_hit/miss
  - small_remote_outbox_push
  - small_inbox_drain_objs

Logs only at boundary APIs (one-shot).

## Fail-fast rules

- ptr in arena but magic mismatch -> abort (debug).
- page meta out of range -> abort (debug).
- sc mismatch on free -> abort (debug).

## Phased implementation

S12-2A: SegHdr + reserved arena + hz3_seg_from_ptr
S12-2B: SmallPageBox (page meta + per-page freelist)
S12-2C: TCacheBox + CentralBox (object batch transfer)
S12-2D: RemoteBox (outbox/inbox)
S12-2E: SSOT run (small/medium/mixed)
S12-2F: Unified TCache (small+mid bins in one TLS struct, no extra TLS lookup)
S12-2G: Learning v0 knobs (event-only, freeze-by-default)

## GO criteria

- LD_PRELOAD hz3 standalone completes smoke tests.
- small/medium/mixed benches complete (RUNS=10).
- No segmap dependency in small free path.
