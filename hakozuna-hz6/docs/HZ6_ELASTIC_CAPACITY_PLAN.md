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

## Current Lane Status

```text
Selected minimum-RSS sibling:
  source10k combined packed lane

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

ElasticDescriptorRouteOverflow-L1:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescroute-desc16k-front4k-thindesc-
  nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
  ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route16k-run512

shape:
  descriptor local cap = 16k
  route local cap      = 16k
  shared descriptor depot
  shared exact route overflow
  shared SourceBlock invalid-envelope overflow
  source/frontcache remain source10k-control sized

full10k run-1:
  33.184M ops/s
  246,824 KB peak RSS
  main-warmup descriptor depot alloc = 143,664
  safety clean:
    route_invalid=0
    route_miss=0
    route_register_fail=0
    descriptor_exhausted=0
    source_block_exhausted=0
    alloc_fail=0

read:
  Descriptor overflow works and gives a major RSS reduction. The speed drop
  shows that depot descriptors need a first-class storage/locality contract;
  descriptor storage lookup currently misses for many depot descriptors and
  falls back to expensive scans. Keep this as mechanism evidence, not the
  promoted lane.
```

Depot storage fast path:

```text
change:
  depot descriptors resolve their storage owner through SourceBlock side
  metadata instead of scanning visible allocator descriptor tables.

diagnostic full10k:
  40.951M ops/s
  246,912 KB peak RSS
  descriptor_storage_miss = 0
  descriptor_storage_hit  = 409,709,924
  safety clean

non-diagnostic full10k:
  43.843M ops/s
  246,888 KB peak RSS

non-diagnostic repeat-3 guards:
  main10k   = 42.770M / 246,880 KB
  worker10k = 46.060M / 237,056 KB
  main1k    = 56.686M / 115,460 KB
  worker1k  = 57.326M / 115,272 KB
  main4k    = 55.410M / 162,072 KB
  worker4k  = 51.050M / 156,040 KB
  alloc_fail / descriptor_exhausted / route_register_fail /
  source_block_exhausted = 0 on all runs

read:
  The original speed loss was storage-owner lookup locality, not the descriptor
  depot contract itself. ElasticDescriptorRouteOverflow-L1 is now the strongest
  ElasticCapacity RSS shape so far. It is still slightly below the selected
  source10k throughput class, so treat it as candidate-watch/evidence rather
  than broad promotion. The next missing piece is shared source-block overflow
  or a unified overflow drain/localization contract, not another route or
  descriptor-only knob.
```

ElasticDescriptorSourceRouteOverflow-L1:

```text
lane:
  ownerlocalityfast-rsscap-2-elasticdescsource-route-desc16k-front4k-
  thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-
  storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-
  source64-route16k-run512

shape:
  descriptor local cap = 16k
  route local cap      = 16k
  source block local cap = 64
  shared descriptor depot
  shared SourceBlock depot
  shared exact route overflow
  shared SourceBlock invalid-envelope overflow

diagnostic smoke main1k:
  56.362M ops/s
  106,048 KB peak RSS
  source_block_exhausted = 0
  alloc_fail / descriptor_exhausted / route_register_fail = 0

diagnostic full10k:
  41.516M ops/s
  225,212 KB peak RSS
  main-warmup local source_block_used = 64
  main-warmup source_block_active_max = 10,003
  source_block_exhausted = 0
  descriptor_storage_miss = 0
  alloc_fail / descriptor_exhausted / route_register_fail = 0

non-diagnostic full10k:
  41.733M ops/s
  227,852 KB peak RSS
  safety clean

read:
  SourceBlock depot behavior works as a subcomponent: a tiny local SourceBlock
  table can survive the full main-warmup source spike while cutting RSS below
  ElasticDescriptorRouteOverflow-L1. The tradeoff is throughput: it is slower
  than the current ElasticDescriptorRouteOverflow candidate-watch. Keep this as
  lower-RSS source-depot evidence/control, not broad promotion.

caution:
  This is not a SourceBlock-only solution. It depends on the already-enabled
  descriptor depot and route overflow. Before promotion, the source depot needs
  explicit depot accounting and an owner-safe drain/localize contract so depot
  blocks do not remain merely as a shared static escape hatch.

depot accounting follow-up:
  SourceBlock depot counters are now exposed as
    elastic_source_block_overflow_alloc
    elastic_source_block_overflow_release
    elastic_source_block_overflow_exhausted
  and RSS attribution reports source_block_depot_bytes separately. Smoke shows
  the counters on main-warmup pressure rows and adds the shared depot cost
  (`2,097,152` bytes for the current 16k SourceBlock depot) to static table
  accounting.

localize dry-run:
  Whole-SourceBlock localize was tested as diagnostic-only. The dry-run counts
  transfer-reused depot blocks whose storage owner differs from the current
  allocator and only marks a block localizable if it is exclusive
  (`ref_count <= 1`) and a local SourceBlock slot is free.

  smoke:
    elastic_source_block_localize_probe > 0
    elastic_source_block_localize_storage_mismatch == probe
    elastic_source_block_localize_would_move = 0
    elastic_source_block_localize_block_shared == probe

  read:
    The source-depot pressure is shared-run pressure, not exclusive block
    pressure. Localizing whole SourceBlock metadata would be unsafe or mostly
    ineffective because the same physical run still owns multiple live/cached
    slots. Do not implement whole-block localize behavior from this evidence.
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
   Route overflow is now implemented as ElasticRouteOverflow-L1 and kept as a
   component/control. The next metadata class should be descriptor overflow,
   because descriptor warmup pressure is the largest remaining static table
   contributor. Source-block overflow should follow after descriptor ownership
   is safe.

4. Add source-block overflow only as part of the same contract, not as an
   isolated fix.
```

