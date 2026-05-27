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
    uses a front-local 8K / 32K page-run policy seed
    can explicitly prefill one 64KiB SourceBlock into local-cache slots
    alloc miss tries transfer/local first, then run prefill before one-off source
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
  source block envelope pointer without exact slot descriptor -> INVALID
  exact slot route takes priority over source block invalid envelope
  foreign pointer -> MISS
  allocator routes through profile-selected `Hz6RouteBackend`
  backend wrapper preserves VALID / INVALID / MISS contract
  PAGE_TABLE backend carries explicit page granularity and is covered by
  contract smoke, including invalid-range envelopes
  SPEED / REMOTE profiles select PAGE_TABLE through ProfileConfig
  allocator smoke verifies SPEED / REMOTE PAGE_TABLE and STRICT / RSS EXACT_TABLE

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
  strict profile uses ACTIVE -> REMOTE_PENDING
  explicit owner drain moves REMOTE_PENDING -> LOCAL_FREE
  remote free clears descriptor owner while object is in transfer
  transfer-first malloc consumes transfer before source allocation
  transfer pop restores descriptor owner to the consuming allocator
  second remote free before reuse is rejected
  allocator routes through `Hz6TransferBackend`
  strict/rss profiles use single-cache transfer
  speed/remote profiles use sharded transfer
  backend wrapper preserves bounded push / class pop semantics
  backend exposes class counts, per-shard counts, and per-shard capacity for
  smoke diagnostics
  profile transfer capacity is applied during backend init and capped by
  `HZ6_TRANSFER_CACHE_CAPACITY`

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
  descriptors keep user ptr and source ptr separate
  SourceLayer release uses source ptr / source bytes
  front util can expose one user slot inside a larger source block
  `Hz6SourceBlock` can retain one source block across multiple slot descriptors
  and unregisters its invalid route envelope before releasing the backing source
  block
  duplicate SourceBlock slot registration is rejected without leaking refcount
  or poisoning the descriptor pool
  allocator destroy releases Large128 mappings through SourceLayer
  explicit front prefill can source objects into LOCAL_FREE cache using
  profile source_batch without changing the malloc hit path
  MidPage 8K and 32K run prefill paths both share one 64KiB SourceBlock per run

scavenge:
  `scavenge/hz6_scavenge.*` provides bounded release accounting
  frontcache supports exact removal before LOCAL_FREE release
  allocator-level scavenging releases ORPHAN and LOCAL_FREE descriptors
  shared SourceBlock descriptors are budgeted by user slot bytes
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
Sharded transfer observability is single-thread diagnostic only.
Cross-owner orphan adoption is seed-only and not a shared owner registry yet.
No real MidPage page-run/span policy yet.
No HZ5 source migration.
No HZ3/HZ4/HZ5 implementation copy.
```

## Next Engineering Targets

```text
1. Add route backend variants:
   Linux region/page route implementation
   Windows sidecar route

2. Add MidPage page-run front policy.
```
