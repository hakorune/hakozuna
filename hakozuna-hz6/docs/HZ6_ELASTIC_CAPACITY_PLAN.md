# HZ6 ElasticCapacity Plan

This note is the short design target for the next HZ6 Windows work after the
Larson low-RSS lane cleanup. Keep detailed benchmark history in
`current_task.md`; use this file to decide what ElasticCapacity-L1 is allowed
to change.

## Current Selected Lane

```text
ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-
directfree-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-
noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512
```

Status:

```text
KEEP as current Larson/Elastic low-RSS sibling.
Not broad promotion/default.
Not a speed-ranking diagnostic row.
```

Evidence:

```text
guard repeat-3:
  improves every main/worker 1k/4k/10k row over DepotOwnerDirect
  average +2.60%, minimum +1.34%
  main10k = 43.75M ops/s / 224,724 KB peak RSS
  safety clean

role:
  selected source-depot low-RSS sibling for Larson/Elastic comparisons
  clean control for future conditional transfer/rehome policy
  not a broad mixed_ws/default allocator profile
```

## Current Lane Status

```text
Selected Larson/Elastic low-RSS sibling:
  DepotOwnerDirect + DirectFreeTrustedLocalCache source-depot lane

Candidate-watch:
  ElasticDescriptorRouteOverflow-L1

Lower-RSS component/control:
  ElasticDescriptorSourceRouteOverflow-L1

Diagnostic no-go/control:
  SourceBlockLocalizeDryRun-L1

Diagnostic slot-level evidence:
  SourceRunLocalityDryRun-L1

Prerequisite/control:
  SourceRunMetadataOnDepot-L1

Diagnostic slot-owner evidence:
  SlotOwnerLocalityDryRun-L1

Sparse side metadata evidence:
  SlotOwnerSparseMeta-L1

Diagnostic consumer evidence:
  SlotOwnerConsumerDryRun-L1

Latest no-promotion evidence:
  DFTLC + DepotDescriptorRehomeBudget2048 intersection dry-run
  Budgeted rehome composes poorly with DFTLC on main10k and raises RSS.
  Do not run another fixed budget sweep unless a new diagnostic signal appears.

Next if reopened:
  conditional transfer/rehome policy
  trigger only when transfer-depot pressure is present and the selected DFTLC
  path is not already the dominant solution
  keep broad mixed_ws safety guards separate from Larson/Elastic ranking
```

SourceBlockLocalizeDryRun-L1 result:

```text
read:
  whole-SourceBlock localize is not the next behavior.
  Transfer reuse sees depot blocks with storage-owner mismatch, but every
  localize probe is blocked by shared SourceBlock ownership.

next:
  slot-level/source-run ownership
  or descriptor storage-locality policy
  or unified depot drain/localize criteria

do not:
  move whole shared SourceBlocks as the next ElasticCapacity behavior
```

SourceRunLocalityDryRun-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-runlocalitydryrun-
  desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
  routebytes16-storageowner16-ownersourcel2-frontcachepacked-
  sourceblockpacked-source64-route16k-run512

full10k run-1:
  42.733M ops/s
  225,168 KB peak RSS
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0

counters:
  elastic_source_run_locality_probe             = 79,485
  elastic_source_run_locality_storage_mismatch  = 79,485
  elastic_source_run_locality_run_miss          = 79,485
  elastic_source_run_locality_class_mismatch    = 0
  elastic_source_run_locality_slot_match        = 79,485
  elastic_source_run_locality_would_rehome_slot = 79,485

read:
  The current source-depot lane has no source-run metadata, so run_miss is
  expected.  However, every probed transfer object can be physically identified
  as a block/offset/bytes slot.  This supports a future slot-level ownership or
  storage-locality behavior.  It does not justify whole-SourceBlock movement.
