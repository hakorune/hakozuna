# HZ10OwnerLocalPageIndex-L0

Status: design candidate. Do not implement until reviewed.

## Goal

Reduce the remaining `hz10_free()` local-owner instruction block after
`HZ10FreeFastLeafSplit-L0` and `HZ10MallocFastLeafSplit-L0`.

The current local free path still starts with the global pagemap reader:

```text
ptr -> root load -> leaf load -> record loads -> base/span/align/interior
    -> page pointer -> page owner token compare -> local free
```

That is correct for unknown pointers and foreign frees, but it duplicates work
for the common owner-local case: the owner thread already has a live
`Hz10ThreadOwner`, and each page already records its `owner_thread_token`.

This box adds a per-owner page index as a cache for owned small pages. It is
not a replacement for pagemap. The global pagemap remains the authority for
unknown pointers, large allocations, foreign frees, and diagnostics.

## Non-Goals

- Do not weaken fail-closed pointer validation.
- Do not change remote-free publish/claim ordering.
- Do not change orphan/partial adoption semantics.
- Do not optimize large allocations; they keep the pagemap/large path.
- Do not remove marker writes or `free_count` accounting.

## Proposed Shape

Add an opt-in build flag:

```c
HZ10_ENABLE_OWNER_LOCAL_PAGE_INDEX=1
```

Extend `Hz10ThreadOwner` with a bounded owner-local index keyed by quantum base:

```text
base = ptr & ~(HZ10_PAGE_QUANTUM - 1)
page_index = base >> HZ10_PAGE_SHIFT
```

The first implementation should be a cache, not a complete sparse map:

```text
entry:
  base
  page
  generation
  slot_size
  slot_count
```

A direct-mapped or small set-associative table is acceptable for L0. On miss
or collision, fall back to the existing pagemap route. This keeps the rollback
and correctness story simple: the index can only make local frees faster when
it hits; it must never be required for correctness.

## Fast Path

`hz10_free(ptr)` becomes:

```text
owner = current_owner_if_any()
if owner:
  base = align_down(ptr, HZ10_PAGE_QUANTUM)
  entry = owner_index_lookup(owner, base)
  if entry hit:
    page = entry.page
    if page owner token == owner->record
       and entry/page generation still match
       and ptr is a valid slot start inside the page:
         local_free(page, ptr)
         return 1

fallback:
  existing hz10_pagemap_route_local_fast / slow route
```

The slot-start validation is load-bearing. The owner token proves that this
thread owns the page; it does not prove that `ptr` is the beginning of a valid
slot. Keep the same validation properties as `hz10_pagemap_route_local_fast()`:

- `offset < slot_size * slot_count`
- `offset` is at least `HZ10_MIN_ALIGN` aligned
- `slot_count == 1` requires `offset == 0`
- otherwise `offset % slot_size == 0`

This means L0 removes the global root/leaf/record lookup from owner-local hits,
but it does not claim to remove the interior modulo check.

## Index Maintenance Boundary

Index updates must live at ownership transitions, not at arbitrary free sites.

Register in the owner index when a page becomes owned by this thread:

- fresh page creation, after `owner_thread_token` is set and before returning
  the first allocation;
- partial/idle orphan adoption, after owner transfer and before adding the
  page to the adopter's class list;
- any future owner-handoff path, at the same boundary that writes
  `owner_thread_token`.

Remove from the owner index before a page stops being owned by this thread:

- before destroy/reclaim of an owned page;
- before explicit owner transfer away from this thread, if such a path is ever
  added for live owners.

Do not remove on active-to-retired movement. Retired pages are still owned by
the same thread, and local frees into retired pages should still hit the local
index if the page has live objects.

Thread exit needs no per-entry cleanup because the whole `Hz10ThreadOwner` is
released after active pages are published as orphan pages. The destructor
already clears `hz10_current_owner` before late destructor frees can observe
the orphan-published pages as owner-local.

## Safety Proof Sketch

P1. Unknown and foreign pointers still have the old authority.
If owner-local lookup misses or any guard fails, `hz10_free()` falls back to
the current pagemap path.

P2. Index hits are still fail-closed.
The fast path validates the page owner token, generation, span, alignment, and
interior slot boundary before calling `hz10_public_entry_free_local_page()`.

P3. Stale entries fail conservatively.
Entries carry `base` and `generation`. A reused quantum or reused metadata node
must not be accepted unless it still matches the current page identity and the
current page owner token is this thread's owner record.

P4. Foreign frees never read another thread's index.
The index is reached only through `hz10_current_owner`; a cross-thread free has
the freeing thread's owner, not the page owner's index, and therefore falls back
unless the page is actually owned by the freeing thread.

P5. Orphan/partial adoption has one ownership boundary.
Adoption already pops from the orphan registry before changing
`owner_thread_token`. L0 must insert into the adopter index only after that
owner write. The old owner's index is dead with the old `Hz10ThreadOwner`.

P6. Destroy/reclaim must unregister first.
Any path that calls `hz10_pooled_page_destroy()` or
`hz10_freelist_page_destroy*()` for an owned page must remove the owner-index
entry before releasing the page/pagemap record.

## Observability

Add opt-in counters:

```text
owner_index_hit
owner_index_miss
owner_index_guard_fail_owner
owner_index_guard_fail_generation
owner_index_guard_fail_interior
owner_index_insert
owner_index_remove
owner_index_collision
```

The first A/B should report hit rate and guard-fail counts alongside macro
wall time. A low hit rate means the table shape is wrong, not that the idea is
unsafe.

## Gates

Correctness:

- existing smoke suite and `hz10-standalone-check`
- `smoke-shim-foreign` must still fail closed for unknown pointers
- add focused smoke:
  - owner-local valid pointer hits index and frees correctly
  - interior pointer with matching owner page is rejected/falls back, not freed
  - destroyed/reused page does not accept a stale index entry
  - adopted orphan page becomes index-visible for the adopter

Performance:

- hz10-only RUNS=5 macro matrix first, target sh6bench.
- Full all-allocator RUNS=5 guard if hz10-only is not worse.
- Required diagnostic output: owner-index hit rate on sh6bench and
  python_alloc.

GO bar:

- sh6bench improves repeatably without python/larson/mstress regression;
- guard-fail counters stay near zero except intentionally negative tests;
- RSS remains flat.

NO-GO:

- broad macro regression;
- meaningful guard-fail count in normal workloads;
- hit rate too low to justify keeping the index in product code.

## Open Questions

1. Direct-mapped vs small set-associative table. Start with the simplest table
   that gives a measurable hit rate; avoid building a complete sparse map in
   L0.
2. Exact table size. It must be large enough for sh6bench's active page set but
   small enough that `Hz10ThreadOwner` footprint remains reasonable.
3. Whether the slot validation should use cached `slot_size/slot_count` or
   reload from `page`. Reloading from `page` costs a few instructions but
   makes stale-cache bugs easier to fail closed.
4. Whether to make this shim-only at first. Product evidence currently comes
   from `libhz10.so`, so an opt-in shim lane may be enough for L0.
