# HZ6 Lane Guide

This document is the compact lane map for HZ6 Windows benchmarking. Keep long
experiment notes in `current_task.md`; use this file to answer "which lane is
which?" before running or comparing benchmarks.

## Current Recommendation

```text
Primary candidate-control:
  route4k

Low-capacity / low-RSS baseline:
  control

Capacity / completion control:
  appcap

Evidence-only source-run controls:
  sourcerun-route4k
  sourcerun-sameclass-route4k

Descriptor lifecycle prototype:
  descriptorless-route4k
  descriptorreserve-route4k

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

appcap:
  High-capacity application-like control. Use it to separate capacity failure
  from policy failure. Do not treat it as the HZ6 default lane.
```

## Focused Mechanism Lanes

```text
desc4k-route4k:
  route4k plus descriptor capacity 4096. Descriptor-pressure probe only.

source512-route4k:
  route4k plus source-block capacity 512. Source-block-pressure probe only.

desc4k-source512-route4k:
  route4k plus descriptor 4096 and source-block 512. Combined pressure probe.

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
  actually survive more realistic access mixes.
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

## Lane Rules

- Speed / paper lanes must not include diagnostic atomics or trace-only probes.
- Research lanes must have explicit names and must not silently replace
  `route4k`.
- `appcap` is a capacity/completion control, not a production default.
- A lane is not promoted from one good row. It needs RSS, safety, repeat
  stability, and guard behavior.
- Owned-looking invalid pointers must never be converted into foreign fallback.
- Do not put OS queries such as `VirtualQuery` on hot free paths.
