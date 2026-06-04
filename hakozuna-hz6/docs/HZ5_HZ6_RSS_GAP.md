# HZ5 / HZ6 RSS Gap

This note fixes the current HZ5-vs-HZ6 RSS read before the next HZ6
optimization pass. It uses existing Windows benchmark artifacts only.

## Short Read

```text
HZ5 is still the stronger RSS reference on Larson-style cross-owner stress.
HZ6 has caught up on throughput in the selected Larson lane, but its RSS is
still dominated by route / descriptor / source metadata tables.

HZ6 is already competitive or better on several low-RSS selected-family rows:
  random_mixed selected rows are around 5 MB peak
  larger_sizes selected rows are around 71 MB peak
  mixed_ws clean rows are around 111-140 MB peak

The next high-ROI HZ6 RSS work is not another static capacity trim.
It is owner-source-aware descriptor / route ownership metadata.
```

## Paper-Aligned Rows

These rows compare HZ5 policy rows with the closest HZ6 rows already recorded
in the repository.

| Family | HZ5 policy | HZ6 selected/control | Read |
| --- | ---: | ---: | --- |
| `random_mixed / medium` | `6.612M / 10,184 KB` | `42.408M / 4,964 KB` selected same-owner | HZ6 is clearly better on RSS and throughput after the same-owner selected lane work. |
| `Larson T16 main-warmup` | `47.485M / 183,180 KB` | `40.754M / 439,912 KB` OwnerSourceSideMeta-L2 | HZ6 L2 preserves the routebytes16 comparison-control throughput shape in same-run repeat-3 while cutting RSS, but still uses about 2.40x HZ5 RSS. |
| `Larson T16 worker-warmup` | `42.987M / 155,640 KB` | `40.787M / 439,740 KB` OwnerSourceSideMeta-L2 run=1 | HZ6 L2 does not break same-owner warmup and lowers RSS versus routebytes16, but HZ5 remains the stronger RSS reference. |

Sources:
- `docs/benchmarks/windows/paper/20260601_081924_paper_random_mixed_windows.md`
- `docs/benchmarks/windows/paper/20260601_140855_paper_larson_windows.md`
- `hakozuna-hz6/docs/HZ6_SELECTED_FAMILY_SUMMARY.md`

## HZ6 Larson RSS Ladder

The HZ6 Larson row has improved substantially, but the remaining gap is still
metadata dominated.

| HZ6 Larson lane | ops/s | peak KB | Read |
| --- | ---: | ---: | --- |
| `ownerlocalityfast-rsscap-2-desc160k` | 44.754M | 808,488 | Throughput/RSS balance baseline. |
| `ownerlocalityfast-rsscap-2-desc160k-front4k` | 45.092M | 716,324 | Lower-RSS sibling, similar throughput. |
| `...front4k-thindesc-source16k-route192k-run512` | 48.512M | 499,820 | Previous lowest-RSS sibling/control. |
| `...nobackptr-noroutebackptr-dir192k-source16k-route192k-run512` | 41.107M | 469,868 | Clean minimum-RSS isolation control. |
| `...routepacked-routebytes16-source16k-route192k-run512` | 40.750M | 449,128 | Superseded routebytes16 comparison control in same-run L2 repeat-3. |
| `...routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512` | 40.754M | 439,912 | Current selected lowest-RSS sibling. |
| `...storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512` | 42.024M | 444,520 | RSS-first evidence/control; lower RSS than selected, but too much throughput cost. |

## Design Consequence

Static capacity trimming is mostly closed:

```text
route table:
  route192k is the current clean lower bound for full Larson.
  route160k/run512 and route128k/run512 fail warmup.

descriptor table:
  desc158k is clean but only saves about 1.7 MB.
  desc156k and below fail warmup.

owner/shared directory:
  dir192k is clean.
  dir128k and dir96k create owner locality misses / full-table probes.
```

The remaining RSS path is layout / ownership, not blind capacity:

```text
1. Keep OwnerSourceSideMeta-L2 as the selected low-RSS sibling.
2. Keep routebytes16 as the comparison control under the selected L2 lane.
3. Keep storageowner16 as RSS-first evidence/control.
4. Reopen descriptor-side metadata only with owner-source awareness or a
   broader ownership representation.
5. Avoid allocator-local side-owner metadata; it already failed cross-owner safety.
6. Avoid another route/descriptor/directory static trim unless a new
   representation changes the pressure source.
```

## OwnerSourceSideMeta-L2 Result

Implementation:

```text
HZ6_OWNER_SOURCE_SIDE_META_L2=1
HZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1

Meaning:
  keep descriptor hot entries ownerless
  keep packed side owner in descriptor_side_owner16[]
  use descriptor->source_block->owner_source_storage_allocator as an O(1)
  descriptor storage hint before falling back to the StorageOwner16 scan
```

