# Current Task

## Active Goal

HZ5 Linux general allocator work is still targeting `tcmalloc`, but the current
MidPage bottleneck has moved from one safety check to class-mix cache topology.

Immediate focus:

```text
MidPageFront-M4 overflow-array diagnostic
```

Reason:

```text
M6 deferred-free was no-go, and same-class MidPage is already tcmalloc-class.
The large gap appears when random traffic crosses multiple MidPage classes.
M4 stats show many magazine-full events followed by local_pop refills,
especially in the 16K/32K classes. The next experiment should remove the
payload linked-list overflow path from that class-mix case.
```

Design doc:

```text
hakozuna-hz5/docs/HZ5_MIDPAGEFRONT_M6_DEFERRED_FREE_DESIGN.md
```

Implementation order:

```text
1. Record the tagged-free wrapper diagnostic as no-go. It wraps the same
   page_for_ptr lookup and does not represent a real route-tag table.
2. Add a MidPage M4 overflow array:
   primary magazine full -> TLS secondary array {page, slot, ptr} instead of
   writing node->page/node->next into user payload.
3. Refill primary magazine from the overflow array before local_pop/new slab.
4. Keep 64KiB slabs, class table, slot_state2, and M4 packet remote path
   unchanged so the result isolates overflow topology.
5. Measure direct range sweep and hakmem range sweep against the current
   superfast-freeelide baseline and tcmalloc.
```

M6 safety contract:

```text
raw_free quarantine:
  untrusted, never allocated from directly

validated magazine:
  trusted, alloc may pop after CACHE -> LIVE transition

invalid/double-free:
  may not be reported inside free(); it must fail closed before reuse during
  raw flush and must never be promoted to the validated magazine
```

Initial lane:

```text
--linux-hz5-general-midpage-m6-deferred-free-direct

compiled as:
  M4 magazine
  M4 remote packet
  M5 hit-only
  M6 deferred free
  no alloc/free state-elision unsafe flags
  no stats in speed lane
```

## M6 Deferred-Free Result

Raw output:

```text
private/raw-results/linux/midpage_m6defer_direct_smoke_20260525_160954
private/raw-results/linux/midpage_m6defer_cap64_direct_smoke_20260525_161054
private/raw-results/linux/midpage_m6batch_cap64_direct_smoke_20260525_161155
private/raw-results/linux/midpage_m6batch_perf_20260525_161204
```

RUNS=5, direct MidPage API, threads=8, iters=300000, ws=100,
size 2049..32768:

```text
M6 raw_cap=256, per-object flush:
  101.02M

M6 raw_cap=64, per-object flush:
  102.40M

M6 raw_cap=64, page-batch state update:
   79.30M

previous direct-freeelide:
  125.02M

tcmalloc reference from same mid_only shape:
  219.37M
```

Perf one-shot, M6 page-batch:

```text
79.55M ops/s
985.0M instructions
220.5M branches
441.4M cycles
```

Decision:

```text
M6 deferred-free is no-go for the tcmalloc local-r0 chase. Deferring free
validation does not reduce total path length in this workload; it delays reuse,
adds raw-buffer/refill pressure, and page-batch grouping is even more expensive
than the immediate-validation path.
```

Read:

```text
The remaining gap is not just fail-closed immediate validation timing. Direct
API + freeelide was already only ~125M, and M6 drops below that. Next design
should stop chasing deferred free and inspect the whole MidPage object/class
shape against tcmalloc/HZ4: class distribution, span packing, cache capacity,
and whether the direct benchmark is stressing random class misses rather than
same-class ThreadCache hits.
```

## Tagged-Free Wrapper Diagnostic

Raw output:

```text
private/raw-results/linux/midpage_tagfree_hakmem_range_sweep_20260525_163526
```

Comparison, hakmem range sweep:

```text
alloc    case          median_ops_s
base     mix_3_16k     133696274.15
base     mix_3_32k     123484326.37
base     mix_3_4k      161577252.51
base     mix_3_8k      150580563.36
base     r32768        172400801.12

tagfree  mix_3_16k     131676614.74
tagfree  mix_3_32k     119012877.44
tagfree  mix_3_4k      160765468.70
tagfree  mix_3_8k      141559622.52
tagfree  r32768        169033999.57

tcmalloc mix_3_16k     232260971.52
tcmalloc mix_3_32k     220255281.38
tcmalloc mix_3_4k      240582377.76
tcmalloc mix_3_8k      236736282.59
tcmalloc r32768        242252145.52
```

Decision:

```text
Tagged-free wrapper is no-go. It calls page_for_ptr before free_tagged and
therefore does not remove the actual ownership lookup. A real RouteTag/page-tag
table remains a separate future design, but this wrapper should not be promoted.
```

## M4 Overflow-Array Diagnostic

Raw output:

```text
private/raw-results/linux/midpage_overarray_direct_range_sweep_20260525_164100
```

RUNS=5, direct MidPage API, threads=8, iters=300000, ws=100:

```text
case          base        overarray
mix_2_4k     192.75M     173.89M
mix_3_8k     149.73M     144.85M
mix_4_16k    131.98M     120.70M
mix_5_32k    128.30M     120.50M
r32768       234.43M     227.21M
```

Decision:

