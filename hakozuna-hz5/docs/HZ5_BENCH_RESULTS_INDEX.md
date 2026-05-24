# HZ5 Bench Results Index

This file indexes raw benchmark directories and the main decision they support.
Raw logs remain under `private/raw-results/linux/`.

## tcmalloc Target

### `tcmalloc_target_shadow_r3_20260525_033348`

Path:

```text
private/raw-results/linux/tcmalloc_target_shadow_r3_20260525_033348
```

Purpose:

```text
Compare region, midpage_region, shadow, and tcmalloc on main/mid_only r0/r50/r90.
```

Decision:

```text
shadow is the current tcmalloc-chase lead.
main rows are tcmalloc-class in this focused run.
mid_only_r0 remains the large gap.
```

Key medians:

```text
main_r0:     shadow 58.03M, tcmalloc  53.11M
main_r50:    shadow 31.85M, tcmalloc  30.91M
main_r90:    shadow 22.74M, tcmalloc  21.64M
mid_only_r0: shadow 71.36M, tcmalloc 139.58M
mid_only_r50:shadow 40.56M, tcmalloc  77.68M
mid_only_r90:shadow 41.27M, tcmalloc  48.23M
```

### `tcmalloc_shadow_frontfirst_r3_20260525_033543`

Path:

```text
private/raw-results/linux/tcmalloc_shadow_frontfirst_r3_20260525_033543
```

Purpose:

```text
Test shadow + MidPageFront-first free dispatch.
```

Decision:

```text
diagnostic only.
It helps pure mid_only slightly but hurts main/cross128.
```

Key medians:

```text
mid_only_r0:  shadow 74.17M, shadow_frontfirst 76.10M, tcmalloc 142.85M
mid_only_r90: shadow 28.77M, shadow_frontfirst 31.78M, tcmalloc  46.18M
cross128_r90: shadow 17.38M, shadow_frontfirst 11.87M, tcmalloc  7.73M
```

### `tcmalloc_tlscache_r3_20260525_033858`

Path:

```text
private/raw-results/linux/tcmalloc_tlscache_r3_20260525_033858
```

Purpose:

```text
Test shadow + TLS region lookup cache.
```

Decision:

```text
no-go for broad tcmalloc chase.
Region lookup is not the main remaining bottleneck.
```

Key medians:

```text
main_r90:     shadow 22.90M, tlscache 29.76M, tcmalloc 22.24M
mid_only_r0:  shadow 72.91M, tlscache 70.22M, tcmalloc 139.80M
mid_only_r50: shadow 40.79M, tlscache 36.47M, tcmalloc  80.02M
mid_only_r90: shadow 40.73M, tlscache 38.43M, tcmalloc  46.92M
cross128_r90: shadow 15.41M, tlscache 12.00M, tcmalloc  7.69M
```

### `midpage_hotslot_smoke_r3_20260525_052514`

Path:

```text
private/raw-results/linux/midpage_hotslot_smoke_r3_20260525_052514
```

Purpose:

```text
Test shadow + one-entry TLS hot object cache per MidPageFront class.
```

Decision:

```text
no-go.
One-entry local object bypass does not close the tcmalloc gap and regresses
mid_only r90 versus shadow.
```

Key medians:

```text
mid_only_r0:
  shadow   53.86M
  hotslot  51.28M
  tcmalloc 107.21M

mid_only_r90:
  shadow   29.88M
  hotslot  26.94M
  tcmalloc 41.77M
```

### `midpage_activetrust_smoke_r3_20260525_052755`

Path:

```text
private/raw-results/linux/midpage_activetrust_smoke_r3_20260525_052755
```

Purpose:

```text
Test shadow without local alloc-side remote bitmap check.
```

Decision:

```text
diagnostic only.
It improves mid_only_r0 modestly but makes mid_only_r90 unstable and slower.
```

Key medians:

```text
mid_only_r0:
  shadow      51.73M
  activetrust 55.62M
  tcmalloc   109.90M

mid_only_r90:
  shadow      33.50M
  activetrust 24.44M
  tcmalloc    39.06M
```

### `midpage_allocfirst_r3_20260525_053010`

Path:

```text
private/raw-results/linux/midpage_allocfirst_r3_20260525_053010
```

Purpose:

```text
Test preload MidPageFront alloc-before-can-handle dispatch to remove duplicate
size-class lookup on supported MidPageFront allocations.
```

Decision:

```text
promising diagnostic.
It improves mid_only_r0 and keeps main rows close to shadow, but needs broad
confirmation before promotion.
```

Key medians:

```text
mid_only_r0:
  shadow     66.46M
  allocfirst 73.74M
  tcmalloc  153.38M

mid_only_r50:
  shadow     41.02M
  allocfirst 39.92M
  tcmalloc   75.63M

mid_only_r90:
  shadow     30.02M
  allocfirst 18.53M
  tcmalloc   46.74M
```

### `midpage_allocfirst_r90_verify_r5_20260525_053037`

Path:

```text
private/raw-results/linux/midpage_allocfirst_r90_verify_r5_20260525_053037
```

Purpose:

```text
Verify whether the allocfirst r90 drop was persistent.
```

Decision:

```text
allocfirst r90 is roughly shadow-equivalent in the r5 verify.
The useful signal remains mid_only_r0 improvement.
```

Key medians:

```text
mid_only_r90:
  shadow     32.34M
  allocfirst 32.06M
  tcmalloc   43.50M
```

### `midpage_allocfirst_tryalloc_r3_20260525_054204`

Path:

```text
private/raw-results/linux/midpage_allocfirst_tryalloc_r3_20260525_054204
```

Purpose:

```text
Verify the cleaned allocfirst implementation after replacing the preload
alloc-then-can-handle shortcut with an explicit MidPageFront try-alloc API.
```

Decision:

```text
allocfirst remains a useful diagnostic. It improves mid_only_r0 versus shadow
and does not show a r90 loss in this focused r3.
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

### `midpage_slotswitch_r3_20260525_054521`

Path:

```text
private/raw-results/linux/midpage_slotswitch_r3_20260525_054521
```

Purpose:

```text
Test whether replacing MidPageFront slot-index variable division with
fixed-class switch/shift dispatch closes the mid_only local gap.
```

Decision:

```text
No-go. The division removal does not beat allocfirst on r0 and hurts r90.
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

### `midpage_tlslink_r3_20260525_054807`

Path:

```text
private/raw-results/linux/midpage_tlslink_r3_20260525_054807
```

Purpose:

```text
Focused test for preload-wide initial-exec TLS and speed link flags after
assembly showed repeated MidPageFront hot-path __tls_get_addr calls.
```

Decision:

```text
Promising. tlslink improves mid_only r0/r90 versus allocfirst.
```

Key medians:

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

### `midpage_tlslink_broad_r3_20260525_054859`

Path:

```text
private/raw-results/linux/midpage_tlslink_broad_r3_20260525_054859
```

Purpose:

```text
Short broad check for tlslink across main and cross128.
```

Decision:

```text
Do not promote yet. main improves, but cross128_r90 needs verification because
the short r3 showed a regression versus allocfirst.
```

Key medians:

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
```

### `midpage_tlslink_cross128_verify_r5_20260525_055003`

Path:

```text
private/raw-results/linux/midpage_tlslink_cross128_verify_r5_20260525_055003
```

Purpose:

```text
Focused r5 verification of tlslink on cross128_r90.
```

Decision:

```text
tlslink remains above tcmalloc on cross128_r90 but regresses versus allocfirst.
Keep it diagnostic instead of broad default.
```

Key medians:

```text
cross128_r90:
  allocfirst 14.52M
  tlslink    10.16M
  tcmalloc    7.80M
