# HZ5 Linux Status

This is the short current-status entry point for HZ5 Linux work.

For full chronology, see:

```text
docs/archive/current_task_2026-05-hz5-linux.md
```

## Current Goal

HZ5 Linux is now targeting `tcmalloc` on ordinary malloc workloads.

Current bottleneck:

```text
MidPageFront local object topology
```

The remaining gap is not primarily:

```text
front dispatch order
region lookup
cross-size ownership handoff
large allocation handling
```

It is most likely:

```text
per-local alloc/free active-state work
metadata/freelist topology
ordinary mid-size object path length
```

## Active Branch

```text
codex/hz5-linux-p43-port
```

Recent commits:

```text
b74bdc5 Add tcmalloc-target MidPage diagnostics
41cd182 Add MidPageFront free-dispatch diagnostic
```

## Lead Lanes

```text
--linux-hz5-general-midpage-region-shadow
  current tcmalloc-chase lead

--linux-hz5-general-midpage-region
  MidPageFront-M2.2 stable remote-heavy mid-size candidate

--linux-hz5-local2p-linkflags
  exact 64K/a8192 local speed appendix profile

--linux-hz5-local2p-rssretain2048tls
  exact retained-RSS appendix profile

--linux-hz5-local2p-remotebatch
  exact remote-free appendix profile
```

## Diagnostic Lanes

```text
--linux-hz5-general-midpage-region-frontfirst
  MidPageFront-first free dispatch
  diagnostic only; not broad default

--linux-hz5-general-midpage-region-shadow-frontfirst
  shadow + MidPageFront-first free dispatch
  diagnostic only; helps pure mid_only slightly but hurts main/cross128

--linux-hz5-general-midpage-region-shadow-tlscache
  shadow + TLS region lookup cache
  no-go; region lookup is not the main tcmalloc gap

--linux-hz5-general-midpage-region-shadow-hotslot
  shadow + one-entry TLS hot object cache per MidPageFront class
  no-go; local object freelist bypass does not close the tcmalloc gap

--linux-hz5-general-midpage-region-shadow-activetrust
  shadow without local alloc-side remote bitmap check
  diagnostic only; r0 improves slightly but r90 becomes unstable

--linux-hz5-general-midpage-region-shadow-allocfirst
  shadow with MidPageFront alloc-before-can-handle dispatch in preload
  promising diagnostic; improves mid_only r0 without a verified r90 loss
```

## Current tcmalloc Read

`shadow` is the most promising tcmalloc-chase profile so far.

Latest focused read:

```text
private/raw-results/linux/tcmalloc_target_shadow_r3_20260525_033348
```

Key medians:

```text
main_r0:
  shadow    58.03M
  tcmalloc  53.11M

main_r50:
  shadow    31.85M
  tcmalloc  30.91M

main_r90:
  shadow    22.74M
  tcmalloc  21.64M

mid_only_r0:
  shadow    71.36M
  tcmalloc 139.58M

mid_only_r50:
  shadow    40.56M
  tcmalloc  77.68M

mid_only_r90:
  shadow    41.27M
  tcmalloc  48.23M
```

Interpretation:

```text
main rows are tcmalloc-class in the focused run.
mid_only remote-heavy is closer but still below tcmalloc.
mid_only local-only remains the clear structural gap.
The hot-slot and tlscache diagnostics indicate this gap is not a simple
dispatch, region-lookup, or one-entry freelist bypass issue.
The allocfirst diagnostic shows duplicate preload class lookup is part of the
local-only cost, but not enough to close the tcmalloc gap alone.
```

## Next Engineering Target

Prototype a MidPageFront local topology diagnostic:

```text
Goal:
  reduce active-state / metadata / freelist work per local alloc/free

Constraints:
  keep fail-closed ownership
  keep remote-shadow or equivalent remote protection
  do not reintroduce alloc_failed under r90
  do not change Windows P43i/P45
```

## Important References

```text
docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
docs/HZ5_BENCH_RESULTS_INDEX.md
docs/HZ5_MIDPAGEFRONT_M2_DESIGN.md
docs/HZ5_LINUX_DEV_BRIEF.md
docs/source-map.md
```