```text
M4 overflow-array is no-go. Avoiding payload node overflow is not enough; the
extra TLS secondary array and branch cost outweigh the saved local_pop path.
This confirms that the main class-mix gap is not just magazine-full fallback.
```

Next read:

```text
Same-class and single-class variable ranges are already near tcmalloc-class.
The big gap appears when one benchmark stream alternates between multiple
MidPage size classes. The next useful diagnostic should measure class switch
pressure directly: per-class sequence patterns, grouped-by-class phases, and
whether a thin class-router/front-cache can avoid repeated cross-class miss
and refill churn.
```

## MidPage Class-Pattern Diagnostic

Raw output:

```text
private/raw-results/linux/midpage_pattern_direct_sweep_20260525_164427
```

RUNS=5, direct MidPage API, threads=8, iters=300000, ws=100:

```text
pattern  mix_2_4k   mix_3_8k   mix_4_16k  mix_5_32k
random   199.56M    134.21M    136.73M    127.15M
phase    177.70M    162.84M    148.30M    123.17M
cycle    185.46M    162.02M    142.42M    112.36M
```

Read:

```text
Class locality helps some 8K/16K mixes, but it does not close the 32K mixed
gap. The next upper-bound test should intentionally collapse 2049..32768 to
one 32K class. If that jumps to same-class speed, class dispersion is the
dominant issue; if it does not, the bottleneck is elsewhere.
```

## Wide32K Speed Upper Bound

Raw output:

```text
private/raw-results/linux/midpage_wide32k_direct_range_sweep_20260525_164606
private/raw-results/linux/midpage_wide32k_hakmem_range_sweep_20260525_164642
```

Direct MidPage API, RUNS=5:

```text
case          base        wide32k
mix_2_4k     182.86M     223.88M
mix_3_8k     164.86M     254.27M
mix_4_16k    126.67M     258.04M
mix_5_32k    131.02M     258.60M
r32768       232.23M     247.72M
```

Hakmem preload r0, RUNS=5:

```text
case          base        wide32k     tcmalloc
mix_2_4k     167.47M     198.53M     245.12M
mix_3_8k     143.23M     199.85M     235.59M
mix_4_16k    128.36M     197.03M     232.08M
mix_5_32k    121.27M     180.86M     225.99M
r32768       167.66M     204.28M     239.88M
```

Read:

```text
Wide32K proves the direct MidPage class-dispersion hypothesis: collapsing
2049..32768 to one class restores or exceeds tcmalloc-class direct throughput.
In full preload it still improves 1.2x-1.6x, but remains below tcmalloc,
meaning preload/free route and broader application-visible overhead remain.

This is not a final allocator profile because it over-allocates smaller mid
objects to 32K and will hurt RSS. It is a speed upper bound and points to the
next real design: reduce class dispersion without a single wasteful 32K class,
probably by coarser mid bands or adaptive class grouping.
```

## Coarse MidPage Bands

Raw output:

```text
private/raw-results/linux/midpage_coarse_band_direct_range_sweep_20260525_164849
private/raw-results/linux/midpage_band8_32_hakmem_range_sweep_20260525_164919
```

Direct MidPage API, RUNS=5:

```text
case        base     band4/16/32  band8/32  band16/32  wide32k
mix_2_4k   209.85M  193.97M      235.27M   223.61M    254.28M
mix_3_8k   150.60M  183.84M      200.88M   237.13M    242.68M
mix_4_16k  132.34M  153.99M      196.46M   237.82M    191.22M
mix_5_32k  111.69M  174.22M      203.07M   186.77M    268.51M
r32768     250.30M  226.71M      210.94M   247.34M    238.19M
```

Hakmem preload r0, RUNS=5:

```text
case        base     band8/32  wide32k  tcmalloc
mix_2_4k   171.62M  194.38M   198.55M  231.14M
mix_3_8k   148.65M  203.97M   202.00M  234.42M
mix_4_16k  127.02M  166.83M   181.07M  230.16M
mix_5_32k  123.61M  171.47M   195.28M  223.13M
r32768     170.06M  166.31M   196.31M  238.65M
```

Decision:

```text
Coarse classing is a real speed lever. band8/32 is the best balanced direct
candidate and gives large preload gains without collapsing every allocation to
32K. wide32k remains the speed upper bound and wins the broad 32K preload row,
but it is too wasteful for a default profile.

Next work should focus on turning this into an explicit profile family:
  strict: original classes, lower RSS
  coarse: 8K/32K bands, better mixed-mid speed
  wide32k: speed upper bound / diagnostic only

Then measure RSS/memory overhead before promoting any coarse profile.
```

## Class-Mix Finding

Raw output:

```text
private/raw-results/linux/midpage_direct_class_sweep_20260525_162333
private/raw-results/linux/midpage_direct_class_range_sweep_20260525_162444
private/raw-results/linux/hakmem_tcmalloc_range_sweep_20260525_162557
```

Direct MidPage API using the `superfast-freeelide` binary:

```text
fixed class:
  3072   239.21M
  4096   240.61M
  8192   220.88M
  16384  227.18M
  32768  224.80M

same-class variable-size ranges:
  2049..3072    200.37M
  3073..4096    236.13M
  4097..8192    234.03M
  8193..16384   226.36M
  16385..32768  228.08M

multi-class ranges:
  2049..4096    170.66M
  2049..8192    145.88M
  2049..16384   135.62M
  2049..32768   127.68M
```

