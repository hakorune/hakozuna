# HZ12 Windows Ordered Span Detach L2-E

## Purpose

L2-E is a dedicated single-thread/quiescent behavior smoke. It removes every
known reference to one fully accounted span in a fixed order, then invalidates
the direct-index route last. It does not decommit or release memory.

## Safety Prerequisite

An in-arena pointer whose span route is detached must never fall through to
the system allocator. HZ12 therefore treats an arena-contained classify miss
as fail-closed on free and returns zero from usable-size queries. The additional
check occurs only after the normal direct-index classify misses.

## Ordered Detach

```text
1. require clean full-span accounting
2. require an explicit single-thread/quiescent scope
3. remove this span's objects from the current thread front cache
4. remove this span's objects from the class returned sink
5. clear the current-span reference
6. verify that all slot-capacity objects were detached
7. invalidate the direct-index route last
8. verify the read-only reclaim gate opens
```

The smoke process owns the entire lifecycle. This is not yet a multi-thread
protocol and must not be called from the normal allocator path.

## Acceptance

```text
front_detached + returned_detached == slot_capacity
current_span_refs becomes zero
returned sink and front cache counts become zero
route is attached until the final step
post-detach reclaim gate opens
stale free after route detach is fail-closed
no VirtualFree/decommit/release occurs
```

## No-Go

```text
route is invalidated before cache/sink/current detachment is verified
arena classify miss reaches system free or usable-size
partial-span accounting is accepted
the behavior is enabled in the normal xowner lane
multi-thread quiescence is inferred from current-thread state
```

## Next Gate

L2-F may measure decommit of an already detached span. Reuse, recommit, and
multi-thread lifecycle remain separate gates.

## Initial Windows Evidence

The controlled 64-byte-class smoke produced this ordered transition:

```text
before:
  accounting_candidate = 1
  current_span_refs = 1
  front_cache_objects = 256
  returned_sink_objects = 768
  route_attached = 1
  gate_open = 0

detach:
  front_cache_detached = 256
  returned_sink_detached = 768
  current_span_detached = 1
  route_detached = 1

after:
  current_span_refs = 0
  front_cache_objects = 0
  returned_sink_objects = 0
  route_attached = 0
  gate_open = 1
```

The test then called usable-size and free with the detached stale arena address.
Usable size returned zero and free remained fail-closed; it did not enter the
system allocator. No page was decommitted or released. Existing owner shadow,
retired adoption, and normal xowner smoke tests remained green.
