# HZ6 Folder Layout

HZ6 should start with a clean source tree. The first implementation should not
reuse HZ5 file names unless the code is intentionally shared through a common
contract.

## Proposed Tree

```text
hakozuna-hz6/
  README.md

  include/
    hz6.h
    hz6_config.h
    hz6_contract.h

  api/
    hz6_allocator.h
    hz6_allocator.c

  route/
    hz6_route.h
    hz6_route.c
    hz6_route_backend.h
    hz6_route_backend.c
    win_route_sidecar.c
    linux_route_region.c

  frontcache/
    hz6_frontcache.h
    hz6_frontcache.c
    hz6_size_class.h
    hz6_size_class.c

  transfer/
    hz6_transfer.h
    hz6_transfer.c
    hz6_transfer_backend.h
    hz6_transfer_backend.c
    hz6_transfer_shard.h
    hz6_transfer_shard.c

  owner/
    hz6_owner.h
    hz6_owner.c
    hz6_orphan.h
    hz6_orphan.c

  source/
    hz6_source.h
    hz6_source.c
    hz6_source_registry.h
    hz6_source_registry.c
    linux_source_mmap.c
    win_source_virtualalloc.c

  scavenge/
    hz6_scavenge.h
    hz6_scavenge.c

  fronts/
    hz6_front.h
    hz6_front.c
    hz6_front_util.h
    hz6_front_util.c
    toy/
      hz6_toy_front.h
      hz6_toy_front.c
    local2p/
      hz6_local2p.h
      hz6_local2p.c
    midpage/
      hz6_midpage.h
      hz6_midpage.c
    large/
      hz6_large128_front.h
      hz6_large128_front.c
      hz6_large.h
      hz6_large.c

  policy/
    hz6_policy.h
    hz6_policy.c
    hz6_profiles.h
    hz6_profiles.c

  preload/
    linux_preload.c

  win/
    build scripts and Windows adapter entrypoints

  linux/
    build scripts and Linux runners

  tests/
    route_smoke.c
    transfer_smoke.c
    local2p_smoke.c
    safety_smoke.c

  docs/
    HZ6_BLUEPRINT.md
    HZ6_R1_MINIMUM_CONTRACT_BLUEPRINT.md
    HZ6_ARCHITECTURE_DRAFT.md
    HZ6_FOLDER_LAYOUT.md
    HZ6_MIGRATION_FROM_HZ5.md
    current_task.md
```

R1 currently starts with this modular subset:

```text
include/hz6.h
include/hz6_config.h
include/hz6_contract.h
api/hz6_allocator.h
api/hz6_allocator.c
route/hz6_route.h
route/hz6_route.c
route/hz6_route_backend.h
route/hz6_route_backend.c
frontcache/hz6_frontcache.h
frontcache/hz6_frontcache.c
frontcache/hz6_size_class.h
frontcache/hz6_size_class.c
transfer/hz6_transfer.h
transfer/hz6_transfer.c
transfer/hz6_transfer_backend.h
transfer/hz6_transfer_backend.c
owner/hz6_owner.h
source/hz6_source.h
source/hz6_source.c
source/hz6_source_registry.h
source/hz6_source_registry.c
source/linux_source_mmap.h
source/linux_source_mmap.c
policy/hz6_profiles.h
policy/hz6_profiles.c
fronts/hz6_front.h
fronts/hz6_front.c
fronts/hz6_front_util.h
fronts/hz6_front_util.c
fronts/toy/hz6_toy_front.h
fronts/toy/hz6_toy_front.c
fronts/large/hz6_large128_front.h
fronts/large/hz6_large128_front.c
fronts/local2p/hz6_local2p_front.h
fronts/local2p/hz6_local2p_front.c
tests/hz6_r1_contract_smoke.c
tests/hz6_r1_allocator_smoke.c
linux/build_hz6_r1_contract_smoke.sh
```

The Linux R1 smoke script uses explicit `HZ6_SOURCES` and `HZ6_INCLUDES`
arrays. Keep new modules visible there until a real build system is introduced.
It builds both the low-level contract smoke and allocator/front integration
smoke.
Linux mmap source ops are present as SourceLayer contract code, but Large128
now uses them in the Linux R1 smoke path through `source/hz6_source_registry.*`.
The toy front still uses the system source path because it exists only as a
contract-validation front.

The current API path is intentionally small:

```text
hz6_malloc:
  size -> coarse class
  frontcache pop
  source allocation through source/
  exact route registration

hz6_free:
  route backend lookup
  VALID exact pointer -> ACTIVE to LOCAL_FREE
  INVALID interior/double-free -> fail-closed stats
  MISS foreign pointer -> miss stats

hz6_free_remote:
  route backend lookup
  VALID exact pointer -> ACTIVE to TRANSFER_FREE
  bounded transfer backend push
  duplicate remote free -> fail-closed stats
```