Hakmem preload comparison, RUNS=3:

```text
tcmalloc:
  2049..4096    239.47M
  2049..8192    235.13M
  2049..16384   230.84M
  2049..32768   230.07M
  16385..32768  227.71M

HZ5 superfast-freeelide preload:
  2049..4096    162.86M
  2049..8192    151.10M
  2049..16384   132.90M
  2049..32768   120.67M
  16385..32768  168.25M
```

Decision:

```text
HZ5's same-class MidPage hit path is already tcmalloc-class in the direct API.
The large gap comes from multi-class random traffic and, in preload mode, some
remaining free/front routing cost. The next local-r0 experiment should tune
cache capacity/refill behavior for class mismatch, not more state elision.
```

Next experiment:

```text
MidPage M4 high-cap diagnostic:
  keep 64KiB slabs and current class table
  set all M4 magazine caps to 64
  compare direct class-mix and hakmem preload r0 vs current superfast-freeelide
```

## FlatCap / ClassIndex Follow-Up

Flat M4 magazine cap result:

```text
private/raw-results/linux/midpage_flatcap_direct_range_sweep_20260525_162858
```

Direct MidPage API, RUNS=5:

```text
flatcap:
  2049..4096    132.96M
  2049..8192    112.91M
  2049..16384   109.37M
  2049..32768   108.35M
  16385..32768  173.73M
```

Decision:

```text
flatcap is no-go. Increasing every class magazine to 64 worsens random ranges
and same-class upper-mid. The problem is not simply "cap too small"; a larger
front cache increases footprint/pressure without solving the class-mix path.
```

M4 stats lane:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-m4stats
```

Observation output:

```text
private/raw-results/linux/midpage_m4stats_direct_20260525_163131
```

Key stats, direct mix 2049..32768, iters=100000:

```text
class 16384:
  alloc_call=213373
  mag_hit=213373
  refill_call=329
  refill_local_pop=1857
  refill_new_page=56
  mag_full=1953

class 32768:
  alloc_call=426699
  mag_hit=426699
  refill_call=4317
  refill_local_pop=31777
  refill_new_page=163
  mag_full=32039
```

Read:

```text
allocs still hit the magazine, but wide random mix causes many 32K local-list
overflows/refills. That contributes, but the count is not large enough to
explain the whole fixed-class vs random-class gap by itself.
```

Class-index if-tree:

```text
Changed hz5_midpagefront_class_index from a linear class loop to an equivalent
fixed threshold tree.
```

Direct result:

```text
private/raw-results/linux/midpage_classindex_direct_range_sweep_20260525_163412

2049..4096    186.68M  (improves from 170.66M)
2049..8192    144.19M  (neutral)
2049..16384   121.70M  (worse/noise vs 135.62M)
2049..32768   125.72M  (neutral vs 127.68M)
16385..32768  223.29M  (same-class remains high)
```

Slotswitch recheck on top of class-index + superfast-freeelide:

```text
private/raw-results/linux/midpage_classindex_slotswitch_direct_range_sweep_20260525_163514

2049..4096    155.94M
2049..8192    124.97M
2049..16384   111.01M
2049..32768   104.52M
16385..32768  214.52M
```

Decision:

```text
slot-index switch/shift remains no-go in this direct/freeelide context. Keep
the class-index threshold tree because it is semantic-preserving and helps the
smallest mixed range, but do not pursue slotswitch.
```

Next read:

```text
HZ5 can match tcmalloc on same-class direct MidPage, but not on random class
mix. tcmalloc is stable across ranges, so its advantage is likely a broader
front-cache transfer/cache policy and/or free route classifier that handles
mixed classes without per-object page/slot work becoming unpredictable.
```

## M5a Result

Raw output:

```text
private/raw-results/linux/midpage_m5hit_r0_smoke_20260525_101247
```

RUNS=5, threads=8, HZ5_PRELOAD_STATS unset:

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
M5a hit-only cache is no-go for the local-r0 tcmalloc gap. Moving remote drain
to miss/refill and trusting internal magazine entries does not move mid_only_r0.
Do not proceed to broad r50/r90 matrix for M5a.
```

Read:

```text
The next r0 problem is not M4 hit validation or remote-drain-on-hit. RouteTag
can still help free classification/cross-front rows, but it is unlikely to
solve mid_only_r0 by itself. The remaining r0 gap likely needs a thinner
front ABI or a different slot-state/cache representation.
```

## SuperFast Upper-Bound Lane

Lane:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast
```

Purpose:

```text
Find the physical local-r0 ceiling before spending more work on safety-shaped
M5/RouteTag designs.
```

Composition:

```text
base:
  m4packet-freefirst-tlslink

extra:
  BENCHLAB_HZ5_PRELOAD_MIDPAGE_SUPERFAST=1
  BENCHLAB_HZ5_PRELOAD_MIDPAGE_ALLOC_ABS_FIRST=1
  BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY=1
  BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE=1
  BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG=1
```

Safety status:

```text
unsafe diagnostic only
not paper/reportable
not remote/RSS evidence
do not use for invalid/double-free claims
```

What it removes from the hot path:

```text
MidPage successful malloc/free bypasses preload stat accounting and generic
front routing. MidPage magazine pop may return the cached pointer without the
normal CACHE->LIVE transition.
```

Go/no-go:

```text
GO:
  mid_only_r0 >= 150M or tlslink +35%

