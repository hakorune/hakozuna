# HZ12 Windows Owner Retire Gate L3-D

## Purpose

L3-D composes the L3-B token-inbox drain and L3-C producer epoch into one
diagnostic retirement decision. An owner may become DEAD only after both the
retirement epoch is acknowledged and its generation-bound inbox is empty.

## Gate

```text
ready_to_dead = epoch_ready(owner) && token_inbox_pending(owner) == 0
```

The query is intentionally a controlled teardown snapshot, not a lock-free
global proof. Producers must be enrolled before epoch start and must not publish
after their checkpoint.

## Acceptance

```text
gate blocks before producer checkpoints
gate blocks after checkpoints while inbox still has final batches
owner drains the exact final batch count
gate opens only after both conditions are true
only then may RETIRING -> DEAD execute
```

## Boundary

The retire gate is diagnostic-only and is not called by normal `hz12_malloc`,
`hz12_free`, or the L1 benchmark path. It makes teardown preconditions visible;
it does not make owner metadata a free-path safety authority.