## Next Implementation Order

```text
1. DescriptorOverflow-L0 compatibility audit:
   Verify whether descriptors can live outside the allocator-local descriptor
   array without breaking owner16 side metadata, owner-source side metadata,
   route generation, or descriptor storage lookup.

2. DescriptorOverflow-L1 narrow behavior:
   local descriptor cap remains small
   overflow descriptors live in a shared descriptor depot
   descriptor ownership side metadata has a depot-compatible storage path
   route exact/envelope overflow remains enabled

   Current status:
     implemented as ElasticDescriptorRouteOverflow-L1
     RSS win is strong; depot storage fast path recovers source10k-class
     throughput in non-diagnostic full10k while keeping the lower RSS class.
     Repeat-3 guards are safety-clean; keep as candidate-watch/evidence while
     designing source-block overflow / unified ElasticCapacity drain.

3. SourceBlockOverflow-L1:
   implemented narrowly as ElasticDescriptorSourceRouteOverflow-L1.
   The source block metadata can live in a shared depot, and invalid envelope
   registration remains route-overflow compatible. Current evidence proves the
   warmup spike can be absorbed with local source cap 64, but the lane is a
   lower-RSS component/control rather than the promoted shape.

4. SourceRunMetadataOnDepot-L1:
   implemented as metadata-only prerequisite/control. It removes the prior
   source-run metadata gap in the source-depot lane without enabling source-run
   reuse behavior. This prepares the next slot-owner locality dry-run.

5. SlotOwnerLocalityDryRun-L1:
   implemented as diagnostic-only evidence. It shows the source-depot transfer
   objects can be represented as worker-local slots without whole-block
   movement. The next behavior should be sparse per-slot owner/locality state,
   not route rehome or SourceBlock storage-owner mutation yet.

6. SlotOwnerSparseMeta-L1:
   implemented as diagnostic side metadata. It proves observed transfer slots
   fit into a sparse table without overflow, but does not show same-slot hits in
   this workload. The next behavior must use the metadata to remove real work.

7. Unified ElasticCapacity-L1:
   descriptor + route + source overflow in one lane
   full10k target: keep route-overflow RSS class while recovering source10k
   throughput
   next missing piece: sparse slot-owner locality accounting and owner-safe
   drain/localize
```

Descriptor overflow caution:

```text
The current owner16 side metadata indexes `descriptor_side_owner16` by
descriptor pointer within an allocator-local descriptor array. A descriptor
from a global/depot array will not have a valid local index unless the owner
side metadata path is made depot-aware. Therefore descriptor overflow must not
simply return a pointer to an unrelated global descriptor array without a
matching storage-owner contract.
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
