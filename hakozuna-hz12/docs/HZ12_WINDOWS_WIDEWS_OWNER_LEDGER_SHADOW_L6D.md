# HZ12 Windows Wide Working-Set Owner Ledger Shadow L6-D

L6-D applies the L6-C combined authority to a bounded multi-span working set.
It is a read-only reclaimability experiment: no span is detached, decommitted,
inserted into the depot, or reused.

## Workload

```text
owner threads:             1
cross-thread consumers:    1
span size:                 64 KiB
span budget:               64
slot size:                 64 bytes
slots per span:            1024
total slots:               65536
flush-owner inbox pacing:  drain after each span
```

The per-span drain keeps the real owner inbox bounded instead of raising its
capacity to hold the complete working set. Foreign publish does not write the
owner ledger. Only the owner drain/checkpoint path advances ledger state.

## Result

The Windows diagnostic passed repeat-10:

```text
compared spans:            64
ledger candidates:         64
atomic candidates:         64
matching spans:            64
mismatched spans:          0
reclaimable candidate:     4,194,304 bytes (4 MiB)
real flush-owner pending:  0
combined gate open:        1
```

This is the first multi-span evidence that owner-local batch authority can
identify the same reclaim set as per-operation atomic accounting without
foreign-thread span-counter updates. It also reconnects the mechanism to the
wide working-set parking-lot problem with a concrete byte count.

## Next Gate

L6-E may pass only this fixed 64-span success set to the already proven ordered
detach/decommit machinery. It must remain a diagnostic behavior sibling and
must compare the selected set with atomic shadow before mutation. A mismatch,
non-empty real inbox, incomplete epoch, or generation mismatch is a hard
fail-closed result.
