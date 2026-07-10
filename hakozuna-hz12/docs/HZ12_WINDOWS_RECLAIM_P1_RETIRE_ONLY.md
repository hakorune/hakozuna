# HZ12 Windows Reclaim P1: Retire-Only Snapshot Behavior

## Decision

P1 is a bounded diagnostic behavior lane. It is not an automatic pressure or
epoch policy and it does not enter the normal speed lane.

The authority sequence is:

1. the owner retirement epoch is acknowledged;
2. the token inbox and real flush-owner inbox are empty;
3. the span belongs to the retiring owner in the side table;
4. a lock-held returned-sink snapshot proves every physical slot is present,
   aligned, and unique;
5. the same lock-held helper validates again and detaches the complete set;
6. the route is detached last, then Windows decommits the 64 KiB payload.

The behavior is capped at 64 spans (4 MiB) per call. A route or OS failure
restores the route and complete returned-slot set where possible and stops the
batch. Continuous bitmap/count ledgers remain diagnostic controls only.

## Lane Separation

- `bench_hz12_widews_owner_ledger_shadow.exe`: P0-D authority judge; no detach
  or decommit.
- `bench_hz12_widews_snapshot_reclaim.exe`: P1 retire-only behavior sibling.
- Existing L6-E/L6-F lifecycle executables remain atomic-ledger diagnostics.

The P1 executable still links the atomic and bitmap judges to verify the test
fixture before mutation. `hz12_snapshot_reclaim.c` itself does not query or
link either accounting authority.

## Windows Result

Repeat-10 passed:

- candidates: 64/64
- detached: 64/64
- decommitted: 64/64
- failures: 0
- payload released: 4,194,304 bytes
- working-set reduction: 4,194,304 bytes in every run

The P0-D shadow control continued to report 64 snapshot candidates, zero
mismatches, and zero detach/decommit behavior.

## Next Gate

P1 proves that retirement-only snapshot authority can safely produce the RSS
effect without continuous hot-path accounting. It does not justify automatic
reclaim. The next experiment should add a bounded depot handoff for these P1
spans, still at owner retirement and still outside the speed lane, before any
pressure-triggered policy is considered.
