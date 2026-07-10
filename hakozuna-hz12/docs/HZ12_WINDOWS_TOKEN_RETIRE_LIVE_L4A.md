# HZ12 Windows Token Retire Live L4-A

## Purpose

L4-A is the first bounded live diagnostic lane for the L3 lifecycle pieces. Two
producers repeatedly allocate small objects and publish batches to a token-bound
inbox while the owner drains it. Retirement then freezes publication, collects
one final RETIRING batch from each producer, checks the L3-D gate, drains, and
transitions to DEAD.

## What It Proves

```text
live ACTIVE publish/drain remains bounded
RETIRING final batches are accepted
each producer checkpoint is observed by L3-C
L3-D blocks with queued final batches
the exact final queue drains before DEAD
no publish overflow or registry rejection occurs in the controlled lane
```

## What It Does Not Prove

```text
production allocator integration
per-free owner routing
unbounded dynamic producer enrollment
remote throughput improvement
thread-exit handling beyond the controlled quiescent protocol
```

The lane stays diagnostic-only. Its lifecycle locks and counters must not enter
normal `hz12_malloc` or `hz12_free`.
