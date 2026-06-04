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
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
```

Route capacity is now at the clean lower bound. The run512 route re-check found
that route160k-run512 and route128k-run512 saturate during full-10k warmup:
`route_active_current` reaches table capacity, with `route_register_fail=3` and
`alloc_fail=1`. Further RSS work should target descriptor table/lifecycle or a
new route representation, not another static route cut.

Static descriptor-capacity trimming is also effectively at the lower bound for
the current representation:

```text
desc160k route192k run512:
  40.578M ops/s
  499804 KB
  clean repeat-3 control

desc158k route192k run512:
  40.400M ops/s
  498080 KB
  clean tiny-RSS sibling/control

desc156k and below:
  warmup no-go
  descriptor_exhausted=3
  alloc_fail=1
```

Read:

```text
Static descriptor cuts can save only about 1.7MB more before warmup failure.
Descriptor layout slimming is now the better RSS path. The no-backptr L1
removes the allocator back-pointer from `Hz6ObjectDescriptor` and passes
allocator explicitly to descriptor lifecycle helpers:

```text
baseline run512:
  descriptor_entry_bytes = 48
  descriptor_table_bytes = 127926272
  repeat-3 = 40.498M / 499812 KB

no-backptr run512:
  descriptor_entry_bytes = 40
  descriptor_table_bytes = 106954752
  repeat-3 = 40.710M / 476784 KB
```

Read:

```text
Static descriptor cuts are exhausted, but descriptor layout still has useful
RSS headroom. No-backptr is a strong keep and should be guarded as the selected
lowest-RSS Larson sibling candidate before the next descriptor representation
change.
```
```

### Route Table Is Already Capacity-Bounded

The route capacity ladder found:

```text
route224k:
  clean control

route192k:
  clean route-capacity control

route160k / route128k:
  warmup no-go from route saturation
```

Do not trim route capacity again under the current representation.
SourceBlockMetaSlim-L1 moved the selected lowest-RSS sibling to
route192k-run512; route160k-run512 and route128k-run512 remain no-go controls.

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

Implemented experiment lanes:

```text
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run2048
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
```

Runner:

```powershell
.\win\run_win_hz6_selected_family.ps1 -LarsonSourceRunMetaSlim
```

Initial repeat-3 result:

```text
route192k:
  44.610M ops/s
  628844 KB

route192k-run1024:
  44.396M ops/s
  518256 KB

route192k-run512:
  48.512M ops/s
  499820 KB
  clean selected lowest-RSS sibling
```

Run512 attribution check:

```text
descriptor_table_bytes       = 127926272
route_table_bytes            = 100663296
source_block_table_bytes     = 37748736
frontcache_table_bytes       = 33560576
ownerlocality_index_bytes    = 18874368
source_block_payload_bytes   = 24969216
```

Read:

```text
SourceBlockMetaSlim-L1 succeeded.
The next layout target is no longer SourceBlock or static route capacity.
Route192k is the clean lower bound under run512; desc158k is the clean static
descriptor-capacity boundary and desc156k/below fail. Descriptor no-backptr L1
succeeded as the next layout target by reducing descriptor entry size from 48
to 40 bytes. Guard no-backptr across the selected family before trying owner
token compression, descriptor side metadata, or a route/descriptor ownership
representation rewrite.
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
