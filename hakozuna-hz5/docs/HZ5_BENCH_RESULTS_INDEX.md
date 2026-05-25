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

### `midpage_nodeless_ptrcache_r3_20260525_071403`

Path:

```text
private/raw-results/linux/midpage_nodeless_ptrcache_r3_20260525_071403
```

Purpose:

```text
Short r3 comparison for M3.2 per-class TLS pointer cache.
```

Decision:

```text
Diagnostic only. ptrcache improves mid_only_r0 versus nodeless/allocfirst and
can help main_r90, but mid_only_r90 and cross128_r90 regress versus allocfirst.
```

Key medians:

```text
mid_only_r0:   allocfirst 70.63M, nodeless 73.89M, ptrcache 76.48M, tcmalloc 139.49M
mid_only_r90:  allocfirst 35.31M, nodeless 25.18M, ptrcache 25.82M, tcmalloc  41.78M
main_r90:      allocfirst 25.23M, nodeless 18.73M, ptrcache 27.15M, tcmalloc  21.72M
cross128_r90:  allocfirst 21.58M, nodeless 13.64M, ptrcache 10.26M, tcmalloc   7.88M
```

### `midpage_nodeless_ptrcache_stats_20260525_071429`

Path:

```text
private/raw-results/linux/midpage_nodeless_ptrcache_stats_20260525_071429
```

Purpose:

```text
Stats-only observation for M3.2 ptrcache. This is not a speed lane.
```

Decision:

```text
ptrcache fixes local r0 refill churn but does not solve remote-heavy churn.
mid_only_r0 has ptrcache_hit=636934 and refill=1569, while mid_only_r90 still
has refill=265631 and remote_drained=556792.
```

### `midpage_design_check_20260525_073659`

Path:

```text
private/raw-results/linux/midpage_design_check_20260525_073659
```

Purpose:

```text
Design-check packet before the next MidPageFront architecture decision:
size-class histogram, unsafe local bitmap-check upper bound, alloc+free
MidPage-first dispatch A/B, and perf record spot checks.
```

Decision:

```text
The tcmalloc gap is structural. Unsafe local bitmap-check removal gives only a
small r0 gain, and alloc+free MidPage-first dispatch helps main_r90 but hurts
mid_only. The next design should focus on a real front-cache / magazine /
batch-refill shape rather than smaller state-check or dispatch tweaks.
```

Key rows:

```text
mid_only_r0 unsafe upper bound:
  allocfirst   72.83M
  localunsafe  76.79M
  tcmalloc    147.13M

alloc+free MidPage-first:
  mid_only_r0:   allocfirst 69.02M, allocfreefirst 67.02M, tcmalloc 139.85M
  mid_only_r90:  allocfirst 31.50M, allocfreefirst 26.17M, tcmalloc  47.02M
  main_r90:      allocfirst 25.35M, allocfreefirst 29.39M, tcmalloc  19.90M
  cross128_r90:  allocfirst 14.67M, allocfreefirst 14.71M, tcmalloc   7.80M
```

### `midpage_m4mag_localfast_smoke_20260525_080203`

Path:

```text
private/raw-results/linux/midpage_m4mag_localfast_smoke_20260525_080203
```

Purpose:

```text
First MidPageFront-M4 magazine smoke after switching owner-local slot_state2
transitions from CAS to load/store. Performance runs do not enable
HZ5_PRELOAD_STATS.
```

Decision:

```text
M4 magazine is not the local r0 tcmalloc answer yet, but it is a strong
remote-heavy candidate. It improves mid_only_r90 versus allocfirst and tcmalloc,
while mid_only_r0 remains slightly below allocfirst and far below tcmalloc.
```

Key rows:

```text
mid_only_r0:
  allocfirst 93.53M
  m4mag      90.92M
  tcmalloc  230.97M

mid_only_r90:
  allocfirst 23.83M
  m4mag      37.22M
  tcmalloc   26.23M
```

### `midpage_m4mag_broad_smoke_20260525_080228`

Path:

```text
private/raw-results/linux/midpage_m4mag_broad_smoke_20260525_080228
```

Purpose:

```text
Short broad smoke for M4 magazine on main/cross128 r0/r90.
```

Decision:

