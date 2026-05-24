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

Follow-up cleanup:

```text
private/raw-results/linux/midpage_allocfirst_tryalloc_r3_20260525_054204
```

Key medians:

```text
mid_only_r0:
  shadow     66.04M
  allocfirst 70.63M
  tcmalloc  141.00M

mid_only_r90:
  shadow     33.57M
  allocfirst 37.00M
  tcmalloc   48.94M
```

Interpretation:

```text
The explicit MidPageFront try-alloc API preserves the allocfirst signal while
removing the previous alloc-then-can-handle preload shortcut.
```

Slot-index diagnostic:

```text
private/raw-results/linux/midpage_slotswitch_r3_20260525_054521
```

Decision:

```text
slotswitch is no-go. Replacing variable slot-index division with fixed-class
switch/shift does not improve mid_only_r0 and regresses mid_only_r90.
```

Key medians:

```text
mid_only_r0:
  allocfirst 66.17M
  slotswitch 65.72M
  tcmalloc  141.17M

mid_only_r90:
  allocfirst 40.66M
  slotswitch 37.04M
  tcmalloc   52.19M
```

TLS/linkage diagnostic:

```text
private/raw-results/linux/midpage_tlslink_r3_20260525_054807
private/raw-results/linux/midpage_tlslink_broad_r3_20260525_054859
private/raw-results/linux/midpage_tlslink_cross128_verify_r5_20260525_055003
```

Decision:

```text
tlslink is promising but not promoted as the broad default. It removes
MidPageFront hot-path __tls_get_addr calls and improves main/mid rows.
cross128_r90 remains above tcmalloc, but regresses versus allocfirst in the
focused r5 verify.
```

Focused key medians:

```text
mid_only_r0:
  allocfirst 68.98M
  tlslink    73.87M
  tcmalloc  144.14M

mid_only_r90:
  allocfirst 37.17M
  tlslink    39.72M
  tcmalloc   46.35M
```

Broad key medians:

```text
main_r0:
  allocfirst 64.24M
  tlslink    69.21M
  tcmalloc  131.17M

main_r90:
  allocfirst 26.18M
  tlslink    35.32M
  tcmalloc   46.51M

cross128_r90:
  allocfirst 19.70M
  tlslink    10.71M
  tcmalloc    7.81M

cross128_r90 verify r5:
  allocfirst 14.52M
  tlslink    10.16M
  tcmalloc    7.80M
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
