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
| Slot owner side metadata | `api/hz6_allocator_slot_owner_sparse.*` | Diagnostic sparse per-slot owner table and future logical-owner fast-path helper boundary. |
| Policy/profile | `policy/`, capacity flags | Profile semantics and capacity shape. |

## Current Pressure Points

### 2026-06-13 Ubuntu/API Modularization Audit

Worker review confirms the high-level HZ6 layering is still sound:

```text
include/:
  public allocator ABI is intentionally narrow

route/:
  mostly independent route backend/table contract
  stores descriptors as opaque pointers

source/:
  OS memory ops and registry are cleanly separated from fronts

fronts/:
  expected bridge between policy, frontcache, descriptors, route registration,
  and source blocks

preload/:
  libc interposition remains isolated from allocator core; real libc symbol
  resolution lives in a separate preload real-wrapper module
```

Current cleanup targets:

```text
P0 docs/build hygiene:
  linux/hz6_preload_flags.sh is now the authoritative Ubuntu preload selected
  flag bundle for build_hz6_preload.sh and A/B runners.
  A/B scripts should use key-based hz6_preload_replace_define overrides, not
  positional array indexes.
  high-risk no-go/control flags should be explicitly default-off in preload

P1 source split:
  linux/hz6_sources.sh is now the shared source/include manifest for preload,
  benchmark, and R1 smoke builds. Keep new Linux build scripts on this manifest
  unless they deliberately need a smaller proof-only source set.

P2 preload facade:
  hide direct Hz6RouteResult / Hz6ObjectDescriptor use from preload behind
  allocator-owned helpers:
    hz6_preload_alloc()
    hz6_preload_free_if_owned()
    hz6_preload_usable_size_if_owned()
    hz6_preload_realloc_owned_or_copy()

P2 preload module split:
  hz6_preload.c is the largest current preload coupling hub. Split stable
  non-interposition code only after the current performance lane is closed:
    preload/hz6_preload_stats.c/.h for stats aggregation/printing
    preload/hz6_preload_midpage.c/.h for MidPage preload-boundary dispatch
  Keep libc hook control flow in hz6_preload.c and do not mix this cleanup with
  behavior changes.

P3 internal type split:
  split api/hz6_allocator_types.h into narrower descriptor/source-block/core
  internal type headers once the current preload lane is stable
```

Largest current coupling hubs:

```text
api/hz6_allocator_types.h:
  central internal coupling hub for allocator, descriptors, source blocks,
  route cache, active maps, backend state, and stats-owned internals

api/hz6_allocator_route.c:
  visibility registry, exact register/unregister, tombstone compact, and
  lookup variants

api/hz6_allocator_route_last_hit.c:
  per-allocator exact-pointer last-hit route cache validation, fill, and clear

api/hz6_allocator_route_owner_locality.c:
  owner-locality index register/unregister, memory accounting, and locality
  hint lookup

api/hz6_allocator_route_shared_directory.c:
  shared exact-route directory, elastic overflow invalid ranges, shared-dir
  dry-run diagnostics, and shared-dir first lookup helpers

api/hz6_allocator_source_block_create.c:
  source-block allocation, source-run bitmap, descriptor map ownership,
  elastic depot, dry-run stats, and creation flow

api/hz6_allocator_source_block_range_index.c:
  source-block page-range side index registration, unregistration, lookup,
  diagnostics counters, and small-run armed count ownership

fronts/hz6_front_reuse_transfer.c:
  transfer reuse core plus elastic depot diagnostics, source-run locality,
  slot-owner metadata, descriptor rehome, and route replacement dry-runs

fronts/hz6_front_source_block.c:
  source-run reuse, descriptor acquisition, invalid-range registration, exact
  route registration, source-block lifetime, and prefill orchestration

source/linux_source_mmap_memory.c:
  Linux mmap backing plus retained mapping cache, 64K retain stack, TLS
  retained no-go lane, and stats
```

Header-inline risk rule:

```text
Keep tiny proven hot accessors inline.
Do not move broad diagnostic or policy logic into headers by default.
Header helpers that mutate stats or inspect descriptor/source internals make
preload and unrelated translation units sensitive to code layout and macro
drift. The route header-inline and active-map code-shape attempts are no-go
evidence for this risk.
```