```text
M4 magazine improves main_r90 strongly but regresses cross128_r90 versus
allocfirst. Keep as diagnostic/candidate, not a broad default.
```

Key rows:

```text
main_r90:
  allocfirst 23.65M
  m4mag      39.36M
  tcmalloc   29.96M

cross128_r90:
  allocfirst 17.33M
  m4mag      14.98M
  tcmalloc   14.65M
```

### `midpage_m4mag_attrib_20260525_080241`

Path:

```text
private/raw-results/linux/midpage_m4mag_attrib_20260525_080241
```

Purpose:

```text
Separate HZ5_PRELOAD_STATS attribution smoke for M4 magazine.
```

Result:

```text
malloc_hz5=40208
malloc_real=0
free_hz5=40238
free_real=0
track_insert_fail=0
```

### `midpage_m4packet_smoke_20260525_082809`

Path:

```text
private/raw-results/linux/midpage_m4packet_smoke_20260525_082809
```

Purpose:

```text
First MidPageFront-M4b page-descriptor packet smoke. Performance runs do not
enable HZ5_PRELOAD_STATS.
```

Decision:

```text
M4b is a remote-heavy candidate. It improves over m4mag on mid_only_r90,
main_r90, and cross128_r90, and beats tcmalloc in this short smoke. It is not a
local r0 solution.
```

Key rows:

```text
mid_only_r90:
  allocfirst 27.53M
  m4mag      23.25M
  m4packet   36.12M
  tcmalloc   24.31M

main_r90:
  allocfirst 24.33M
  m4mag      34.07M
  m4packet   36.28M
  tcmalloc   31.43M

cross128_r90:
  allocfirst 15.02M
  m4mag      15.94M
  m4packet   16.72M
  tcmalloc   15.60M
```

### `midpage_m4packet_attrib_20260525_082826`

Path:

```text
private/raw-results/linux/midpage_m4packet_attrib_20260525_082826
```

Purpose:

```text
Separate HZ5_PRELOAD_STATS attribution smoke for M4b page packets.
```

Result:

```text
malloc_hz5=40208
malloc_real=0
free_hz5=40238
free_real=0
track_insert_fail=0
```

### `midpage_m4packet_gated_verify_r5_20260525_084409`

Path:

```text
private/raw-results/linux/midpage_m4packet_gated_verify_r5_20260525_084409
```

Purpose:

```text
RUNS=5 verify after adding gated owner-inbox drain before M4 magazine pop.
Performance runs do not enable HZ5_PRELOAD_STATS.
```

Decision:

```text
M4b gated drain is a strong mid/main remote-heavy candidate. It wins main and
mid_only r50/r90 versus allocfirst and tcmalloc. It also wins cross128 r0/r50,
but cross128 r90 remains below allocfirst and roughly tcmalloc-class.
```

Key rows:

```text
mid_only:
  r0   allocfirst 90.20M, m4packet 88.07M, tcmalloc 221.24M
  r50  allocfirst 30.96M, m4packet 37.34M, tcmalloc  24.01M
  r90  allocfirst 24.79M, m4packet 33.82M, tcmalloc  25.46M

main:
  r0   allocfirst 92.54M, m4packet 88.49M, tcmalloc 210.87M
  r50  allocfirst 29.64M, m4packet 35.28M, tcmalloc  26.30M
  r90  allocfirst 23.77M, m4packet 31.50M, tcmalloc  29.86M

cross128:
  r0   allocfirst 53.77M, m4packet 56.83M, tcmalloc 46.12M
  r50  allocfirst 21.63M, m4packet 23.52M, tcmalloc 21.37M
  r90  allocfirst 18.64M, m4packet 14.62M, tcmalloc 14.68M
```

### `midpage_m4packet_gated_attrib_20260525_084433`

Path:

```text
private/raw-results/linux/midpage_m4packet_gated_attrib_20260525_084433
```

Purpose:

```text
Separate HZ5_PRELOAD_STATS attribution smoke for gated M4b.
```

Result:

```text
malloc_hz5=40208
malloc_real=0
free_hz5=40238
free_real=0
track_insert_fail=0
```

### `midpage_m4packet_crossdrain_smoke_20260525_085453`

Path:

```text
private/raw-results/linux/midpage_m4packet_crossdrain_smoke_20260525_085453
```

