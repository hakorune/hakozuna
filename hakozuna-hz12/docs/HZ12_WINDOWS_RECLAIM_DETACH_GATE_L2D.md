# HZ12 Windows Reclaim Detach Gate L2-D

## Purpose

L2-D is a read-only reclaim gate. It combines the L2-C accounting candidate
with explicit detachment witnesses. It does not remove an object, clear a
route, decommit a page, or release a span.

## Module Boundary

```text
hz12_shadow:
  advisory owner routing only

hz12_span_accounting:
  diagnostic alloc/free/live accounting only

hz12_inbox:
  bounded handoff and retired-owner adoption only

hz12_reclaim_gate:
  read-only composition of accounting and detachment witnesses
```

## Gate Inputs

```text
accounting_candidate:
  full slot capacity issued, matching releases, live == 0

current_span_refs:
  current thread still names the span as a bump source

front_cache_objects:
  current thread still caches objects from the span

returned_sink_objects:
  global returned sink still contains objects from the span

route_attached:
  direct-index span class still recognizes the span

thread_scope_complete:
  caller proves the diagnostic snapshot is single-thread/quiescent
```

The gate is open only when accounting is complete and every detachment witness
is zero. A multi-thread snapshot is blocked unless global thread quiescence is
explicitly proven.

## Expected First Result

The controlled L2-C full-span smoke should remain blocked after all objects are
freed because its exhausted current span, front cache, returned sink, and route
still reference that span. This is useful evidence: `live == 0` is not a
reclaim contract.

## No-Go

```text
gate state changes allocator behavior
returned-sink inspection runs outside a diagnostic/smoke path
current-thread inspection is treated as global thread quiescence
route attachment is cleared by the read-only gate
gate open is treated as permission to decommit before a behavior lane exists
```

## Next Gate

L2-E may prototype ordered detachment in a dedicated single-thread smoke:
remove cached/sink objects, clear current-span ownership, then invalidate the
route last. Decommit remains a later, separately measured behavior.

## Initial Windows Evidence

The controlled 64-byte-class smoke issued and released all 1,024 slots from a
single span. L2-C reported one clean accounting candidate. L2-D then reported:

```text
current_span_refs = 1
front_cache_objects = 256
returned_sink_objects = 768
route_attached = 1
gate_open = 0
```

The object counts account for all 1,024 slots while the route and exhausted
current-span reference remain attached. This is the expected closed-gate
result. No object was removed from either cache and no route or page state was
changed.
