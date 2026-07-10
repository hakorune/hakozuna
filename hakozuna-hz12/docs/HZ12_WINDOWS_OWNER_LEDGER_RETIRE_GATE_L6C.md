# HZ12 Windows Owner Ledger Retirement Gate L6-C

L6-C closes the observation gap between the diagnostic retirement protocol and
the allocator's real flush-owner inbox. It remains read-only and diagnostic.

The combined gate opens only when all conditions hold:

```text
existing owner gate:
  enrolled producer epochs acknowledged
  diagnostic token inbox empty

real allocator inbox:
  owner slot and generation match
  flush-owner pending count is zero

ledger:
  at least one owner span compared
  owner-local ledger candidate agrees with frozen atomic shadow
  zero mismatched spans
```

The flush-owner pending query takes the existing inbox lock and is used only at
the retirement checkpoint. It is not added to malloc, free, refill, or normal
cache flush.

## Result

The cross-owner retirement smoke passed repeat-20:

```text
foreign slots:             1024
pending before owner drain:1024
pending after owner drain: 0
owner-drained slots:       1024
final outstanding slots:  0
ledger/atomic matches:     1
ledger/atomic mismatches:  0
combined gate open:        1
```

Before the owner ack/drain, the combined gate remains closed. After the owner
drains the real inbox, performs its local class-flush checkpoint, acknowledges
the epoch, and the ledger agrees with atomic accounting, the gate opens.

L6-C does not call detach, decommit, depot insertion, or automatic reclaim.
The next step is a bounded shadow scan using this combined authority on the
wide working-set lane. Production authority remains closed until that scan has
zero mismatches across all enrolled owner spans.
