# HZ10 Front Cache Design L0

Design memo for the structural page-selection/refill redesign that
HZ10LocalPathTrim-L0's measurement pass left "real, understood, deliberately
not attempted without a dedicated design pass" (current_task.md, 20260705).
This is that design pass. Review document, not an implementation patch.

## Problem Statement (measured, not hypothesized)

HZ10LocalPathTrim-L0, fixed-size local0 alloc_free vs real tcmalloc
(bench_results/20260705T013024Z_hz10_local_path_trim_l0/combined.log plus the
LD_PRELOAD tcmalloc rerun recorded in current_task.md):

```text
class_size=16    hz10=13.97ns  tcmalloc~9.2-9.5ns    ~1.4-1.5x slower
class_size=64    hz10=16.90ns  tcmalloc~11.4-11.6ns  ~1.4-1.5x slower
class_size=24576 hz10=58.41ns  tcmalloc~11.0-12.0ns  ~5x slower
class_size=32768 hz10=60.88ns  tcmalloc~10.1-11.0ns  ~5-6x slower
class_size=65536 hz10=99.93ns  tcmalloc~9.2-9.7ns    ~10x slower
```

Two load-bearing facts from the same investigation thread:

1. The gap is NOT uniform: it tracks slot_count. Classes 20/21 (2 slots) and
   22/23 (1 slot) carry ~93% of main-row scan work
   (HZ10ActiveHitDepthByClass-L0) and the 5-10x local0 loss.
2. The cost is NOT primarily list scanning. The working_set=2 sweep (one
   page, zero possible scan) still measured ~24-33ns for class 24576 --
   2-3x tcmalloc before any scan cost exists. The per-op page-SWITCH
   machinery itself (active-cache miss, drain attempt, find/harvest entry,
   active reassignment) is the floor, and scanning only widens it.

Why tcmalloc does not pay this: its per-thread cache is an OBJECT freelist
per size class. malloc = TLS pop, free = TLS push, identical cost whether the
objects behind it span one span or fifty. Span/page bookkeeping is only
touched on batched refill/release against the central freelist. HZ10's local
hot path is page-centric: every alloc must run against the freelist of one
specific page, so a class whose page holds 1-2 slots re-enters page-selection
machinery every 1-2 operations. No cache policy over pages can fix that ratio
-- MTF and second-active were both measured NO-GO for exactly this workload
shape (docs/HZ10_NO_GO_LEDGER.md). The fix has to decouple the hot path from
the page.

## Design: HZ10FrontCache-L1

Add a per-thread, per-class OBJECT freelist (the "front cache") in front of
the existing page substrate. The page layer (Box 2/3/6, retired/ready,
lifecycle flush) stays intact underneath and keeps every current contract;
the front cache is a purely thread-local layer, no atomics, no new
cross-thread state.

```text
Hz10FrontCache (TLS, one per class):
  void*    head        intrusive singly-linked object freelist
  uint32_t count
  uint32_t cap         overflow threshold (policy below)
  uint32_t refill_batch
```

### Hot paths

```text
hz10_malloc(size):
  class_id = hz10_size_class_for(size)
  fc = &front[class_id]
  p = fc->head
  if (p):                      # fast path: TLS pop, plain loads/stores
    fc->head = *(void**)p
    fc->count -= 1
    clear in-slot local-free marker
    return p
  return hz10_front_refill_and_alloc(class_id)   # cold, batched

hz10_free(ptr):
  route = hz10_pagemap_route(ptr)                # UNCHANGED, fail-closed
  ... large / invalid handling unchanged ...
  page = route.owner
  if (page->owner_thread_token == HZ10_THREAD_TOKEN):
    fc = &front[page->class_id]                  # fast path: TLS push
    set in-slot local-free marker
    *(void**)ptr = fc->head
    fc->head = ptr
    fc->count += 1
    if (fc->count > fc->cap):
      hz10_front_flush_overflow(fc, class_id)    # cold, batched
    return 1
  ... remote claim/note/publish path BYTE-FOR-BYTE unchanged ...
```

Implementation boundary: the front-cache push is NOT
`hz10_freelist_page_free()`. That existing helper is the page-layer boundary:
it sets the local-free marker, links the slot into `page->local_free_head`,
and increments `page->free_count`. A front-cached slot must remain allocated
from the page's point of view, so the fast path needs a small front-cache-only
helper that sets the marker and links the slot into `fc->head`, but does not
touch `page->local_free_head` or `page->free_count`.

In steady state (any working set that fits under `cap`), alloc and free never
touch a page struct, never attempt a drain, never enter find/harvest. That is
the mechanism by which the 5-10x collapses: the bench's alloc_free cycle
becomes route + TLS push/pop for every class, large or small.

Expected fast-path cost math (from HZ10PublicFreeStageCost-L0's measured
route=2.37ns and local_free=3.07ns): free ~= route + owner check + push
~= 6-8ns; alloc ~= size_class + pop ~= 4-5ns; pair ~= 10-13ns against
tcmalloc's ~9-12ns. Target: every class lands in the ~1.0-1.3x band the
16/64 classes already occupy, instead of 5-10x.

