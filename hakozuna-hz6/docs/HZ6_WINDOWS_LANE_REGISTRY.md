# HZ6 Windows Lane Registry Cleanup

HZ6 Windows lanes are currently registered in two places:

```text
win/run_win_hz6_capacity_matrix.ps1:
  lane name -> executable suffix

win/bench_app_like_allocator_build_common.ps1:
  lane name -> executable suffix + compiler flags
```

The maps are currently aligned, but they are large enough that future lane
work should avoid more copy/paste growth.

## Current Rule

Until a shared manifest exists, add or modify a lane in the same commit in both
files:

```text
1. win/bench_app_like_allocator_build_common.ps1
   add capacity flags and lane map entry

2. win/run_win_hz6_capacity_matrix.ps1
   add matching suffix entry

3. win/run_win_hz6_selected_family.ps1
   add only if the lane is selected, selected sibling, or a narrow diagnostic
   preset

4. hakozuna-hz6/docs/HZ6_LANE_GUIDE.md
   record lane status:
     selected / selected sibling / control / evidence / pressure / no-go
```

Use this checker after lane-map edits:

```powershell
powershell -ExecutionPolicy Bypass -File win/check_win_hz6_capacity_lanes.ps1
```

It compares the capacity lane keys in the build-common map and the capacity
matrix suffix map. It does not run benchmarks; it only catches lane-key drift.

## Current Selected-Family Elastic Rows

The Elastic DFTLC family currently has two selected-family rows:

```text
front4k:
  status: speed-balance control
  lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512

front1k:
  status: selected lower-RSS sibling
  lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front1k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512

front2k:
  status: evidence/control only
  lane:
    ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front2k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512
```

`win/run_win_hz6_selected_family.ps1 -LarsonElasticLowRssSelected` runs both
rows together.  Front2k remains evidence/control only and is intentionally not
part of the selected-family preset.

Use `-LarsonElasticFrontcacheGuard` when front2k should be included in the
boundary check across main/worker 1k/4k/10k.

## Desired Manifest Shape

A future `Get-Hz6WinCapacityLaneRegistry` helper should return records like:

