# HZ8 SmallTransitionInventory-L1

Updated: 2026-07-14

## Decision

```text
status:
  L1-A local behavior implemented
  L1-B normal-owner remote collection implemented
  correctness smoke GO
  performance/default gate pending

next experiment:
  SmallTransitionInventory-L1

do not add:
  object cache
  new slot state
  per-free atomic or global lock
  Mag16 plus a second publication path
```

The next small-front experiment replaces the semantic role of Mag16 rather
than placing another cache in front of it. An inactive span is published only
when it changes from exhausted to available. The existing per-span free list
remains the sole authority for reusable objects.

## Implementation Checkpoint

The behavior lives behind `H8_SMALL_TRANSITION_INVENTORY_L1`. The speed target
contains no diagnostic counter atomics; `H8_SMALL_TRANSITION_INVENTORY_DIAG`
builds a separate counter-bearing sibling.

Implemented boundaries:

```text
local free:
  publish only exhausted-to-available transitions

allocation:
  transition inventory before owner scan/source commit
  recheck after remote-pressure collection

normal owner remote collection:
  collector reports through the current owner context

owner exit:
  reset inventory before quiesce/handoff

remote producer and orphan collector:
  never mutate owner-local inventory
```

Initial GCC/Clang smoke and GCC safety stress pass. A short diagnostic run had
zero duplicate insertions and zero pop rejections. Single-run mixed samples
were roughly neutral-to-slightly-negative and had lower peak RSS, but they are
not a performance gate; paired fresh-process Windows/Linux rows remain
required.

## Evidence

The counter-free small hot-pair audit showed that warmed HZ8 allocation/free is
already cheaper than tcmalloc at 64 and 128 bytes. The mixed attribution probe
then separated two transition problems:

```text
LCG:
  empty class-local visibility causes unnecessary span commits

xorshift:
  commits are rare
  Mag pop/hit traffic reaches about 15-21% of active small allocations
  local frees repeatedly replace the active span and publish the old span
```

The P1 diagnostic also measured far fewer exhausted-to-available transitions
than current Mag pushes:

| Row | Transition events | Current Mag pushes | Approx. reduction |
|---|---:|---:|---:|
| balanced | 59,889 | 191,207 | 3.2x |
| wide_ws | 40,597 | 195,319 | 4.8x |
| larger_sizes | 9,622 | 43,795 | 4.5x |

This makes availability transition, not object caching or replacement policy,
the narrowest measured control point.

## Contract

### Authority

```text
slot state and per-span free list:
  object reuse authority

transition inventory:
  advisory span discoverability only

owner token, generation, and span state:
  lifetime and fail-closed authority

remote pending bits:
  remote publication authority until owner collection
```

An inventory entry never makes a pointer valid and never owns an object. A
FREE slot appears only in the existing span free list. No `LOCAL_FAST_FREE` or
other cache-specific slot state is introduced.

### Owner-local representation

Each class has one intrusive LIFO head in `H8ThreadCtx`. Each small span gains:

```c
H8Span* next_transition_available;
bool transition_indexed;
```

These fields exist only in the experimental sibling. They are owner-local and
non-atomic: remote producers never write them. Capacity is bounded by the
allocator's existing owned-span population, so there is no separate allocation,
fixed-cap ladder, eviction policy, or duplicate hint history.

The intended invariant is:

```text
active available span:
  current allocation source, not indexed

inactive available span:
  appears exactly once in its class inventory

inactive exhausted span:
  not indexed
```

### Local free

The existing validation and slot publication order remain unchanged.

```text
1. Validate class, owner token, generation, span state, slot state, and pending.
2. Capture the old local free-list head.
3. Publish the slot as FREE and link it into the span free list.
4. If this is the active span, stop.
5. If old_head was empty and bump is exhausted, the span changed from
   exhausted to available.
6. If not already indexed, push the span once into the owner-local inventory.
7. Preserve the current active span; do not replace it with the freed span.
```

The transition predicate prevents every free from becoming publication work.

### Allocation

