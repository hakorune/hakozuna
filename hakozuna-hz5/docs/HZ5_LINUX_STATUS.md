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
MidPageFront front-cache / remote packet shape
```

The remaining gap is not primarily:

```text
front dispatch order
region lookup
cross-size ownership handoff
large allocation handling
owner-local bitmap checks alone
```

It is most likely:

```text
per-class front-cache shape
remote handoff payload shape
ordinary mid-size object path length and free-route fixed cost
```

## Active Branch

```text
codex/hz5-linux-p43-port
```

Recent commits:

```text
0b9d632 Add MidPageFront M4 design checks
9659944 Clean up HZ5 Linux lane organization
```

## Lead Lanes

```text
--linux-hz5-general-midpage-region-shadow
  remote-shadow control for MidPageFront state representation

--linux-hz5-general-midpage-region-shadow-allocfirst
  current local-r0 comparison baseline for later MidPage diagnostics

--linux-hz5-general-midpage-region-shadow-m4mag
  M4 owner-local slot magazine candidate; improves mid/main r90, not local r0

--linux-hz5-general-midpage-region-shadow-m4packet
  M4b page-descriptor remote packet candidate; wins mid/main r50/r90 and
  cross128 r0/r50 in gated RUNS=5

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
  current local-r0 comparison baseline

--linux-hz5-general-midpage-region-shadow-localunsafe
  unsafe r0 upper-bound diagnostic; showed owner-local bitmap checks alone are
  not the main tcmalloc gap

--linux-hz5-general-midpage-region-shadow-nodeless
--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache
  M3 diagnostics; fixed some refill churn but did not become broad leads

--linux-hz5-general-midpage-region-shadow-m4mag
  M4 magazine diagnostic/candidate; strong mid/main r90 signal, cross128 and
  local r0 still need more work

--linux-hz5-general-midpage-region-shadow-m4packet
  M4b page-packet diagnostic/candidate; strong mid/main remote signal,
  cross128_r90 still below allocfirst
```

## Current tcmalloc Read

MidPageFront has split into separate roles:

```text
allocfirst:
  local-r0 comparison baseline

m4mag:
  remote-heavy mid/main candidate

m4packet:
  remote-heavy mid/main/cross candidate

shadow/region:
  controls for state representation and region lookup
```

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
The early shadow result was promising, but later diagnostics show the local r0
gap is structural. M4 magazine improves remote-heavy mid/main rows but does
not close local r0. M4b page packets improve remote-heavy mid/main/cross rows.
Treat `allocfirst` as the local baseline and `m4packet` as the current
remote-heavy mid/main candidate. Cross128_r90 still needs separate recovery.
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

TLS split:

```text
private/raw-results/linux/midpage_tls_split_r3_20260525_055155
private/raw-results/linux/midpage_linkonly_broad_r3_20260525_055224
private/raw-results/linux/midpage_linkonly_verify_r7_20260525_055344
```

Decision:

```text
linkonly is diagnostic only after r7. initial-exec TLS alone helps mid_only_r0
but is weaker on remote/cross. Combined tlslink is worse than linkonly, and
linkonly does not beat allocfirst on main_r90/mid_only_r90 in the r7 verify.
```

Split key medians:

```text
mid_only_r0:
  allocfirst 69.59M
  tlsie      80.03M
  linkonly   77.63M
  tlslink    78.16M

mid_only_r90:
  allocfirst 26.57M
  tlsie      30.68M
  linkonly   35.73M
  tlslink    20.22M

cross128_r90:
  allocfirst 12.66M
  tlsie      11.83M
  linkonly   13.89M
  tlslink    10.40M
```

Linkonly broad key medians:

```text
main_r90:
  allocfirst 23.69M
  linkonly   36.68M
  tcmalloc   50.92M

cross128_r90:
  allocfirst 13.57M
  linkonly   11.02M
  tcmalloc    7.82M
```

Linkonly verify r7:

```text
main_r90:
  allocfirst 38.71M
  linkonly   29.83M
  tcmalloc   48.97M

mid_only_r90:
  allocfirst 37.52M
  linkonly   33.50M
  tcmalloc   44.69M

cross128_r90:
  allocfirst 10.87M
  linkonly   10.92M
  tcmalloc    7.83M
```

M3 nodeless diagnostic:

```text
private/raw-results/linux/midpage_nodeless_r3_20260525_065654
private/raw-results/linux/midpage_perf_allocfirst_nodeless_20260525_070623
```

Decision:

```text
Nodeless is diagnostic only in the first implementation. It confirms that
removing local node->page/node->next traffic helps mid_only_r0 a little, but
the gain is far below the acceptance target and remote rows regress.
```

Key medians:

```text
mid_only_r0:
  allocfirst 67.55M
  nodeless   70.83M
  tcmalloc  141.93M

mid_only_r90:
  allocfirst 39.89M
  nodeless   32.29M
  tcmalloc   46.23M

main_r90:
  allocfirst 27.91M
  nodeless   22.96M
  tcmalloc   51.59M

cross128_r90:
  allocfirst 12.23M
  nodeless   10.82M
  tcmalloc    7.72M
```

Perf spot check:

```text
mid_only_r0:
  allocfirst 90.71M ops/s, cycles 474.3M, instr 660.1M, branches 136.3M,
             cache-misses 2.20M
  nodeless   89.31M ops/s, cycles 455.2M, instr 657.6M, branches 147.3M,
             cache-misses 1.52M
  tcmalloc  165.16M ops/s, cycles 394.3M, instr 298.4M, branches 55.9M,
             cache-misses 2.20M

mid_only_r90:
  allocfirst 40.97M ops/s, cycles 1.24B, instr 1.07B, branches 242.9M,
             branch-misses 9.43M
  nodeless   23.35M ops/s, cycles 1.36B, instr 1.31B, branches 298.5M,
             branch-misses 14.58M
  tcmalloc   46.37M ops/s, cycles 1.67B, instr 866.7M, branches 165.0M,
             branch-misses 15.95M
```

Interpretation:

```text
Nodeless reduced cache misses, but did not reduce instructions or branches.
The next fix should target M3 refill/partial/remote control flow rather than
only payload cache traffic.
```

Stats-only follow-up:

```text
private/raw-results/linux/midpage_nodeless_stats_20260525_070957
```

Interpretation:

```text
The nodeless current_page[class] model is too narrow for random mid traffic.
mid_only_r0 already shows refill=463246 and partial_push=463165, while
refill_new_page is only 1568. The cost is partial-list churn, not page source.
```

M3.2 pointer-cache diagnostic:

```text
private/raw-results/linux/midpage_nodeless_ptrcache_r3_20260525_071403
private/raw-results/linux/midpage_nodeless_ptrcache_stats_20260525_071429
```

Read:

```text
Per-class TLS pointer cache reduces r0 churn:
  mid_only_r0 allocfirst 70.63M, ptrcache 76.48M
  ptrcache_hit=636934, refill=1569

It is not a broad default:
  mid_only_r90 allocfirst 35.31M, ptrcache 25.82M
  cross128_r90 allocfirst 21.58M, ptrcache 10.26M

main_r90 can improve in short runs:
  allocfirst 25.23M, ptrcache 27.15M, tcmalloc 21.72M
```

## Next Engineering Target

Refine or replace the MidPageFront-M3 local topology diagnostic:

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
