# HZ6 Current Task

HZ6 is now in R1 implementation. Keep the implementation modular from the
start: API, route, frontcache, transfer, source, owner, policy, and future
fronts should stay separated.

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
hakozuna-hz6/docs/HZ6_R1_STATUS.md
hakozuna-hz6/docs/HZ6_R1_MINIMUM_CONTRACT_BLUEPRINT.md
hakozuna-hz6/docs/HZ6_ARCHITECTURE_DRAFT.md
hakozuna-hz6/docs/HZ6_FOLDER_LAYOUT.md
hakozuna-hz6/docs/HZ6_MIGRATION_FROM_HZ5.md
```

## Proposed First Engineering Steps

```text
1. Review and tighten HZ6 folder layout. DONE.
2. Review HZ6_BLUEPRINT.md and select the first prototype. DONE.
3. Add headers/contracts first. DONE:
   include/hz6_contract.h
   include/hz6.h
   api/hz6_allocator.h
   route/hz6_route.h
   transfer/hz6_transfer.h
   source/hz6_source.h
4. Add route smoke before allocator behavior. DONE.
5. Add transfer smoke before full malloc/free integration. DONE.
6. Add first toy allocation path. DONE:
   api/hz6_allocator.c
   source/hz6_source.c
   frontcache/hz6_frontcache.c
   route/hz6_route.c
7. Add first transfer-first toy path. DONE:
   hz6_free_remote()
   ACTIVE -> TRANSFER_FREE
   transfer pop before source allocation
8. Choose first real target. DONE:
   Linux LargeFront 128K transfer seed was selected first.
9. Keep modular cleanup moving before real target:
   size-class mapping moved to frontcache/hz6_size_class.*
   toy allocation/free transitions moved to fronts/toy/
   allocator API calls toy front through Hz6FrontOps
   front selection moved to fronts/hz6_front.*
10. First real-front seed. DONE:
   fronts/large/hz6_large128_front.*
   handles >4KiB..128KiB through the front registry
   uses transfer-first reuse in HZ6_PROFILE_REMOTE
11. Front utility common path. DONE:
   fronts/hz6_front_util.*
   removes duplicated descriptor/cache/transfer transitions from toy/large
12. Front range correctness. DONE:
   toy front limited to <=4KiB
   Large128 front handles >4KiB..128KiB
   unsupported >128KiB allocation returns NULL in R1 smoke
13. Build script hygiene. DONE:
   linux/build_hz6_r1_contract_smoke.sh uses source/include arrays
   new modules should be added to HZ6_SOURCES/HZ6_INCLUDES explicitly
14. Smoke split. DONE:
   tests/hz6_r1_contract_smoke.c covers route/transfer/owner/profile/source
   tests/hz6_r1_allocator_smoke.c covers allocator/front integration
15. Linux SourceLayer seed. DONE:
   source/linux_source_mmap.*
   reserve/commit/decommit/release covered by contract smoke
16. Large128 SourceLayer backing. DONE:
   descriptors carry source kind / release metadata
   Large128 uses Linux mmap source ops in the Linux R1 smoke
   destroy and cache overflow release through descriptor SourceLayer metadata
```

Current R1 smoke:

```bash
./hakozuna-hz6/linux/build_hz6_r1_contract_smoke.sh
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
No HZ5 source file migration.
No HZ5 code copy.
No new performance claim.
No benchmark table until an HZ6 prototype exists.
Toy allocation and transfer paths are for contract validation only.
Large128 is still a seed front, not a performance claim.
Large128 has Linux mmap backing, but no full span/source refill policy yet.
```