```

SourceRunMetadataOnDepot-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-
  desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
  routebytes16-storageowner16-ownersourcel2-frontcachepacked-
  sourceblockpacked-source64-route16k-run4096

mode:
  metadata-only prerequisite/control
  HZ6_ELASTIC_DEPOT_SOURCE_RUN_META_L1=1
  HZ6_SOURCE_RUN_REUSE_L1 remains off

full10k run-1:
  42.148M ops/s
  230,032 KB peak RSS
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0

counters:
  elastic_source_run_locality_probe             = 79,485
  elastic_source_run_locality_storage_mismatch  = 79,485
  elastic_source_run_locality_run_miss          = 0
  elastic_source_run_locality_class_mismatch    = 0
  elastic_source_run_locality_slot_match        = 79,485
  elastic_source_run_locality_would_rehome_slot = 79,485

main-warmup depot metadata:
  elastic_depot_run_meta_init                   = 9,939
  elastic_depot_run_meta_mark                   = 159,024
  elastic_depot_run_meta_class_mismatch         = 0
  elastic_depot_run_meta_slot_misaligned        = 0
  elastic_depot_run_meta_too_many_slots         = 0
  elastic_depot_run_meta_used_count_mismatch    = 0

read:
  Depot SourceBlocks now carry source-run metadata, so the previous run_miss
  blocker is removed.  The remaining slot-level locality signal is still
  storage-owner mismatch with perfect physical slot identification.  Next
  should be SlotOwnerLocalityDryRun-L1, not whole-SourceBlock movement and not
  source-run reuse behavior inside the depot lane.
```

SlotOwnerLocalityDryRun-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerdryrun-
  desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
  routebytes16-storageowner16-ownersourcel2-frontcachepacked-
  sourceblockpacked-source64-route16k-run4096

mode:
  diagnostic-only A0 projection
  no descriptor owner mutation beyond existing transfer activation
  no route rehome
  no SourceBlock storage-owner movement

full10k run-1:
  43.174M ops/s
  230,020 KB peak RSS
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0

counters:
  elastic_slot_owner_locality_probe              = 79,485
  elastic_slot_owner_locality_storage_mismatch   = 79,485
  elastic_slot_owner_locality_run_miss           = 0
  elastic_slot_owner_locality_class_mismatch     = 0
  elastic_slot_owner_locality_slot_match         = 79,485
  elastic_slot_owner_locality_owner_match        = 79,485
  elastic_slot_owner_locality_owner_mismatch     = 0
  elastic_slot_owner_locality_would_set_owner    = 79,485
  elastic_slot_owner_locality_would_hit_owner    = 79,485

read:
  Every probed transfer object is still a SourceBlock storage-owner mismatch,
  but every one is a source-run slot match whose descriptor owner is current
  after transfer activation.  This supports sparse per-slot owner/locality
  metadata as the next behavior candidate.  Keep A0 as evidence only.
```

SlotOwnerSparseMeta-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownersparse-
  desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
  routebytes16-storageowner16-ownersourcel2-frontcachepacked-
  sourceblockpacked-source64-route16k-run4096

mode:
  diagnostic side-metadata feasibility
  no allocation/free behavior change
  no route rehome

full10k run-1:
  43.039M ops/s
  233,124 KB peak RSS
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0

counters:
  elastic_slot_owner_sparse_lookup          = 79,485
  elastic_slot_owner_sparse_miss            = 79,485
  elastic_slot_owner_sparse_insert          = 79,485
  elastic_slot_owner_sparse_hit             = 0
  elastic_slot_owner_sparse_update          = 0
  elastic_slot_owner_sparse_collision       = 62,673
  elastic_slot_owner_sparse_full            = 0

read:
  The sparse side table has enough capacity for observed transfer slots, but
  this Larson slice inserts each observed slot once and does not demonstrate a
  same-slot hit path.  Keep it as metadata feasibility evidence.  A future
  behavior must consume this metadata to avoid storage-owner mismatch work or
  guide owner-local lookup; otherwise it is only an RSS-cost side table.
```

Next after SlotOwnerSparseMeta-L1:

```text
1. DescriptorDepotOwnerDirectFastPath-L1
   Depot descriptors should read/write the shared depot owner table before
   OwnerSourceSideMeta-L2 storage lookup.  This is the lowest-risk work reducer:
   no sparse table, no route rehome, no SourceBlock owner movement, and no new
   production diagnostic counter.

2. SlotOwnerConsumerDryRun-L1
   If the direct depot fast path does not close the source-depot speed gap,
   consume sparse slot-owner metadata in diagnostic-only downstream owner checks.
   Required evidence: would_skip_l2 > 0 and false_positive = 0.

3. SlotOwnerLogicalOwnerFastPath-L1
   Behavior only after the consumer dry-run proves positive hits.  The sparse
   entry may prove a logical current-owner match, but it must not replace the
   descriptor storage-owner contract.
```

