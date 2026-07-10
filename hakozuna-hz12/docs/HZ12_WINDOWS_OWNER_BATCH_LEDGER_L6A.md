# HZ12 Windows Owner-Local Batch Ledger L6-A

## Purpose

L6-A is the first production-authority candidate after the bounded L5 reclaim
lifecycle. It does not reclaim memory. It asks whether an owner-local ledger,
updated only at cold batch boundaries, can agree with the frozen per-operation
atomic accounting shadow at a quiescent retirement checkpoint.

## Accounting Contract

The ledger tracks physical slot exposure, not user-visible allocation count.

```text
span carve / reusable-source acquire:
  owner marks the acquired physical slots outstanding

foreign publish:
  producer does not write the owner's span ledger

owner inbox drain:
  owner records the transit batch, but slots remain outstanding because they
  are now cached by the owner

owner local cache flush:
  owner marks the flushed physical slots returned

retirement checkpoint:
  producer epoch acknowledged
  token inbox empty
  enrolled caches flushed
  ledger stable
  compare ledger candidate with atomic shadow candidate
```

An inbox drain must not make a span reclaimable by itself. The drained objects
still reside in the owner's cache until the class-flush checkpoint returns
them. This separation prevents reclaiming a span that still has cached slots.

## L6-A Scope

L6-A adds a diagnostic-only side table and a dedicated smoke. It is not linked
into normal allocator or speed lanes. The module uses one bit per physical slot
to reject duplicate acquire/return transitions and preserves generation-tagged
owner identity from the existing span-owner side table.

The first smoke proves:

- a full span can be acquired in batches;
- owner drain observation does not reduce outstanding slots;
- a quiescent local return clears every outstanding slot;
- the resulting ledger candidate equals the atomic accounting candidate;
- no automatic detach, decommit, depot insertion, or reuse policy runs.

## Acceptance

```text
ledger compared spans == atomic tracked spans
ledger candidate == atomic candidate for every compared span
rejected transitions == 0
owner drain alone never opens the ledger candidate
normal speed lane build inputs remain unchanged
source layout check passes
```

Any mismatch is a hard no-go for production reclaim authority. Later L6 work
may connect the same transitions to actual carve/flush/drain boundaries, but
automatic reclaim remains closed until a multi-thread retirement checkpoint
shows exact agreement.

## Initial Result

The dedicated Windows smoke passed repeat-20:

```text
slot size:                 64 bytes
full-span slots:           1024
owner-drain observations:  512
final outstanding slots:   0
ledger/atomic matches:     1
ledger/atomic mismatches:  0
```

The existing whole-span accounting, retired-owner reclaim shadow, and owner
retire gate smokes also pass in the same build. This proves the side-table
state machine, not production integration. In particular, the existing retire
gate checks the diagnostic token inbox; it does not yet prove that the actual
flush-owner inbox is empty. L6-B must wire and verify that real allocator
boundary before the ledger can participate in reclaim authority.
