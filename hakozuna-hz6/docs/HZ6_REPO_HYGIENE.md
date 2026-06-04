# HZ6 Repo Hygiene

HZ6 now has enough lanes and evidence that the repository needs a stable
reading order. This file is the cleanup map: use it before adding another
benchmark lane or source-level experiment.

## Reading Order

| Purpose | File |
| --- | --- |
| Short paper-facing selected rows | `HZ6_SELECTED_FAMILY_SUMMARY.md` |
| Which lane is which | `HZ6_LANE_GUIDE.md` |
| Cross-allocator comparison table | `HZ6_CROSS_ALLOCATOR_COMPARISON.md` |
| Windows lane registry cleanup | `HZ6_WINDOWS_LANE_REGISTRY.md` |
| Source/module cleanup plan | `HZ6_SOURCE_MODULARIZATION.md` |
| Long experiment ledger | `current_task.md` |

Rule:

```text
Use current_task.md as a ledger, not as the primary orientation document.
If a decision is stable, summarize it in the selected summary or lane guide.
```

## Lane Status Vocabulary

Use these terms consistently in docs and scripts.

| Status | Meaning |
| --- | --- |
| selected | Clean row used for current HZ6 family comparisons. |
| selected sibling | Clean alternate row with a specific tradeoff, usually lower RSS or higher speed. |
| control | Useful comparison row, not promoted. |
| evidence | Mechanism result that explains a design point. |
| pressure | Intentionally stressed/unsafe-capacity row; never a paper default without caveat. |
| no-go | Frozen failed lane; keep only when it explains a boundary. |

## Current Selected Lanes

The current selected family is:

```text
mixed balanced/wide:
  rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k

random_mixed:
  strict + sameownerfast-descavail-noboost-route4k

larger_sizes:
  speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k

Larson throughput:
  speed + ownerlocalityfast-rsscap-2-desc160k

Larson lower RSS:
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k

Larson lowest RSS:
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512
```

Do not add new lanes to the selected family unless they pass:

```text
alloc_fail = 0
descriptor_exhausted = 0
route_register_fail = 0
source_block_exhausted = 0
```

## Cleanup Backlog

### P0: Keep Orientation Docs Small

Do:

```text
HZ6_SELECTED_FAMILY_SUMMARY.md:
  selected rows and latest numbers only

HZ6_LANE_GUIDE.md:
  lane dictionary and runner commands

current_task.md:
  historical ledger and detailed experimental notes
```

Do not:

```text
copy every benchmark table into every doc
leave superseded lanes near the top without a status label
```

### P1: Script Lane Registry

Current issue:

```text
win/run_win_hz6_capacity_matrix.ps1 has lane suffixes.
win/bench_app_like_allocator_build_common.ps1 has build flags and suffixes.
win/run_win_hz6_selected_family.ps1 has selected presets.
```

This is workable, but new lane names are easy to mistype.

Near-term rule:

```text
Add a lane in both capacity_matrix and build_common in the same commit.
Add selected-family presets only after the lane has a clean result.
Keep no-go lanes available only when they define a boundary.
```

Longer-term cleanup:

```text
Create a single PowerShell lane manifest for:
  name
  suffix
  status
  family
  notes

Then let build and matrix scripts consume it.
```

See `HZ6_WINDOWS_LANE_REGISTRY.md` for the target shape and migration order.

### P2: Source Metadata Split

Current Larson RSS read:

```text
route192k is the clean static route lower bound.
route160k/128k fail warmup because route capacity saturates.
route192k-run512 is the previous selected lowest-RSS sibling/control.
route192k-run512 + descriptor no-backptr is the current selected lowest-RSS
sibling candidate.
```

So the next RSS reduction should not be another route-capacity cut or another
blind source-run slot cut.

Next source cleanup target:

```text
Descriptor layout:
  guard no-backptr as the selected low-RSS sibling
  then consider descriptor owner/token compression or side metadata

SourceBlock metadata layout:
  split or compress run bitmap / run metadata only after descriptor layout
  guard work
  keep route/source lifetime semantics unchanged
  keep safety counters clean
```

See `HZ6_SOURCE_MODULARIZATION.md`.

## Commit Hygiene

Prefer small commits with one of these scopes:

```text
Docs:
  lane/status/current-task cleanup only

Scripts:
  runner or lane registry changes only

Source:
  allocator implementation changes only

Results:
  benchmark markdown files only, usually paired with docs
```

Generated logs should usually stay out of commits unless a log is the result.
