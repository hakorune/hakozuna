# HZ12 Windows Bounded Retired Detach L5-C

## Purpose

L5-C performs the first bounded multi-span behavior after L5-B proves all
enrolled class caches are flushed. It reuses the existing L2 ordered-detach
helper and processes at most 64 retired-owner spans (4 MiB) per invocation.

## Admission

```text
owner retirement gate open
exact slot + generation side-table match
thread scope complete
physical full-span accounting candidate
current/front cache empty
returned sink count equals physical slot capacity
route still attached
```

The existing helper removes returned objects first and detaches the route last.
L5-C does not decommit, release, or place spans in the depot.

## Initial Result

The Windows wide_ws-like diagnostic passed repeat-3 with a 64-span budget.
Every run detached 64/64 spans, failed zero times, and converted exactly
4.00 MiB into fully gated reclaimable bytes. The physical candidate pool stayed
between 67.94 and 73.44 MiB, so the fixed budget was the limiting factor. No
decommit or depot behavior ran.

## Acceptance

```text
attempts and successes stay within fixed budget
every success becomes exactly one 64 KiB reclaimable span
no stale-generation span is touched
all earlier lifecycle/reclaim smokes pass
```

## No-Go

```text
partial returned-sink detach
route detach before object detach completes
behavior when owner/thread scope gate is closed
decommit or depot insertion in L5-C
unbounded scan behavior in a production path
```
