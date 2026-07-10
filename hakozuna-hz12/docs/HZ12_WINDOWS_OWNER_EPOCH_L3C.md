# HZ12 Windows Owner Epoch L3-C

## Purpose

L3-C replaces the L3-B smoke's external producer phase barrier with a bounded
diagnostic checkpoint protocol. It establishes that every producer enrolled at
retirement start has observed the retirement epoch before the owner becomes
DEAD.

## Contract

```text
enrol producers before retirement begins
owner: ACTIVE -> RETIRING
capture the enrolled participant set and create retirement epoch E
each enrolled producer reaches a cold checkpoint and acknowledges E
only when all enrolled participants acknowledged E:
  owner may transition RETIRING -> DEAD
```

This is not a per-free owner check and does not prove safety for producers that
join after the epoch begins. L3-C deliberately requires enrollment before
retirement, so that its participant set is explicit and bounded.

## Boundary

`hz12_owner_epoch` is a diagnostic lifecycle coordinator. It is not used by
normal allocation/free, L1 inbox publication, source reclamation, or a speed
lane. Its lock and acknowledgement operations live only at a teardown
checkpoint.

## Acceptance

```text
ready_to_dead is false before all registered producers checkpoint
ready_to_dead becomes true after every registered producer checkpoints
owner registry accepts RETIRING -> DEAD only after readiness
participants unregister cleanly
repeat stability and no invalid owner transitions
```

## No-Go

```text
DEAD transition before all enrolled producers acknowledge the epoch
normal malloc/free calls an epoch function
late producer enrollment silently changes an in-flight participant set
epoch acknowledgement becomes the route/free safety authority
```