Lane:

```text
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-
ownersourcel2-source16k-route192k-run512
```

Larson T16 main-warmup full 10k, non-diagnostic, run=1:

```text
routebytes16 comparison control:
  43.911M ops/s
  449128 KB peak

storageowner16 + ownersourcel2:
  44.546M ops/s
  440364 KB peak
```

Larson T16 main-warmup full 10k, diagnostic dry-run comparison, run=1:

```text
storageowner16 + ownersourcel2 + dryrun:
  44.239M ops/s
  439936 KB peak
  owner_source_side_meta_l2_lookup = 1253200849
  owner_source_side_meta_l2_hit = 1253200849
  owner_source_side_meta_l2_miss_no_block = 0
  owner_source_side_meta_l2_miss_inactive = 0
  owner_source_side_meta_l2_storage_mismatch = 0

safety:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
  remote_free_transfer_fail = 0
  lifecycle_foreign_free_invalid = 0
  alloc_fail = 0
  descriptor_exhausted = 0
```

Read:

```text
OwnerSourceSideMeta-L2 is the first positive RSS-gap mechanism after
routebytes16. It keeps the StorageOwner16 RSS direction while avoiding the
scan-based hot read in the behavior lane.

OwnerSourceSideMeta-L2 is promoted to the current Larson lowest-RSS selected
sibling after repeat-3 and worker-warmup checks:
  main-warmup repeat-3:
    routebytes16 control = 40.750M / 449128 KB
    L2 = 40.754M / 439912 KB
  worker-warmup run=1:
    routebytes16 control = 40.126M / 448948 KB
    L2 = 40.787M / 439740 KB
  full diagnostic comparison has zero L2 mismatch
```

Next:

```text
Closeout:
  keep routebytes16 as the clean comparison control
  use storageowner16 + ownersourcel2 as the selected lowest-RSS Larson sibling
  rerun repeat-3 only when touching descriptor/source-block ownership again

Keep as no-go/control:
  plain StorageOwner16 scan-based owner reads
```

## Prior HZ6 Attack

```text
HZ6 OwnerSourceSideMeta-L1 dry-run

Goal:
  project the cost of making descriptor owner side metadata first-class and
  owner-source-aware, without paying StorageOwner16 lookup cost on every hot
  owner read.

Questions:
  can descriptor storage ownership be cached or indexed without becoming
  allocator-local?
  can the side-owner lookup use descriptor index / storage epoch instead of
  current allocator or route allocator?
  can we preserve routebytes16 throughput while approaching storageowner16 RSS?

Acceptance:
  safety counters remain zero
  projected owner-source lookup miss is zero
  projected hot lookup count is lower than StorageOwner16
  no fallback from owned-looking invalid pointers
```

## OwnerSourceSideMeta-L1 Dry-Run Result

Initial implementation:

```text
HZ6_OWNER_SOURCE_SIDE_META_DRYRUN=1
diagnostic-only
no side-owner behavior change
no StorageOwner16 behavior change
routebytes16 comparison-control hot path preserved
```

Lane:

```text
ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-
noroutebackptr-dir192k-routepacked-routebytes16-ownersourcedryrun-
source16k-route192k-run512
```

Larson T16 main-warmup full 10k, run=1:

```text
ops/s = 46.202M
peak  = 449164 KB

safety:
  route_invalid = 0
  route_miss = 0
  route_register_fail = 0
  remote_free_transfer_fail = 0
  lifecycle_foreign_free_invalid = 0
  alloc_fail = 0
  descriptor_exhausted = 0

projection:
  owner_source_side_meta_lookup = 925750890
  owner_source_side_meta_local = 53771176
  owner_source_side_meta_foreign = 871979714
  owner_source_side_meta_miss = 0
  owner_source_side_meta_probe_total = 871979714
  owner_source_side_meta_probe_max = 1

comparison diagnostic:
  descriptor_storage_lookup = 462912401
  descriptor_storage_probe_total = 691140834
  descriptor_storage_probe_max = 17
```

Read:

```text
The owner-source problem is frequency, not lookup depth.
Almost all projected owner side-metadata reads are foreign after route rehome,
but the owner-source dry-run can resolve them with probe_max=1 and miss=0.

This supports an O(1) owner-source side metadata design, but not a scan-based
StorageOwner16 hot read. HZ6 should not promote StorageOwner16 directly.
```

Next:

```text
OwnerSourceSideMeta-L2 design:
  cache or encode descriptor storage/source owner without allocator-local state
  keep descriptor hot entry ownerless or owner16-side capable
  avoid per-owner-read visibility scans
  preserve routebytes16 comparison-control throughput/RSS safety
```