```

### `midpage_tls_split_r3_20260525_055155`

Path:

```text
private/raw-results/linux/midpage_tls_split_r3_20260525_055155
```

Purpose:

```text
Split tlslink into initial-exec TLS only and speed link flags only.
```

Decision:

```text
linkonly is the best balanced split. tlsie helps mid_only_r0 most, but linkonly
is stronger on mid_only_r90 and cross128_r90 in this r3.
```

Key medians:

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

### `midpage_linkonly_broad_r3_20260525_055224`

Path:

```text
private/raw-results/linux/midpage_linkonly_broad_r3_20260525_055224
```

Purpose:

```text
Short broad check for linkonly after the TLS split.
```

Decision:

```text
linkonly improves main_r90 in this r3 and stays above tcmalloc on cross128_r90,
but later r7 verification does not promote it.
```

Key medians:

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

### `midpage_linkonly_verify_r7_20260525_055344`

Path:

```text
private/raw-results/linux/midpage_linkonly_verify_r7_20260525_055344
```

Purpose:

```text
Higher-run verification for linkonly on remote-heavy main, mid_only, and
cross128.
```

Decision:

```text
No promotion. linkonly is roughly cross128-equivalent to allocfirst and remains
above tcmalloc there, but loses to allocfirst on main_r90 and mid_only_r90.
```

Key medians:

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

### `midpage_nodeless_r3_20260525_065654`

Path:

```text
private/raw-results/linux/midpage_nodeless_r3_20260525_065654
```

Purpose:

```text
First MidPageFront-M3 nodeless local page-run diagnostic.
```

Decision:

```text
Diagnostic only. It improves mid_only_r0 slightly, but misses the acceptance
target and regresses remote rows.
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

### `midpage_perf_allocfirst_nodeless_20260525_070623`

Path:

```text
private/raw-results/linux/midpage_perf_allocfirst_nodeless_20260525_070623
```

Purpose:

```text
Perf spot check for allocfirst, nodeless, and tcmalloc on mid_only r0/r90.
```

Decision:

```text
Nodeless reduces cache misses but increases branch/control-flow pressure,
especially on remote-heavy r90. Payload linked-list traffic is not the only
bottleneck.
```

Key counters:

```text
mid_only_r0:
  allocfirst 90.71M ops/s, instr 660.1M, branches 136.3M, cache-misses 2.20M
  nodeless   89.31M ops/s, instr 657.6M, branches 147.3M, cache-misses 1.52M
  tcmalloc  165.16M ops/s, instr 298.4M, branches  55.9M, cache-misses 2.20M

mid_only_r90:
  allocfirst 40.97M ops/s, instr 1.07B, branches 242.9M, branch-misses  9.43M
  nodeless   23.35M ops/s, instr 1.31B, branches 298.5M, branch-misses 14.58M
  tcmalloc   46.37M ops/s, instr 866.7M, branches 165.0M, branch-misses 15.95M
```

## MidPageFront-M2

### `midpage_region_broad_r5_20260525_031852`

Path:

```text
private/raw-results/linux/midpage_region_broad_r5_20260525_031852
```

Purpose:

```text
Broad r5 comparison for region, midpage_region, HZ4, and tcmalloc.
```

Decision:

```text
midpage_region is a valid remote-heavy mid-size candidate.
It is not a broad default because r0 and large_only_r90 regress.
```

Key medians:

```text
main_r50:      region 30.92M, midpage_region 41.20M, tcmalloc 78.47M
main_r90:      region 29.48M, midpage_region 32.12M, tcmalloc 53.51M
mid_only_r50:  region 28.26M, midpage_region 44.43M, tcmalloc 78.08M
mid_only_r90:  region 29.81M, midpage_region 35.61M, tcmalloc 51.56M
cross128_r90:  region 13.88M, midpage_region 20.83M, tcmalloc  7.61M
large_only_r90:region 10.88M, midpage_region  6.88M, tcmalloc  3.63M
```

### `frontfirst_verify_r5_20260525_032806`

Path:

```text
private/raw-results/linux/frontfirst_verify_r5_20260525_032806
```

Purpose:

```text
Verify MidPageFront-first free dispatch after focused r3.
```

Decision:

```text
frontfirst is diagnostic only.
It helps mid_only_r90 but regresses main/cross128/guard.
```

### `midpage_nodeless_stats_20260525_070957`

Path:

```text
private/raw-results/linux/midpage_nodeless_stats_20260525_070957
```

Purpose:

```text
Stats-only observation for MidPageFront-M3 nodeless. This build enables
BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_STATS and is not a speed lane.
```

Decision:

```text
M3 nodeless is bottlenecked by partial/refill churn. mid_only_r0 has
refill=463246 and partial_push=463165 with only refill_new_page=1568, so the
single current_page[class] design is too narrow.
```

## Older Results

The full chronological result log remains in:

```text
docs/archive/current_task_2026-05-hz5-linux.md
```

Use that archive for older Local2P, SmallFront, MidFront, LargeFront, OwnerHub,
and paper-main observations that have not yet been promoted into this index.