This is not a performance allocator yet. It is the first executable contract
that proves the module boundaries compose.

Size-class mapping lives in `frontcache/hz6_size_class.*`, not in `api/`.
The API layer should call it rather than own class policy.

The current allocation behavior lives in `fronts/toy/`. It exists only to
exercise the contract while real fronts are still being selected.
The API reaches it through `Hz6FrontOps`, so the next real front can replace
the toy implementation without moving allocation policy back into `api/`.
Front selection lives in `fronts/hz6_front.*`; the API should not name a
specific real front.

`fronts/large/hz6_large128_front.*` is the first real-front seed. It handles
requests above the toy/small range up to 128KiB except exact 64KiB and exercises
`ACTIVE -> TRANSFER_FREE -> ACTIVE` through the same front registry.
`fronts/local2p/hz6_local2p_front.*` owns exact 64KiB requests first, so the
Local2P contract can evolve independently from the broader Large128 seed.
The toy front owns only requests up to 4KiB. Larger unsupported requests must
miss all fronts instead of being rounded down into a smaller allocation.
On Linux, this seed is backed by `source/linux_source_mmap.*`. Descriptors
store source kind and release metadata so allocator destroy and cache overflow
release through the correct SourceLayer instead of assuming system `free()`.
The Large128 front names only `HZ6_SOURCE_LINUX_MMAP`; it does not include the
Linux mmap implementation directly.
It is still a seed front, not a full LargeFront span policy.

Shared descriptor/cache/transfer transitions live in
`fronts/hz6_front_util.*`. Real fronts should keep only range/class decisions
and front-specific policy locally.

## Boundaries

### `route/`

Owns pointer classification only.

R1 uses `Hz6RouteBackend` as the allocator-facing seam. The only implemented
backend is the exact-table backend, but allocator/front utility code should go
through the backend wrapper so Linux region/page routing and Windows sidecar
routing can replace it without changing front logic.

```text
Allowed:
  sidecar lookup
  region/page lookup
  route generation checks

Forbidden:
  OS query on hot path
  allocation
  source refill
  transfer drain
```

### `frontcache/`

Owns class bins and hot cache hit/miss shape.

### `transfer/`

Owns cross-thread handoff shape.

R1 uses `Hz6TransferBackend` as the allocator-facing seam. The only
implemented backends are single-cache and fixed-shard cache. Front utility code
must go through the backend wrapper so later consumer-visible synchronized
transfer can replace the smoke-only shard backend without moving transfer
policy into individual fronts.

```text
Allowed:
  class lookup
  bin pop/push
  miss to refill

Forbidden:
  OS allocation directly
  unbounded remote drain
  policy learning
```

### `transfer/`

Owns cross-thread reusable object movement.

```text
Allowed:
  bounded transfer shards
  packet/chunk push
  consumer-visible pop

Forbidden:
  unbounded queues
  owner-dead publication
  hidden source refill
```

### `owner/`

Owns owner token lifecycle and strict owner handoff.

R1 seeds owner identity in `Hz6Allocator` and stores owner tokens on
descriptors. Local free requires the current owner, remote transfer clears the
owner while the object is `TRANSFER_FREE`, and transfer pop restores ownership
to the consuming allocator. Owner death/orphan handling is still future work.

```text
Allowed:
  owner generation
  liveness
  orphan handling
  strict owner inbox

Forbidden:
  being the only remote-fast-path mechanism
```

### `source/`

Owns OS memory.

```text
Allowed:
  reserve / commit / decommit / release
  platform-specific page granularity

Forbidden:
  route decisions
  front-cache policy
```

### `scavenge/`

Owns RSS return.

```text
Allowed:
  payload release
  empty span/slab release
  pressure thresholds

Forbidden:
  required work on cache hits
```

### `policy/`

Owns slow-path profile decisions.

```text
Allowed:
  choose source batch
  choose transfer cap
  choose scavenge threshold

Forbidden:
  malloc/free hit-path updates
```

## Initial Implementation Order

```text
1. include/ + docs/
2. route/ contract and smoke
3. source/ OS abstraction stubs
4. transfer/ bounded shard smoke
5. owner/ liveness contract
6. fronts/local2p exact profile
7. fronts/large 128K transfer proof
8. fronts/midpage general malloc proof
```

## Naming Rules

Use stable layer names in files and build lanes:

```text
route-r1
transfer128-r1
frontcache-mid-r1
local2p-win-r1
linux-general-r1
rss-profile-r1
strict-profile-r1
```

Avoid names that encode temporary numbers unless they are diagnostic only:

```text
bad default names:
  slots4
  rb64
  batch16
  sidecarfast

acceptable diagnostic names:
  transfer128-shard16-diagnostic
  local2p-slots4-control
```
