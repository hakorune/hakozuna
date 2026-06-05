# HZ6 ElasticCapacity Plan

This note is the short design target for the next HZ6 Windows work after the
Larson low-RSS lane cleanup. Keep detailed benchmark history in
`current_task.md`; use this file to decide what ElasticCapacity-L1 is allowed
to change.

## Current Selected Lane

```text
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512
```

Status:

```text
KEEP as current Larson minimum-RSS sibling.
Not broad promotion/default.
Not a speed-ranking diagnostic row.
```

Evidence:

```text
main10k repeat-3:
  44.864M ops/s
  412,280 KB peak RSS
  safety clean

guards:
  worker10k clean
  main/worker4k clean
  main/worker1k clean

source-cap boundary:
  source12k = backup/control
  source8k/source2k = no-go from warmup SourceBlock exhaustion
```

## What The Diagnostics Proved

ElasticProjection-L1:

```text
current static table size:
  234,502 KiB

projected static table size from final local high-water:
  21,616 KiB

projected static + payload:
  46,000 KiB

read:
  There is large RSS upside if local tables can be made elastic.
```

ElasticHighWater-L1:

```text
selected source10k final worker high-water:
  descriptor_live_max      = 400
  source_block_active_max  = 25
  frontcache_total_max     = 401
  route occupied max       = 5,425

read:
  The final worker steady-state is tiny compared with static caps.
```

MainWarmupCapacity-L1:

```text
main-warmup full10k before worker handoff:
  descriptor_used   = 160,048
  route_occupied    = 170,051
  source_block_used = 10,003
  frontcache_used   = 48

worker/final after handoff:
  descriptor max worker = 400
  route max worker      = 5,425
  source max worker     = 25
  frontcache max worker = 401

read:
  Low local caps fail because the main allocator temporarily owns the whole
  seeded live set. Final worker high-water alone is not a safe sizing rule.
```

ElasticOverflowProjection-L0:

```text
full10k with final high-water local caps:
  descriptor local cap  = 1,024
  route local cap       = 16,384
  source block local cap= 64
  frontcache local cap  = 1,024
  transfer local cap    = 128

projected main-warmup overflow:
  descriptor overflow   = 159,024 / 5,591 KiB
  route overflow        = 153,667 / 3,902 KiB
  source block overflow = 9,939 / 1,242 KiB
  frontcache overflow   = 0
  transfer overflow     = 0
  total overflow        = 10,735 KiB

read:
  The first ElasticCapacity behavior must cover descriptor, route, and source
  metadata together. Frontcache and transfer are not first-order pressure in
  this selected Larson row.
```

ElasticRouteOverflow-L1:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticroute-desc160k-front4k-thindesc-
  nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route16k-run512

shape:
  route local cap = 16k
  shared exact route overflow
  shared SourceBlock invalid-envelope overflow
  descriptor/source/frontcache remain source10k-control sized

full10k run-1:
  41.723M ops/s
  335,132 KB peak RSS
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    alloc_fail=0

read:
  Shared route overflow is a viable safety mechanism and gives a large RSS
  reduction. It is not the promotion lane because throughput is below the
  selected source10k control. Route-only overflow also leaves descriptor and
  source-block static capacity untouched, so it is a component/control for the
  unified ElasticCapacity design rather than the final shape.
```

## Design Target

ElasticCapacity-L1 should reduce replicated per-worker static metadata while
preserving the HZ6 route/ownership safety contract.

Required shape:

```text
local small tables:
  descriptor
  route
  source block
  frontcache

shared overflow / depot:
  descriptor metadata
  route metadata
  source-block metadata

handoff:
  main-warmup burst can live in overflow
  worker handoff/rehome drains or bounds overflow
  final worker steady-state returns to small local tables
```

Non-goals:

```text
SourceBlock-only overflow as the main behavior.
Another entry-packing-only lane.
Hot-path diagnostic counters or global scans.
Changing P43/P45/HZ5 Windows selected lanes.
```

## Safety Contract

Route result semantics must not weaken:

```text
MISS:
  not HZ6-owned in the searched route domain

VALID:
  exact route for a currently owned object

INVALID:
  owned-looking pointer, stale pointer, interior pointer, or protected source
  envelope hit
```

Elastic overflow must preserve:

```text
owned-looking invalid never falls through to CRT/libc fallback
route INVALID must not become MISS
route rehome failure must not create reusable stale objects
source block release must not leave exact routes or invalid envelopes behind
descriptor generation remains fresh across reuse/materialization
```

## First Implementation Direction

Start with a unified metadata overflow skeleton or diagnostic prototype rather
than a SourceBlock-only behavior.

Recommended L1 order:

```text
1. Define shared-overflow accounting and ownership terms:
     local table entry
     overflow entry
     overflow owner/origin
     localize/drain/rehome

2. Keep `[HZ6_ELASTIC_OVERFLOW_PROJECTION]` as the L0 sizing diagnostic:
     descriptor_overflow
     route_overflow
     source_block_overflow
     frontcache_overflow
     transfer_overflow
     overflow_total_bytes

3. Implement one metadata class only after the unified contract is clear.
   Prefer route or descriptor first, because MainWarmupCapacity shows they
   spike higher than source blocks.

4. Add source-block overflow only as part of the same contract, not as an
   isolated fix.
```

## Acceptance

Weak accept:

```text
main10k safety clean
worker10k safety clean
main/worker1k and 4k guard rows clean
peak RSS lower than source10k selected lane
no route_miss / route_invalid / route_register_fail regressions
no descriptor/source-block exhaustion
overflow current is bounded after handoff
```

Strong accept:

```text
main10k stays near source10k throughput
peak RSS drops by at least 50 MiB
overflow drains close to final worker high-water
local static capacity can be reduced without warmup failures
```

No-go:

```text
owned-looking invalid becomes MISS
route rehome failure is nonzero
descriptor/source-block exhaustion returns
overflow grows with chunks or repeat count without bound
worker steady-state slows by more than 5%
SourceBlock-only overflow improves one counter but leaves descriptor/route
warmup pressure unresolved
```
