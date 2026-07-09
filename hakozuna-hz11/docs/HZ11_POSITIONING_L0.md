# HZ11 Positioning L0

Status: design scoped, no allocator behavior implemented.

## Decision

HZ11 is a separate speed-first allocator line.

It is not a faster HZ8/HZ10. The HZ11 premise is that tcmalloc-class throughput
requires a tcmalloc-shaped allocator: front-end caches, transfer caches, central
spans, and a pageheap. HZ8/HZ10 ownership and reclaim mechanisms may exist in
debug or checked lanes, but they must not become default hot-path work.

```text
HZ8:
  public recommended line
  low post-workload RSS
  fail-closed ownership and cross-thread correctness

HZ10:
  active speed/RSS-aware research candidate
  keeps explicit route, ownership, orphan, and reclaim boundaries

HZ11:
  speed-first tcmalloc competitor
  RSS-capped rather than low-RSS-first
  diagnostics in checked/debug lanes
```

## Windows Bring-Up Position

Windows support is a bring-up track, not the primary Linux fine128 line.

```text
selected Windows row:
  hz11-span

scope:
  Windows random_mixed and allocator-matrix connectivity
  HZ11_CLASSIFY_SPAN=1
  VirtualAlloc arena
  span classify path
  CRITICAL_SECTION returned-object locks

evidence:
  random_mixed RUNS=3:
    small  149.284M ops/s, peak 4.18MB
    medium 147.110M ops/s, peak 4.99MB
    mixed  148.464M ops/s, peak 5.04MB

not claimed:
  not a default allocator
  not a Windows DLL replacement
  not fine128 parity
  not a remote/mixed speed-lane claim
  not a replacement for HZ8/HZ10
```

The Windows row exists so HZ11 can be measured in the existing Windows
cross-allocator harness without pretending that the Linux fine128 / transfer
line has already been ported.

## Why A New Line

HZ10 proved that keeping route, ownership, free-count, remote-drain, and reclaim
boundaries in or near the hot path has a real fixed cost. Bump-init and leaf
splits made HZ10 much stronger, but the remaining local gap to tcmalloc is tied
to semantics HZ10 intentionally keeps.

HZ11 therefore changes the question:

```text
Not:
  How do we make HZ10 as fast as tcmalloc while preserving HZ10 semantics?

Instead:
  What does a Hakozuna speed-first allocator look like if it is allowed to move
  fail-closed diagnostics and strict owner-return semantics out of the default
  hot path?
```

## Relation To HZ3

HZ11 overlaps with HZ3 in local-heavy intent, compact hot paths, and a
lookup/cache-first posture. It should still be a new line.

```text
HZ3:
  historical local-heavy public profile
  useful reference for compact fast paths and old benchmark baselines

HZ11:
  explicit tcmalloc-speed competitor
  starts with front-end cache / transfer cache / central span design
  defines checked-mode diagnostics separately from speed mode
```

HZ3 should be treated as design history and a baseline, not as the substrate to
mutate into HZ11.

## Required Architecture

### Front End

Default malloc/free hits must be thread-cache or CPU-cache operations.

```text
malloc cache hit:
  size -> class table lookup
  cache count check
  pointer pop
  return

free cache hit:
  token exact hit or route fallback
  current thread/CPU cache push
```

The hot path must not touch:

```text
pagemap route on every local free
owner-return remote protocol
slot_state authority
pending bitmap
qstate
owner queue
generation validation
diagnostic counters
remote drain
RSS accounting
```

### Remote Free Policy

HZ11 does not return ordinary remote frees to the origin owner in the default
speed path.

```text
HZ8/HZ10:
  remote free -> publish to owner -> owner drains later

HZ11:
  free(ptr) pushes reusable objects into the freeing thread/CPU cache, then
  spills batches to transfer cache / central spans when capacity is exceeded
```

This is the largest intentional semantic split from HZ8/HZ10.

### Middle And Back End

```text
transfer cache:
  per size class
  batch refill and batch return
  sharded lock or simple mutex in early boxes

central spans:
  per size class spans with free object batches
  span metadata touched only on refill/flush/large/fallback paths

pageheap:
  page runs
  coalescing later
  explicit release policy later
```

## Size Classes

HZ11 should prefer a table lookup over closed-form class computation.

```text
target:
  16B..64KiB or 128KiB
  about 60-80 classes
  uint8 class_map[(size + 15) >> 4] for small sizes
```

The exact table is a measured box. The principle is fixed: do not spend branch
and arithmetic budget on elegant classification if a small table wins.

## Checked Mode

HZ11 speed mode is for valid C programs first.

```text
speed mode:
  fast malloc/free
  gross unknown pointer route fallback
  no full per-op double-free guarantee

checked mode:
  marker or state bitmap
  generation checks
  route reason counters
  optional quarantine / canary / sampled guard
```

Do not describe HZ11 speed mode as equivalent to HZ8 fail-closed behavior.

## RSS Contract

HZ11 may retain more memory than HZ8, but it must have explicit caps.

```text
initial target:
  peak RSS <= tcmalloc * 1.25 on macro rows
  post RSS <= tcmalloc * 1.50 on macro rows
  thread-exit cache flush
  transfer cache cap
  central/pageheap release policy tracked as later boxes
```

HZ11 is speed-first, not unbounded-retention-first.

## Public Claims

Allowed before implementation:

```text
HZ11 is a planned speed-first Hakozuna research line.
HZ11 targets tcmalloc-class throughput.
HZ11 intentionally separates speed mode from checked diagnostics.
HZ11 does not replace HZ8.
```

Forbidden until evidence exists:

```text
HZ11 beats tcmalloc.
HZ11 replaces HZ8.
HZ11 is fail-closed like HZ8 while matching tcmalloc speed.
HZ11 is a hardened allocator.
```

## First Gates

The first prototype should be killed early if it cannot move local rows.

```text
fixed64_local0:
  first gate >= tcmalloc * 0.95

guard_local0:
  first gate >= tcmalloc * 0.85

main_local0:
  first gate >= tcmalloc * 0.80
```

If a per-thread HZ11 prototype cannot approach those gates, do not stack more
complexity. Re-open the front-end design.
