# HZ10 Tests

First tests should cover `HZ10PageMapRoute-L0`:

```text
exact pointer valid
interior pointer invalid
misaligned pointer invalid
generation mismatch invalid
unknown pointer miss
```

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

