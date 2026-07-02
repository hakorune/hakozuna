# HZ9 Fresh Local Page Substrate L0

This is the design-prep box after closing TLS cache, LocalArena, SlabPage
entry-bypass, and integrated SlabPage as default candidates. It defines the
next HZ9 substrate before adding another hot-path branch.

## Decision

```text
HZ9 default behavior:
  unchanged

SlabPage:
  keep as remote/profile evidence
  do not add more default-admission flags

LocalArena:
  keep as page-substrate evidence
  do not retune page mode without a new local/remote ownership shape

next:
  do not continue SlabPage admission/entry tuning for default
  first fresh substrate is HZ9OwnerLocalPagePoolScaffold-L0
  see docs/HZ9_OWNER_LOCAL_PAGE_POOL_L0.md
  build behavior only after scaffold/layout/code-shape proof is clean

current follow-up:
  scaffold/layout/code-shape proof is complete enough for local development
  HZ9OwnerLocalPagePoolPureLocal-L1 is active
  next implementation step is owner-page local allocation pop
```

## Lessons To Preserve

```text
TLS object cache:
  local reuse can be high
  HZ8 medium-run substrate still carries too much fixed cost

LocalArena:
  owner-local pages can win local rows
  mixed local/remote frees collapse when the same page is mutated by both sides

SlabPage:
  local/remote split can win remote-heavy rows
  public all-medium entry and fallback checks can regress rows that never use
  the substrate
```

## L0 Requirement

A next substrate must avoid both failure modes:

```text
1. no-use classes must not pay a repeated HZ9 entry tax
2. remote frees must not mutate owner-local page state directly
3. no-use frees must not pay a broad HZ9 route tax
```

This means a candidate must have a cheap size/class exclusion before it does
any owner lookup, remote-hot lookup, sidecar lookup, or page lookup.

## Required First Box

```text
HZ9EntryBypassProof-L0:
  behavior:
    no new allocator state
    no new page mode

  purpose:
    prove that non-covered classes can bypass the HZ9 substrate path cheaply

  accepted implementation shapes:
    compile-time size cut before class lookup for profile variants
    or class-selection integration inside the existing HZ8 medium class path

  not accepted:
    public dispatch sends all medium allocations into a substrate entry
    and the entry then blocks/falls back after owner or sidecar checks
```

Gate:

```text
main_local0:
  >= baseline * 0.99

main_r90:
  >= baseline * 0.99

medium_r50:
  keep any profile win if the covered classes are still exercised

small_interleaved_remote90:
  >= baseline * 0.99

code shape:
  no growth in baseline h8_malloc_inner / h8_free_inner
  no public free small-owned tax
```

If this proof fails, HZ9 should not open another page substrate on the current
entry seam.

## Entry Bypass Result

```text
implementation:
  H9_SLAB_ENTRY_SIZE_BYPASS_L1
  min4 SlabPage proof target
  size bypass added before class/owner/sidecar work
  public dispatch also bypasses non-candidate medium sizes

release gate:
  bench_results/20260702T091047Z_hz9_candidate_gate
  RUNS=3 THREADS=2 ITERS=20000

ratios:
  medium_local0 0.988
  main_local0 0.951
  medium_r50 1.138
  main_r90 0.943
  small_remote90 1.007

decision:
  NO-GO for public entry split default path
```

Read:

```text
The bypass keeps a remote/profile win, but it does not restore main_local0 or
main_r90. The remaining cost is the public entry split shape itself, not only
the SlabPage owner/sidecar/admission work.

Next substrate candidates should not route every medium allocation through a
separate public HZ9 entry. They should integrate at the existing non-small
malloc/classification point, where HZ8 already pays the size/class decision.
```

## Integrated Min4 Result

```text
implementation:
  HZ9SlabIntegratedMin4Proof-L0
  min4 SlabPage proof target
  no H9_SLAB_ENTRY_SPLIT_L1
  no H9_SLAB_ENTRY_SIZE_BYPASS_L1
  integrated through the existing non-small malloc/classification path

release gate:
  bench_results/20260702T092113Z_hz9_candidate_gate
  RUNS=3 THREADS=2 ITERS=20000

ratios:
  medium_local0 0.970
  main_local0 1.007
  medium_r50 1.107
  main_r90 0.947
  small_remote90 0.998

decision:
  HOLD as remote/profile evidence
```

Read:

```text
Avoiding the public all-medium entry split restores main_local0, but main_r90
still regresses beyond default tolerance. The remote-heavy win is real, but the
current SlabPage substrate shape is not default-clean even when integrated at
the existing non-small class path.
```

## Route-Off Proofs

```text
implementation:
  HZ9SlabNoFreeRouteProof-L0
  proof-only min4 integrated SlabPage target
  H9_SLAB_FREE_ROUTE_OFF_PROOF_L1 disables slab free/realloc route checks

warning:
  this target is not allocator-correct for rows that allocate SlabPage objects
  it is only a no-use-row fixed-cost proof

release gate:
  bench_results/20260702T093128Z_hz9_candidate_gate
  RUNS=3 THREADS=2 ITERS=20000

ratios:
  main_local0:
    slabintegrated_min4 0.947
    nofreeroute proof 0.996
  main_r90:
    slabintegrated_min4 0.911
    nofreeroute proof 0.940
  small_remote90:
    slabintegrated_min4 1.016
    nofreeroute proof 1.020

decision:
  proof only
```

Follow-up:

```text
HZ9SlabNoRouteProof-L0:
  H9_SLAB_FREE_ROUTE_OFF_PROOF_L1
  H9_SLAB_ALLOC_ROUTE_OFF_PROOF_L1

release gate:
  bench_results/20260702T093952Z_hz9_candidate_gate
  RUNS=3 THREADS=2 ITERS=20000

ratios:
  main_local0:
    integrated 0.985
    nofreeroute 0.994
    noroute 0.997
  main_r90:
    integrated 0.953
    nofreeroute 0.934
    noroute 0.953
  small_remote90:
    integrated 0.990
    nofreeroute 0.979
    noroute 1.036

focused main_r90:
  bench_results/20260702T094148Z_hz9_candidate_gate
  RUNS=5
  integrated 0.982
  nofreeroute 0.952
  noroute 0.941

code shape:
  bench_results/20260702T094446Z_hz9_code_shape_audit
  baseline non-small:
    131 bytes / 39 insn
  slabintegrated_min4 non-small:
    298 bytes / 83 insn
  slabintegrated_min4_nofreeroute:
    non-small remains 298 bytes / 83 insn
    non-arena-free returns to baseline
  slabintegrated_min4_noroute:
    non-small returns to 131 bytes / 39 insn
    non-arena-free returns to 95 bytes / 28 insn

layout:
  bench_results/20260702T094647Z_hz9_layout_audit
  H8OwnerRecord:
    baseline 440 bytes
    integrated/nofreeroute/noroute 448 bytes
    owned_lock offset 352 -> 360
    pending_lock offset 392 -> 400
  H8ThreadCtx:
    baseline 144 bytes
    integrated/nofreeroute/noroute 152 bytes
    h9_slab_state pointer at offset 144
```

Read:

```text
The broad free-side slab maybe-route is a measurable fixed cost on some no-use
rows, especially main_local0 in short gates. However, malloc/free route removal
does not solve main_r90 and can be worse in repeat gates. The route-off proofs
also restore the relevant internal code shape, so the remaining main_r90 issue
is not explained by route-call shape alone. The layout audit shows that
SlabPage sidecar state still changes H8OwnerRecord/H8ThreadCtx layout even when
routes are disabled. They are therefore entry/layout-shape evidence, not a
route-flag path toward default.

Next substrate candidates must either:
  activate free-side routing only after a class/thread has created HZ9-owned
  pages
  or encode route identity so no-use misses avoid global atomics/range checks

That requirement is necessary, but not sufficient for main_r90.

Next substrate candidates must also keep baseline H8OwnerRecord/H8ThreadCtx
layout unchanged until a class/thread actually selects the new substrate.
```

## Layout-Neutral Proof

```text
implementation:
  HZ9SlabLayoutNeutralProof-L0
  proof-only min4 integrated SlabPage target
  H9_SLAB_FREE_ROUTE_OFF_PROOF_L1
  H9_SLAB_ALLOC_ROUTE_OFF_PROOF_L1
  H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1

purpose:
  prove that no-use rows can restore both route/code shape and
  H8OwnerRecord/H8ThreadCtx layout when SlabPage owner/thread fields are absent

layout audit:
  bench_results/20260702T095306Z_hz9_layout_audit
  H8OwnerRecord:
    baseline 440 bytes
    layoutneutral 440 bytes
  H8ThreadCtx:
    baseline 144 bytes
    layoutneutral 144 bytes

code shape:
  bench_results/20260702T095306Z_hz9_code_shape_audit
  layoutneutral non-small:
    131 bytes / 39 insn
  layoutneutral non-arena-free:
    95 bytes / 28 insn
  no slab calls from no-use non-small/free paths

short no-use gate:
  bench_results/20260702T095408Z_hz9_candidate_gate
  RUNS=3 THREADS=2 ITERS=20000
  main_local0 0.948
  main_r90 1.062
  small_remote90 1.068

decision:
  proof only
```

Read:

```text
The layout-neutral proof closes the structural-contamination question: no-use
rows can restore baseline owner/thread layout and internal code shape. The
short release gate still moves in mixed directions, especially main_local0, so
this is not behavior evidence and not a reason to add more SlabPage route flags.

The actionable constraint is narrower and stronger:
  next substrates must not add no-use route checks
  next substrates must not change H8OwnerRecord/H8ThreadCtx layout for rows
  that have not selected the substrate
```

## Fresh Page Substrate Shape

Any next behavior substrate should be a separate HZ9-owned page body with
these properties:

```text
local pages:
  owner-thread only
  local free mutates owner-local bits only
  no remote producer writes local free bits

remote frees:
  keep HZ8 pending bitmap/qstate authority
  remote producer records pending only
  owner collector decides whether to refill local pages or return to HZ8

page transition:
  once a page sees remote traffic, it leaves the pure-local fast set
  it is drained by owner-side collection, not concurrently shared as a mixed
  local/remote mutation object

fallback:
  non-covered classes and non-hot rows go directly to HZ8 medium
  without sidecar/owner/admission polling
  integrated covered classes must also avoid broad main-r90 fallback damage
  no-use rows must not carry owner/thread layout changes

entry:
  avoid H9_SLAB_ENTRY_SPLIT_L1-style public all-medium dispatch for default
  candidates
  integrate the first substrate decision into the existing non-small medium
  classification path when possible
```

## Source Layout

```text
entry seam:
  keep src/h8_hz9_local_entry.c thin

body:
  new HZ9-owned source or inc file

build rules:
  add target-family rules under mk/
  do not grow the top-level Makefile

stats:
  add HZ9-specific counters in split HZ9 report/snapshot files
  do not push active files over 800 lines
```

## Stop Conditions

```text
NO-GO:
  non-covered class rows still regress after entry bypass
  remote safety requires replacing pending/qstate in L1
  owner-exit drain cannot be proven with a deterministic test
  any active source/script/doc/Makefile/mk file exceeds 800 lines

HOLD:
  remote/profile rows improve but local/main/small rows regress
  RSS overhead is unbounded or not reported
```
