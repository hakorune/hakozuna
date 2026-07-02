# HZ9 Local Slab Page Route Boundary L0

## Decision

```text
state:
  first L0 route scaffold implemented

goal:
  connect the fast fused local body evidence to a public allocator-shaped
  split malloc/free boundary

non-goal:
  do not add another cache on top of HZ8 medium runs
  do not chase more fused-only microbench variants as behavior evidence
  do not reopen the remote protocol in L0
```

SegmentEntry showed that prevalidated token-local bodies are fast. It also
showed that splitting public `malloc` and `free` is the real boundary cost.
`tlstokencachetrustbody` only partly recovered the split loss, so push-side
revalidation alone is not the blocker.

## Required Shape

```text
malloc side:
  use a TLS prevalidated local page/token
  pop a slot without route lookup
  keep generation/owner validation at acquire/retire boundaries

free side:
  derive segment/page/slot from the pointer in O(1)
  validate exact slot pointer
  classify interior/stale/double free as owned INVALID
  use the same route authority for free, usable_size, and realloc
```

The free route cannot disappear. The C API provides only `void*`, so public
`free` must still distinguish:

```text
MISS:
  not HZ9-owned, may forward

VALID:
  exact currently allocated HZ9 slot

INVALID:
  HZ9-owned-looking bad pointer, do not forward
```

The target is to remove global scan/table-style routing from the hot boundary,
not to remove identity work entirely.

## Route Authority

L0 should introduce one route result shared by the public boundary:

```c
typedef enum H9RouteKind {
  H9_ROUTE_MISS = 0,
  H9_ROUTE_VALID = 1,
  H9_ROUTE_INVALID = 2
} H9RouteKind;

typedef struct H9RouteResult {
  H9RouteKind kind;
  void* segment;
  void* page;
  uint32_t slot;
  uint32_t class_id;
  uint32_t slot_size;
  uint32_t reason;
} H9RouteResult;
```

The intended route shape is address-derived:

```text
ptr
  -> aligned segment base
  -> segment header magic/generation
  -> page metadata
  -> exact slot decode
  -> state bits
```

For L0, the exact segment size and page layout are implementation details. The
important property is that `free`, `usable_size`, and `realloc` do not maintain
separate pointer authorities.

## Preserved Contracts

```text
owned INVALID fail-closed:
  owned-looking interior, misaligned, stale, or double free is not forwarded

exact pointer only:
  only slot base addresses are VALID

shared route:
  free / usable_size / realloc share one route authority

remote duplicate-free authority:
  pending bitmap / qstate remains the future remote authority

owner lifecycle:
  thread and owner exit must drain local page state

HZ8 isolation:
  no HZ8 default behavior change
```

## Relaxed HZ9 Contract

HZ9 may retain more memory than HZ8, but only under explicit caps.

```text
allowed:
  per-thread local segments
  bounded retired segment cache
  higher post-workload RSS while threads are alive

required:
  cached bytes drain at thread/owner exit
  segment cap is reported
  remote-contaminated segments are bounded separately
```

## Counters

Minimum route counters:

```text
h9_route_attempt
h9_route_miss
h9_route_valid
h9_route_invalid_interior
h9_route_invalid_misaligned
h9_route_invalid_tail
h9_route_invalid_stale
h9_route_invalid_double
h9_route_invalid_cache
h9_route_invalid_pending
```

Minimum boundary counters:

```text
h9_local_malloc_call
h9_local_malloc_tls_page_hit
h9_local_malloc_page_empty
h9_local_malloc_page_create
h9_free_same_owner_local
h9_free_remote_publish_stub
h9_free_invalid_owned
h9_free_miss_forward
h9_usable_route_valid
h9_realloc_route_valid
```

Lifecycle/RSS counters:

```text
h9_segment_create
h9_segment_retire
h9_segment_release
h9_segment_live_bytes
h9_segment_peak_bytes
h9_segment_remote_contaminated
h9_thread_exit_segment_remaining
h9_owner_exit_segment_remaining
```

## Hard Zero Gates

```text
route_interior_valid = 0
route_misaligned_valid = 0
route_tail_slack_valid = 0
route_stale_valid = 0
route_double_free_valid = 0

owned_invalid_forwarded_to_platform = 0
usable_size_route_mismatch = 0
realloc_route_mismatch = 0

remote_duplicate_claim_accepted = 0
remote_pending_lost = 0
remote_mutated_local_free_bits = 0

thread_exit_segment_remaining = 0
owner_exit_segment_remaining = 0
segment_magic_false_positive = 0
segment_generation_stale_accepted = 0
```

## L0 Performance Gates

Use fused token bodies only as ceilings, not promotion proof.

```text
class64 split boundary:
  >= 430M ops/s:
    GO to broader route-boundary integration

  350M..430M ops/s:
    HOLD and inspect route asm/counters

  < 350M ops/s:
    NO-GO for this boundary shape

global-scan route comparison:
  O(1) route/free should be >= 1.25x current route/free scaffold
```

Later matrix gates, after public integration:

```text
medium_local0:
  >= HZ8 * 1.10

main_local0:
  >= HZ8 * 1.05

remote rows:
  <= 5% regression for L0
  <= 2% regression for L1

RSS:
  within explicit HZ9 segment cap
  cached bytes drain to zero on owner/thread exit
```

## Implementation Order

```text
done:
  route result type and smoke-only O(1) segment route scaffold
  same-owner split malloc/free smoke
  shared usable_size/realloc route smoke
  split malloc/free microbench for same-owner local slots

latest short read:
  class64 split touch=1: about 93M..97M ops/s
  class64 splitdirect touch=1: about 109M ops/s
  class64 usable/realloc touch=1: about 50M..53M ops/s

reading:
  safe registry route is not the only blocker
  direct-owned address-derived route improves only modestly
  the current public-shaped debug boundary is far below the 350M..430M HOLD/GO
  band and needs a tighter inline/local body before broader integration

next:
  remote-publish stub counters only; do not design remote protocol yet
  compare against SegmentEntry split/token-cache probes
```

Stop if correctness requires weakening owned INVALID, slot-state validity, or
future pending/qstate remote authority.
