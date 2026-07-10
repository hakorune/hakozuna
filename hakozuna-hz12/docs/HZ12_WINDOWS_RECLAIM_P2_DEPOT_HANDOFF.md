# HZ12 Windows Reclaim P2: Bounded Depot Handoff

## Decision

P2 keeps P1's retire-only authority and hands each successfully decommitted
span to a fixed 64-entry depot. It does not add automatic pressure reclaim or
normal malloc/free accounting.

The depot was split into two responsibilities:

- `hz12_span_depot_core.c`: bounded storage of already-decommitted span/class
  pairs; no atomic span-accounting dependency.
- `hz12_span_depot.c`: diagnostic compatibility wrapper that recommits a span,
  resets atomic accounting, attaches its route, and installs it as current.

P1/P2 links only the core storage path. Existing lifecycle tests continue to
use the wrapper and retain their previous behavior.

## Windows Result

Repeat-10 passed:

- candidates: 64/64
- detached: 64/64
- decommitted: 64/64 (4,194,304 payload bytes)
- inserted into depot: 64/64
- failures: 0
- observed working-set reduction: 4,186,112 bytes in every run

Regression checks passed for whole-span accounting, depot cycle, depot
capacity, and the mutation-free P0-D shadow lane.

## Boundary

P2 proves bounded retention after retirement-only reclaim. It does not yet
prove that a production allocation miss can safely recommit a core-only depot
entry without diagnostic accounting. The next gate is a core-only take path:
recommit, route attach, and fresh current-span installation, with rollback on
every failure and no atomic accounting authority.
