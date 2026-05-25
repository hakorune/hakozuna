# HZ5 MidPageFront-M6 Deferred-Free Design

## Goal

Test whether the remaining Linux MidPage local-r0 gap to tcmalloc is caused by
free-time validation topology.

Current diagnostics show that smaller toggles are exhausted:

```text
direct-freeelide:
  ~125M ops/s

tcmalloc:
  ~219M ops/s

freeelide perf:
  ~1.9x tcmalloc instructions
  ~2.1x tcmalloc branches
```

Read:

```text
Skipping one slot-state check is not enough. HZ5 is still doing too much
per-object work on free: page lookup, slot index, owner check, and state
transition all happen before the pointer can leave free().
```

## Direction

M6 changes the detection timing, not the ownership model.

```text
free hit:
  push ptr to a raw quarantine buffer
  no page lookup
  no slot_index
  no owner check
  no slot-state transition

alloc miss / raw full:
  batch validate raw quarantine
  ptr -> page -> slot
  LIVE -> CACHE or LIVE -> REMOTE
  promote only validated slots into the magazine

alloc hit:
  pop from validated magazine
  CACHE -> LIVE
  return ptr
```

## Safety Contract

Raw quarantine is untrusted.

```text
raw_free:
  may contain duplicate, foreign, stale, or wrong-route pointers
  must never be used as an allocation source directly

validated magazine:
  contains only batch-validated MidPage slots
  may be used by alloc hit after CACHE -> LIVE transition
```

This preserves fail-closed-before-reuse:

```text
double free before reuse:
  first raw entry validates LIVE -> CACHE
  duplicate raw entry sees non-LIVE state and is dropped/fails closed

foreign pointer:
  raw entry fails page/slot validation
  it is not promoted to the validated magazine

wrong route:
  raw entry fails MidPage ownership lookup
  it is not promoted to the validated magazine
```

M6 does not promise immediate invalid-free reporting inside `free()`. It is a
diagnostic speed lane. Strict profiles keep immediate validation.

## Initial Lane

```text
--linux-hz5-general-midpage-m6-deferred-free-direct
```

Composition:

```text
M4 magazine
M4 remote packet
M5 hit-only alloc contract
M6 deferred free
no alloc-state elision
no free-state elision
no pointer-only unsafe magazine
no stats in speed lane
```

## Implementation

TLS state:

```c
void* m6_raw_free[64];
uint16_t m6_raw_count;
```

Free:

```text
hz5_midpagefront_free(ptr):
  raw_free[raw_count++] = ptr
  if raw_count == cap:
    flush_raw()
  return OK
```

Flush:

```text
for ptr in raw_free:
  page = page_for_ptr(ptr)
  slot = slot_index(page, ptr)
  if owner-local:
    require LIVE -> CACHE
    push validated magazine
  else:
    require LIVE -> REMOTE
    publish remote packet
```

Alloc:

```text
if magazine has entries:
  pop entry
  CACHE -> LIVE
  return ptr

if magazine empty:
  flush raw quarantine
  then normal M4 refill
```

## No-Go

Direct proof no-go:

```text
mid_only_r0 < 145M
or instructions remain above 400M in the direct/perf shape
```

Initial cap:

```text
raw_cap = 64
```

Rationale:

```text
The direct benchmark uses ws=100. A larger raw quarantine can delay reuse too
long and shift cost from free() into refill/new-slab pressure. Sweep 64, 128,
256 only after the direct proof shows a signal.
```

Weak keep:

```text
mid_only_r0 145M..160M
instructions < 400M
branches < 85M
```

Strong keep:

```text
mid_only_r0 >= 165M
instructions < 350M
branches < 70M
```

Safety no-go:

```text
duplicate free is promoted to the validated magazine
foreign pointer is promoted to the validated magazine
wrong-route pointer is promoted to the validated magazine
raw quarantine becomes unbounded
remote-owned pointer is returned to an owner-local magazine
```

## Next If It Wins

```text
1. Add MidPage page-tag lookup for faster batch validation.
2. Integrate a preload/general M6 lane.
3. Recheck r0/r50/r90 and cross-size rows.
4. Only then revisit RouteTag-R1 for the broader free classifier.
```

## Result

Raw output:

```text
private/raw-results/linux/midpage_m6defer_direct_smoke_20260525_160954
private/raw-results/linux/midpage_m6defer_cap64_direct_smoke_20260525_161054
private/raw-results/linux/midpage_m6batch_cap64_direct_smoke_20260525_161155
private/raw-results/linux/midpage_m6batch_perf_20260525_161204
```

Direct MidPage API, RUNS=5:

```text
M6 raw_cap=256, per-object flush:
  101.02M

M6 raw_cap=64, per-object flush:
  102.40M

M6 raw_cap=64, page-batch state update:
   79.30M

previous direct-freeelide:
  125.02M

tcmalloc reference from mid_only shape:
  219.37M
```

Decision:

```text
M6 is no-go. The raw quarantine delays reuse and shifts work into refill/new
slab pressure. Page-batch grouping adds enough local overhead to regress below
the simpler immediate-validation path.
```

Implication:

```text
The tcmalloc gap is not explained by fail-closed immediate validation timing.
Further work should compare the whole MidPage class/cache/span shape against
tcmalloc and HZ4 rather than adding more deferred-free variants.
```