### Cold path: refill

When the front cache is empty, acquire `refill_batch` objects by running
today's EXACT alloc slow path in a loop (active page pop -> active drain ->
find_with_capacity -> harvest_retired -> fresh pooled page), pushing each
object into the front cache, then pop one for the caller. Nothing about page
selection changes; its cost is simply amortized over the batch instead of
paid per op. For the 2-slot classes this alone cuts find/drain entry
frequency by ~refill_batch/2.

The refill helper must bypass the public front-cache entry point; otherwise
an empty front cache would recurse into itself. Shape: split today's
`hz10_malloc()` page-layer body into an internal
`hz10_public_entry_alloc_from_page_layer(class_id)` helper, call it
`refill_batch` times, then front-cache-push those returned slots.

v1 keeps the per-object loop (hz10_freelist_page_alloc per slot). An O(1)
whole-page freelist splice is a possible later refinement, not v1 -- slot
counts for the classes that matter are 1-2, so the loop is already cheap.

### Cold path: overflow flush

When a free pushes `count` past `cap`, pop `cap/2` objects and return each to
its owning page: route the pointer again (2.37ns, boundary-only cost) and run
today's hz10_freelist_page_free(). Pages that become idle or gain capacity
are then discovered by the EXISTING eviction/harvest/ready machinery --
this design adds no new reclaim path.

This is exactly where `hz10_freelist_page_free()` becomes legal again: while a
slot sits in `fc->head`, page accounting still treats it as allocated. Flushing
the slot back to the page is the single point that increments `free_count` and
links the slot into `page->local_free_head`.

### Accounting rule (the one invariant this design adds)

A slot sitting in the front cache counts as ALLOCATED from its page's point
of view: refill decrements free_count exactly like a user alloc, flush
increments it exactly like a user free. Consequences, checked against the
load-bearing constraints in HZ10_SPEED_ATTACK_PLAN_L0.md:

```text
page destruction requires idle verification:
  PRESERVED conservatively -- a page with front-cached slots can never
  read free_count == slot_count, so it can never be destroyed while the
  front cache holds its slots. No ABA, no new generation concern.

owner local_free_head stays owner-thread-only plain load/store:
  PRESERVED -- refill and flush are owner-thread operations by
  construction (front cache is only fed by the owner-token fast path).

accepted remote free == drainable before hz10_free() returns:
  UNTOUCHED -- remote path unchanged.

pending bit remains the duplicate-free authority; claim/note/publish order:
  UNTOUCHED -- remote path unchanged.

in-slot local-free marker semantics:
  PRESERVED exactly -- marker set on push into the front cache, cleared
  on pop, so a foreign thread's duplicate remote free of a front-cached
  (i.e. genuinely freed) slot is classified and rejected the same way a
  page-freelist slot is today.
```

Marker transitions are intentionally redundant at the boundary:

```text
page alloc during refill:      clears marker, decrements free_count
front-cache push after refill: sets marker, leaves free_count unchanged
front-cache pop for malloc:    clears marker, leaves free_count unchanged
front-cache overflow/flush:    calls page free; marker is set again and
                               free_count increments exactly once
```

### Lifecycle flush integration

