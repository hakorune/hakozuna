# HZ12 Windows Owner Ledger Recycle L6-F

L6-F closes the bounded owner-ledger lifecycle after L6-E decommit. The same
fixed 64-span set enters the depot, recommits at the same addresses, exposes
and touches every physical slot, returns every slot, and completes a second
ordered detach/decommit cycle.

## Ledger Recycle Contract

Depot take starts a new physical-slot generation for the span:

```text
recommit backing
reset atomic accounting generation
reset owner-ledger slot bitmap while preserving owner token/class
attach route and install diagnostic current span
acquire/touch/free every slot
flush local cache
return every slot in the owner ledger
require ledger/atomic candidate agreement
detach route last
decommit again
return span to bounded depot
```

The owner-ledger query now derives span identity from the arena side table and
stored class rather than live route classification. This is required because a
correct diagnostic query must continue to work after route detach and payload
decommit.

## Result

The Windows L6-F lane passed repeat-5:

```text
initial detached/decommitted spans: 64 / 64
recommitted spans exercised:        64
physical slots touched:             65,536
second detach:                      64
second decommit:                    64
final depot entries:                64
address mismatch:                   0
owner mismatch:                     0
ledger/atomic mismatch:             0
RSS reduction retained:             about 3.99 MiB
```

The read-only L6-D executable remains non-mutating and reports zero detach,
decommit, recycle, and depot operations.

## Status

The fixed diagnostic lifecycle is complete. This does not yet justify an
automatic production policy or HZ8 integration. The next decision is policy:
define when a production checkpoint may spend a bounded reclaim budget, then
measure throughput, RSS, teardown, and repeated workload stability without
linking atomic accounting into the speed lane.
