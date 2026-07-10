# HZ12 Windows Owner Routing Shadow L0

## Scope

Windows is the first implementation target because the opening cross-owner
evidence and repeatable native benchmark runner exist there. L0 is a shadow
diagnostic, not an allocator behavior lane.

## Proposed Shadow Flow

```text
producer allocates from a span
consumer frees into its own current-holder cache
consumer cache flushes
  -> group arena objects by advisory span owner
  -> record would-route / would-keep-ownerless
  -> estimate target inbox pressure
  -> leave the actual HZ12 behavior ownerless
```

No free path is changed. The owner relation is read only while processing a
flush batch, and the result is diagnostic-only.

The initial projection uses a 256-object inbox batch cap, equal to the shadow
flush cap. A later cap sweep may lower it, but L0 first establishes whether
routing itself is material before treating a smaller cap as a policy.

## Required Counters

Counters exist only in a diagnostic build.

```text
flush_objects_total
flush_owner_local
flush_owner_foreign
flush_owner_unknown
would_route_batches
would_route_objects
would_keep_ownerless_overflow
projected_inbox_current_max
projected_orphan_objects
projected_reclaim_blocked_spans
```

## Acceptance

```text
cross-owner pipeline:
  foreign flush objects are material (>20%)
  projected routing is bounded
  no unknown-owner safety ambiguity

RSS/reclaim signal:
  spans with foreign flushes are identified as reclaim-blocked until L2 adds a
  verified whole-span free authority

implementation:
  speed build has zero new owner checks or atomics
  HZ11 behavior remains unchanged
```

## No-Go

```text
foreign flush objects are negligible
projected inbox grows with runtime without a bounded fallback
owner relation is required for pointer validity or double-free safety
shadow requires a per-free owner read
span reclaim cannot be distinguished from partial-span retention
```

## Initial Windows R3

The initial 4 producer / 4 consumer, 5-second xowner R3 produced the following
per-run shadow shape:

```text
foreign flush objects: 100%
unknown owners:         0
ownerless cap256 fallbacks: 0
projected inbox peak:   256
foreign-flush spans:    61
```

The routing signal is material, bounded, and unambiguous. This satisfies L0
and authorizes the separate L1 behavior prototype. It does not authorize span
reclaim yet: the 61 spans are explicitly reclaim-blocked until L2 provides a
verified whole-span free authority.