DescriptorDepotOwnerDirectFastPath-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-
  desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
  routebytes16-storageowner16-ownersourcel2-frontcachepacked-
  sourceblockpacked-source64-route16k-run512

main10k non-diagnostic run-1:
  45.258M ops/s
  224,600 KB peak RSS
  safety clean

worker10k guard run-1:
  47.481M ops/s
  214,284 KB peak RSS
  safety clean

diagnostic A/B:
  source-depot control:
    42.121M / 224,608 KB
    owner_source_side_meta_l2_lookup = 1,547,776,055

  depot-owner direct:
    42.946M / 227,748 KB
    owner_source_side_meta_l2_lookup = 489,480,577

read:
  Direct depot owner get/set removes about 68% of the L2 lookup pressure in the
  source-depot lane and improves throughput without adding sparse side-table
  RSS.  Keep as behavior candidate-watch.  Remaining L2 lookups imply the next
  pressure is outside depot descriptor owner table access.

repeat/guard closeout:
  main10k repeat-3:
    46.273M ops/s
    224,612 KB peak RSS
    safety clean

  guards:
    main1k    58.318M / 92,068 KB, safety clean
    worker1k  57.710M / 91,784 KB, safety clean
    main4k    47.912M / 139,052 KB, safety clean
    worker4k  52.784M / 132,804 KB, safety clean
    worker10k 42.459M / 214,292 KB, safety clean

decision:
  Upgrade to ElasticCapacity candidate-watch for the source-depot family.  Not
  broad default/promotion yet; use it as the current best source-depot
  speed/RSS shape while deciding whether SlotOwnerConsumerDryRun-L1 can remove
  the remaining owner-path cost.
```

SlotOwnerConsumerDryRun-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-
  slotownerconsumerdryrun-desc16k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096

mode:
  diagnostic-only consumer projection
  no allocation/free behavior change
  no route rehome
  no storage-owner override

full10k run-1:
  36.691M ops/s
  233,556 KB peak RSS
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0
    remote_free_transfer_fail=0

counters:
  elastic_slot_owner_consumer_probe            = 687,695,410
  elastic_slot_owner_consumer_hit              = 687,536,440
  elastic_slot_owner_consumer_miss             = 158,970
  elastic_slot_owner_consumer_owner_match      = 687,536,440
  elastic_slot_owner_consumer_owner_mismatch   = 0
  elastic_slot_owner_consumer_stale_generation = 0
  elastic_slot_owner_consumer_false_positive   = 0
  elastic_slot_owner_consumer_would_skip_l2    = 687,536,440
  elastic_slot_owner_consumer_fallback         = 158,970
  owner_source_side_meta_l2_lookup             = 418,621,565

read:
  The sparse slot-owner table is a real downstream consumer candidate, not only
  producer-side metadata.  Generation-checked sparse hits would skip a very
  large number of owner-equality heavy-path checks, and the dry-run reports no
  false positives or stale-generation hits.  The diagnostic counter volume is
  intentionally heavy, so this is not a speed-ranking lane.  The next narrow
  behavior candidate is SlotOwnerLogicalOwnerFastPath-L1: on a sparse
  generation-checked owner match, answer logical owner equality directly;
  miss/stale/mismatch must fall back to the existing descriptor owner path.
```

SlotOwnerLogicalOwnerFastPath-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerlogical-
  desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-
  routebytes16-storageowner16-ownersourcel2-frontcachepacked-
  sourceblockpacked-source64-route16k-run4096

mode:
  behavior no-go/control
  positive logical-owner match only
  no negative proof
  no storage-owner override
  miss/stale/mismatch falls back to the existing descriptor owner path

full10k non-diagnostic run-1:
  38.494M ops/s
  239,484 KB peak RSS
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0
    remote_free_transfer_fail=0

