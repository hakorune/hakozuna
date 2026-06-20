# HZ8 Ownership Contract

This document fixes the ownership and safety contract for HZ8 v0.  Implement
these rules before tuning throughput.

## Boundary Results

HZ8 boundary classification returns one of:

```text
MISS:
  Pointer is outside all HZ8 reserved arenas and registered HZ8 direct ranges.
  Platform free is allowed.

VALID:
  Pointer is in an HZ8-owned range, the segment/span is active, the slot is
  aligned, and the object is live.

INVALID:
  Pointer is HZ8-owned-looking but invalid, stale, misaligned, interior,
  inactive, double-freed, or in a retired segment.
```

Only `MISS` can fall through to the platform allocator.  `INVALID` must fail
closed in research builds and must never call platform free.

## Small Ownership

Small object identity:

```text
segment + span + slot index
```

Small owner:

```text
span owner slot + owner generation
```

The span owner does not change while the span is active.  Remote free sends a
message to the owner; it does not transfer ownership of the object, descriptor,
source block, route, or storage.

## Owner States

Owner lifecycle states:

```text
ALIVE:
  Normal local allocation/free and remote collection are allowed.

DYING:
  Thread exit has started.  Local allocation stops.  The owner drains local
  pending spans and publishes orphanable spans to the slow path.

DEAD:
  No new remote publish may target this owner as a normal live owner.  Foreign
  frees route to orphan slow path.
```

Object-level rehome is forbidden.  Orphan handling adopts whole spans or
retires whole spans on slow paths only.

`OwnerLifecycle-0` is the highest-risk v0 box.  Do not treat owner death as a
small wrapper detail.  The design must answer these questions before promotion:

```text
who can collect a span after the owner enters DYING
who can collect a span after the owner reaches DEAD
when owner_generation is bumped
how foreign producers detect the transition
how a pending MPSC writer is excluded from adoption
how dead_owner_publish_lost remains zero
```

Required owner-death ordering:

```text
1. owner publishes DYING with release ordering
2. owner stops creating new active spans
3. owner drains its pending-span queue while producers still see DYING
4. owner marks remaining spans ORPHAN_CANDIDATE only after remote_head,
   remote_queued, and remote-pending bitmap are stable for adoption
5. adopter claims the whole span with an owner_generation bump
6. new owner may collect only after the generation bump is visible
```

Foreign producers that observe `DEAD` or a generation mismatch must not publish
to the old owner's pending queue.  They route to orphan slow path or fail closed
until the span has a visible adopted owner.

## Local Same-Owner Free

Required checks:

```text
1. pointer is inside the small arena
2. segment is active and has the expected tag
3. span metadata is active
4. slot offset is aligned to the class size
5. owner token matches the current thread owner
6. live bit is set
7. remote-pending bit is not set for the same slot
```

After validation, the owner may:

```text
clear live bit
write local free-list next link
push local free-list
decrement used_count
```

The same-owner local path must not perform global route lookup, exact route
lookup, shared directory lookup, owner rehome, OS query, or hot-path diagnostic
atomic increments.

## Foreign Free

Foreign free validates the pointer as an HZ8 small candidate, then publishes to
the span owner:

```text
1. pointer is inside the small arena
2. segment and span are active
3. slot offset is aligned
4. live bit is set
5. owner token identifies a live or slow-path-resolvable owner
6. remote-pending bit is atomically claimed
7. object payload is linked as an intrusive MPSC node
8. node is pushed to span remote_head
9. span is queued to the owner pending-span queue once
```

Remote producer forbidden actions:

```text
decrement used_count
clear live bit
push local free-list
drain remote_head
retire/decommit span
mutate source state
mutate global route state
change owner
fallback to platform free for owned-looking pointers
```

## Publish Ordering

The remote-pending bit is the duplicate-claim guard.  The object must be
published only after the bit is claimed.

Conceptual ordering:

```c
if (!atomic_test_and_set(remote_pending_bit(slot))) {
    return H8_INVALID_DUPLICATE_REMOTE_FREE;
}

node->next = atomic_load_explicit(&span->remote_head, memory_order_relaxed);
do {
    old = node->next;
    node->next = old;
} while (!atomic_compare_exchange_weak_explicit(
    &span->remote_head,
    &old,
    (uintptr_t)node,
    memory_order_release,
    memory_order_relaxed));

if (!atomic_exchange_explicit(&span->remote_queued, 1, memory_order_acq_rel)) {
    h8_owner_pending_span_push(owner, span);
}
```

Exact atomics may change during implementation, but the contract must preserve:

```text
remote_duplicate_claim = 0
remote_publish_lost = 0
remote_node_cycle = 0
remote_collect_duplicate = 0
```

## Owner Collect Ordering

Owner collect is the only path that consumes remote nodes:

```c
head = atomic_exchange_explicit(
    &span->remote_head,
    0,
    memory_order_acquire);

for each node in head:
    slot = h8_slot_from_node(span, node);
    verify live bit is set;
    verify remote-pending bit is set;
    clear remote-pending bit;
    clear live bit;
    push local free-list;
    span->used_count--;

if (span has no remote_head and no remote-pending bits) {
    atomic_store_explicit(&span->remote_queued, 0, memory_order_release);
}
```

The implementation must close the race where a producer publishes after the
owner observes an empty list but before `remote_queued` is cleared.

The pending-span queue is intentionally one-notification-per-span, but its
collection cadence is a performance-sensitive contract.  Benchmark the three
triggers separately:

```text
local miss collect:
  best locality, highest risk of charging remote work to unrelated local allocs

high-water collect:
  best burst control, must avoid oscillation

maintenance epoch collect:
  lowest hot-path pressure, highest latency/locality risk if used alone
```

Promotion requires a single balanced default trigger policy, not row-specific
collection profiles.

## Retirement Rules

A span may retire or decommit only when all are true:

```text
used_count == 0
remote_head == NULL
remote_queued == 0
remote-pending bitmap empty
span is not on an owner pending queue
owner lifecycle permits retirement
```

Retire/decommit forbidden states:

```text
span_decommit_while_pending
span_release_while_queued
segment_release_with_live_span
orphan_adoption_with_pending_writer
```

## Zero Gates

Safety gates that must remain zero:

```text
invalid_owned_fallback
invalid_owned_real_free
segment_directory_false_miss
stale_segment_valid
slot_alignment_escape

remote_duplicate_claim
remote_publish_lost
remote_node_cycle
remote_collect_duplicate
remote_pending_count_mismatch

span_decommit_while_pending
span_release_while_queued
segment_release_with_live_span

owner_generation_mismatch
dead_owner_publish_lost
orphan_adoption_with_pending_writer
```