STRONG:
  mid_only_r0 >= 170M and instructions/op materially closer to tcmalloc

NO-GO:
  mid_only_r0 < 150M
  or main_r0/cross128_r0 regress enough to make the lane non-informative
```

## SuperFast Result

Raw output:

```text
private/raw-results/linux/midpage_superfast_r0_smoke_20260525_103116
private/raw-results/linux/midpage_superfast_perf_20260525_103138
```

RUNS=5, threads=8, HZ5_PRELOAD_STATS unset:

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
SuperFast is no-go for the 150M upper-bound bar. It removes some instructions
and branches, but local-r0 is still roughly 2x tcmalloc in instructions and
branches. The remaining gap is not explained by preload stat calls,
remote-drain-on-hit, pointer-only magazine pop, or M4 CACHE->LIVE transition.
```

Next read:

```text
Stop combining M4/M5 toggles. The next tcmalloc-chase design needs a deeper
front-path representation change: route-tag/free classifier plus a genuinely
thin thread-cache ABI, or a tcmalloc-like batch pointer cache that moves more
state work to refill/drain rather than the per-object path.
```

## Next Cut: Direct MidPage API Bench

Question:

```text
Is the remaining local-r0 gap mostly in LD_PRELOAD/front dispatch, or inside
MidPageFront itself?
```

Plan:

```text
1. Add a standalone direct benchmark that calls:
     hz5_midpagefront_try_alloc(size, 16, &ptr)
     hz5_midpagefront_free(ptr)

2. Match the hakmem mid_only shape:
     threads=8
     iters=300000
     ws=100
     min_size=2049
     max_size=32768
     remote_pct=0

3. Compare:
     direct MidPage API
     SuperFast preload
     tcmalloc preload
```

Interpretation:

```text
direct >> SuperFast:
  preload/free classification is still the main problem.

direct ~= SuperFast:
  MidPage internal state/cache representation is the main problem.

direct still < 150M:
  stop preload-focused work and redesign MidPage front-cache internals.
```

## Direct MidPage API Result

Raw output:

```text
private/raw-results/linux/midpage_direct_api_smoke_20260525_153359
private/raw-results/linux/midpage_direct_api_perf_20260525_153417
```

RUNS=5, threads=8, mid_only_r0 shape:

```text
direct:
  122.13M

superfast:
  119.51M

tcmalloc:
  218.03M
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
direct ~= SuperFast. The preload/front dispatch layer is not the main local-r0
limit for mid_only. MidPageFront's internal alloc/free representation is the
main problem.
```

Next design direction:

```text
Stop preload-first work for this gap. Build a new MidPage internal upper-bound:
per-thread pointer-array/bin cache with batched refill/drain, and move slot
state work out of the per-object hit path as much as the safety model allows.
```

## SuperFast-FreeElide Probe

Question:

```text
How much of the remaining MidPage local-r0 gap is owner-local free-side
slot-state transition/check cost?
```

Lane:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast-freeelide
```

Composition:

```text
superfast
+ BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE=1
```

Safety status:

```text
unsafe diagnostic only
not safety-preserving
not remote/RSS/paper evidence
```

Interpretation:

```text
freeelide >> superfast:
  free-side state check is still a large part of the gap.

freeelide ~= superfast:
  page lookup / slot index / cache topology / benchmark shape dominate.
```

## SuperFast-FreeElide Result

Raw output:

```text
private/raw-results/linux/midpage_freeelide_smoke_20260525_153939
private/raw-results/linux/midpage_freeelide_perf_20260525_154001
```

RUNS=5, threads=8, mid_only_r0 shape:

```text
superfast:
  118.38M

freeelide:
  121.59M

direct-freeelide:
  125.02M

tcmalloc:
  219.37M
```

Perf one-shot:

```text
freeelide:
  124.49M ops/s, 493.8M instructions, 101.5M branches

tcmalloc:
  212.40M ops/s, 256.3M instructions, 47.3M branches
```

Decision:

```text
freeelide ~= superfast. Owner-local free-side slot-state check is not the main
remaining gap. Even after skipping it, HZ5 still has about 1.9x tcmalloc's
instruction count and 2.1x branch count.
```

Next read:

```text
The tcmalloc gap is now narrowed to MidPage topology: page lookup, slot index,
class/bin routing, and per-object cache representation. The next experiment
should not be another state-elision toggle; it needs a different thread-cache
shape.
```

## Branch

```text
codex/hz5-linux-p43-port
```

Recent commits:

```text
5f5c9ff Add MidPageFront freefirst tlslink diagnostic
5d2f414 Record MidPage route matrix results
b13000b Add MidPageFront routefree diagnostic
32b0777 Document HZ5 Linux lane combinations
24e0631 Add MidPageFront M4 freefirst lane
```

## Current Result: Freefirst TLS/Link Matrix

Raw output:

```text
private/raw-results/linux/midpage_freefirst_tlslink_matrix_r5_20260525_093316
```

RUNS=5, threads=8, HZ5_PRELOAD_STATS unset:

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

Read:

```text
tlslink is a strong local/r50 candidate and pushes main/mid_only r0 over 100M,
but r90 still prefers freefirst in main/mid_only/cross128. Do not replace
freefirst as balanced default yet.
```

## Next Diagnostic

```text
MidPageFront / preload path-length upper-bound diagnostics.
```

Purpose:

```text
Measure whether the remaining local-r0 gap is dominated by isolated fixed
costs in the M4 magazine and preload routing path.
```

Rules:

```text
1. Do not mix counters into raw speed runs.
2. Keep unsafe lanes out of candidate/reporting rows.
3. Compare against freefirst-tlslink and tcmalloc first on mid_only/main r0.
4. Treat no-go diagnostics as evidence for the next structural design.
```

Results:

```text
allocelide r0 smoke:
  private/raw-results/linux/midpage_allocelide_r0_smoke_20260525_093736

  main:
    tlslink    105.97M
    allocelide 108.76M
    tcmalloc   220.60M

  mid_only:
    tlslink    100.81M
    allocelide 114.85M
    tcmalloc   225.86M

