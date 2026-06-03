# HZ6 Source Modularization

HZ6 should keep algorithmic contracts common while letting each OS backing and
benchmark profile specialize only where needed. This file tracks the source
cleanup direction before the next optimization pass.

## Current Module Boundaries

| Area | Current files | Contract |
| --- | --- | --- |
| Public API / allocator state | `api/`, `include/` | Owns allocator lifecycle, descriptors, source blocks, stats snapshots. |
| Route | `route/` | `MISS / VALID / INVALID`; exact routes must outrank invalid ranges. |
| Front cache | `frontcache/`, `fronts/` | Owns hot cached objects and class routing. |
| Transfer | `transfer/` | Bounded cross-owner transfer and reuse. |
| Source | `source/`, source-block helpers in `api/` | Reserve/commit/release backing and source-block lifetime. |
| Owner | `owner/` | Owner token/liveness contract. |
| Policy/profile | `policy/`, capacity flags | Profile semantics and capacity shape. |

## Current Pressure Points

### SourceBlock Is Too Heavy

`Hz6SourceBlock` currently stores both source lifetime fields and source-run
slot metadata:

```text
source lifetime:
  ptr / bytes / source kind / release callback / route backend / ref_count

source-run slot policy:
  slot bytes / class id / slot count / used count / hint / bitmap
```

This is clean logically, but expensive for high-capacity Larson lanes because
the run bitmap is present in every static source-block entry.

Current selected Larson lowest-RSS lane:

```text
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k
```

Route capacity is now at the clean lower bound, so further RSS work should
target SourceBlock metadata layout rather than another static route cut.

### Route Table Is Already Capacity-Bounded

The route capacity ladder found:

```text
route224k:
  clean control

route192k:
  clean selected lowest-RSS sibling

route160k / route128k:
  warmup no-go from route saturation
```

Do not trim route capacity again until the source-block metadata shape changes.

## Next Refactor Candidate

### SourceBlockMetaSlim-L1

Goal:

```text
reduce static SourceBlock metadata RSS without changing route/source lifetime
semantics.
```

Safe first experiments:

```text
1. compile-time run-slot ladder:
   HZ6_SOURCE_RUN_MAX_SLOTS = 2048 / 1024 / 512
   paired with the selected route192k Larson lane

2. source-run metadata split:
   keep Hz6SourceBlock lifetime fields compact
   move run bitmap / run policy fields to a side metadata table

3. lazy run metadata:
   allocate or attach run metadata only when a block becomes a source-run block
```

Before the metadata-layout experiment, keep small shared helpers out of the
platform files:

```text
source/hz6_source_util.h:
  common reserve-size rounding for Linux mmap and Windows VirtualAlloc backing

api/hz6_allocator_scavenge_internal.h:
  shared descriptor scavenge cost calculation
```

Acceptance:

```text
Larson full-10k:
  throughput within -5% of route192k
  RSS lower than route192k
  safety counters stay zero

Other selected family rows:
  unchanged unless explicitly run with the new source-block lane
```

No-go:

```text
route_register_fail > 0
source_block_exhausted > 0
alloc_fail > 0
source block release leaves exact routes or invalid ranges behind
mixed/larger/random selected rows are affected by a Larson-only experiment
```

## Refactor Rules

Keep these stable:

```text
Route:
  exact VALID outranks invalid range INVALID
  owned-looking invalid must not become MISS

Descriptor:
  descriptor generation changes on reuse
  thin descriptor lanes must not starve normal descriptor allocation

SourceBlock:
  invalid range stays while any materialized or detached slot is retained
  source release only after routes and retained slots are gone

Transfer:
  bounded; no unbounded cross-owner backlog
```

Do not mix these changes in one experiment:

```text
SourceBlock metadata slimming
remote handoff policy
frontcache spill/borrow/cap
descriptorless governor
```

The next implementation should be a narrow metadata layout experiment, not a
new allocator policy.
