# HZ9 Lane History

This file is the compact archive for HZ9 lane decisions. Keep the active
orientation in `HZ9_CURRENT_STATUS.md`.

## Standalone Preparation

```text
HZ9StandaloneFork-L0:
  hakozuna-hz9 is forked from the HZ8 KeepRefill baseline.
  HZ8 remains the balanced release/paper line.

HZ9StandaloneSelfContainment:
  local Makefile / mk / include / src / bench / tests / scripts / docs exist.
  generated binaries and bench_results are ignored.
  standalone check rejects hidden sibling hakozuna-* paths unless explicitly
  allowed as comparison artifacts.

source-shape splits:
  bench aliases moved to mk/hz9_bench_aliases.mk.
  local-arena targets moved to mk/hz9_local_arena_targets.mk.
  HZ9 debug fields moved to include/h8_debug_hz9_fields.inc.
  medium alloc helpers moved to src/h8_medium_alloc_helpers.inc.
  local-mag inline helpers moved to src/h8_hz9_local_mag_inline.inc.
```

## Entry And Object Cache

```text
local-entry scaffold:
  pre-arena-skip local rows regressed.
  arena-owned pointer skip removed the worst entry tax.
  local-entry is only a seam; it must be repaid by a high-hit substrate.

TLS object cache:
  fixed64 can improve.
  medium/main local and remote gates are not clean.
  HOLD as evidence.

active-run magazine:
  high hit/reuse did not translate to clean release speedup.
  HOLD.
```

## SlabPage Evidence

```text
SlabPage route / sidecar / freebits variants:
  remote-heavy rows can improve strongly.
  main_local0 and small_remote p25 remain promotion blockers.
  route-last and mutation-ceiling variants are evidence only.

decision:
  freeze SlabPage as profile/evidence.
  do not keep tuning route/scan/freebits for default.
```

## Substrate Evidence Highlights

```text
tail split:
  kept h8_malloc_inner / h8_free_inner small
  slabadaptive no longer grows inner malloc/free shape

local-entry scaffold:
  pre-arena-skip local rows regressed
  arena-owned pointer skip removed the worst small/arena entry tax
  local-entry is only a seam and must be repaid by a high-hit substrate

localentry_tlscache:
  helped selected main rows
  regressed fixed64 / medium_local / medium_r50
  HOLD; do not build the next default on HZ8 medium-run object cache

slabclasses_min0:
  proved 8K/16K coverage value for main_local0 and main_r90
  R10 showed strong medium/main wins
  small_remote90 and local stability remained near or below no-regression gates

min0_midcap / layout32 / min0_entry:
  cap and layout variants explained pressure but did not close small/local gates
  public-entry split showed small-owned pointers can pay dispatch tax despite
  never using slab pages

layout audit:
  SlabPage increased H8ThreadCtx and H8OwnerRecord footprint
  next default candidate should avoid embedding large experimental state in
  hot structs

storemut / freebits:
  removing owner-local CAS/RMW and scan/load costs helped some remote rows
  did not repair medium/main local default gates

ownerpage ownerfast-bits:
  pure local pages replaced local_free_bits CAS/fetch_or with load/store.
  fixed64_local0 and medium_local0 recovered materially.
  medium_r50 and main_r90 regressed, so the proof is attribution only.

post-hygiene next-substrate probe:
  SlabPage remained profile/evidence
  next default candidate must avoid local blocked checks and must not depend on
  SlabPage route/scan/mutation tuning
```

## LocalArena Evidence

```text
LocalArena L0:
  true local pages can create medium_local0 wins.
  raw remote direct-free path is not acceptable.
  adaptive class cuts reduce damage but miss default gates.

dense ownerfast:
  main_local0 and medium_local0 improve strongly.
  medium_r50 / main_r90 still regress.

retire-after-remote:
  preserves some local speed.
  creates severe remote-heavy page churn.
  NO-GO for promotion.
```

## Remote Shape And Admission