ptrmag r0 smoke:
  private/raw-results/linux/midpage_ptrmag_r0_smoke_20260525_093901

  main:
    tlslink    103.29M
    ptrmag     104.80M
    tcmalloc   223.57M

  mid_only:
    tlslink    103.15M
    ptrmag     114.58M
    tcmalloc   222.91M

absalloc r0 smoke:
  private/raw-results/linux/midpage_absalloc_r0_smoke_20260525_094053

  main:
    tlslink    100.25M
    absalloc   104.61M
    tcmalloc   220.93M

  mid_only:
    tlslink    107.19M
    absalloc   109.50M
    tcmalloc   225.91M

regcache r0 smoke:
  private/raw-results/linux/midpage_regcache_r0_smoke_20260525_094151

  main:
    tlslink    106.23M
    regcache    97.85M
    tcmalloc   221.30M

  mid_only:
    tlslink    103.86M
    regcache   103.72M
    tcmalloc   224.91M

slotswitch r0 smoke:
  private/raw-results/linux/midpage_slotswitch_r0_smoke_20260525_094257

  main:
    tlslink    102.55M
    slotswitch  97.22M
    tcmalloc   222.33M

  mid_only:
    tlslink    100.71M
    slotswitch 100.00M
    tcmalloc   229.08M
```

Read:

```text
M4 alloc transition, pointer-only magazine pop, SmallFront-before-MidPage
malloc routing, TLS region cache, and slot-index switch each explain only a
small slice. The remaining tcmalloc gap is structural path length: interposer
routing plus descriptor-owned free classification versus tcmalloc's compact
front cache.
```

## Cleanup Pass

Current cleanup target:

```text
Lane and source-code organization before the next remote/cross-size design.
```

Actions:

```text
1. Keep preset names stable for reproducibility.
2. Consolidate repeated build-script preset bodies into helper functions.
3. Record canonical / diagnostic / archive lane status in:
   hakozuna-hz5/docs/HZ5_LINUX_LANE_CLEANUP.md
4. Do not split MidPageFront into multiple translation units yet; the diagnostic
   flag matrix is still moving.
```

Validation:

```text
bash -n linux/build_linux_hz5_standalone.sh
--linux-hz5-general-midpage-region-shadow-allocfirst build OK
--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache-stats build OK
--linux-hz5-general-region-outbox build OK
short MidPage malloc smoke OK
```

## Design Check Before Pro Consultation

Raw output:

```text
private/raw-results/linux/midpage_design_check_20260525_073659
```

Size histogram:

```text
mid_only allocation distribution:
  2304..3072      21,190
  3073..4096      21,396
  4097..8192      85,519
  8193..16384    170,952
  16385..32768   342,543

main allocation distribution:
  <=2048          20,042
  2049..32768   301,179
  32769..65536  320,399

cross128 allocation distribution:
  <=2048           9,992
  2049..32768    150,498
  32769..65536   160,700
  >65536         320,430
```

Unsafe local upper-bound:

```text
mid_only_r0:
  allocfirst   72.83M
  localunsafe  76.79M
  tcmalloc    147.13M
```

Read:

```text
Skipping owner-local MidPage bitmap checks gives only about +5%.
The tcmalloc gap is not primarily local active/free bitmap validation.
```

Alloc+free MidPage-first dispatch:

```text
mid_only_r0:
  allocfirst      69.02M
  allocfreefirst  67.02M
  tcmalloc       139.85M

mid_only_r90:
  allocfirst      31.50M
  allocfreefirst  26.17M
  tcmalloc        47.02M

main_r90:
  allocfirst      25.35M
  allocfreefirst  29.39M
  tcmalloc        19.90M

cross128_r90:
  allocfirst      14.67M
  allocfreefirst  14.71M
  tcmalloc         7.80M
```

Read:

```text
MidPage-first free dispatch can help main_r90, but it hurts mid_only.
Dispatch order alone is not the missing tcmalloc-class local design.
```

Perf note:

```text
perf record/report is noisy but shows MidPage r0 still paying substantial
preload ownership dispatch, including smallfront_page_for_ptr, and MidPage
try_alloc/free. tcmalloc still has much lower instruction/branch footprint in
perf-stat runs.
```

## Read First

```text
hakozuna-hz5/docs/HZ5_LINUX_STATUS.md
hakozuna-hz5/docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
hakozuna-hz5/docs/HZ5_BENCH_RESULTS_INDEX.md
```

Full chronological log was archived here:

```text
hakozuna-hz5/docs/archive/current_task_2026-05-hz5-linux.md
```

## Current Lead Lanes

```text
--linux-hz5-general-midpage-region-shadow
  current tcmalloc-chase lead for general ordinary malloc rows

