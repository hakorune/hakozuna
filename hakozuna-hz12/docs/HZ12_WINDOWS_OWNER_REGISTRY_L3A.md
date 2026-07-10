# HZ12 Windows Owner Registry L3-A

## Purpose

L3-A introduces a generation-tagged owner registry for multi-thread lifecycle
diagnostics. It prevents a retired slot from being mistaken for a new owner
after slot reuse.

## Token and State

```text
OwnerToken:
  slot
  generation

OwnerState:
  FREE
  ACTIVE
  RETIRING
  DEAD
```

`ACTIVE` and `RETIRING` tokens may receive a bounded final-flush publish.
`DEAD`, stale-generation, invalid-slot, and free tokens are rejected.

## Lifecycle

```text
register:
  FREE or DEAD slot
  generation += 1
  state = ACTIVE

thread exit begins:
  ACTIVE -> RETIRING

after final drain:
  RETIRING -> DEAD

slot reuse:
  DEAD -> ACTIVE with a new generation
  old token becomes stale
```

## Module Boundary

The registry is a separate slow-path module. L3-A does not add owner lookup or
registry access to malloc/free. The first smoke validates lifecycle and publish
guard semantics only; inbox integration is a later gate.

## Acceptance

```text
four workers register concurrently
ACTIVE and RETIRING publish guards accept
all workers reach DEAD without a hang
four replacement owners reuse slots with newer generations
all old tokens are rejected as stale
all replacement tokens are accepted
invalid transition count remains zero
```

## No-Go

```text
generation zero is issued
DEAD token remains publishable
old token becomes valid after slot reuse
registry lookup enters normal malloc/free
slot reuse occurs before DEAD
```

## Next Gate

L3-B connects tokens to owner-inbox publication and adds a quiescent epoch.
The generation registry remains advisory lifecycle authority, not pointer
ownership or route-validity authority.

## Initial Windows Evidence

The 4-thread lifecycle smoke completed twenty consecutive process runs with the
same counters in every run:

```text
register_success = 8
register_reuse = 4
retire_success = 4
dead_success = 4
publish_accept = 12
publish_dead_reject = 4
publish_stale_reject = 4
invalid_transition = 0
```

All four initial owners registered before retirement began. After they reached
DEAD, four replacements reused the same slots with newer generations. Dead
tokens were rejected before reuse and became stale-generation rejects after
reuse. Existing depot and xowner smokes remained green.
