# HZ12 Windows Reclaim Review Fixes

## Review Disposition

The external review approved a retirement-only behavior sibling after one
blocking issue: reclaim could decommit and detach a span before discovering
that the depot was full. That could leave a decommitted, unrouted span outside
the depot.

The blocker is closed as follows:

1. reserve depot capacity before scanning or mutating spans;
2. cap the reclaim loop to the granted reservation;
3. serialize reclaim for one owner with a generation-aware CAS entry gate;
4. clear both retired-owner side tables only after decommit and before the
   reserved depot commit;
5. on owner-clear or depot-commit failure, recommit, reattach the route,
   restore the old owner token, and restore every returned slot;
6. count any failed restoration as a limbo span and require limbo to remain
   zero.

## Evidence

- Normal reclaim repeat-10: reserved/decommitted/depot/owner-cleared 64/64,
  limbo zero.
- No-cap repeat-10: reservation zero, detach/decommit/owner-clear/depot zero,
  limbo zero, RSS unchanged.
- Depot reservation smoke: a full reservation makes available capacity zero;
  releasing it restores 64; a full depot grants no further reservation.
- Reclaim entry smoke: the second concurrent entry for the same owner is
  rejected; release allows the next entry.
- P3 success and rollback lanes remain green after owner-token clearing.

## Lock-Tail Observation

During the P4 policy scan, a second thread repeatedly measured lock/unlock
latency on the same returned sink. Across repeat-10:

- p99: 0.1-0.2 microseconds;
- maximum: approximately 3.45-3.83 ms;
- full policy query: approximately 3.50-3.88 ms under the probe.

Most operations are unaffected, but one unlucky waiter can span nearly the
whole repeated scan. This is acceptable only for owner retirement evidence.
It reinforces the prohibition on frequent pressure polling. A future
production policy should use a cheaper owner-span index before periodic use.

## Line-Limit Correction

The previous PowerShell check used `Measure-Object -Line`, which did not match
physical file-line counting, and its default limit was 999. The checker now
uses `Get-Content(...).Count` with a default hard limit of 800. The actual
largest source is `hz12_thread_cache.c` at 799 physical lines.
