# HZ5 FrontCache-M5 / RouteTag-R1 Design

## Goal

Close the remaining Linux ordinary-malloc gap to tcmalloc without giving up
HZ5's fail-closed ownership model.

Recent measurements show the local-r0 gap is structural:

```text
mid_only r0:
  HZ5 m4packet-freefirst-tlslink ~107M ops/s
  tcmalloc                       ~226M ops/s

perf snapshot:
  HZ5 tlslink  ~909M instructions, ~187M branches
  tcmalloc     ~359M instructions,  ~64M branches
```

Local diagnostics that only remove one fixed cost did not close the gap:

```text
allocelide:
  modest gain only

ptrmag:
  modest gain only

absalloc:
  small gain only

regcache:
  no-go / neutral

slotswitch:
  no-go
```

Read:

```text
The remaining cost is not one atomic/state transition, slot arithmetic, or a
small route-order tweak. The hot path is still too branch/instruction heavy.
```

## Direction

Use a hybrid of HZ4 and tcmalloc ideas:

```text
From HZ4:
  one-shot free route classification
  page/tag ownership routing
  owner-aware remote handoff

From tcmalloc:
  thin per-thread size-class front cache
  cache hit does not touch middle-end/remote state
  refill/drain happens on miss

From HZ5:
  fail-closed route ownership
  slot-state transitions
  no invalid/wrong-route fallback wins
```

## Phase M5a: MidPage Hit-Only Front Cache

M5a is the first implementation step. It does not add a full RouteTag table
yet. It changes only the MidPage M4 hit contract.

Lane:

```text
BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY=1
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-m5hit
```

M5a changes:

```text
1. M4 magazine entries carry {ptr,page,slot}.
2. Alloc hit does not check remote packet inbox first.
3. Alloc hit trusts the internal magazine entry and skips page magic/class
   validation.
4. Alloc hit still performs CACHE -> LIVE slot-state transition.
5. Remote drain stays in the refill/miss path.
```

M5a keeps:

```text
64KiB slab geometry
current class table
M4 remote packet representation
free-side ptr -> page -> slot validation
LIVE -> CACHE and LIVE -> REMOTE transitions
```

This keeps double-free-before-reuse detection:

```text
first free:
  LIVE -> CACHE

second free before reuse:
  expected LIVE, actual CACHE
  fail closed
```

## Later Phase R1: RouteTag

RouteTag-R1 should replace the preload free trial chain with one route lookup:

```text
current:
  free -> MidPage try -> Small try -> MidFront try -> Large try -> fallback

R1:
  free -> route_lookup(ptr) -> switch(kind) -> free_tagged
```

First scope should be MidPage-only:

```text
MidPage slab create:
  register 16 route entries for the 64KiB slab at 4KiB granularity

free:
  if route kind is MIDPAGE:
    call hz5_midpagefront_free_tagged(ptr, tag)
  else:
    existing chain
```

Do not start with Small/MidFront/Large all at once. RouteTag must be proven
with MidPage first.

## No-Go

M5a no-go:

```text
mid_only_r0 < 135M
or improvement over tlslink < 25%
or main_r0 / cross128_r0 regress materially
```

Remote no-go:

```text
mid_only_r90 worse than freefirst by >8%
main_r90 worse than freefirst by >8%
cross128_r90 worse than freefirst by >10%
```

Safety no-go:

```text
foreign pointer treated as HZ5-owned
wrong-route pointer treated as OK
double-free-before-reuse not detected
same slot exists in local cache and remote packet
remote free can claim CACHE slot as REMOTE
```

## Current Recommendation

```text
Step 1:
  implement M5a hit-only MidPage front cache

Step 2:
  run local r0 smoke against tlslink and tcmalloc

Step 3:
  if M5a clears 135M mid_only_r0, run r0/r50/r90 matrix

Step 4:
  implement MidPage-only RouteTag-R1
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
M5a is no-go for the local-r0 tcmalloc gap. Hit-only cache and trusted
{ptr,page,slot} entries do not materially improve main/mid_only r0.
```

Implication:

```text
The current r0 bottleneck is not remote-drain-on-hit or rich magazine-entry
validation. RouteTag-R1 may still help free classification and cross-front
workloads, but it should not be expected to solve mid_only_r0 alone.
```

## SuperFast Upper-Bound Diagnostic

M5a did not move the local-r0 ceiling. Before adding a full RouteTag table or a
new descriptor state representation, use one deliberately unsafe upper-bound
lane:

```text
--linux-hz5-general-midpage-region-shadow-m4packet-freefirst-tlslink-superfast
```

Compile-time composition:

```text
BENCHLAB_HZ5_PRELOAD_MIDPAGE_SUPERFAST=1
BENCHLAB_HZ5_PRELOAD_MIDPAGE_ALLOC_ABS_FIRST=1
BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY=1
BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE=1
BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG=1
```

This lane is not safety preserving. It is only an upper-bound diagnostic for
the MidPage local-r0 path. It bypasses preload accounting on successful MidPage
malloc/free and allows pointer-only magazine pop without the normal
`CACHE -> LIVE` transition.

Decision rule:

```text
GO:
  mid_only_r0 >= 150M or tlslink +35%

NO-GO:
  mid_only_r0 < 150M
```

If SuperFast is still far from tcmalloc, then the remaining gap is not explained
by MidPage M4 validation, remote-drain-on-hit, preload stat calls, or
pointer-only magazine entries. The next design should be a deeper front-path
rewrite rather than more M4/M5 toggles.

### SuperFast Result

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

Decision:

```text
No-go. SuperFast does not clear the 150M upper-bound bar. It reduces
instructions and branches versus tlslink in the perf one-shot, but remains
around 2x tcmalloc's instruction and branch count on mid_only_r0.
```

Implication:

```text
The remaining tcmalloc gap is deeper than the tested M4/M5 toggles. Do not keep
stacking unsafe shortcuts. Move to a real front-path representation change.
```

### Direct MidPage API Result

Raw output:

```text
private/raw-results/linux/midpage_direct_api_smoke_20260525_153359
private/raw-results/linux/midpage_direct_api_perf_20260525_153417
```

RUNS=5, mid_only_r0 shape:

```text
direct:
  122.13M

superfast:
  119.51M

tcmalloc:
  218.03M
```

Decision:

```text
direct ~= SuperFast. The MidPage local-r0 limit is not mainly LD_PRELOAD
dispatch. It is MidPageFront internal path length.
```

Design implication:

```text
RouteTag may still simplify free classification for mixed/cross-front rows, but
it is not the next answer for mid_only_r0. The next local-r0 design must change
the MidPage internal cache/state representation.
```
