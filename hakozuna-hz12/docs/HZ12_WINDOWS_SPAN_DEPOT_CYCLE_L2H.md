# HZ12 Windows Span Depot Cycle L2-H

## Purpose

L2-H verifies repeated detach, decommit, depot, recommit, and reuse cycles in a
single quiescent process. It also adds depot slow-path counters needed for cap
and rollback diagnosis.

## Diagnostic Counters

```text
put_attempt / put_success
put_duplicate / put_full / put_reject_state
take_attempt / take_hit / take_miss
recommit_success / recommit_fail
route_fail / current_install_fail / rollback
depot_current / depot_max
```

These counters live in `hz12_span_depot` and are updated only when the explicit
depot API is called. The allocator malloc/free hot path does not update them.

## Cycle Acceptance

```text
same span address survives every cycle
each generation reaches full clean accounting
each detach accounts for every slot
each decommit reaches MEM_RESERVE
each take recommits before route attach
each generation resets accounting
depot_current returns to zero after every take
recommit_fail / rollback / route_fail / current_install_fail remain zero
```

## No-Go

```text
counter update is added to normal malloc/free
old accounting generation leaks into the next cycle
depot count grows across balanced put/take cycles
route becomes visible while payload remains decommitted
cycle success is treated as multi-thread proof
```

## Next Gate

After repeated-cycle stability, add a separate cap saturation smoke. RSS and
production policy remain later gates.

## Initial Windows Evidence

The same-process lifecycle smoke completed eight generations of one 64-byte
class span:

```text
cycles = 8
slots per generation = 1,024
put_success = 8
take_hit = 8
recommit_success = 8
depot_current = 0
depot_max = 1
all failure/rollback counters = 0
```

The separate capacity smoke created 65 fully accounted, detached, and
decommitted spans. A 64-entry depot accepted exactly 64 spans and rejected the
65th with one `put_full` event:

```text
put_attempt = 65
put_success = 64
put_full = 1
depot_current = 64
depot_max = 64
```

The capacity run's earlier inbox overflow used the existing bounded ownerless
fallback; span accounting still observed all 66,560 objects released with zero
live objects and zero underflow. The depot counters remain slow-path-only.
