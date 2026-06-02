# HZ6 Lane Guide

This document is the compact lane map for HZ6 Windows benchmarking. Keep long
experiment notes in `current_task.md`; use this file to answer "which lane is
which?" before running or comparing benchmarks.

## Current Recommendation

```text
Primary candidate-control:
  route4k
  noboost-route4k

Low-capacity / low-RSS baseline:
  control

Redis-like candidate-control:
  redislowrss-route4k

Capacity / completion control:
  appcap

Route-lifecycle diagnostic:
  visiblefirst-appcap
  negativefilter-appcap
  sharedir-appcap
  sharedirfirst-appcap
  ownerlocality-appcap
  ownerlocalityfast-appcap

Evidence-only source-run controls:
  sourcerun-route4k
  sourcerun-sameclass-route4k

Descriptor lifecycle prototype:
  descriptorless-route4k
  descriptorreserve-route4k
  descriptorcold-route4k
  descriptorcoldgov-route4k

Frozen no-go controls:
  spill-route4k
  borrow-route4k
  cap-route4k
  sourcerun-reclaim-route4k
```

`route4k` is the current HZ6 Windows lane to use first when checking whether
HZ6 remains low-RSS while avoiding tiny-route-table artifacts. It is not a
promotion claim. It is the cleanest candidate-control shape so far.

`appcap` proves whether a workload can complete when HZ6 gets very large
descriptor / route / transfer / source-block / front-cache capacities. It is a
diagnostic capacity control, not the default production shape.

`visiblefirst-appcap` is a diagnostic-only route-lifecycle upper-bound test.
It uses appcap capacity plus `HZ6_VISIBLE_FIRST_FREE_L1=1` and must be built
with `HZ6_DIAGNOSTIC_PROBES=1`. Use it to measure whether skipping expensive
worker-local route MISS scans can recover Larson main-warmup; do not use it as
a production or paper-facing lane.

## Capacity Lanes

