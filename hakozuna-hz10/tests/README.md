# HZ10 Tests

First tests should cover `HZ10PageMapRoute-L0`:

```text
exact pointer valid
interior pointer invalid
misaligned pointer invalid
generation mismatch invalid
unknown pointer miss
```

Bonus case added for src/hz10_large_alloc.h: a single-slot (slot_count ==
1) registration may span more than one HZ10_PAGE_QUANTUM (an address in a
later quantum of that span correctly MISSes, since only the base's own
quantum is ever registered), while a multi-slot registration spanning
more than one quantum is still rejected -- the relaxation is single-slot
only. Also checks owner/flags round-trip through route() unaffected.

`HZ10ThreadLocalFreelistPage-L0` tests:

```text
page exhaustion: alloc returns slot_count distinct pointers, then NULL
LIFO reuse: the most recently freed slot is the next one allocated
non-LIFO reuse: alloc all slots, free in shuffled order, re-alloc all;
  the recovered set matches the original set exactly (no duplicates, no
  drops) -- this is the case HZ9's ProductEntry free-cache never covered
pagemap integration: create() publishes to HZ10PageMapRoute-L0 (route is
  VALID at the page base immediately after create), destroy() releases it
  (route is no longer VALID with the pre-destroy generation afterward)
```

Known, accepted gap (not a smoke requirement): a same-thread double-free is
not rejected at this layer. Layer 0's local_free_head/local_free_count are
trusted, plain-load/store state by design (see docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md
"Forbidden fast-path shapes" and "Design rule"); fail-closed validation for
untrusted pointers belongs to the route boundary (Layer 1), not the local
freelist itself.

`HZ10RemoteStackDrain-L0` tests:

```text
basic push/drain: a remote free is not visible in local_free_head until
  the owner drains; after drain, the exact same pointer comes back
duplicate rejected: a second remote free of an already-pending pointer is
  rejected before it drains, so the slot is recovered exactly once, not
  twice, after drain
foreign double-free rejected before push: if a foreign thread remote-frees a
  slot that the owner has already returned to local_free_head, the local-free
  marker rejects it before Treiber push can overwrite the local freelist next
  pointer
stale generation / invalid pointer rejected: a remote free using a stale
  (pre-recreate) generation, or a misaligned/interior/out-of-range
  pointer, is rejected immediately and counted, never pushed
concurrent stress: N threads each remote-free a disjoint slice of
  already-allocated slots concurrently; a single drain afterward must
  recover every slot exactly once (no lost pushes under CAS contention,
  no duplicates) -- this is the actual proof the Treiber stack is correct,
  not just the single-threaded API shape
```

Known, accepted gap: Box 2 (`hz10_freelist_page_destroy()`) does not drain
the remote stack itself -- Box 2 has no dependency on Box 3's module, only
the reverse. A caller that has exposed a page to `hz10_page_remote_free()`
must call `hz10_page_drain_remote()` itself before destroy(), or an
already-pushed-but-undrained remote free is silently lost; a foreign thread
starting a *new* push concurrently with destroy is not defended against
either way. Both are owner-exit/thread-lifecycle protocol questions left to
a later box (see docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md's RSS Contract
"exit" section), not something Box 2 or Box 3 alone can guarantee.

`HZ10BoundedPagePool-L0` tests:

```text
pool cap and reuse: acquiring from an empty pool returns NULL; releasing
  more blocks than the cap keeps cached_count pinned at the cap and counts
  the excess as real releases; acquiring drains the cache back down and
  counts as reuse, never fabricating a block
pooled_page basic: hz10_pooled_page_create/destroy behave exactly like
  plain hz10_freelist_page_create/destroy; destroy offers its block back
  to the pool
generation bump on reuse: a block recycled from the pool is re-registered
  with a bumped generation (same contract as any HZ10PageMapRoute-L0
  re-registration), so routing with the pre-recycle generation is
  rejected as stale, not silently treated as still valid
sustained churn bounds the cache: creating more pages than the cap, all
  kept alive, then destroying all of them forces real releases for the
  excess -- a deterministic, counter-based proof of "release pressure"
  that a raw OS RSS sample cannot give reliably run to run
```

Scope note: this box proves the page-pool mechanism and its integration
with Box 2 for a single size class, per the L0 discipline every prior box
in this line has kept. It does not add multi-class size dispatch or a
public malloc()/free() entry point comparable to HZ8/HZ9's medium_local0/
main_local0/medium_r50/main_r90 bench rows -- that wiring is future work,
not something this box claims to deliver.

