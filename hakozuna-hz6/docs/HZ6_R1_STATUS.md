# HZ6 R1 Status

HZ6-R1 is now an executable modular seed, not only a design note.

## Implemented

```text
API:
  include/hz6.h
  api/hz6_allocator.*
  api/hz6_allocator.c owns owner token helpers and debug owner slot hooks
  api/hz6_allocator_init.c owns allocator initialization
  api/hz6_allocator_destroy.c owns allocator destruction, with descriptor and
  source-block cleanup split into helper units
  api/hz6_allocator_descriptor.c owns descriptor lifecycle helpers
  api/hz6_allocator_frontcache_mutation.c and
  api/hz6_allocator_frontcache_query.c own allocator-facing FrontCache
  wrappers
  api/hz6_allocator_malloc.c / api/hz6_allocator_free.c /
  api/hz6_allocator_free_remote.c own public malloc/free/remote-free
  entrypoints
  api/hz6_allocator_owns.c and api/hz6_allocator_stats_snapshot.c own
  owns/stats observation entrypoints
  api/hz6_allocator_prefill.c owns allocator-facing front prefill wrappers
  api/hz6_allocator_profile_query.c and api/hz6_allocator_profile_source.c own
  allocator-facing profile/source policy queries and source allocation stats
  notes
  policy/hz6_profiles_config.c and policy/hz6_profiles_policy.c split profile
  construction from shard/batch/budget policy helpers
  api/hz6_allocator_descriptor_local_cache.c and
  api/hz6_allocator_descriptor_remote_transfer.c split descriptor cache and
  remote-free state transitions
  api/hz6_allocator_orphan_release.c and
  api/hz6_allocator_orphan_adopt_prepare.c / api/hz6_allocator_orphan_adopt_commit.c own
  orphan release and cross-owner adoption separately
  api/hz6_allocator_remote_pending.c owns strict remote-pending drain
  api/hz6_allocator_route.c owns allocator-facing RouteLayer wrappers
  api/hz6_allocator_scavenge_orphans.c, api/hz6_allocator_scavenge_local_free.c,
  and api/hz6_allocator_scavenge_profile.c own allocator-facing bounded
  scavenge execution
  api/hz6_allocator_source_block.c owns SourceBlock lifecycle helpers
  api/hz6_allocator_descriptor_prepare.c / api/hz6_allocator_descriptor_release.c
  own descriptor source setup and release helpers
  source/hz6_source_system_ops.c and source/hz6_source_system_memory.c own
  source-system validation and malloc/free/release backing
  source/linux_source_mmap_ops.c and source/linux_source_mmap_memory.c own
  Linux mmap source ops assembly and reserve/commit/decommit/release backing
  source/win_source_virtualalloc_ops.c and source/win_source_virtualalloc_memory.c
  own Windows VirtualAlloc source ops assembly and reserve/commit/decommit/
  release backing
  fronts/hz6_front_source_kind.c and fronts/hz6_front_source_ops.c own
  source-kind wrappers and direct source-ops allocation
  api/hz6_allocator_owner_dead.c owns owner-dead transitions
  api/hz6_allocator_transfer_query.c and api/hz6_allocator_transfer_dispatch.c
  split allocator-facing TransferLayer observation from dispatch helpers
  api/hz6_allocator_types.h and api/hz6_allocator_api.h split allocator type
  definitions from public API declarations
  api/hz6_allocator_api_front.h, api/hz6_allocator_api_profile.h,
  api/hz6_allocator_api_route_transfer.h, api/hz6_allocator_api_scavenge.h,
  and api/hz6_allocator_api_state.h split public declarations by concern
  api/hz6_allocator_source_block_create.c and
  api/hz6_allocator_source_block_lifetime.c split SourceBlock creation from
  retain/release lifetime helpers
  api/hz6_allocator_init_state.c and api/hz6_allocator_init_backends.c split
  allocator state initialization from backend initialization, and
  api/hz6_allocator_init_state_owner.c / source_blocks.c / descriptors.c /
  frontcache.c split allocator state initialization into owner, source-block,
  descriptor, and frontcache helpers
  api/hz6_allocator_destroy_descriptors.c and
  api/hz6_allocator_destroy_source_blocks.c split allocator destruction into
  descriptor cleanup and source-block cleanup helpers
  hz6_stats_snapshot() is the public stats observation boundary
  allocator note helpers are the front-facing stats update boundary
  hz6_allocator_owner_token() is the front-facing owner token read boundary
  hz6_allocator_prepare_descriptor() is the front-facing descriptor source
  setup boundary
  allocator active-descriptor transition helpers centralize local-cache and
  remote-transfer state changes
  prefill smoke covers slow-path source/front/size prefill separately from
  allocator/front integration smoke
  SourceBlock smoke covers descriptor source release, shared source block
  slots, route envelopes, and MidPage run prefill separately from
  allocator/front integration smoke
  transfer smoke covers transfer backend profile wiring, Local2P transfer
  reuse, generic remote transfer reuse, producer/consumer shard behavior, and
  strict remote-pending drain separately from allocator/front integration smoke
  reclaim smoke covers orphan release/adoption, strict remote-pending drain,
  and profile scavenge separately from allocator/front integration smoke

Contracts:
  route MISS / VALID / INVALID
  route smoke covers exact table, invalid envelopes, backend wrappers, and page
  route backend separately from the lower-level mixed contract smoke
  public contract split keeps owner state and route result constructors in
  separate headers under include/
  allocator api state is now a thin umbrella over init/descriptor/owner/source
  /orphan helper headers
  route backend page-table variant is split into page-table wrapper, exact
  scan, and invalid scan helper units so PAGE_TABLE lookup stays separate from
  exact-table init/register helpers, while shared page-alignment math lives in
  route/hz6_route_backend_util.h
  route backend init/register/unregister/lookup helpers are split into separate
  helper units so table setup stays separate from route dispatch
  route table management is split into core/exact/invalid modules so entry
  init/register stays separate from route lookup and invalid-envelope
  handling
  transfer backend sharded variant is split into push/pop helpers so sharded
  push/pop and shard accounting stay separate from single-cache dispatch
  transfer backend observability is split into its own module so aggregate
  count/capacity helpers stay separate from sharded push/pop
  transfer backend sharded init/push/pop helpers are split into separate
  helper units so shard creation stays separate from shard push/pop dispatch
  transfer backend stats aggregate/shard helpers are split into separate helper
  units so aggregate count/capacity stays separate from shard indexing
  transfer contract smoke covers bounded single-cache and sharded backend
  push/pop, class filtering, shard preference, fallback, and uneven capacities
  source contract smoke covers source ops validation, Linux mmap source ops,
  source registry lookup, and ScavengeLayer budget accounting separately from
  the lower-level mixed contract smoke
  core contract smoke covers OwnerLayer equality/liveness, profile policy,
  size classes, and FrontCache primitives
  front source helpers and their header are split from reuse/free helpers so
  SourceLayer-backed allocation and prefill stay out of the reusable/free-route
  helper unit
  front registry lookup and prefill dispatch are split into separate helper
  units so front selection stays separate from prefill routing
  front source slot helpers are split from reusable source-reserve helpers so
  direct source-backed slot creation stays separate from reuse/prefill helpers
  SourceBlock-backed front slots are split into their own helper unit so
  shared source-block lifetime is not mixed with direct source reserve/prefill
  helpers
  descriptor source setup and release helpers are split into their own helper
  units so descriptor lifecycle state stays separate from source-backed
  initialization and release
  SourceLayer system ops and system memory helpers are split into separate
  helper units so validation stays separate from malloc/free/release backing
  Linux mmap source ops and Linux mmap memory helpers are split into separate
  helper units so page-size / ops assembly stays separate from reserve /
  commit / decommit / release
  Windows VirtualAlloc source ops and memory helpers are split into separate
  helper units so page-size / ops assembly stays separate from reserve /
  commit / decommit / release
  owner-dead transition helper is split into its own helper unit so owner
  shutdown state stays separate from orphan release/adoption flows
  cross-owner orphan adoption is split into prepare and commit helper units so
  source validation stays separate from route registration / front-cache
  adoption / source cleanup
  source block route-envelope registration is split from source block lifecycle
  helpers so route-backed invalid range setup stays separate from source block
  allocation and release
  front reuse/free helpers are split into separate helper units so transfer
  cache reuse and free-path dispatch stop sharing one utility module
  MidPage prefill helpers are split into their own helper unit so run-fill
  seeding stays separate from front policy and free-path dispatch
  MidPage policy helpers are split into their own helper unit so class-size
  mapping stays separate from run-fill and alloc/free dispatch
  MidPage front alloc/free/ops helpers are split into separate helper units so
  page-run selection stays separate from reuse and transfer dispatch
  MidPage prefill policy helper is split into its own helper unit so run
  geometry stays separate from the source-backed run-fill loop
  front source prefill helpers are split into their own helper unit and header
  so direct source reserve logic stays separate from slow-path prefill loops
  front source prefill one-shot helper is split from the loop wrapper so
  descriptor setup stays separate from repeated source prefill batching
  front reuse transfer helper is split from the cached/transfer wrappers so
  transfer-first reuse stays separate from wrapper selection
  Large128 front alloc/free/ops helpers are split into separate helper units so
  source refill stays separate from reuse and free-path dispatch
  shared object states
  owner token equality/liveness helpers
  profile configuration
  allocator-level profile queries for front/profile smoke diagnostics

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
    front alloc/free/prefill/ops helpers are split into separate units so exact
    64KiB routing stays separate from reuse and prefill dispatch

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
  allocator API wraps route lookup, backend kind, and page granularity
  diagnostics so callers do not need to inspect the route backend field directly
  hz6_allocator_route_unregister_exact() is the front-facing exact route
  removal boundary
  allocator route registration helpers wrap exact route registration and
  SourceBlock invalid-range registration for fronts

local free:
  ACTIVE -> LOCAL_FREE
  local free requires descriptor owner == allocator owner
  hz6_allocator_cache_active_descriptor() owns the local cache transition
  second free before reuse is rejected
  local cache reuse returns the same pointer
  owner-dead descriptors become ORPHAN and are not locally reusable
  explicit orphan release unregisters route and releases SourceLayer memory
  ORPHAN descriptors can be adopted by a live allocator as LOCAL_FREE
  ACTIVE descriptors cannot be adopted

remote free:
  ACTIVE -> TRANSFER_FREE
  strict profile uses ACTIVE -> REMOTE_PENDING
  hz6_allocator_remote_free_active_descriptor() owns the remote free transition
  explicit owner drain moves REMOTE_PENDING -> LOCAL_FREE
  remote free clears descriptor owner while object is in transfer
  transfer-first malloc consumes transfer before source allocation
  transfer pop restores descriptor owner to the consuming allocator
  second remote free before reuse is rejected
  allocator routes through `Hz6TransferBackend`
  strict/rss profiles use single-cache transfer
  speed/remote profiles use sharded transfer
  sharded push can take an explicit producer shard through
  hz6_transfer_backend_push_to_shard()
  FrontLayer remote free asks PolicyLayer for the producer shard before
  sharded transfer push
  backend wrapper preserves bounded push / class pop semantics
  sharded class pop retains non-target classes
  backend exposes class counts, per-shard counts, and per-shard capacity for
  smoke diagnostics
  allocator API wraps backend kind, capacity, count, class count, shard count,
  and shard capacity diagnostics so callers do not need to inspect the backend
  field directly
  sharded pop can steal from a non-home shard when the class home shard is empty
  sharded pop can also take an explicit consumer home shard through
  hz6_transfer_backend_pop_from_shard()
  FrontLayer transfer reuse asks PolicyLayer for the consumer home shard before
  sharded transfer pop
  uneven sharded capacity is filled without dropping remainder slots
  profile transfer capacity is applied during backend init and capped by
  `HZ6_TRANSFER_CACHE_CAPACITY`
  allocator API wraps profile id, transfer-first mode, strict remote mode,
  source kind, source refill batch, and profile transfer capacity diagnostics
  fronts and shared front utilities use allocator/profile helper APIs for
  profile decisions instead of reading allocator profile fields directly

  front registry:
  toy handles <=4KiB
  midpage handles >4KiB..32KiB
  local2p handles exact 64KiB
  large128 handles >32KiB..128KiB except exact 64KiB
  >128KiB is unsupported in R1 and returns NULL
  front source slot creation is split into kind/ops helpers so source-kind
  lookup stays separate from direct source-backed slot creation
  FrontOps carries an optional class-aware prefill hook for front-specific
  slow-path refill
  hz6_front_prefill_by_id() exposes prefill through the front registry
  hz6_front_prefill_by_id_class() exposes class-specific prefill through the
  front registry
  hz6_allocator_prefill_front() and hz6_allocator_prefill_front_class() expose
  registry prefill through the allocator API
  hz6_allocator_prefill_size() can select a prefillable front by allocation size
  without exposing front ids to callers
  allocator frontcache helpers wrap push, pop, remove, count, and capacity so
  fronts do not need to inspect allocator frontcache bins directly
  allocator transfer helpers wrap transfer push/pop and profile shard selection
  so fronts do not need to inspect transfer backend or shard policy directly

source:
  Linux mmap ops validate through SourceLayer
  Windows VirtualAlloc ops are present behind _WIN32
  allocator-level source registry selects system vs OS-paged source
  source registry init and lookup are split into separate units so platform
  seeding stays separate from registry lookup
  hz6_allocator_source_ops() is the front-facing source ops lookup boundary
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
  profile config owns the default source kind used by source-backed fronts
  explicit front prefill can source objects into LOCAL_FREE cache using
  profile source_batch without changing the malloc hit path
  allocator size prefill routes Large128, Local2P, and MidPage through their
  front hooks
  Large128 alloc miss uses profile source_batch as a slow-path refill policy
  capped by the frontcache bin capacity
  Local2P alloc miss uses profile source_batch as a slow-path refill policy
  capped by the frontcache bin capacity
  Local2P exposes a front-specific prefill wrapper for profile source_batch
  Large128 exposes a front-specific prefill wrapper for profile source_batch
  MidPage exposes run prefill through the class-aware FrontOps prefill hook
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
  profile config exposes scavenge budgets through PolicyLayer helpers
  profile config carries source kind for source-backed fronts
  profile config owns source refill batch selection through
  hz6_profile_source_refill_batch()
  profile config names the route backend policy as EXACT_TABLE or PAGE_TABLE
  profile config owns transfer producer/consumer shard seed selection
  profile config names the transfer shard policy as OWNER_SLOT or CLASS_ID
  RSS profile can trigger explicit profile scavenging
  strict profile keeps automatic profile scavenging disabled
  policy-driven scavenging is covered by allocator and safety smoke
```