--linux-hz5-general-midpage-region
  stable MidPageFront-M2.2 remote-heavy mid-size candidate

--linux-hz5-local2p-linkflags
  exact 64K/a8192 appendix/local speed profile

--linux-hz5-local2p-rssretain2048tls
  exact retained-RSS profile

--linux-hz5-local2p-remotebatch
  exact producer/consumer remote-free profile
```

## Diagnostic / No-Go

```text
--linux-hz5-general-midpage-region-frontfirst
  helps pure mid_only r90 in one check, but regresses main/cross128/guard

--linux-hz5-general-midpage-region-shadow-frontfirst
  helps pure mid_only slightly, but hurts main/cross128

--linux-hz5-general-midpage-region-shadow-tlscache
  no-go; region lookup cache does not close the tcmalloc gap

--linux-hz5-general-midpage-region-shadow-hotslot
  no-go; one-entry TLS hot object cache does not improve MidPageFront local path

--linux-hz5-general-midpage-region-shadow-activetrust
  diagnostic only; improves r0 slightly but hurts r90

--linux-hz5-general-midpage-region-shadow-allocfirst
  promising diagnostic; removes duplicate MidPageFront class lookup in preload
  via explicit MidPageFront try-alloc dispatch

--linux-hz5-general-midpage-region-shadow-slotswitch
  no-go; removes variable slot-index division but regresses r90 and does not
  improve r0

--linux-hz5-general-midpage-region-shadow-tlslink
  promising diagnostic; preload-wide initial-exec TLS and speed link flags
  improve main/mid, but cross128_r90 regresses versus allocfirst

--linux-hz5-general-midpage-region-shadow-linkonly
  diagnostic only; speed link flags without initial-exec TLS looked promising
  in r3 but r7 does not beat allocfirst on main_r90/mid_only_r90

--linux-hz5-general-midpage-region-shadow-tlsie
  diagnostic split; initial-exec TLS only helps mid_only_r0 but is weaker on
  remote/cross rows

--linux-hz5-general-midpage-region-shadow-nodeless
  diagnostic only so far; M3 nodeless local page-run cache improves mid_only_r0
  slightly, but remote rows regress
```

## Latest Key Results

```text
private/raw-results/linux/tcmalloc_target_shadow_r3_20260525_033348
private/raw-results/linux/tcmalloc_shadow_frontfirst_r3_20260525_033543
private/raw-results/linux/tcmalloc_tlscache_r3_20260525_033858
private/raw-results/linux/midpage_hotslot_smoke_r3_20260525_052514
private/raw-results/linux/midpage_activetrust_smoke_r3_20260525_052755
private/raw-results/linux/midpage_allocfirst_r3_20260525_053010
private/raw-results/linux/midpage_allocfirst_r90_verify_r5_20260525_053037
private/raw-results/linux/midpage_allocfirst_tryalloc_r3_20260525_054204
private/raw-results/linux/midpage_slotswitch_r3_20260525_054521
private/raw-results/linux/midpage_tlslink_r3_20260525_054807
private/raw-results/linux/midpage_tlslink_broad_r3_20260525_054859
private/raw-results/linux/midpage_tlslink_cross128_verify_r5_20260525_055003
private/raw-results/linux/midpage_tls_split_r3_20260525_055155
private/raw-results/linux/midpage_linkonly_broad_r3_20260525_055224
private/raw-results/linux/midpage_linkonly_verify_r7_20260525_055344
private/raw-results/linux/midpage_nodeless_r3_20260525_065654
private/raw-results/linux/midpage_perf_allocfirst_nodeless_20260525_070623
```

Current read:

```text
shadow:
  real tcmalloc-chase candidate

frontfirst:
  dispatch order is not the main bottleneck

tlscache:
  region lookup is not the main bottleneck

hotslot:
  one-entry local object bypass is not enough and hurts r90

activetrust:
  alloc-side remote-bit check is not the main bottleneck; r90 becomes unstable

allocfirst:
  duplicate class lookup matters for mid_only r0
  try-alloc cleanup keeps r0 ahead of shadow and r90 near/slightly ahead

slotswitch:
  slot-index division is not the main bottleneck; fixed-class switch removes
  div but loses to allocfirst

tlslink:
  TLS/linkage overhead is real; removes MidPageFront hot-path __tls_get_addr
  and improves main/mid. cross128_r90 still beats tcmalloc but regresses versus
  allocfirst, so do not make it the broad default yet.

tls split:
  linkonly is not promoted after r7. initial-exec TLS helps r0, link flags can
  help short runs, but neither is a broad tcmalloc-chase lead yet.

nodeless:
  first M3 implementation validates the hypothesis only weakly. It removes
  local node->page/node->next traffic and improves mid_only_r0 by ~5%, but
  remote rows regress; remote drain / partial page handling needs redesign
  before promotion.

perf:
  nodeless reduces cache misses but increases branches/instructions, especially
  on r90. The remaining problem is control-flow/state management, not payload
  cache traffic alone.