full10k diagnostic run-1:
  38.275M ops/s
  233,568 KB peak RSS
  logical_probe=717,941,382
  logical_hit=717,746,510
  logical_miss=194,872
  stale_generation=0
  owner_mismatch=0
  fallback=194,872
  owner_source_side_meta_l2_lookup=366,129,578

read:
  Safety is good and the positive-match contract is valid, but the lane is a
  speed no-go versus DepotOwnerDirectFastPath-L1 (`46.273M / 224,612 KB`).
  The sparse probe is too broad when attempted at every owner_equal() entry.
  Do not promote.  Future owner-path work needs an admission gate that avoids
  sparse probing on cheap local/depot checks.
```

OwnerEqualCallsiteDryRun-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-
  ownerequalcallsite-dryrun-desc16k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512

mode:
  diagnostic-only
  no behavior change
  callsite attribution for hz6_allocator_descriptor_owner_equal()

smoke main1k:
  55.891M ops/s
  105,844 KB peak RSS
  owner_equal_site_free=56,824,387
  owner_equal_site_local_cache=56,818,739
  owner_equal_site_unknown=0
  safety clean

full10k:
  42.434M ops/s
  224,612 KB peak RSS
  owner_equal_site_free=424,522,978
  owner_equal_site_local_cache=424,449,034
  owner_equal_site_remote_free=0
  owner_equal_site_visible_lookup=0
  owner_equal_site_transfer_locality=0
  owner_equal_site_same_owner_fast=0
  owner_equal_site_unknown=0
  owner_source_side_meta_l2_lookup=483,610,117
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0
    remote_free_transfer_fail=0

read:
  This explains why SlotOwnerLogicalOwnerFastPath-L1 is a speed no-go in broad
  form: the owner-equality pressure is almost entirely the free/local-cache
  path, not remote/visible/transfer-locality.  The next owner-path experiment
  must use a cheaper state/source-depot predicate before sparse slot-owner
  probing, or target a narrower free/local-cache subpath.  Keep this as
  diagnostic evidence; do not use it as a speed-ranking lane.
```

FreeLocalCacheOwnerPredicate-L0 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-
  freelocalownerpredicate-dryrun-desc16k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512

mode:
  diagnostic-only
  no behavior change
  only observes HZ6_OWNER_EQUAL_SITE_FREE and
  HZ6_OWNER_EQUAL_SITE_LOCAL_CACHE

smoke main1k:
  56.124M ops/s
  105,856 KB peak RSS
  flc_owner_predicate_probe=113,649,970
  depot_descriptor=0
  local_descriptor=17,501,244
  foreign_descriptor=96,148,726
  source_block=113,649,970
  source_block_shared=3,650,677
  source_release=113,649,970
  safety clean

full10k:
  41.049M ops/s
  224,628 KB peak RSS
  flc_owner_predicate_probe=821,934,976
  site_free=411,004,460
  site_local_cache=410,930,516
  depot_descriptor=693,768,653
  local_descriptor=47,477,340
  foreign_descriptor=774,457,636
  no_source_block=0
  source_block=821,934,976
  source_block_active=821,934,976
  source_block_shared=698,397,445
  source_run_active=0
  source_release=821,934,976
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0
    remote_free_transfer_fail=0

read:
  This is a strong predicate witness.  The full10k free/local-cache owner_equal
  pressure is largely depot descriptor backed and foreign descriptor-storage
  backed; the source block is present for all observed calls.  The next narrow
  behavior may test a depot-descriptor-only owner equality shortcut.  Do not
  gate on source_run_active for this lane because it is zero in the observed
  source-depot shape.
