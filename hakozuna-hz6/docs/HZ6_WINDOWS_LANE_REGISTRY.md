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

## Desired Manifest Shape

A future `Get-Hz6WinCapacityLaneRegistry` helper should return records like:

```powershell
@{
  Name = "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512"
  Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_source16k_route192k_run512"
  Status = "selected sibling"
  Family = "larson"
  Tags = @("owner-locality", "thindesc", "no-backptr", "source16k", "route192k", "run512", "lowest-rss")
  ExtraFlags = @(
    "/DHZ6_OWNER_LOCALITY_FAST_L1=1",
    "/DHZ6_THIN_DESCRIPTOR_L1=1",
    "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
    "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
    "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
    "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
    "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)"
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