stats:
  nodeless stats-only run shows very high refill/partial churn even on r0.
  A single current_page[class] is too small for the random mid workload and
  repeatedly round-trips pages through the partial list.

ptrcache:
  per-class TLS pointer cache fixes most r0 partial/refill churn and gives a
  small r0 gain. It is still diagnostic only: mid_only_r90 and cross128_r90
  remain weaker than allocfirst, while main_r90 can improve.

next:
  move to MidPageFront-M4. The next useful design is a descriptor-owned
  owner-local magazine with canonical slot state, followed by page+bitmask
  remote packets if M4a improves local r0.
```

## Next Step

Implement MidPageFront-M4 as a diagnostic lane. Keep M2/M3 lanes available for
reproduction, but do not promote them.

```text
--linux-hz5-general-midpage-region-shadow-m4mag
```

Design intent:

```text
Keep HZ4's page/header-owned and owner-aware remote principles.
Borrow tcmalloc's per-class front-cache shape.
Keep HZ5 fail-closed descriptor ownership.
Do not mutate Windows P43i/P45 or Local2P.
```

M4a scope:

```text
64KiB slab unchanged
class table unchanged: 3072, 4096, 8192, 16384, 32768
magazine entry: { page, slot }
slot_state2 canonical states: LIVE/CACHE/REMOTE/DEAD
alloc pop: CACHE -> LIVE, no page lookup or slot division
local free: validate ptr -> page -> slot, LIVE -> CACHE, push magazine
remote path: existing object-list handoff is allowed for first M4a smoke
```

Acceptance:

```text
Keep if mid_only_r0 >= 90M or allocfirst best +15%.
No-go if mid_only_r90/main_r90 regress more than 8% or cross128_r90 more than
10%.
Do not mix speed runs with runtime stats/counters.
```

First M4a smoke:

```text
private/raw-results/linux/midpage_m4mag_smoke_20260525_080112
private/raw-results/linux/midpage_m4mag_localfast_smoke_20260525_080203
private/raw-results/linux/midpage_m4mag_broad_smoke_20260525_080228
private/raw-results/linux/midpage_m4mag_attrib_20260525_080241
```

Result:

```text
mid_only_r0:
  allocfirst 93.53M
  m4mag      90.92M
  tcmalloc  230.97M

mid_only_r90:
  allocfirst 23.83M
  m4mag      37.22M
  tcmalloc   26.23M

main_r90:
  allocfirst 23.65M
  m4mag      39.36M
  tcmalloc   29.96M

cross128_r90:
  allocfirst 17.33M
  m4mag      14.98M
  tcmalloc   14.65M
```

Read:

```text
M4 magazine is not yet the local r0 tcmalloc chase answer. It improves
remote-heavy mid/main rows strongly, but cross128 regresses versus allocfirst.
The next M4 work should either add page+bitmask remote packets to remove
object-list remote payloads, or stop local-r0 chasing here and inspect
free-route/classifier fixed cost.
```

First M4b page-packet smoke:

```text
private/raw-results/linux/midpage_m4packet_smoke_20260525_082809
private/raw-results/linux/midpage_m4packet_attrib_20260525_082826
```

Result:

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

Read:

```text
M4b page-descriptor packet is a real remote-heavy candidate. It fixes the M4a
cross128_r90 regression in the short smoke and beats tcmalloc on mid_only_r90,
main_r90, and cross128_r90. It still does not address local r0.
```

M4b gated-drain verify:

```text
private/raw-results/linux/midpage_m4packet_gated_smoke_20260525_084328
private/raw-results/linux/midpage_m4packet_gated_verify_r5_20260525_084409
private/raw-results/linux/midpage_m4packet_gated_attrib_20260525_084433
```

Result:

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

Read:

```text
Gated M4b is a strong mid/main remote-heavy candidate and beats tcmalloc on
main/mid r50/r90. It is not a local-r0 answer. It is not a broad cross128_r90
default yet because allocfirst remains better there.
```

M4b cross-drain diagnostic:

```text
private/raw-results/linux/midpage_m4packet_crossdrain_smoke_20260525_085453
private/raw-results/linux/midpage_m4packet_crossdrain_r0_smoke_20260525_085539
private/raw-results/linux/midpage_m4packet_crossdrain_attrib_20260525_085513
```

Change:

```text
Add a separate m4packet-crossdrain lane.
Remote packet publish sets a MidPageFront owner/class pending mask.
SmallFront/MidFront/LargeFront alloc miss can opportunistically drain a small
MidPageFront budget without enabling generic OwnerHub-R2/R3.
```

Result:

```text
main r90:
  m4packet    11.02M
  crossdrain  11.52M
  tcmalloc    11.83M

mid_only r90:
  m4packet    11.23M
  crossdrain  12.79M
  tcmalloc    11.92M

cross128 r90:
  m4packet     7.11M
  crossdrain   5.88M
  tcmalloc    12.70M

r0:
  m4packet and crossdrain are similar; both remain far below tcmalloc.