```text
default:
  Tiny R1 smoke/control shape. Useful for build sanity, not performance claims.

broad:
  Historical broad-capacity control. Often faster than tiny lanes, but can
  raise RSS substantially.

control:
  Low-capacity baseline: small descriptor / route / transfer / source-block /
  front-cache limits. Useful for seeing how low-RSS HZ6 behaves under pressure.

route4k:
  control plus a 4096-entry route table. This isolates route capacity while
  keeping other capacities near control values.

noboost-route4k:
  route4k plus `HZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1`. This keeps the
  low-capacity shape but prevents alloc-fail pressure from increasing source
  refill batch size. Latest repeat-3 strongly improves balanced and wide_ws
  while preserving larger_sizes, and random_mixed repeat-3 is neutral to
  positive. Treat it as the current Windows low-capacity candidate-control
  lane.

redislowrss-route4k:
  Redis-like low-RSS candidate-control. It keeps the route table at 4096,
  combines descriptor capacity 4096 with source-block capacity 512, keeps
  frontcache/transfer modest, and disables starvation source-refill boost.
  L0 redis_short showed descriptor capacity is the main collapse point and
  descriptor+source-block capacity gives a better Redis SET/LPUSH/RANDOM shape
  without moving all the way to appcap. Mixed_ws guard strongly regresses, so
  keep this lane Redis-like only; do not use it as a general HZ6 primary lane.

appcap:
  High-capacity application-like control. Use it to separate capacity failure
  from policy failure. Do not treat it as the HZ6 default lane.

visiblefirst-appcap:
  Diagnostic-only appcap variant. In non-strict `free()`, visible/shared route
  lookup is tried before local route lookup; visible MISS falls back to local
  lookup so local INVALID is not converted to MISS. This is an upper-bound
  probe for cross-owner warmup, not a promotion lane. It is now treated as
  no-go evidence rather than a production candidate.

negativefilter-appcap:
  Diagnostic-only appcap variant. It uses a conservative local owned-range hint
  to skip local route lookup only when the pointer is definitely not local.
  Diagnostic builds shadow-verify the skip and record false-skip counters.
  The source-block-only hint is now no-go as an optimization lane because
  route rehome can create local exact routes without local source-block
  ownership. Keep this lane as evidence that the next design needs rehome-aware
  owner ranges or a shared route directory.

sharedir-appcap:
  Diagnostic-only appcap variant. It publishes exact routes into a process-wide
  shared route directory and probes it only after local MISS. Behavior is
  unchanged. Use it to measure whether a shared directory could avoid the
  worker-local route MISS scan in cross-owner warmup.

sharedirfirst-appcap:
  Experimental behavior variant. After the allocator has observed a foreign
  visibility hit, it tries the shared directory first and reconstructs exact
  route results from the directory. Compact/moderate main-warmup can recover
  strongly, but 10k-chunk stress main-warmup currently times out. Keep it
  evidence-only until large live-set scaling is fixed.

ownerlocality-appcap:
  Diagnostic-only hint/backend lane. It uses a cheap owner-locality index to
  decide whether a pointer is definitely foreign, then consults the shared
  route directory backend for foreign exact lookup before falling back to the
  ordinary local route path. Use it to measure whether a low-cost locality hint
  can prune worker-local MISS scans without turning source-block ownership into
  the only truth source. The Larson runner now releases each worker live set
  before worker allocator teardown, so main-warmup no longer adds an accidental
  post-measurement owner-death cleanup stress on top of the timed cross-owner
  handoff lane.

ownerlocalityfast-appcap:
  Non-diagnostic owner-locality behavior lane. It uses the same owner-locality
  index and shared-directory exact backend as `ownerlocality-appcap`, but does
  not force `HZ6_DIAGNOSTIC_PROBES=1`. Use it after the diagnostic lane has
  shown clean counters to check whether the route-lifecycle fix is still fast
  without diagnostic probe overhead. Repeat-3 now makes this the preferred
  candidate-control lane for the fast Larson owner-locality comparison rows.
```

## Focused Mechanism Lanes

```text
desc4k-route4k:
  route4k plus descriptor capacity 4096. Descriptor-pressure probe only.

source512-route4k:
  route4k plus source-block capacity 512. Source-block-pressure probe only.

desc4k-source512-route4k:
  route4k plus descriptor 4096 and source-block 512. Combined pressure probe.

redislowrss-route4k:
  noboost plus descriptor 4096 and source-block 512. This is the named
  Redis-like low-RSS candidate-control lane after redis_short L0 showed
  `desc4k-source512-route4k` avoids Redis allocation-failure spam at a much
  lower peak working set than appcap. Evidence-only outside Redis-like rows:
  mixed_ws guard shows the capacity shape over-retains and slows broad mixed
  profiles.

front1k-desc4k-source512-route4k:
  route4k plus descriptor/source/front-cache widening. Reproduces broad-like
  behavior more than it solves the low-RSS lane. Evidence/control only.

sourcerun-route4k:
  Source-run slot reuse evidence lane. It proves reusable-run mechanics can
  reduce some source pressure, but it did not solve balanced mixed rows.

sourcerun-sameclass-route4k:
  Narrower same-class source-run reclaim evidence lane. Safer than broad donor
  reclaim and mildly positive on the latest rows, but still not a promotion
  lane.

descriptorless-route4k:
  Descriptor lifecycle prototype. Source-block cached slots can drop their
  descriptor while staying in frontcache; reuse materializes a fresh descriptor
  and exact route. This lane includes source-run metadata because descriptorless
  cached slots need a physical slot owner after the descriptor is detached. Use
  it to test whether cached slots should stop pinning descriptor capacity. The
  current L1 is evidence/control only: it preserves route safety in the latest
  checks, but descriptor materialization can still fail under a full descriptor
  table, so it is not a promotion lane.

descriptorreserve-route4k:
  Descriptor materialization-reserve prototype. Extends descriptorless-route4k
  by reserving the detached descriptor for the same cached physical slot, so
  reuse can materialize without a fresh descriptor-table search. Evidence only:
  it removes descriptorless materialization failures in the latest checks, but
  it does not solve balanced / wide_ws and can reduce the descriptor pool
  available to normal allocation.

descriptorcold-route4k:
  Selective descriptorless prototype. Only over-soft-cap frontcache bins detach
  descriptors, reducing the broad descriptorless failure loop. Evidence only:
  it has a small balanced / wide_ws signal, but larger_sizes regresses, so the
  simple soft-cap gate is not a promotion policy.

descriptorcoldgov-route4k:
  Class/pressure-aware descriptor governor prototype. Descriptorless is treated
  as cold-cache descriptor compression, not as a hot reuse path. The first L1
  gates detach to small/mixed classes, limits detached entries, and admits
  materialization only when a descriptor is available. Latest repeat-3 is
  promising on balanced and preserves larger_sizes, but wide_ws is still weak,
  so keep it as candidate-control evidence rather than default promotion.

descriptorcoldgov-widews-route4k:
  Budget-expanded descriptor governor variant for wide_ws probing. It keeps the
  same class gate as descriptorcoldgov-route4k but raises the detach budget to
  test whether wide_ws is limited by detached-slot budget saturation rather
  than by the class gate itself. Latest repeat-3 did not improve wide_ws and
  badly regressed balanced, so keep it as no-go/control evidence.
```