```

DepotDescriptorOwnerEqualFastPath-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-
  depotownerequal-desc16k-front4k-thindesc-nobackptr-noroutebackptr-
  dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-
  frontcachepacked-sourceblockpacked-source64-route16k-run512

mode:
  behavior no-go/control
  only FREE/LOCAL_CACHE sites
  only depot descriptors
  non-depot and other sites fall back to the existing owner path

full10k non-diagnostic:
  40.531M ops/s
  227,708 KB peak RSS
  safety clean

full10k diagnostic:
  41.221M ops/s
  227,744 KB peak RSS
  depot_owner_equal_fastpath_probe=824,802,008
  depot_owner_equal_fastpath_hit=696,139,014
  depot_owner_equal_fastpath_miss=71,811
  depot_owner_equal_fastpath_fallback=128,591,183
  depot_owner_equal_fastpath_other_site=0
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0
    remote_free_transfer_fail=0

read:
  The shortcut is safe and the hit volume is real, but it is slower than
  DepotOwnerDirectFastPath-L1 (`46.273M / 224,612 KB`).  The existing
  descriptor_owner() depot-direct path already performs the same owner read
  before the L2 storage path; this lane only adds an earlier branch/predicate
  in the hot free/local-cache owner-equality path.  Do not promote.  Future
  owner-path work must remove a different expensive operation, not just move
  the depot owner read earlier.
```

UnifiedDepotDrainDryRun-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-
  depotownerdirect-depotdraindryrun-desc16k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096

mode:
  diagnostic-only
  behavior unchanged
  DepotRunMeta + DepotOwnerDirectFastPath + unified depot drain/localize
  projection

full10k diagnostic:
  43.101M ops/s
  235,420 KB peak RSS
  elastic_depot_drain_probe=79,485
  elastic_depot_drain_storage_match=0
  elastic_depot_drain_storage_mismatch=79,485
  elastic_depot_drain_run_match=79,485
  elastic_depot_drain_run_miss=0
  elastic_depot_drain_class_mismatch=0
  elastic_depot_drain_slot_mismatch=0
  elastic_depot_drain_ref_exclusive=0
  elastic_depot_drain_ref_shared=79,485
  elastic_depot_drain_owner_match=79,485
  elastic_depot_drain_owner_mismatch=0
  elastic_depot_drain_would_slot_localize=79,485
  elastic_depot_drain_would_keep_shared=79,485
  elastic_depot_drain_would_block_whole_localize=79,485
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0
    remote_free_transfer_fail=0

read:
  This closes the unified-drain diagnostic gap.  Whole-SourceBlock localize is
  still blocked because every probed depot block is shared, but every probed
  transfer object is run-matched, owner-matched after activation, and
  slot-localizable.  The next behavior should localize per-slot storage/owner
  state for depot transfer reuse.  Do not move whole SourceBlocks and do not
  add another owner_equal shortcut.
```

DepotSlotLocalize-L1 result:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-
  depotownerdirect-depotslotlocalize-desc16k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096

mode:
  behavior no-go/control
  write slot-local storage allocator into the sparse slot-owner table during
  depot transfer reuse
  let owner_source_side_meta_storage() return the slot-local storage owner
  before the SourceBlock-level storage owner

full10k non-diagnostic:
  44.658M ops/s
  240,636 KB peak RSS
  route_invalid=125
  remote_free_transfer_fail=125

full10k diagnostic:
  36.191M ops/s
  240,656 KB peak RSS
  elastic_depot_slot_localize_attempt=30,733,367
  elastic_depot_slot_localize_success=30,733,367
  elastic_depot_slot_localize_storage_hit=401,643,367
  elastic_depot_slot_localize_storage_miss=4,465,070
  elastic_depot_slot_localize_storage_stale=0
  route_invalid=125
  remote_free_transfer_fail=125

read:
  The mechanism is active and heavily hit, but the storage override is not
  safe enough as a general owner_source side-meta replacement.  A small number
  of stale/ownership mismatches reaches real free/remote-transfer behavior.
  Keep as no-go/control.  The next slot-local design must remain scoped to
  transfer reuse or another descriptor-local fail-closed path; do not use it
  as broad SourceBlock storage-owner override.
```

DepotSlotTransferScoped-L1:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-
  depotownerdirect-depotslottransfer-desc16k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096

mode:
  safe transfer-scoped control/evidence
  record depot transfer-reuse slots in the sparse slot-owner table
  do not let owner_source_side_meta_storage() return slot-local storage

full10k non-diagnostic:
  41.596M ops/s
  route_invalid=0
  remote_free_transfer_fail=0

