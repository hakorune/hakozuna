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
| `Larson T16 main-warmup` | `47.485M / 183,180 KB` | `48.367M / 449,144 KB` selected lowest-RSS | HZ6 matches/slightly beats HZ5 throughput, but uses about 2.45x HZ5 RSS. |
| `Larson T16 worker-warmup` | `42.987M / 155,640 KB` | no current selected-family HZ6 worker row | HZ5 remains a useful RSS reference for same-owner warmup too. HZ6 work has focused on main-warmup cross-owner completion. |

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
| `...routepacked-routebytes16-source16k-route192k-run512` | 48.367M | 449,144 | Current selected lowest-RSS sibling. |
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
1. Keep routebytes16 as the selected low-RSS sibling.
2. Keep storageowner16 as RSS-first evidence/control.
3. Reopen descriptor-side metadata only with owner-source awareness.
4. Avoid allocator-local side-owner metadata; it already failed cross-owner safety.
5. Avoid another route/descriptor/directory static trim unless a new
   representation changes the pressure source.
```

## Next HZ6 Attack

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
selected routebytes16 hot path preserved
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
  preserve routebytes16 selected throughput/RSS safety
```
