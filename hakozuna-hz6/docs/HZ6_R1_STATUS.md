# HZ6 R1 Status

HZ6-R1 is now an executable modular seed, not only a design note.

## Implemented

```text
API:
  include/hz6.h
  api/hz6_allocator.*

Contracts:
  route MISS / VALID / INVALID
  shared object states
  owner token/liveness helper
  profile configuration

Modules:
  route/
  transfer/
  source/
  frontcache/
  fronts/
  policy/

Fronts:
  toy:
    <= 4KiB contract-validation front

  local2p:
    exact 64KiB seed front
    exercises the Local2P route/front contract
    uses Linux mmap SourceLayer backing in the Linux R1 smoke

  midpage:
    >4KiB..32KiB seed front
    exercises the MidPage route/front contract
    uses OS-paged SourceLayer backing in the Linux R1 smoke

  large128:
    >4KiB..128KiB except exact 64KiB seed front
    exercises transfer-first reuse
    uses Linux mmap SourceLayer backing in the Linux R1 smoke
```

## Verified Behaviors

```text
route:
  exact pointer -> VALID
  interior HZ6-owned pointer -> INVALID
  foreign pointer -> MISS
  allocator routes through `Hz6RouteBackend`, currently exact-table backed
  backend wrapper preserves VALID / INVALID / MISS contract

local free:
  ACTIVE -> LOCAL_FREE
  local free requires descriptor owner == allocator owner
  second free before reuse is rejected
  local cache reuse returns the same pointer
  owner-dead descriptors become ORPHAN and are not locally reusable
  explicit orphan release unregisters route and releases SourceLayer memory
  ORPHAN descriptors can be adopted by a live allocator as LOCAL_FREE
  ACTIVE descriptors cannot be adopted

remote free:
  ACTIVE -> TRANSFER_FREE
  remote free clears descriptor owner while object is in transfer
  transfer-first malloc consumes transfer before source allocation
  transfer pop restores descriptor owner to the consuming allocator
  second remote free before reuse is rejected
  allocator routes through `Hz6TransferBackend`
  strict/rss profiles use single-cache transfer
  speed/remote profiles use sharded transfer
  backend wrapper preserves bounded push / class pop semantics

front registry:
  toy handles <=4KiB
  midpage handles >4KiB..32KiB
  local2p handles exact 64KiB
  large128 handles >32KiB..128KiB except exact 64KiB
  >128KiB is unsupported in R1 and returns NULL

source:
  Linux mmap ops validate through SourceLayer
  Windows VirtualAlloc ops are present behind _WIN32
  allocator-level source registry selects system vs OS-paged source
  OS-paged source maps to mmap on Linux and VirtualAlloc on Windows
  source registry lookup is covered by contract smoke
  reserve / commit / decommit / release smoke passes
  Large128 descriptors carry source kind / release metadata
  allocator destroy releases Large128 mappings through SourceLayer

scavenge:
  `scavenge/hz6_scavenge.*` provides bounded release accounting
  frontcache supports exact removal before LOCAL_FREE release
  allocator-level scavenging releases ORPHAN and LOCAL_FREE descriptors
  over-budget ORPHAN descriptors remain owned by the scavenge boundary
  over-budget LOCAL_FREE descriptors remain in the local cache
  route unregister + SourceLayer release is covered by safety smoke

policy:
  profile config carries slow-path scavenge budgets
  RSS profile can trigger explicit profile scavenging
  strict profile keeps automatic profile scavenging disabled
  policy-driven scavenging is covered by allocator and safety smoke
```

## Verification Command

```bash
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Expected output:

```text
hz6-r1-contract-smoke ok
hz6-r1-allocator-smoke ok
hz6-r1-safety-smoke ok
```

## Current Non-Claims

```text
No performance claim.
No preload integration.
No general span policy beyond the Local2P/Large128 mmap seeds.
No threaded transfer synchronization or performance claim for sharded transfer yet.
Cross-owner orphan adoption is seed-only and not a shared owner registry yet.
No real MidPage page-run/span policy yet.
No HZ5 source migration.
No HZ3/HZ4/HZ5 implementation copy.
```

## Next Engineering Targets

```text
1. Add route backend variants:
   Linux region/page route
   Windows sidecar route

2. Add MidPage real front seed.
```