```powershell
@{
  Name = "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512"
  Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_s16_r192_run512"
  Status = "control"
  Family = "larson"
  Tags = @("owner-locality", "thindesc", "no-backptr", "no-route-backptr", "dir192k", "routepacked", "routebytes16", "source16k", "route192k", "run512", "route-entry-control")
  ExtraFlags = @(
    "/DHZ6_OWNER_LOCALITY_FAST_L1=1",
    "/DHZ6_THIN_DESCRIPTOR_L1=1",
    "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
    "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
    "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
    "/DHZ6_ROUTE_PACKED_META_L1=1",
    "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
    "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
    "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
    "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
    "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)"
    "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
  )
}

@{
  Name = "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512"
  Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_s16_r192_run512"
  Status = "selected sibling"
  Family = "larson"
  Tags = @("owner-locality", "thindesc", "no-backptr", "no-route-backptr", "dir192k", "routepacked", "routebytes16", "storageowner16", "ownersourcel2", "source16k", "route192k", "run512", "lowest-rss-balance")
  ExtraFlags = @(
    "/DHZ6_OWNER_LOCALITY_FAST_L1=1",
    "/DHZ6_THIN_DESCRIPTOR_L1=1",
    "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
    "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
    "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
    "/DHZ6_ROUTE_PACKED_META_L1=1",
    "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
    "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
    "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
    "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
    "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
    "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
    "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
    "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
  )
}

@{
  Name = "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512"
  Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_s16_r192_run512"
  Status = "selected sibling"
  Family = "larson"
  Tags = @("owner-locality", "thindesc", "no-backptr", "no-route-backptr", "dir192k", "routepacked", "routebytes16", "storageowner16", "ownersourcel2", "frontcachepacked", "source16k", "route192k", "run512", "lowest-rss-candidate")
  ExtraFlags = @(
    "/DHZ6_OWNER_LOCALITY_FAST_L1=1",
    "/DHZ6_THIN_DESCRIPTOR_L1=1",
    "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
    "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
    "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
    "/DHZ6_ROUTE_PACKED_META_L1=1",
    "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
    "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
    "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
    "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
    "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
    "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
    "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
    "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
    "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
  )
}

@{
  Name = "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512"
  Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_sbp_s16_r192_run512"
  Status = "selected-sibling candidate"
  Family = "larson"
  Tags = @("owner-locality", "thindesc", "no-backptr", "no-route-backptr", "dir192k", "routepacked", "routebytes16", "storageowner16", "ownersourcel2", "sourceblockpacked", "source16k", "route192k", "run512", "lowest-rss-candidate")
  ExtraFlags = @(
    "/DHZ6_OWNER_LOCALITY_FAST_L1=1",
    "/DHZ6_THIN_DESCRIPTOR_L1=1",
    "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
    "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
    "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
    "/DHZ6_ROUTE_PACKED_META_L1=1",
    "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
    "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
    "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
    "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
    "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
    "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
    "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
    "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
    "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
  )
}

@{
  Name = "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512"
  Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s16_r192_run512"
  Status = "minimum-RSS candidate"
  Family = "larson"
  Tags = @("owner-locality", "thindesc", "no-backptr", "no-route-backptr", "dir192k", "routepacked", "routebytes16", "storageowner16", "ownersourcel2", "frontcachepacked", "sourceblockpacked", "source16k", "route192k", "run512", "minimum-rss-candidate")
  ExtraFlags = @(
    "/DHZ6_OWNER_LOCALITY_FAST_L1=1",
    "/DHZ6_THIN_DESCRIPTOR_L1=1",
    "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
    "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
    "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
    "/DHZ6_ROUTE_PACKED_META_L1=1",
    "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
    "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
    "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
    "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
    "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
    "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
    "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
    "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
    "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
    "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
  )
}

@{
  Name = "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512"
  Suffix = "_ownerloc_rss2_d160_f4_thin_nb_so16_nrb_d192_rp_s16_r192_run512"
  Status = "evidence"
  Family = "larson"
  Tags = @("owner-locality", "thindesc", "no-backptr", "storageowner16", "no-route-backptr", "dir192k", "routepacked", "source16k", "route192k", "run512", "rss-first", "descriptor-layout")
  ExtraFlags = @(
    "/DHZ6_OWNER_LOCALITY_FAST_L1=1",
    "/DHZ6_THIN_DESCRIPTOR_L1=1",
    "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
    "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
    "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
    "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
    "/DHZ6_ROUTE_PACKED_META_L1=1",
    "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
    "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
    "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
    "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
    "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
  )
}

@{
  Name = "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512"
  Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_sideowner16_source16k_route192k_run512"
  Status = "no-go"
  Family = "larson"
  Tags = @("owner-locality", "thindesc", "no-backptr", "sideowner16", "source16k", "route192k", "run512", "descriptor-layout")
  ExtraFlags = @(
    "/DHZ6_OWNER_LOCALITY_FAST_L1=1",
    "/DHZ6_THIN_DESCRIPTOR_L1=1",
    "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
    "/DHZ6_DESCRIPTOR_SIDE_OWNER16_L1=1",
    "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
    "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
    "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
    "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
    "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
  )
}

@{
  Name = "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512"
  Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_dir192k_source16k_route192k_run512"
  Status = "control"
  Family = "larson"
  Tags = @("owner-locality", "thindesc", "no-backptr", "dir192k", "source16k", "route192k", "run512", "rss", "lowest-rss")
  ExtraFlags = @(
    "/DHZ6_OWNER_LOCALITY_FAST_L1=1",
    "/DHZ6_THIN_DESCRIPTOR_L1=1",
    "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
    "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
    "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
    "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
    "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
    "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
    "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
  )
}
```

Then:

```text
build script:
  consumes Name / Suffix / ExtraFlags

capacity matrix:
  consumes Name / Suffix

selected-family runner:
  consumes Status / Family / Tags or explicit preset lists
```

## Naming Rules

Keep lane names mechanical:

```text
<mechanism>-<capacity-shape>-<main caps>
```

Examples:

```text
mixedclean-front16k-sourcerun-desc17k-source2k-route17k
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024
largerlowrss-front8k-sourcerun-desc8k-route8k
```

Avoid:

```text
best
new
final
fastest
paper
```

Those are statuses, not lane names.

## Status Rules

```text
selected:
  clean row used in current HZ6 comparison tables

selected sibling:
  clean row with explicit speed/RSS tradeoff

control:
  stable comparison lane

evidence:
  mechanism explanation, not a selected row

pressure:
  deliberately capacity-stressed; not paper/default without caveat

no-go:
  frozen failed boundary
```

## Refactor Order

```text
Step 1:
  leave existing scripts working
  add this cleanup contract

Step 2:
  add a read-only registry helper that mirrors the existing lane map
  compare helper output against both existing maps

Step 3:
  switch run_win_hz6_capacity_matrix.ps1 to use the helper

Step 4:
  switch bench_app_like_allocator_build_common.ps1 to use the helper

Step 5:
  delete duplicated lane suffix maps only after script parity tests pass
```

Do not combine the registry refactor with allocator source changes or new
benchmark results.