Purpose:

```text
Focused RUNS=3 smoke for the separate m4packet-crossdrain lane. Performance
runs keep HZ5_PRELOAD_STATS unset.
```

Decision:

```text
No promote. Cross-drain helps mid_only/main r90 but hurts cross128 r50/r90.
Keep it as a diagnostic that the remaining cross128 gap needs a different
control point than other-front MidPage draining.
```

Key rows:

```text
main r90:
  allocfirst  6.50M, m4packet 11.02M, crossdrain 11.52M, tcmalloc 11.83M

mid_only r90:
  allocfirst  6.81M, m4packet 11.23M, crossdrain 12.79M, tcmalloc 11.92M

cross128 r90:
  allocfirst  5.17M, m4packet  7.11M, crossdrain  5.88M, tcmalloc 12.70M
```

### `midpage_m4packet_crossdrain_attrib_20260525_085513`

Path:

```text
private/raw-results/linux/midpage_m4packet_crossdrain_attrib_20260525_085513
```

Result:

```text
m4packet:    malloc_hz5=10049 malloc_real=0 free_hz5=10057 free_real=0 track_insert_fail=0
crossdrain:  malloc_hz5=10049 malloc_real=0 free_hz5=10057 free_real=0 track_insert_fail=0
```

### `midpage_m4packet_freefirst_smoke_20260525_085727`

Path:

```text
private/raw-results/linux/midpage_m4packet_freefirst_smoke_20260525_085727
```

Purpose:

```text
Focused RUNS=3 A/B for MidPageFront-first preload free dispatch on top of
M4packet. Performance runs keep HZ5_PRELOAD_STATS unset.
```

Decision:

```text
Keep as an incremental candidate. Freefirst improves main/mid_only r0 and
slightly improves main/cross128 r90 without changing allocator ownership
metadata. It does not close the local-r0 tcmalloc gap.
```

Key rows:

```text
main r0:
  m4packet 33.09M, freefirst 36.33M, tcmalloc 117.42M

mid_only r0:
  m4packet 35.33M, freefirst 37.66M, tcmalloc 102.98M

cross128 r90:
  m4packet  6.30M, freefirst  6.76M, tcmalloc 10.83M
```

### `midpage_m4packet_freefirst_attrib_20260525_085745`

Path:

```text
private/raw-results/linux/midpage_m4packet_freefirst_attrib_20260525_085745
```

Result:

```text
m4packet:   malloc_hz5=10049 malloc_real=0 free_hz5=10057 free_real=0 track_insert_fail=0
freefirst:  malloc_hz5=10049 malloc_real=0 free_hz5=10057 free_real=0 track_insert_fail=0
```

### `midpage_m4packet_routefree_smoke_20260525_090830`

Path:

```text
private/raw-results/linux/midpage_m4packet_routefree_smoke_20260525_090830
```

Purpose:

```text
Focused RUNS=3 C1 free-route diagnostic. `routefree` uses M4packet with
MidPageFront -> LargeFront -> SmallFront -> MidFront preload free dispatch.
Performance runs keep HZ5_PRELOAD_STATS unset.
```

Decision:

```text
Candidate-watch, not broad default. Routefree improves local r0 and cross128
r0 versus freefirst, and it improves mid_only r90. It does not beat freefirst
on cross128 r90, so freefirst remains the cleaner incremental candidate for
broad comparison.
```

Key rows:

```text
main r0:
  m4packet 32.95M, freefirst 33.39M, routefree 36.18M, tcmalloc 116.96M

mid_only r0:
  m4packet 36.14M, freefirst 36.39M, routefree 36.89M, tcmalloc 119.44M

cross128 r0:
  m4packet 19.85M, freefirst 21.19M, routefree 21.68M, tcmalloc 64.50M

cross128 r90:
  m4packet 5.52M, freefirst 6.45M, routefree 6.15M, tcmalloc 15.08M
```

### `midpage_m4packet_routefree_attrib_20260525_090850`

Path:

```text
private/raw-results/linux/midpage_m4packet_routefree_attrib_20260525_090850
```

Result:

