# HZ6 Current Task

HZ6 is in documentation-first design. Do not start implementation until the
contracts and folder layout are accepted.

## Current Claim

```text
HZ6 should be a route-safe, transfer-first, RSS-aware allocator family.

It should promote:
  HZ3 thin local cache
  HZ4 remote grouping
  HZ5 fail-closed descriptor ownership and low-RSS profile discipline

It should not directly port HZ3/HZ4/HZ5 internals.
```

## Active Documents

```text
hakozuna-hz6/README.md
hakozuna-hz6/docs/HZ6_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_R1_MINIMUM_CONTRACT_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_ARCHITECTURE_DRAFT.md
hakozuna-hz6/docs/HZ6_FOLDER_LAYOUT.md
hakozuna-hz6/docs/HZ6_MIGRATION_FROM_HZ5.md
```

## Proposed First Engineering Steps

```text
1. Review and tighten HZ6 folder layout.
2. Review HZ6_BLUEPRINT.md and select the first prototype.
3. Add only headers/contracts first:
   include/hz6_contract.h
   route/hz6_route.h
   transfer/hz6_transfer.h
   source/hz6_source.h
4. Add route smoke before allocator behavior.
5. Add transfer smoke before full malloc/free integration.
6. Choose first target:
   Windows Local2P exact route, or Linux LargeFront 128K transfer.
```

## Open Decisions

```text
Route table shape:
  Windows sidecar table and Linux region/page map should share result contract,
  not implementation.

Transfer shard policy:
  consumer-visible shard remains the default idea, but HZ5 variants were no-go
  as patches. HZ6 must test it from a clean contract.

Fail-closed timing:
  strict profile uses immediate validation.
  speed remote profile may use batch-boundary validation only if invalid
  pointers cannot be promoted to reusable bins.

Profile names:
  use role names such as hz6-speed, hz6-rss, hz6-remote, hz6-strict.
  keep numeric knobs diagnostic-only.
```

## Not Yet

```text
No source file migration.
No HZ5 code copy.
No new performance claim.
No benchmark table until an HZ6 prototype exists.
```
