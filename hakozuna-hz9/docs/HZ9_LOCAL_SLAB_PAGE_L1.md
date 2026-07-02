# HZ9 Local Slab Page L1

## Status

```text
state:
  implemented as profile/evidence
  not default

reason:
  remote-heavy medium/main rows can improve strongly
  local and small no-regression gates are not consistently clean
  entry micro-tuning did not produce a stable default point
```

SlabPage is useful because it proves an HZ9-local medium substrate can carry
remote-heavy traffic while preserving the HZ8-style boundary. It is not the
next default substrate because local-only rows still pay too much entry or
fallback cost.

## Implemented Scope

```text
build flags:
  H9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY
  H9_MEDIUM_LOCAL_SLAB_PAGE_L1
  optional class coverage / adaptive / entry split evidence flags

runtime:
  per-thread bounded slab pages
  route/usable/free/realloc classification before HZ8 medium fallback
  fail-closed owned INVALID handling
  remote free through pending-bit claim + slab qstate / owner queue
  owner-exit and final-free cleanup for slab-owned pages
```

The default HZ9 tree keeps the behavior opt-in. HZ8 balanced behavior is not
changed by this lane.

## Evidence Summary

```text
64K-only SlabPage:
  strong fixed64 signal
  partial medium remote signal
  cannot repair main_local0 because main has no 48K/64K traffic

class coverage:
  improves remote-heavy rows
  unconditional coverage regresses local/main rows

remote-pressure adaptive:
  remote-heavy wins are real
  local rows often show blocked/pages0 and fall back
  therefore local-only rows still pay entry cost without using slab pages

entry split + small tail split:
  restores h8_malloc_inner / h8_free_inner code shape
  moves cost to public/non-small entry
  still not promotion-clean

hotmask:
  confirms entry hot lookup shape matters
  not cleaner than slabadaptive_entry across small/main/remote gates
```

Latest consolidated probe:

```text
script:
  scripts/run_hz9_next_substrate_probe.sh

smoke:
  bench_results/20260702T010029Z_hz9_next_substrate_probe_summary_smoke_hz9_next_substrate_probe/

read:
  main_local0/main_r90 require 8K/16K/24K/32K coverage
  medium_local0/medium_r50 also need 48K/64K
  48K/64K-only designs cannot become default
  hotmask is noisy and not cleaner than slabadaptive_entry
```

## Contracts Preserved

```text
owned INVALID:
  interior / stale / double free remain fail-closed

remote authority:
  pending bitmap / qstate / owner queue remains the duplicate-free authority

owner lifecycle:
  owner exit drains pending work and purges empty slab pages
  final free after owner death does not leave orphaned pending work

RSS:
  per-thread page cap is an experiment guard
  retained route metadata is accepted only for evidence builds
```

## Hold Reasons

```text
default blockers:
  local-only rows can pay blocked entry checks and fall back
  small/guard p25 is not consistently clean
  main_local0 needs 24K/32K support, not only 48K/64K
  route metadata retention is not yet a product RSS contract
```

Do not continue with more SlabPage entry micro-tuning unless a new substrate
shape removes the local blocked-check cost.

## Next Direction

```text
freeze:
  SlabPage profile/evidence lane

open:
  new HZ9 local substrate / entry design

requirements:
  cover 8K/16K/24K/32K cleanly for main rows
  also handle 48K/64K for medium rows
  avoid per-allocation blocked checks on local rows that will fall back
  keep remote pending/qstate authority
  keep owned INVALID fail-closed
```

Use `scripts/run_hz9_next_substrate_probe.sh` before opening a new behavior
box so class distribution, entry mechanism, and release timing are evaluated
together.