hz10_public_entry_flush_thread_cache_quiescent() gains a phase 0: fully
flush every class's front cache back to its pages (route each pointer,
plain local free), BEFORE the existing ready-drain / sublist walk phases.
With phase 0 in place the existing postcondition ("every idle page this
thread owns is reclaimed") is preserved unchanged; without it, front-cached
slots would pin their pages as busy across the boundary. The existing
lifecycle smoke gets one new case: populate a front cache, call quiescent
flush, assert pages_busy == 0 and the cached slots' pages were reclaimed.

Thread-exit abandonment story is unchanged in kind: an unflushed exiting
thread today abandons its page lists; with this design it abandons front
caches pinning some of those pages. Same diagnosis, same fix (the quiescent
flush contract), no new mechanism.

### Capacity policy (RSS discipline)

HZ10's positioning is "HZ8-class RSS discipline with much better speed," so
the front cache must be visibly bounded:

```text
cap_objects(class) = clamp(HZ10_FRONT_CACHE_CLASS_BYTES / slot_size,
                           HZ10_FRONT_CACHE_MIN_OBJECTS,
                           HZ10_FRONT_CACHE_MAX_OBJECTS)

proposed defaults:
  HZ10_FRONT_CACHE_CLASS_BYTES = 128 KiB
  HZ10_FRONT_CACHE_MIN_OBJECTS = 4      (a 1-slot class needs >=4 cached
                                         objects to escape per-op page churn)
  HZ10_FRONT_CACHE_MAX_OBJECTS = 128
  refill_batch = cap / 2
```

Worst-case per-thread pin is sum over 24 classes of cap*slot_size, ~3 MiB if
every class is simultaneously hot -- comparable to a tcmalloc thread cache
and bounded, but it must be measured, not asserted: hz10-rss-guard and the
steady-state RSS bench are gate rows below. A tcmalloc-style total-budget
slow-start (grow cap on refill pressure, decay when cold) is explicitly a v2
refinement; v1 ships fixed caps to keep the A/B interpretable.

### What this deliberately does not touch

```text
remote claim/note/publish        unchanged (NO-GO ledger respected)
pending bits                     unchanged
retired/ready queue              unchanged
eviction / harvest policy        unchanged (entered less often)
pagemap route validation         unchanged, still on every public free
```

One small page-struct addition: a `class_id` field (fits in existing padding
near slot_size/slot_count in the first cache line) so the free fast path can
index the front cache without recomputing the class from slot_size.
Implementation must keep both `owner_thread_token` and `class_id` in the
first cache line; moving `owner_thread_token` from its current offset is not
automatically forbidden, but any offset change should be called out in the
implementation note and checked against the stage-cost bench.

## Alternative considered and rejected as primary

"Design B": keep the page-centric hot path and make page selection O(1) with
a per-class stack of pages-with-capacity (push on full->nonfull transition,
pop instead of scanning). Cheaper to build and preserves per-page accounting
exactly, BUT the measured working_set=2 floor (~24-33ns, zero scan) proves
per-switch machinery alone is 2-3x tcmalloc -- B optimizes the scan away and
still cannot reach parity for 1-2-slot classes that switch every 1-2 ops.
Rejected as the primary shape; a nonfull-page stack remains a compatible
follow-up to make the front cache's REFILL O(1) if refill cost shows up in
the A/B.

## Box plan and gates

```text
HZ10FrontCache-L1 (implementation, opt-in):
  build flag HZ10_ENABLE_FRONT_CACHE, default OFF until A/B is favorable.
  gates:
    smokes + hz10-standalone-check + ASan/UBSan green, including the new
      lifecycle phase-0 smoke case
    accounting smoke: a front-cached slot does not increment page free_count
      or make its page idle until overflow/lifecycle flush returns it to the
      page layer
    duplicate-free smoke: a remote duplicate free of a front-cached slot is
      rejected through the existing local-free marker path
    flag-off smoke/A-B: HZ10_ENABLE_FRONT_CACHE=0 preserves the current
      local page path and benchmark rows
    bench-public-entry-local-path per class 16/64/24576/32768/65536 vs real
      tcmalloc same protocol as HZ10LocalPathTrim-L0; GO bar: every class
      inside ~1.5x tcmalloc (large classes from 5-10x)
    same-run vs tcmalloc RUNS=10 medians, all refreshed-table rows
      (main_local0/r50/r90, medium_local0, small_*, slot_count1_*);
      GO bar: main_local0 0.556 -> >=0.75, no row regresses >3%
    hz10-rss-guard passes with flush phase 0; steady-state RSS bench shows
      bounded current_rss_kb (front pin visible but flat)
  rollback: flag off restores byte-identical current behavior.

HZ10FrontCacheCapacityTune-L0 (only if L1 is GO):
  sweep HZ10_FRONT_CACHE_CLASS_BYTES x MIN_OBJECTS on the same rows plus
  RSS columns; pick defaults; consider total-budget slow-start (v2).
```

Expected side effect worth measuring, not assuming: main_r50/r90 should also
improve (alloc side currently pays 3.4-4.8 pages visited per find; refill
batching plus front-cache hits cut find_calls sharply), but the remote
publish cost itself is untouched, so remote rows will not reach local-row
ratios. That remaining remote gap stays owned by the deferred pending-bit /
publication-V2 designs, unchanged by this box.

## Open questions for reviewers

```text
1. class_id on the free path: page->class_id field (proposed) vs encoding in
   the pagemap entry flags -- does either add a cache line the free path
   does not already touch? (owner_thread_token is at offset 32; the field
   must stay inside the first line with base/local_free_head.)
2. Front-cache pop clears the local-free marker; page-alloc also clears it.
   Any path where a slot bypasses both? (Refill uses page-alloc, so the
   marker is cleared then set again on front push -- acceptable, or worth a
   splice-without-clear refinement?)
3. Is refill-loop-into-front for slot_count>=64 classes a regression risk
   (today's single active-page pop is already one branch)? If A/B shows the
   small classes prefer the old path, is a slot_count threshold (front cache
   only for slot_count <= 8) an acceptable shape, or does the extra branch
   cost more than it saves?
4. Overflow flush routes every pointer again. At cap/2 objects per flush this
   is boundary-amortized, but a mostly-free-only phase (working set shrink)
   pays it repeatedly -- is cap/2 the right hysteresis, or flush-to-empty?
5. Does the fixed ~3 MiB/thread worst-case pin need the v2 slow-start before
   default-ON, given HZ8-class RSS positioning?
```
