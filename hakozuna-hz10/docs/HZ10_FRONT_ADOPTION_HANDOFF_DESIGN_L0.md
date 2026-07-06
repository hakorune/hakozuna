# HZ10FrontAdoptionHandoff-L0

Status: implemented as an opt-in sibling lane. The handoff mechanism is GO;
front cache defaulting is NO-GO on the first RUNS=5 macro gate.

## Problem

`HZ10_ENABLE_FRONT_CACHE && HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION` is currently
blocked by a compile-time guard. The guard is correct for the current code:
front-cached slots stay allocated from the page's point of view until a front
overflow or lifecycle flush returns them to the page layer. If a thread exits
while its TLS front cache still holds slots, `hz10_owner_destructor()` publishes
the owner's active pages as orphan pages with hidden free capacity still trapped
in the dead thread's TLS cache.

That creates two bad outcomes:

```text
1. orphan adoption sees less capacity than actually exists;
2. pages can remain permanently non-idle because cached slots died with TLS.
```

The issue is not remote-free correctness. It is only a missing handoff from the
exiting owner's TLS front cache back into its page lists before orphan publish.

## Proposed Box

Add one owner-exit phase before `HZ10_THREAD_OWNER_STATE_EXITED` is published:

```text
hz10_owner_destructor(owner):
  0. front-cache owner-exit flush, if compiled
  1. clear hz10_current_owner
  2. store owner->record->state = EXITED (release)
  3. publish active lists into the orphan registry
  4. release live Hz10ThreadOwner storage
```

The phase 0 function is implemented in `hz10_public_entry.c`, where the front
cache TLS storage lives. `hz10_public_entry_owner.c` only calls a narrow hook:

```text
hz10_public_entry_owner_exit_flush_front_cache()
```

Builds without `HZ10_ENABLE_FRONT_CACHE` compile this hook as a no-op.

## Proof Obligations

P1. Owner-only access:
  A front cache is `_Thread_local` and only the owner thread can push/pop it.
  The pthread-key destructor runs on that same exiting thread, so it has
  exclusive access to the front cache being flushed.

P2. Owner token still live:
  The new phase runs before `owner->record->state = EXITED` and before
  `hz10_current_owner` is cleared. Every front-cached slot belongs to a page
  whose owner token is still this owner record.

P3. Page-layer semantics:
  The existing `hz10_front_cache_flush_to_count(..., 0)` path routes each slot
  and calls `hz10_freelist_page_free(page, slot)`. This is plain owner-local
  free, not adoption, not remote publish, and not page destruction.

P4. Remote frees can still be in flight:
  The destructor still does not destroy pages. It only returns owner-local
  front slots to their pages, then existing orphan publish exposes the pages
  for later adoption. Remote claim/publish remains owner-agnostic and is still
  drained by the adopter before use.

P5. Adoption sees true capacity:
  After phase 0, `free_count` includes every slot formerly held in the dead
  front cache. Partial adoption's existing pop-transfer-drain-add protocol
  therefore sees the same capacity it would see if the owner had manually
  flushed at a quiescent boundary before exit.

P6. Lifecycle flush contract unchanged:
  `hz10_public_entry_flush_thread_cache_quiescent()` keeps its existing phase 0
  front flush. The new destructor hook is narrower: it never reclaims or
  destroys pages, and it does not claim to satisfy the manual quiescent flush
  contract.

## Implementation Plan

1. Remove the compile-time guard blocking front cache + orphan adoption.
2. Add the owner-exit front-cache flush hook.
3. Call the hook at the top of `hz10_owner_destructor()`, before `EXITED`.
4. Add a smoke target that builds front + orphan + partial adoption together.
5. Add a shim sibling `libhz10_front.so` for A/B:
   `orphan + partial adoption + fine classes + front cache`.
6. Gate on:
   - front/orphan/partial smoke;
   - shim API/foreign smoke;
   - TSan ASLR-off smoke;
   - sh6bench and macro subset A/B versus current default.

## Rollback

The default remains unchanged until the A/B gate says GO. `libhz10_front.so`
is an opt-in sibling. If the box regresses RSS or correctness, remove the hook
and restore the compile-time guard.

## Implementation Result

Implemented:

```text
1. removed the compile-time guard;
2. added `hz10_public_entry_owner_exit_flush_front_cache()`;
3. call the hook at the top of `hz10_owner_destructor()`, before EXITED;
4. added front+orphan+partial public-entry and owner smoke targets;
5. added `libhz10_front.so` / `make preload-front`;
6. added `hz10-front` as an opt-in macro/larson allocator name.
```

RUNS=5 macro A/B, `ALLOCATORS_CSV=hz10,hz10-front`:

```text
workload       hz10 wall/RSS          hz10-front wall/RSS      verdict
python_alloc   0.930s / 106,716 KiB   1.000s / 107,820 KiB    regress
redis_setget   0.550s /   8,192 KiB   0.550s /   8,192 KiB    flat
larson         4.172s / 287,360 KiB   4.183s / 284,160 KiB    flat
xmalloc_test   2.000s /  14,336 KiB   2.000s /  13,696 KiB    flat
cache_scratch  1.090s /   3,968 KiB   1.110s /   3,968 KiB    slight regress
mstress        0.210s / 205,872 KiB   0.230s / 207,628 KiB    regress
sh6bench       0.810s / 320,256 KiB   0.800s / 321,792 KiB    slight win
```

Verdict:

```text
handoff safety: GO
`hz10-front` opt-in lane: GO for future attribution
front cache in shim default: NO-GO for now
```

The old expectation was that sh6bench would recover most of the former
front-cache micro win. The current default with fine classes and partial
adoption does not show that: sh6bench is only a 1.2% median win while
python_alloc and mstress regress. Keep the handoff because it is the correct
boundary for any future front lane, but do not flip the default.

Result log:

```text
bench_results/20260707T_front_adoption_handoff_l0/
```
