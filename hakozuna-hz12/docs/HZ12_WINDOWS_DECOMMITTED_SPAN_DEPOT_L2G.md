# HZ12 Windows Decommitted Span Depot L2-G

## Purpose

L2-G adds a bounded depot for already-detached and decommitted spans. The first
implementation is a dedicated single-thread/quiescent smoke, not a normal
allocator policy.

## Reuse Ordering

```text
depot take
  -> recommit the exact 64 KiB payload address
  -> reset diagnostic accounting for a new generation
  -> restore the direct-index route
  -> install the span as the current bump source
  -> allow allocation
```

The route is never restored before recommit succeeds. A failed route or current
source installation rolls back to the decommitted state.

## Depot Contract

```text
fixed capacity
mutex protected
stores only route-detached MEM_RESERVE spans
records span base and class
overflow leaves the span detached/decommitted
no owner lookup or depot operation on the normal hot path
```

## Acceptance

```text
one L2-F span enters the depot
take recommits the same address
MEM_RESERVE changes back to MEM_COMMIT
accounting generation is reset
route is restored after recommit
current span is installed after route restore
next allocation returns a slot from the recommitted span
source layout remains below 1,000 lines per file
```

## No-Go

```text
route attaches before recommit
recommit returns a different address
old accounting generation leaks into the reused span
depot is unbounded
depot is consulted by the production hot path
multi-thread owner lifetime is inferred from this smoke
```

## Next Gate

L2-H may measure repeated detach/decommit/recommit cycles and bounded RSS. A
multi-thread depot and production policy remain separate decisions.

## Initial Windows Evidence

The controlled full-span smoke completed one full lifecycle at the same 64 KiB
address:

```text
depot count after put = 1
duplicate put rejected = yes
before take = MEM_RESERVE
after take = MEM_COMMIT
accounting reset = yes
route attached = yes
current span installed = yes
depot count after take = 0
next malloc returned the recommitted span base = yes
```

The new accounting generation started at `alloc=1, free=0, live=1` and ended
at `alloc=1, free=1, live=0` after the reused object was freed. The old
1,024-object generation did not leak into the reused span. All earlier HZ12
smokes and the normal xowner lane remained green.