## Frozen No-Go Lanes

```text
spill-route4k:
  No-go. Frontcache spill increased source allocation and RSS.

borrow-route4k:
  No-go. Borrowing moved some source-block pressure but did not produce a clean
  throughput/RSS improvement.

cap-route4k:
  No-go. Local cap behavior slowed the lane and worsened RSS.

sourcerun-reclaim-route4k:
  No-go. Broad source-run donor reclaim caused route churn / probe explosions.
```

Keep these lanes buildable as controls, but do not use them as the next
optimization target unless a new diagnostic specifically reopens the question.

## Benchmark Family Read

```text
mixed_ws / random_mixed:
  Best for capacity-shape and low-RSS tradeoff checks.

Larson worker-warmup:
  Same-owner small-object control. This is the lane for hot-path / toy-small
  behavior without cross-owner handoff stress.

Larson main-warmup:
  Cross-owner handoff stress. Treat failures here as route visibility /
  remote-free / transfer ownership evidence, not as a pure hot-path verdict.

Redis workload:
  App-like pattern control. Useful for detecting whether HZ6 capacity changes
  actually survive more realistic access mixes. Current route4k/noboost rows
  need a focused shorter profile before they are useful for candidate ranking,
  because the default Redis-like row can timeout while emitting many allocation
  failure stats lines. Use `redis_short` or `redis_tiny` first when debugging
  Redis-like timeout behavior.
```

## Commands

Build a focused HZ6 capacity suite:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_hz6_capacity_suite.ps1 `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,appcap
```

List the rows without running them:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -ListOnly `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,sourcerun-sameclass-route4k
```

Run a quick one-pass capacity check:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,appcap `
  -Runs 1
```

Run a focused Redis-like timeout triage row:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families redis `
  -Hz6Profiles rss `
  -CapacityLanes route4k,noboost-route4k `
  -BenchmarkProfiles redis_short `
  -Runs 1 `
  -SkipBuild
```

## Lane Rules

- Speed / paper lanes must not include diagnostic atomics or trace-only probes.
- Research lanes must have explicit names and must not silently replace
  `route4k`.
- `appcap` is a capacity/completion control, not a production default.
- A lane is not promoted from one good row. It needs RSS, safety, repeat
  stability, and guard behavior.
- Owned-looking invalid pointers must never be converted into foreign fallback.
- Do not put OS queries such as `VirtualQuery` on hot free paths.