```text
1. Allocate from the current active span exactly as today.
2. When it is exhausted, pop one transition-inventory candidate.
3. Clear transition_indexed before validating the candidate.
4. Validate class, owner token, generation, span state, and local usability.
5. Discard stale advisory entries and continue.
6. Make a valid candidate active and allocate from its authoritative free list.
7. Only after inventory miss use pending collection, adoption, or source commit.
```

Validation failure is fail-closed and cannot turn an invalid object into reuse.

## Remote And Lifecycle Boundary

Remote producers retain the current behavior: they publish pending bits and do
not touch the inventory.

Normal owner collection should return a `became_available` result from the span
collector. The owner thread then applies the same transition helper to its own
`H8ThreadCtx`. This keeps cross-thread writes out of owner-local metadata.

```text
normal owner collect:
  collect pending slots
  -> report exhausted-to-available transition
  -> owner publishes one inventory entry

owner exit:
  quiesce
  -> clear inventory heads and indexed bits
  -> hand off or retire spans
  -> never publish into a closing owner

orphan adoption:
  validate and transfer ownership/generation
  -> clear stale transition links
  -> rebuild discoverability from authoritative bump/free-list state
```

The collector API may accept the current owner context or return transition
information to it. It must not find and mutate a foreign thread context.

## Diagnostics

All new counters are diagnostic-only and absent from speed/default binaries:

```text
transition_candidate
transition_push
transition_duplicate
transition_pop_attempt
transition_pop_hit
transition_pop_reject
transition_depth_current
transition_depth_max
transition_remote_publish
transition_adoption_publish
transition_exit_clear
```

Existing commit, Mag, owner-lifecycle, pending, route, and safety counters remain
the comparison baseline. The behavior sibling must not update production-hot
atomics.

## Implementation Phases

```text
L0:
  freeze this contract and diagnostic schema

L1-A:
  add the owner-local intrusive inventory behind
  H8_SMALL_TRANSITION_INVENTORY_L1
  replace Mag publication/selection only in that sibling
  complete

L1-B:
  connect normal remote collection through a transition result
  add owner-exit clearing and adoption rebuild
  normal collection and exit clearing complete
  adoption returns the adopted span as active; explicit multi-span rebuild is
  not currently required

L1-C:
  add speed and diagnostic build targets
  run safety, LCG, xorshift, fixed-small, remote, and RSS gates

promotion:
  only after Windows and Linux application-like controls pass
```

Do not combine this lane with P1 or Mag16 behavior. Default Mag16 remains the
rollback/control until promotion is complete.

## Acceptance

### Correctness

```text
duplicate inventory insertion:
  0

normal pop owner/generation/class/state rejection:
  0

owner exit inventory depth:
  0

remote duplicate free / invalid free / stale generation:
  fail closed

slot simultaneously pending and allocatable:
  impossible

inventory indexed active span:
  0

adopted span discoverability:
  rebuilt from authoritative state
```

### Performance And RSS

First Windows gate uses fresh-process AB/BA medians with at least R10:

```text
xorshift balanced / wide_ws / larger_sizes:
  experimental signal: each >= default +10%
  strong signal: each >= default +15%
  no measured row below default -3%

fixed small and exact 8K/16K/32K controls:
  >= default -3%

remote-small:
  >= default -3%
  peak and post RSS <= default +5%

LCG:
  source commits reduced >=90% from default
  peak RSS approaches the bounded P1 evidence
```

Linux must reproduce the direction with no major row below -3% and no RSS
regression. Synthetic success alone cannot promote the shared default; Redis or
another application-like gate is required.

## No-Go

```text
per-free atomic or global lock
object-pointer cache or a second object authority
new cache-specific slot state
remote producer writes owner-local inventory
Mag and transition inventory publish the same span
capacity or replacement-policy ladder
weakened owner/generation/state validation
promotion from one synthetic trace
```

If transition publication does not improve both LCG visibility and xorshift
churn without crossing these boundaries, close the experiment rather than add
another cache tier.
