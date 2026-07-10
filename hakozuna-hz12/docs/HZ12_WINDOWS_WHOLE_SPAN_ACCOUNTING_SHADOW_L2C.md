# HZ12 Windows Whole-Span Accounting Shadow L2-C

## Purpose

L2-C measures an object-level prerequisite for whole-span reclaim. The
standalone `hz12_span_accounting` module records per-span tracked allocation,
release, and live counts in the HZ12 benchmark wrapper. It does not modify the
allocator's fast path or release a span.

```text
tracked allocation:
  span.alloc_count += 1
  span.live_count += 1

actual wrapper release:
  span.free_count += 1
  span.live_count -= 1
```

## Accounting Candidate

A span is an accounting candidate only when all of these are true:

```text
the span has a known HZ12 size class
tracked allocation count equals that class's slot capacity
tracked free count equals tracked allocation count
tracked live count is zero
no tracked-release underflow occurred
```

This is deliberately weaker than reclaim eligibility. It does not prove that
the span is detached from a current span, front cache, returned sink, exact
route, or source lifetime. L2-C calls it an `accounting candidate`, never a
reclaimable span.

## Contract

```text
tracking is benchmark/diagnostic-only
release observation occurs immediately before the existing hz12_free call
an untracked or duplicate release is counted, never converted into reclaim
counter state is not used by normal malloc/free behavior
retirement/adoption remains separate from span accounting
```

## Required Evidence

```text
one controlled full-span smoke reaches one accounting candidate
the smoke reports alloc_count == free_count == slot_capacity and live == 0
release underflow = 0
the same span is not released or decommitted
ordinary xowner runs keep pointer/free behavior unchanged
```

## No-Go

```text
live == 0 is treated as enough to decommit/release
current span or front-cache state is inferred from this counter alone
the accounting table is consulted on the production hot path
untracked release or underflow is ignored
L2-C is used to claim RSS improvement before a separate reclaim lane exists
```

## Next Gate

L2-D must combine this accounting witness with explicit current-span, cache,
returned-sink, route, and source-lifetime detachment. Only then can a bounded
reclaim/decommit experiment be considered.

## Initial Windows Evidence

The controlled standalone smoke allocated exactly one 64-byte-class span's
1,024 slots, routed all objects through the existing inbox drain, and observed
`alloc=1024`, `free=1024`, `live=0`, `candidates=1`, with zero untracked release
and zero release underflow. No span was released or decommitted.

The ordinary 2 producer / 2 consumer xowner run ended with `tracked_live=0`,
but saw 39 tracked spans, zero full-capacity spans, and zero accounting
candidates. This is the expected result for a mixed, partially issued span
workload. It confirms that an empty inbox or zero live count is insufficient
evidence for reclaim and that L2-D needs explicit detachment state.
