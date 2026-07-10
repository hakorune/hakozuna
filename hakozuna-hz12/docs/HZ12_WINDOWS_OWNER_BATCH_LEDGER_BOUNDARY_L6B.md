# HZ12 Windows Owner Batch Ledger Boundary L6-B

L6-B connects the L6-A ledger to real allocator cold paths under
`HZ12_OWNER_BATCH_LEDGER_DIAG=1`. The macro defaults to zero, and normal speed
lanes neither link the ledger module nor execute its calls.

Connected boundaries:

```text
fresh/current-span bump -> physical-slot acquire
same-owner class flush  -> physical-slot return
foreign flush publish   -> no owner-ledger mutation
owner inbox drain       -> transit observation only
```

Owner drain deliberately leaves a slot outstanding because the object enters
the owner's thread cache. A later owner-local class flush performs the return.
Span ownership is recorded in the payload-independent generation-tagged side
table only in this diagnostic build.

## Initial Gate

The real-boundary 64-byte-class smoke reports:

```text
full-span slots:          1024
owner token:              0:1
final outstanding slots: 0
ledger/atomic matches:    1
ledger/atomic mismatches: 0
```

This first gate covers real span bump and same-owner cache flush. Cross-thread
foreign publish plus real owner-inbox drain still requires a dedicated L6-B2
smoke. Automatic reclaim remains closed, and the retire gate still needs an
explicit check for the allocator's flush-owner inbox in addition to the
diagnostic token inbox.

## L6-B2 Cross-Owner Result

The dedicated producer/consumer smoke passed repeat-20:

```text
producer-acquired slots:   1024
foreign-published slots:   1024
owner-drained slots:       1024
owner token:               0:1
final outstanding slots:   0
ledger/atomic matches:     1
ledger/atomic mismatches:  0
```

The consumer performs a 100% cross-thread free and flushes its cache. Foreign
publish does not mutate the producer ledger. The producer then drains the real
flush-owner inbox, observes the transit batch without clearing it, and clears
the physical-slot bits only when its local checkpoint flushes those objects.
This validates the intended single-writer ordering. L6-C must expose the real
flush-owner inbox pending count to a diagnostic retirement gate; until then,
this result is lifecycle evidence rather than reclaim authority.
