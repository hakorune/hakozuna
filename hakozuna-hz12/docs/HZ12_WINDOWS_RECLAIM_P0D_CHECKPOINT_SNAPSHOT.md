# HZ12 Windows Reclaim P0-D Checkpoint Snapshot

P0-D moves reclaim authority out of continuous allocation/refill/flush state.
Normal allocator paths perform no reclaim-accounting update. At a quiescent
retirement or explicit pressure checkpoint, the returned sink is inspected
under its existing class lock.

For each owner span, the snapshot validates:

```text
route class is attached
owner slot and generation match
real owner inbox is empty
enrolled caches have checkpointed
each returned pointer is within the span
each pointer is aligned to the class slot size
no duplicate physical slot exists
unique returned slots == class slot capacity
```

A temporary checkpoint-local bitmap verifies uniqueness. It is not retained
and is never touched by malloc, free, refill, or normal cache flush.

## Diagnostic Result

The existing 64-span L6-D wide working-set shadow now compares three views:

1. per-operation atomic accounting;
2. continuous diagnostic bitmap ledger;
3. checkpoint-reconstructed returned-sink snapshot.

Windows repeat-10:

```text
atomic/ledger candidates: 64
snapshot candidates:      64
snapshot mismatches:      0
candidate bytes:          4 MiB
detach/decommit:           0 / 0 in the shadow executable
```

## Decision

P0-D is the first authority placement compatible with the performance gate by
construction: it adds no normal-path accounting. It is not yet an automatic
policy. The next gate is a snapshot-authorized bounded behavior sibling that
detaches/decommits at most 64 spans / 4 MiB at owner retirement, followed by
the existing local, MT, teardown, and RSS checks.

The continuous bitmap and compact-count lanes remain controls. They must not
be promoted or silently reintroduced into the speed lane.