```text
remote-shape counters:
  remote_first nearly tracks page_create in r50-shaped probes.
  damage is page churn after early remote contamination.

remote-first age counters:
  most remote-contaminated pages see only 1..3 local allocations first.

admission after class remote history:
  fixes the retire page-churn failure.
  still misses medium_r50 / main_r90 no-regression gates.
  HOLD.

fixed-count probation:
  still creates too many remote-contaminated pages.
  NO-GO; do not tune the count.

local phase admission:
  localarena_dense_ownerfast_phase8 delayed page creation until 8 local
  allocation attempts and marked class remote history from HZ8 fallback remote
  frees.
  R1 ratios: medium_local0 0.816, main_local0 0.823, medium_r50 0.611,
  main_r90 0.622.
  NO-GO; remote page creation can be blocked, but LocalArena entry/body cost
  still regresses broad rows.
```

## Latest LocalArena Read

```text
thread-admission counters:
  every created LocalArena page became remote-first in the r50 probe.
  creator thread page sequence and allocation history are not selective
  admission signals by themselves.

page-mix counters:
  remote_first_local_free=[0:421,1:284,2_3:244,4_7:70,8p:5]
  remote_after_local=[50,109,111,102,193,198]
  local_after_remote=[0,3,2,3,8,19]

read:
  many pages see first remote before any local free.
  almost all see it by three local frees.
  after remote observation, local frees are rare.
```

## Current Next Hypothesis

```text
No active LocalArena page-mode hypothesis.

Next HZ9 substrate work must start from source-shape/cost evidence:
  keep pending/qstate/route/owner-exit authority intact
  keep HZ9 standalone layout and public entry shape clean
  avoid another threshold-only admission retune
```

## Remote-Safe Page Result

```text
localarena_dense_ownerfast_remotesafe:
  mechanism worked
  remote-safe pages were created after class remote history
  remote-safe pages then mixed local and remote frees heavily

R1 release probe:
  medium_r50 0.480
  main_r90   0.542
  small_remote90 0.958

decision:
  NO-GO for promotion
  keep target as evidence
  do not keep retuning LocalArena page modes without a different substrate
```

## RemoteActive Admission Result

```text
localarena_dense_ownerfast_remoteactive:
  after class remote_seen, reuse active nonfull page only
  after class remote_seen, do not create new LocalArena pages

debug:
  medium_r50 page_create dropped from about 1024 to about 12
  failed arena attempts dropped, peak RSS dropped sharply

R1 release:
  medium_r50 0.913 vs baseline
  main_r90   0.908 vs baseline
  medium_local0 0.757 vs baseline
  small_remote90 1.047 vs baseline

decision:
  HOLD as evidence
  admission lesson is useful, but LocalArena local substrate is not default
  clean enough
```

## Current Freeze Boundary

```text
freeze:
  SlabPage L1 behavior promotion
  SlabPage cap expansion / layout / entry / hotmask micro-tuning
  LocalArena page-mode retuning
  fixed-count remote probation

keep:
  fail-closed route boundary
  pending/qstate remote authority
  class-pressure counters
  min0 / sidecar / LocalArena / remote-safe evidence
  standalone source-shape gate

next substrate must:
  cover 8K..32K for main rows
  cover 48K/64K for medium rows
  structurally isolate small-v0 hot rows
  avoid public-entry checks for small-owned pointers
  avoid large experimental arrays in H8ThreadCtx / H8OwnerRecord
```

## Latest Source-Shape Gate

```text
command:
  RUN_SMOKE=0 RUN_PROBE=0 scripts/run_hz9_pre_substrate_recheck.sh

result:
  PASS

logs:
  bench_results/20260702T121610Z_hz9_layout_audit
  bench_results/20260702T121610Z_hz9_code_shape_audit

read:
  layout and public entry shape are controlled; the remaining blocker is the
  substrate body/local cost, not hidden H8OwnerRecord/H8ThreadCtx growth.
```

## Substrate Cost Matrix

```text
log:
  bench_results/20260702T121943Z_hz9_substrate_cost_matrix
  bench_results/20260702T122124Z_ownerpage_focus_r3
  bench_results/20260702T122205Z_hz9_code_shape_audit

read:
  SlabDirectUse is remote/profile evidence.
  LocalArena phase8 is broad NO-GO.
  OwnerPage purelocal is closest but still not a default candidate:
    medium_local0/main_r90/small_remote90 are not clean.
  OwnerPage public malloc/free/non-arena code shape is unchanged, so the next
  question is purelocal body/text cost, not public route growth.
```