## Large-Size Boundary

HZ6-R1 has a Large128 seed, not a complete large-object family.

```text
implemented:
  >32KiB..128KiB except exact 64KiB

not implemented yet:
  >128KiB broad large-object classes
  ordinary-malloc preload coverage for all large sizes
  HZ3/HZ4/HZ5/tcmalloc cross-family large-size comparison
```

Design direction:

```text
L1:
  stabilize the 128K transfer-first front and keep double-return impossible.

L2:
  add 256K / 512K / 1M LargeSpan classes using the same route, transfer,
  source, and scavenge contracts instead of adding a separate large allocator.

L3:
  run a full same-machine comparison against HZ3/HZ4/HZ5/tcmalloc after broad
  large-size coverage exists.
```

Reporting rule:

```text
Until L2 lands, describe HZ6 large results as Large128 seed results.
Do not claim broad large-object allocator coverage from R1.
```

## Verification Command

```bash
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

Expected output:

```text
hz6-r1-core-contract-smoke ok
hz6-r1-route-smoke ok
hz6-r1-transfer-contract-smoke ok
hz6-r1-source-contract-smoke ok
hz6-r1-allocator-smoke ok
hz6-r1-prefill-smoke ok
hz6-r1-sourceblock-smoke ok
hz6-r1-transfer-smoke ok
hz6-r1-reclaim-smoke ok
hz6-r1-safety-smoke ok
```

## Current Non-Claims

```text
HZ6 has had its first benchmark run, but the evidence is still HZ6-only and
provisional.
Benchmark harnesses are prepared, but no cross-family benchmark table has been
published yet.
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
