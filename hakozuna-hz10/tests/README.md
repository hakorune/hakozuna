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