full10k diagnostic:
  42.801M ops/s
  elastic_depot_slot_localize_attempt=79,485
  elastic_depot_slot_localize_success=79,485
  elastic_depot_slot_localize_ineligible=515
  elastic_depot_slot_localize_storage_hit=0
  elastic_slot_owner_sparse_insert=79,485
  elastic_slot_owner_sparse_hit=79,485
  elastic_slot_owner_sparse_owner_match=79,485
  route_invalid=0
  remote_free_transfer_fail=0

read:
  Sparse slot-owner recording is safe when scoped to transfer reuse.  This
  confirms the DepotSlotLocalize-L1 failures came from consuming the sparse
  slot table as a broad storage-owner override, not from the slot-local witness
  itself.  The next behavior must consume this witness only inside a
  fail-closed descriptor-local or transfer-local decision.
```

DepotDescriptorRehomeDryRun-L1:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-
  depotownerdirect-depotdescrehomedry-desc16k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096

mode:
  diagnostic-only descriptor clone/rehome witness
  run during transfer reuse after activation
  no route unregister/register behavior
  no depot descriptor release behavior

full10k diagnostic:
  43.040M ops/s
  transfer_reuse_hit=80,000
  elastic_depot_descriptor_rehome_probe=80,000
  elastic_depot_descriptor_rehome_depot_descriptor=71,811
  elastic_depot_descriptor_rehome_already_local=0
  elastic_depot_descriptor_rehome_run_match=71,811
  elastic_depot_descriptor_rehome_run_mismatch=0
  elastic_depot_descriptor_rehome_local_descriptor_available=71,811
  elastic_depot_descriptor_rehome_no_local_descriptor=0
  elastic_depot_descriptor_rehome_would_rehome=71,811
  route_invalid=0
  remote_free_transfer_fail=0

read:
  Strong witness for descriptor-local rehome at transfer reuse.  Eligible depot
  descriptors are run-matched and local descriptor capacity is available in
  the consumer allocator.  The next behavior candidate should clone/rehome the
  descriptor with a fail-closed route exact replacement and rollback path; it
  should not revive broad slot-local storage-owner override.
```

RouteExactDescriptorReplace / DepotDescriptorRouteReplaceDryRun next:

```text
why:
  DepotDescriptorRehomeDryRun proved local descriptor capacity and slot/run
  eligibility.  It did not prove route-swap safety.  The next step must verify
  exact route replacement before changing descriptor ownership.

preferred order:
  1. shadow-check old route for ptr
  2. require route.descriptor == old depot descriptor
  3. require route.generation == old descriptor generation
  4. require route bytes/front/class match
  5. prove current allocator can commit an exact route for the same live object
  6. only after route commit, detach/reset old depot descriptor while keeping
     the source slot alive

generation:
  preserve generation during descriptor rehome.  This is descriptor storage
  relocation for the same live object, not a new object lifetime.

hard no-go:
  route_invalid / route_miss / route_register_fail / remote_free_transfer_fail
  become nonzero
  old route points at a reset depot descriptor
  rollback cannot restore old route + old descriptor
  source slot/ref-count is released during descriptor rehome
```

DepotDescriptorRouteReplaceDryRun-L1:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-
  depotownerdirect-depotroutereplacedry-desc16k-front4k-thindesc-nobackptr-
  noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096

mode:
  diagnostic-only route replacement preflight
  run during transfer reuse after activation
  no route register/unregister behavior
  no depot descriptor release behavior

full10k diagnostic:
  41.332M ops/s
  transfer_reuse_hit=80,000
  elastic_depot_route_replace_probe=80,000
  elastic_depot_route_replace_depot_descriptor=71,811
  elastic_depot_route_replace_run_match=71,811
  elastic_depot_route_replace_run_mismatch=0
  elastic_depot_route_replace_origin_missing=0
  elastic_depot_route_replace_old_route_found=71,811
  elastic_depot_route_replace_old_route_missing=0
  elastic_depot_route_replace_old_route_invalid=0
  descriptor/generation/front/class matches=71,811
  elastic_depot_route_replace_current_route_same=71,811
  elastic_depot_route_replace_current_route_conflict=0
  elastic_depot_route_replace_would_commit=71,811
  elastic_depot_route_replace_would_rollback=8,189
  route_invalid=0
  remote_free_transfer_fail=0