```

Decision:

```text
Cross-drain is useful as a diagnostic signal for mid_only/main r90, but it is
not a broad default. It worsens cross128 r50/r90, likely because other-front
alloc miss drain adds fixed work while cross128 is dominated by cross-size
traffic. Keep m4packet as the better current candidate and treat cross-drain as
a no-promote lane.
```

M4packet free-dispatch A/B:

```text
private/raw-results/linux/midpage_m4packet_freefirst_smoke_20260525_085727
private/raw-results/linux/midpage_m4packet_freefirst_attrib_20260525_085745
```

Result:

```text
main r0:
  m4packet   33.09M
  freefirst  36.33M
  tcmalloc  117.42M

mid_only r0:
  m4packet   35.33M
  freefirst  37.66M
  tcmalloc  102.98M

cross128 r0:
  m4packet   19.24M
  freefirst  19.60M
  tcmalloc   60.62M

main r90:
  m4packet   11.50M
  freefirst  11.73M
  tcmalloc   10.50M

cross128 r90:
  m4packet    6.30M
  freefirst   6.76M
  tcmalloc   10.83M
```

Decision:

```text
MidPageFront-first free dispatch is a cleaner candidate than cross-drain. It
improves local r0 and slightly improves main/cross128 r90 without attribution
regression. It still leaves a large tcmalloc local-r0 gap, so this is an
incremental lane, not the final answer.
```

Latest M3 stats:

```text
private/raw-results/linux/midpage_nodeless_stats_20260525_070957

mid_only_r0:
  refill=463246
  refill_partial_hit=461678
  refill_remote_hit=0
  refill_new_page=1568
  partial_push=463165
  remote_drained=0

mid_only_r90:
  refill=321442
  refill_partial_hit=293643
  refill_remote_hit=8051
  refill_new_page=19748
  partial_push=305066
  remote_drained=525393
```

Latest M3.2 ptrcache:

```text
private/raw-results/linux/midpage_nodeless_ptrcache_r3_20260525_071403

mid_only_r0:
  allocfirst 70.63M
  ptrcache   76.48M
  tcmalloc  139.49M

mid_only_r90:
  allocfirst 35.31M
  ptrcache   25.82M
  tcmalloc   41.78M

main_r90:
  allocfirst 25.23M
  ptrcache   27.15M
  tcmalloc   21.72M

cross128_r90:
  allocfirst 21.58M
  ptrcache   10.26M
  tcmalloc    7.88M
```

## Operating Rules

```text
Do not change Windows P43i/P45 behavior.
Do not promote diagnostic lanes without broad matrix evidence.
Do not count unsupported or libc-passthrough routes as HZ5 wins.
Keep detailed run output under private/raw-results/linux/.
Update HZ5_BENCH_RESULTS_INDEX.md after meaningful measurements.
```

## Lane Combination Cleanup

New source of truth:

```text
hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
```

Current MidPage comparison set:

```text
allocfirst:
  local-r0 comparison baseline

m4packet:
  current remote-heavy candidate

m4packet-freefirst:
  cleaner incremental candidate for next matrix

routefree:
  candidate-watch; improves local r0/cross128 r0 but not cross128 r90

tcmalloc:
  target allocator
```

No-promote:

```text
m4packet-crossdrain:
  improves MidPage-heavy r90 rows but hurts cross128 r50/r90

localunsafe:
  unsafe upper bound only

nodeless-stats / ptrcache-stats:
  observation counters only; not speed lanes

OwnerHub-R2/R3:
  historical generic cross-front drain experiments with fixed-cost failures
```

FreeRoute-C1 smoke:

```text
private/raw-results/linux/midpage_m4packet_routefree_smoke_20260525_090830
private/raw-results/linux/midpage_m4packet_routefree_attrib_20260525_090850
```

Result:

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

Decision:

```text
Routefree proves free-route ordering still matters, especially local r0 and
large-heavy cross128 r0. It is not the broad answer because cross128 r90 is
below freefirst. Keep it as candidate-watch and run the next RUNS=5 matrix with
allocfirst / m4packet / freefirst / routefree / tcmalloc.
```

RUNS=5 MidPage route matrix:

```text
private/raw-results/linux/midpage_route_matrix_r5_20260525_091054
private/raw-results/linux/midpage_route_matrix_attrib_20260525_091125
```

Result:

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

Decision:

```text
Freefirst is the balanced MidPage lead. Routefree is only a candidate-watch for
mid_only r90. The local-r0 and cross128 gaps to tcmalloc are structural; free
dispatch order is not enough.
```

Perf / worker-audit phase:

```text
private/raw-results/linux/perf_midpage_freefirst_r0_20260525_091532
private/raw-results/linux/perf_midpage_freefirst_tlslink_20260525_091619
private/raw-results/linux/perf_midpage_freefirst_tlslink_record_20260525_091650
```

Read:

```text
mid_only r0 perf stat:
  freefirst cycles 108.72M, instructions 223.67M, branches 48.79M
  tcmalloc   cycles  52.27M, instructions  82.64M, branches 14.59M

perf report:
  freefirst shows visible __tls_get_addr cost plus
  hz5_midpagefront_try_alloc/free hot samples.

freefirst -> tlslink:
  median ops/s 33.51M -> 38.97M
  instructions 221.28M -> 178.47M
  branches 48.83M -> 37.03M
```

Decision:

```text
Add m4packet-freefirst-tlslink as candidate-watch. TLS/linkage overhead is a
real component. It is not enough alone; after broad verification, the next
structural experiment should test descriptor slot_state2 transition cost or a
pointer-only local magazine upper bound.
```