Size-class table (src/hz10_size_class.{h,c}) tests:

```text
table shape: every class's slot_size matches the documented table
  exactly, strictly increasing, a multiple of HZ10_MIN_ALIGN, and fits
  within one HZ10_PAGE_QUANTUM; class_id >= HZ10_CLASS_COUNT returns 0
exhaustive classification: for every one of the 65536 possible byte
  sizes (not sampled), hz10_size_class_for() matches a from-the-table
  linear-scan oracle -- this closed-form, bit-twiddling classify
  function is exactly the kind of code that hides off-by-one bugs at
  octave boundaries, so every boundary is checked directly, not spot-checked
rejected inputs: size 0 and size > HZ10_PAGE_QUANTUM are invalid; size
  == HZ10_PAGE_QUANTUM is the last valid class
```

Multi-class public entry (src/hz10_public_entry.{h,c}) tests:

```text
multi-class basic: alloc/free/touch across several distinct size classes,
  plus LIFO reuse of the same active page after a free -- same shape as
  Box 2's own smoke, exercised through hz10_malloc/hz10_free
rejected inputs: malloc(0), free(NULL) as a no-op success, and an
  interior/unknown pointer, all rejected per the pipeline
  hz10_public_entry.h documents. malloc(quantum+1) is NOT in this list
  any more -- see large-alloc basic below, it now succeeds
abandoned page still freeable: exhausting a page (its class's active
  page is replaced) does not break freeing the pointers it already handed
  out -- route()'s owner tag recovers the exact Hz10FreelistPage* even
  after it stops being anyone's "active" page
exhausted page recovered via remote free: a page exhausted locally, then
  remote-freed from a foreign thread, is found and drained by
  src/hz10_class_pages.h's scan on the very next malloc for that class --
  proven by getting the exact same address back, not merely a fresh page.
  This is the regression test for the fix described in src/hz10_class_pages.h
  and current_task.md (Box 5's original abandon-on-exhaustion policy
  measured 15-17x slower than system malloc on remote-heavy rows)
scan-limit tradeoff: locks in the accepted cost of
  HZ10_CLASS_PAGES_SCAN_LIMIT (src/hz10_class_pages.h) directly -- a page
  freed while buried deeper than the scan limit's worth of never-freed
  fresh pages has real capacity, but the next malloc must NOT find it
  (proven by getting a genuinely different pointer back); documents the
  bounded-scan/permanently-invisible-capacity tradeoff rather than
  merely asserting it in a comment
cross-thread free: allocate on one thread, free on another -- must route
  through Box 3's remote path (accepted, not yet visible locally), then
  actually come back once the allocating thread's own traffic drains
  that page (proves the owner_thread_token dispatch in hz10_free, not
  just Box 3's mechanism in isolation)
large-alloc basic (src/hz10_large_alloc.h): sizes at and beyond
  HZ10_PAGE_QUANTUM+1 now succeed instead of returning NULL; the full
  requested range is genuinely writable, not just the base pointer; and
  distinct large allocations of different HZ10_PAGE_QUANTUM-multiple
  sizes do not overlap/corrupt each other
large-alloc edges: an interior pointer into a large allocation is
  rejected (same universal rule every small class already enforces),
  and a large allocation freed on a different thread than it was
  allocated on works correctly (hz10_large_alloc.h documents there is no
  per-thread ownership concept for large objects, unlike Box 3's remote
  path for small classes)
```

Known gap, not a smoke requirement: `hz10_free()` cannot carry a
generation from the caller (real `free(void*)` has no such parameter), so
it always routes with `HZ10_GENERATION_ANY`. The generation-mismatch
contract Box 1-4's own smokes exercise directly with an explicit
generation is real and tested there, but is not reachable through this
public API by design -- there is no channel to carry it across a genuine
malloc/free boundary, the same way real allocators do not expose one
either.

ASan/UBSan note: LeakSanitizer reports "still reachable" allocations from
pages this smoke creates and never destroys (by design -- see "abandoned
page" above, and there is no process-wide teardown API for the public
entry's TLS state). This is expected; the smoke passes with
`ASAN_OPTIONS=detect_leaks=0` and is clean of actual memory-safety errors
(use-after-free, overflow, UB) either way.
