# HZ12 Windows Owner Ledger Reclaim L6-E

L6-E is the first behavior step using the combined owner-local authority. It
passes only the fixed L6-D 64-span success set to the previously proven L5
ordered detach and Windows decommit modules.

The shadow and behavior executables remain separate:

```text
bench_hz12_widews_owner_ledger_shadow.exe
  read-only candidate attribution

bench_hz12_widews_owner_ledger_reclaim.exe
  fixed 64-span ordered detach and decommit
```

## Required Authority

Before mutation, the behavior lane requires:

- producer epoch acknowledgement;
- diagnostic token inbox empty;
- real flush-owner inbox empty;
- generation-matched owner side table;
- 64 owner-ledger candidates;
- 64 atomic-shadow candidates;
- zero ledger/atomic mismatches.

The existing L5 detach order remains unchanged: front/cache/sink state first,
route last. Decommit runs only after the detached gate is open.

## Result

The behavior lane passed repeat-5:

```text
candidate spans:           64
detached spans:            64
decommitted spans:         64
failed spans:              0
decommitted bytes:         4,194,304
RSS before:                47,206,400..47,210,496 bytes
RSS after:                 43,020,288..43,024,384 bytes
observed RSS reduction:    4,186,112 bytes (~3.99 MiB)
```

The separate L6-D shadow executable still reports zero detach and zero
decommit, confirming that observation and behavior remain isolated.

## Positioning

This is HZ12 reclaim research, not an HZ8 default integration. HZ12 is proving
the owner-local, atomic-free production authority that may later become an HZ8
backend candidate. HZ8 must not adopt it until throughput, teardown, repeated
recommit/reuse, and broad RSS gates pass independently.

## Next Gate

L6-F connects the same fixed set to the existing bounded depot/recommit cycle,
then exercises real slot reuse and a second reclaim cycle. No automatic
wide-workload policy or HZ8 integration is allowed at L6-E.