```text
freefirst:  malloc_hz5=10049 malloc_real=0 free_hz5=10057 free_real=0 track_insert_fail=0
routefree:  malloc_hz5=10049 malloc_real=0 free_hz5=10057 free_real=0 track_insert_fail=0
```

### `midpage_route_matrix_r5_20260525_091054`

Path:

```text
private/raw-results/linux/midpage_route_matrix_r5_20260525_091054
```

Purpose:

```text
RUNS=5 matrix for the current MidPage comparison set:
allocfirst / m4packet / freefirst / routefree / tcmalloc.
Performance runs keep HZ5_PRELOAD_STATS unset.
```

Decision:

```text
M4packet-freefirst is the balanced lead. Routefree is useful for mid_only r90
but is not a broad default. The remaining local-r0 and cross128 gaps to
tcmalloc are structural and not solved by free ordering.
```

Key rows:

```text
main r0:
  allocfirst 35.62M, m4packet 35.79M, freefirst 36.23M, routefree 36.21M, tcmalloc 116.63M

main r90:
  allocfirst  7.78M, m4packet 11.27M, freefirst 11.65M, routefree 11.25M, tcmalloc 10.29M

mid_only r0:
  allocfirst 36.48M, m4packet 36.19M, freefirst 37.57M, routefree 35.64M, tcmalloc 115.21M

mid_only r90:
  allocfirst  6.68M, m4packet 12.44M, freefirst 11.86M, routefree 12.82M, tcmalloc  6.09M

cross128 r0:
  allocfirst 20.50M, m4packet 20.57M, freefirst 20.68M, routefree 20.17M, tcmalloc 61.49M

cross128 r90:
  allocfirst  5.10M, m4packet  6.43M, freefirst  6.57M, routefree  5.78M, tcmalloc 12.17M
```

### `midpage_route_matrix_attrib_20260525_091125`

Path:

```text
private/raw-results/linux/midpage_route_matrix_attrib_20260525_091125
```

Result:

```text
allocfirst: malloc_hz5=10049 malloc_real=0 free_hz5=10057 free_real=0 track_insert_fail=0
m4packet:   malloc_hz5=10049 malloc_real=0 free_hz5=10057 free_real=0 track_insert_fail=0
freefirst:  malloc_hz5=10049 malloc_real=0 free_hz5=10057 free_real=0 track_insert_fail=0
routefree:  malloc_hz5=10049 malloc_real=0 free_hz5=10057 free_real=0 track_insert_fail=0
```

### `perf_midpage_freefirst_r0_20260525_091532`

Path:

```text
private/raw-results/linux/perf_midpage_freefirst_r0_20260525_091532
```

Purpose:

```text
perf stat/record smoke for mid_only r0 comparing M4packet-freefirst and
tcmalloc. Performance runs keep HZ5_PRELOAD_STATS unset.
```

Read:

```text
freefirst:
  cycles       108.72M
  instructions 223.67M
  branches      48.79M

tcmalloc:
  cycles        52.27M
  instructions  82.64M
  branches      14.59M
```

perf report signal:

```text
freefirst has visible __tls_get_addr cost and hot samples in
hz5_midpagefront_try_alloc/free. The gap is instruction/branch/TLS heavy, not
primarily cache-miss heavy.
```

### `perf_midpage_freefirst_tlslink_20260525_091619`

Path:

```text
private/raw-results/linux/perf_midpage_freefirst_tlslink_20260525_091619
```

Purpose:

```text
Focused mid_only r0 A/B for M4packet-freefirst with preload initial-exec TLS
and speed link flags.
```

Result:

```text
mid_only r0 median:
  freefirst 33.51M
  tlslink   38.97M
  tcmalloc 106.55M

perf stat:
  freefirst instructions 221.28M, branches 48.83M
  tlslink   instructions 178.47M, branches 37.03M
  tcmalloc  instructions  70.81M, branches 13.64M
```

Decision:

```text
Add m4packet-freefirst-tlslink as a candidate-watch lane. It clearly reduces
TLS/instruction overhead, but tcmalloc remains far ahead. It needs a broad
RUNS=5 matrix before promotion.
```

### `midpage_freefirst_tlslink_matrix_r5_20260525_093316`

Path:

```text
private/raw-results/linux/midpage_freefirst_tlslink_matrix_r5_20260525_093316
```

