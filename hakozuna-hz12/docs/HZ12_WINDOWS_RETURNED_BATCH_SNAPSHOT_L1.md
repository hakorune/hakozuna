# HZ12 Windows Returned Batch Snapshot L1

## Problem

The first retirement behavior validated each candidate span by traversing the
same returned-object sink independently. A 64-span, 64-byte-class retirement
therefore repeated a 65,536-object traversal 64 times in both the P4 advisory
pass and the separate P1 authority pass.

## Contract

`ReturnedBatchSnapshot-L1` is retirement-only and bounded to 64 spans:

- targets must be aligned arena spans of one size class;
- one class lock covers one complete sink traversal;
- every target keeps independent unique/duplicate/invalid slot evidence;
- advisory snapshot never mutates the sink;
- authority snapshot detaches only targets proven complete;
- incomplete targets remain unchanged;
- P4 advisory and P1 authority remain separate traversals;
- no counter, atomic, owner lookup, or branch is added to malloc/free.

The P1 caller retains depot reservation, route detach, decommit, owner-clear,
and rollback authority. If a later operation fails, all already detached but
unprocessed spans are restored to the returned sink.

## Windows Evidence

All reclaim, no-cap, rollback, depot, owner, inbox, and accounting smokes pass.
The repeated eight-generation turnover repeat-5 changed retirement p99 from
approximately 9.4-10.3 ms to 0.668-1.039 ms while retaining:

- 64/64 reclaimed spans;
- zero limbo spans;
- depot final count 64;
- post RSS 6.31 MiB and peak RSS 10.33 MiB.

Decision: `GO` as the bounded cold retirement implementation. This does not
change HZ12's speed lane or broaden its public throughput claims.