### Slot Owner Side Metadata Is Now Split From Front Reuse

`hz6_front_reuse_transfer.c` should stay focused on transfer reuse and front
activation.  The sparse slot-owner table used by SlotOwnerSparseMeta-L1 and
SlotOwnerConsumerDryRun-L1 now lives in:

```text
api/hz6_allocator_slot_owner_sparse.h
api/hz6_allocator_slot_owner_sparse.c
```

Contract:

```text
producer note:
  records SourceBlock pointer, block ptr, slot index, generation, owner16

consumer dry-run:
  runs only after the existing descriptor owner path computes the real answer
  may report would_skip_l2 only when generation and owner match the real answer
  must report false_positive if sparse metadata would have lied

future behavior:
  SlotOwnerLogicalOwnerFastPath-L1 may answer logical owner equality only for a
  generation-checked sparse owner match.
  miss / stale / mismatch must fall back to the existing descriptor-owner path.
  The broad owner_equal() entry-point version is safety-clean but speed no-go;
  do not retry it without a narrower admission gate.
```

Do not move this table back into a front helper.  It is not a frontcache policy;
it is owner-path side metadata shared by producer and consumer observations.

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
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512
```

Route capacity is now at the clean lower bound for the selected run512 shape.
The run512 route re-check found that route160k-run512 and route128k-run512
saturate during full-10k warmup: `route_active_current` reaches table capacity,
with `route_register_fail=3` and `alloc_fail=1`. RoutePackedMeta-L1 then reduced
the hot route entry to 24 bytes plus a side bytes array. RoutePackedMeta-L2
stores route bytes as `uint16_t(bytes - 1)` and moves the selected lowest-RSS
row to `48.367M / 449144 KB` in same-run repeat-3. Further RSS work should
target descriptor table/lifecycle or a broader route/descriptor ownership
representation, not another static route cut.

SourceBlockPackedFlags-L1 is the closed small source-block layout candidate:

```text
flag:
  HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1

change:
  pack source_kind / active / route_registered / run_active into one flag word
  keep source_release inline
  keep OwnerSourceSideMeta-L2 storage-owner pointer inline

diagnostic sizeof check:
  selected routepacked/routebytes16/L2 shape:
    source_block_entry_bytes 144 -> 128

Larson T16 main 10k repeat-3:
  sourceblockpacked alone:
    41.070M ops/s
    435304 KB
  combined with FrontCachePackedMeta-L1:
    40.837M ops/s
    426084 KB
```

Read:

```text
This is a low-risk RSS component before a larger source-run side-table split.
It does not change source-run slot semantics, route invalid-range ordering, or
source release ownership. The isolated repeat-3 closeout is clean, and the
combined FrontCachePackedMeta-L1 + SourceBlockPackedFlags-L1 lane is now the
current clean minimum-RSS candidate/sibling.
```

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

descriptor layout L2 dry-run:
  ownerless_hot_entry_bytes = 32
  ownerless_hot_table_bytes = 85983232
  ownerless_hot_savings_bytes = 20971520
  owner16_hot_entry_bytes = 40
  owner16_hot_savings_bytes = 0

side-owner16 L1 behavior:
  descriptor_entry_bytes = 32
  descriptor_table_bytes = 96468992
  one-run = 46.490M / 475672 KB
  no-go:
    route_invalid = 11739
    remote_free_transfer_fail = 11739
    lifecycle_foreign_free_invalid = 11739

descriptor-source diagnostic:
  no-backptr run512:
    descriptor_source_route_allocator_mismatch ~= 447.5M
    safety clean
  side-owner16 run512:
    descriptor_source_route_allocator_mismatch ~= 393.3M
    safety no-go

storage-owner16 L1 behavior:
  descriptor_entry_bytes = 32
  repeat-3 = 42.024M / 444520 KB
  safety clean
```

Read:

```text
Static descriptor cuts are exhausted, but descriptor layout still has useful
RSS headroom. No-backptr is a strong comparison control, routebytes16 is the
clean route-entry comparison control, and the current selected lowest-RSS
balance sibling is OwnerSourceSideMeta-L2 over the routebytes16/routepacked/
no-routebackptr/dir192k lane.
Packing owner slot/generation into 16-bit fields is not useful by itself
because alignment keeps the entry at 40 bytes. The first side-owner16 behavior
lane confirms that a 32-byte hot descriptor is mechanically possible, but
allocator-local side-owner metadata is not a safe cross-owner representation.
The descriptor-source diagnostic confirms that route rehome intentionally makes
route owner and descriptor-storage owner diverge at very high frequency. Side
owner metadata therefore needs a descriptor-storage owner source, not merely
the current allocator or `route.route_allocator`. StorageOwner16-L1 validates
that storage-owner keyed side metadata can be safety-clean, but it is RSS-first
evidence/control rather than selected because it saves RSS versus routepacked
while losing about 12% throughput.
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
SourceBlockMetaSlim-L1 moved the previous selected lowest-RSS sibling to
route192k-run512. RoutePackedMeta-L1 superseded DescriptorNoBackptr and
SourceBlock no-route-backptr controls, and RoutePackedMeta-L2 routebytes16 is
the clean route-entry comparison control. OwnerSourceSideMeta-L2 is the current
selected lowest-RSS balance sibling. FrontCachePackedMeta-L1 and
SourceBlockPackedFlags-L1 are component lower-RSS controls/candidates, and the
combined packed lane is the current clean minimum-RSS candidate/sibling.
StorageOwner16 is RSS-first descriptor side metadata evidence/control, not
selected.
Route160k-run512 and route128k-run512 remain no-go controls.

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
  clean previous selected lowest-RSS sibling/control

route192k-run512-no-backptr:
  40.710M ops/s
  476784 KB
  clean descriptor-layout control

routepacked/no-routebackptr/dir192k:
  47.616M ops/s
  456048 KB
  clean RoutePackedMeta-L1 control

routebytes16/routepacked/no-routebackptr/dir192k:
  48.367M ops/s
  449144 KB
  clean route-entry comparison control

storageowner16/routepacked/no-routebackptr/dir192k:
  42.024M ops/s
  444520 KB
  clean RSS-first descriptor side metadata evidence/control

storageowner16/ownersourcel2/routebytes16/routepacked/no-routebackptr/dir192k:
  40.754M ops/s
  439912 KB
  clean selected lowest-RSS balance sibling

frontcachepacked over OwnerSourceSideMeta-L2:
  41.131M ops/s
  430692 KB
  clean lower-RSS component candidate/control in the SourceBlockPacked
  closeout matrix

sourceblockpacked over OwnerSourceSideMeta-L2:
  41.070M ops/s
  435304 KB
  clean lower-RSS SourceBlock metadata component candidate/control

frontcachepacked + sourceblockpacked over OwnerSourceSideMeta-L2:
  40.837M ops/s
  426084 KB
  clean current minimum-RSS candidate/sibling
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
SourceBlockMetaSlim-L1 and SourceBlockPackedFlags-L1 succeeded as bounded
SourceBlock layout cuts.
The next static layout target is no longer another SourceBlock flag pack or
static route-capacity cut.
Route192k is the clean lower bound under run512; desc158k is the clean static
descriptor-capacity boundary and desc156k/below fail. Descriptor no-backptr L1
succeeded as the next layout target by reducing descriptor entry size from 48
to 40 bytes. RoutePackedMeta-L2 routebytes16 is now the clean route-entry
comparison control, OwnerSourceSideMeta-L2 supersedes it as the selected
low-RSS route/descriptor metadata shape, and combined packed is the current
minimum-RSS sibling/candidate. Descriptor layout L2 dry-run shows owner16 packing is a dead end without
another field move, while ownerless hot descriptor metadata projects a 32-byte
entry. Side-owner16 L1 reached the 32-byte entry but failed safety because the
side owner table is allocator-local and does not reliably read foreign
descriptor ownership. StorageOwner16-L1 makes that side metadata safety-clean
by using descriptor storage ownership, but the throughput loss keeps it as
RSS-first evidence/control instead of selected.
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