Purpose:

```text
Broad RUNS=5 matrix for the freefirst TLS/linkflags diagnostic. Performance
runs keep HZ5_PRELOAD_STATS unset.
```

Result:

```text
main_r0:
  allocfirst  87.62M
  freefirst   89.98M
  tlslink    100.90M
  tcmalloc   226.17M

main_r50:
  allocfirst  31.15M
  freefirst   36.47M
  tlslink     43.22M
  tcmalloc    26.04M

main_r90:
  allocfirst  21.12M
  freefirst   33.05M
  tlslink     30.66M
  tcmalloc    30.43M

mid_only_r0:
  allocfirst  93.51M
  freefirst   92.94M
  tlslink    107.27M
  tcmalloc   226.65M

mid_only_r50:
  allocfirst  30.01M
  freefirst   35.20M
  tlslink     40.68M
  tcmalloc    24.28M

mid_only_r90:
  allocfirst  23.74M
  freefirst   33.25M
  tlslink     31.86M
  tcmalloc    28.61M

cross128_r0:
  allocfirst  55.17M
  freefirst   57.03M
  tlslink     60.20M
  tcmalloc    45.57M

cross128_r50:
  allocfirst  19.60M
  freefirst   20.48M
  tlslink     23.29M
  tcmalloc    21.66M

cross128_r90:
  allocfirst  18.07M
  freefirst   19.89M
  tlslink     17.29M
  tcmalloc    14.37M
```

Decision:

```text
tlslink is a strong local/r50 candidate and reduces the local instruction
path, but it is not a balanced replacement for freefirst because main,
mid_only, and cross128 r90 prefer freefirst. Next diagnostic should isolate the
M4 magazine alloc-side descriptor transition cost.
```

### `midpage_allocelide_r0_smoke_20260525_093736`

Path:

```text
private/raw-results/linux/midpage_allocelide_r0_smoke_20260525_093736
```

Purpose:

```text
Unsafe upper-bound smoke for eliding the M4 owner-local alloc-side
CACHE -> LIVE slot_state2 transition. Performance runs keep HZ5_PRELOAD_STATS
unset.
```

Result:

```text
main_r0:
  freefirst   90.45M
  tlslink    105.97M
  allocelide 108.76M
  tcmalloc   220.60M

mid_only_r0:
  freefirst   94.38M
  tlslink    100.81M
  allocelide 114.85M
  tcmalloc   225.86M
```

Decision:

```text
The alloc-side state transition is real cost, but not the main tcmalloc gap.
Keep allocelide unsafe/diagnostic only because it weakens double-free-before-
reuse attribution.
```

### `midpage_ptrmag_r0_smoke_20260525_093901`

Path:

```text
private/raw-results/linux/midpage_ptrmag_r0_smoke_20260525_093901
```

Purpose:

```text
Unsafe upper-bound smoke for pointer-only M4 local magazine pop, layered on
freefirst-tlslink and allocelide.
```

Result:

```text
main_r0:
  tlslink  103.29M
  ptrmag   104.80M
  tcmalloc 223.57M

mid_only_r0:
  tlslink  103.15M
  ptrmag   114.58M
  tcmalloc 222.91M
```

Decision:

```text
Pointer-only magazine pop does not close the gap. The missing performance is
not just page/slot validation inside M4 alloc pop.
```

### `midpage_absalloc_r0_smoke_20260525_094053`

Path:

```text
private/raw-results/linux/midpage_absalloc_r0_smoke_20260525_094053
```

Purpose:

```text
Diagnostic for trying MidPageFront before SmallFront in malloc routing on top
of freefirst-tlslink.
```

Result:

```text
main_r0:
  tlslink  100.25M
  absalloc 104.61M
  tcmalloc 220.93M

mid_only_r0:
  tlslink  107.19M
  absalloc 109.50M
  tcmalloc 225.91M

cross128_r0:
  tlslink   58.31M
  absalloc  62.14M
  tcmalloc  45.12M
```

Decision:

```text
Absolute MidPage alloc-first gives only small local gains. Keep as diagnostic;
it is not enough to justify changing the balanced routing policy.
```

### `midpage_regcache_r0_smoke_20260525_094151`

Path:

```text
private/raw-results/linux/midpage_regcache_r0_smoke_20260525_094151
```

Purpose:

```text
Diagnostic for adding MidPageFront TLS region lookup cache to freefirst-tlslink.
```

Decision:

```text
No-go. main_r0 and cross128_r0 regress versus tlslink, and mid_only_r0 is
neutral.
```

### `midpage_slotswitch_r0_smoke_20260525_094257`

Path:

```text
private/raw-results/linux/midpage_slotswitch_r0_smoke_20260525_094257
```

Purpose:

```text
Diagnostic for adding fixed-class slot-index dispatch to freefirst-tlslink.
```

Decision:

```text
No-go. Slot arithmetic is not the remaining local-r0 bottleneck.
```

### `midpage_m5hit_r0_smoke_20260525_101247`

Path:

```text
private/raw-results/linux/midpage_m5hit_r0_smoke_20260525_101247
```

Purpose:

```text
FrontCache-M5a smoke: move MidPage remote drain to miss/refill and use trusted
internal {ptr,page,slot} entries on M4 magazine hit. Performance runs keep
HZ5_PRELOAD_STATS unset.
```

Result:

```text
main_r0:
  tlslink  106.62M
  m5hit    107.10M
  tcmalloc 211.32M

mid_only_r0:
  tlslink  109.95M
  m5hit    109.74M
  tcmalloc 226.94M

cross128_r0:
  tlslink   58.96M
  m5hit     60.89M
  tcmalloc  44.59M
```

Decision:

```text
No-go for the local-r0 tcmalloc gap. Do not run a broad matrix unless a later
RouteTag/free-classifier experiment specifically needs this component.
```

### `midpage_superfast_r0_smoke_20260525_103116`

Raw output:

```text
private/raw-results/linux/midpage_superfast_r0_smoke_20260525_103116
private/raw-results/linux/midpage_superfast_perf_20260525_103138
```

Purpose:

```text
Unsafe physical upper-bound lane: combine M4 pointer magazine, alloc-state
elision, M5 hit-only, absolute MidPage malloc routing, and preload MidPage
fast bypass. Performance runs keep HZ5_PRELOAD_STATS unset.
```

Result:

```text
main_r0:
  tlslink   105.63M
  superfast 114.37M
  tcmalloc  221.12M

mid_only_r0:
  tlslink   105.72M
  superfast 117.66M
  tcmalloc  218.46M

cross128_r0:
  tlslink    61.26M
  superfast  58.43M
  tcmalloc   43.57M
```

Perf one-shot, mid_only_r0:

```text
tlslink:
  114.84M ops/s, 596.3M instructions, 123.0M branches

superfast:
  109.13M ops/s, 533.6M instructions, 107.3M branches

tcmalloc:
  201.69M ops/s, 248.4M instructions, 46.3M branches
```

Decision:

```text
No-go for the 150M local-r0 upper-bound bar. Keep as unsafe diagnostic only.
The remaining gap is not explained by the tested M4/M5 shortcut set.
```

### `midpage_direct_api_smoke_20260525_153359`

Raw output:

```text
private/raw-results/linux/midpage_direct_api_smoke_20260525_153359
private/raw-results/linux/midpage_direct_api_perf_20260525_153417
```

Purpose:

```text
Split the local-r0 gap between LD_PRELOAD/front dispatch and MidPageFront
internals by calling hz5_midpagefront_try_alloc/free directly.
```

Result:

```text
mid_only_r0:
  direct    122.13M
  superfast 119.51M
  tcmalloc  218.03M
```

Perf one-shot:

```text
direct:
  122.11M ops/s, 689.2M instructions, 155.5M branches

superfast:
  121.20M ops/s, 526.2M instructions, 107.0M branches

tcmalloc:
  210.11M ops/s, 261.6M instructions, 46.0M branches
```

Decision:

```text
direct ~= SuperFast. The remaining local-r0 gap is inside MidPageFront's
internal alloc/free representation, not primarily in the preload dispatch
layer.
```

## Older Results

The full chronological result log remains in:

```text
docs/archive/current_task_2026-05-hz5-linux.md
```

Use that archive for older Local2P, SmallFront, MidFront, LargeFront, OwnerHub,
and paper-main observations that have not yet been promoted into this index.