read:
  The 8,189 rollback rows are non-depot transfer objects.  Every eligible depot
  descriptor has a matching old route and a same-descriptor current exact route.
  This means the next behavior primitive should be in-place exact-route
  descriptor replacement in the current allocator, not an unregister-first
  rehome path.

RouteExactDescriptorReplace-L0:
  `hz6_route_replace_exact_descriptor()` and
  `hz6_allocator_route_replace_exact_descriptor()` are now available as the
  fail-closed primitive.  They only update an active exact route when base,
  bytes, front, class, old descriptor, and old generation match.  The public
  allocator wrapper also refreshes the shared route directory / owner-locality
  index after a successful local table replacement.  Route smoke verifies the
  happy path and old descriptor / generation mismatch rejection.

DepotDescriptorRehome-L1:
  The first behavior lane uses the route replacement primitive at transfer
  reuse.  It prepares a consumer-local descriptor for the same source-block
  slot, preserves generation, replaces the current exact/shared route entry,
  then detaches the old depot descriptor without releasing the source slot.

  Full10k non-diagnostic:
    44.853M ops/s
    route_invalid=0
    remote_free_transfer_fail=0

  Full10k diagnostic:
    43.616M ops/s
    l1_attempt=80,000
    l1_success=71,811
    l1_ineligible=8,189
    no_local_descriptor=0
    prepare_fail=0
    route_replace_fail=0
    detach_fail=0
    rollback=0
    route_invalid=0
    remote_free_transfer_fail=0

  Read:
    The behavior is safety-clean and consumes the dry-run witness.  It is not a
    promotion yet because descriptor usage rises: diagnostic final
    descriptor_used=77,883 after rehoming.  The next useful work is descriptor
    retention / bounding for rehomed slots, not another owner-route shortcut.

DepotDescriptorRehomeCapFree-L1:
  Control lane that combines DepotDescriptorRehome-L1 with
  `HZ6_FRONTCACHE_CAP_ON_FREE=1`.

  Full10k non-diagnostic:
    43.602M ops/s
    route_invalid=0
    remote_free_transfer_fail=0

  Full10k diagnostic:
    42.119M ops/s
    l1_attempt=80,000
    l1_success=71,811
    route_replace_fail=0
    detach_fail=0
    rollback=0
    descriptor_used=77,883
    active_descriptors=72,327
    local_free_descriptors=5,556
    frontcache_total=6,072

  Read:
    This is a no-go for the simple frontcache-cap explanation.  Safety remains
    clean, but descriptor usage does not fall and throughput regresses from
    DepotDescriptorRehome-L1.  The retention watch item is dominated by live
    Larson objects materialized into consumer-local descriptors, not by cold
    frontcache backlog.  Do not pursue broad cap-on-free as the next control
    plane for this track.

DepotDescriptorRehomeBudget2048-L1:
  Candidate-control lane that keeps DepotDescriptorRehome-L1 but limits
  consumer-local descriptor rehome commits to 2,048 per allocator.

  Full10k non-diagnostic:
    44.919M ops/s
    route_invalid=0
    remote_free_transfer_fail=0

  Full10k diagnostic:
    40.961M ops/s
    l1_attempt=80,000
    l1_success=30,483
    l1_ineligible=8,189
    l1_budget_denied=41,328
    route_replace_fail=0
    detach_fail=0
    rollback=0
    descriptor_used=36,539
    active_descriptors=33,528
    descriptor_live_max=2,448

  Read:
    This is the strongest follow-up after full DepotDescriptorRehome-L1.
    Budgeting removes more than half of the consumer-local materialization
    while preserving the full-rehome speed signal in the non-diagnostic run.
    Unlike capfree, it attacks the actual retention source.  Next validation
    should be repeat/guard on budgeted rehome before promoting it.
```

## Design And Diagnostic Reference

```text
Detailed diagnostic proof, design target, safety contract, implementation order,
and acceptance notes moved to:
  HZ6_ELASTIC_CAPACITY_DESIGN_REFERENCE.md
```
