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
    hz6_route_table.h
    hz6_route_table.c
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
frontcache/hz6_frontcache.h
frontcache/hz6_frontcache.c
frontcache/hz6_size_class.h
frontcache/hz6_size_class.c
transfer/hz6_transfer.h
transfer/hz6_transfer.c
owner/hz6_owner.h
source/hz6_source.h
source/hz6_source.c
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
tests/hz6_r1_contract_smoke.c
linux/build_hz6_r1_contract_smoke.sh
```

The Linux R1 smoke script uses explicit `HZ6_SOURCES` and `HZ6_INCLUDES`
arrays. Keep new modules visible there until a real build system is introduced.

The current API path is intentionally small:

```text
hz6_malloc:
  size -> coarse class
  frontcache pop
  source allocation through source/
  exact route registration

hz6_free:
  route lookup
  VALID exact pointer -> ACTIVE to LOCAL_FREE
  INVALID interior/double-free -> fail-closed stats
  MISS foreign pointer -> miss stats

hz6_free_remote:
  route lookup
  VALID exact pointer -> ACTIVE to TRANSFER_FREE
  bounded transfer push
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
requests above the toy/small range up to 128KiB and exercises
`ACTIVE -> TRANSFER_FREE -> ACTIVE` through the same front registry.
The toy front owns only requests up to 4KiB. Larger unsupported requests must
miss all fronts instead of being rounded down into a smaller allocation.

Shared descriptor/cache/transfer transitions live in
`fronts/hz6_front_util.*`. Real fronts should keep only range/class decisions
and front-specific policy locally.

## Boundaries

### `route/`

Owns pointer classification only.

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
