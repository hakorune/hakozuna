# HZ6 Architecture Draft

HZ6 is a transfer-first allocator family. Its job is to combine the strongest
lessons from HZ3, HZ4, and HZ5 without inheriting their accidental structure.

## Design Thesis

HZ5 proved that descriptor-owned, fail-closed, low-RSS profiles are practical.
It also showed where tcmalloc-like allocators stay hard to catch:

```text
mixed class traffic
remote handoff timing
owner inbox drain cost
source refill pressure
route/free classification cost
```

HZ6 treats those as architecture boundaries instead of per-front-end tuning
details.

## Layers

```text
RouteLayer
  pointer -> route descriptor
  returns MISS / VALID / INVALID

FrontCache
  thin per-thread or per-CPU size-class cache
  hot malloc/free path

TransferLayer
  bounded cross-thread/class handoff
  primary fast remote-free path

OwnerLayer
  owner token, generation, liveness, orphan rules
  strict owner-inbox fallback

SourceLayer
  OS-backed span/slab source
  reserve / commit / decommit / release

ScavengeLayer
  RSS pressure and payload release
  never required on malloc/free hits

PolicyLayer
  slow-path-only selection of caps, batches, and profiles
```

## RouteLayer Contract

Route is the first layer because HZ6 must be safe before it is clever.

```c
typedef enum Hz6RouteKind {
    HZ6_ROUTE_MISS = 0,
    HZ6_ROUTE_VALID = 1,
    HZ6_ROUTE_INVALID = 2
} Hz6RouteKind;

typedef struct Hz6RouteResult {
    Hz6RouteKind kind;
    uint16_t front_id;
    uint16_t class_id;
    uint32_t generation;
    void* descriptor;
} Hz6RouteResult;
```

Rules:

```text
MISS:
  not HZ6-owned; may go to real allocator / foreign path

VALID:
  HZ6-owned; dispatch to front-specific free

INVALID:
  HZ6-owned-looking but invalid; fail closed, do not fallback
```

Windows route should be sidecar-first. Linux route should use region/page or
descriptor maps. Direct header reads are allowed only after a route result makes
the descriptor safe to touch.

## FrontCache Contract

FrontCache is the HZ3 lesson promoted to a first-class layer.

```text
malloc hit:
  size -> class
  bin pop
  return

free local hit:
  route lookup
  local bin push or local state transition
```

Hot-path restrictions:

```text
no diagnostic counter updates
no policy learning
no OS query
no unbounded list walk
no source refill before transfer checks
```

## TransferLayer Contract

TransferLayer is the HZ4 lesson promoted to the main remote path.

```text
remote free:
  ACTIVE -> TRANSFER_FREE
  push to bounded transfer cache

consumer alloc miss:
  local bin/list
  transfer cache
  owner inbox fallback
  source
```

Initial transfer shapes:

```text
small/mid:
  page or slab packet with slot bitsets

large:
  span pointer transfer

exact Local2P:
  exact-object transfer bin
```

The preferred cache is consumer-visible:

```text
push:
  producer distributes into shard/chunk

pop:
  consumer checks home shard, then bounded steal
```

HZ5 already showed the weak variants:

```text
single global transfer:
  can win low-thread rows but may contend at higher thread counts

producer TLS retention:
  can starve consumers

old-owner shard:
  often does not match the next consumer
```

HZ6 should treat those as diagnostics, not default design.

## LargeSpan CentralSpanPool Contract

LargeSpan should not be owner-inbox-first and should not treat transfer as a
temporary add-on. HZ5 LargeFront 128K showed that owner inbox, single global
transfer, producer TLS retention, old-owner shards, and consumer shards all
produce row-specific behavior when layered on top of an owner-centric design.

For HZ6 LargeSpan, make ownerless central reuse a first-class state:

```text
remote free:
  ACTIVE(owner=A) -> CENTRAL_FREE(ownerless)

alloc miss:
  tiny local cache
  CentralSpanPool class pop
  SourceDepot
  OS source

owner inbox:
  strict/debug/fallback profile only
```

LargeSpan states:

```text
ACTIVE:
  user owns the span

LOCAL_FREE:
  tiny owner/thread-local cache owns the span

CENTRAL_FREE:
  ownerless central class pool owns the span

REMOTE_PENDING:
  strict owner-inbox profile owns the span

RELEASED:
  descriptor retained, payload decommitted

ORPHAN:
  owner died, generation mismatched, or route became invalid
```

Rules:

```text
free requires ACTIVE
CENTRAL_FREE -> ACTIVE assigns the consuming owner
CENTRAL_FREE is bounded by bytes, not only span count
source refill must check CentralSpanPool before SourceDepot / OS
payload commit/decommit is controlled by SourceLayer and ScavengeLayer
```

Initial LargeSpan scope:

```text
L1:
  stabilize 128K CentralSpanPool and state transitions

L2:
  add 256K / 512K / 1M classes behind the same LargeSpan backend

L3:
  add full preload coverage and compare against HZ3/HZ4/HZ5/tcmalloc
```

## OwnerLayer Contract

Owner inboxes are still needed, but they should not be the only fast remote
path.

```text
fast profiles:
  remote free -> transfer cache
  owner inbox -> fallback/control path

strict profiles:
  remote free -> owner inbox
  stronger immediate owner semantics
```

Owner tokens must include liveness and generation. Stale owner publication is a
correctness bug.

## Source And Scavenge Contracts

SourceLayer abstracts the OS. ScavengeLayer owns RSS pressure.

```c
typedef struct Hz6OsMemoryOps {
    void* (*reserve)(size_t bytes, size_t align);
    int (*commit)(void* p, size_t bytes);
    int (*decommit)(void* p, size_t bytes);
    int (*release)(void* p, size_t bytes);
    size_t page_size;
    size_t allocation_granularity;
} Hz6OsMemoryOps;
```

Rules:

```text
Linux:
  mmap / madvise / munmap

Windows:
  VirtualAlloc / VirtualFree
  no VirtualQuery on hot paths

Both:
  source refill checks transfer before allocating new OS-backed spans
```

## PolicyLayer Contract

PolicyLayer may select batch size, transfer cap, source batch, and scavenge
thresholds. It must run only at slow points:

```text
source refill
transfer flush
owner drain
checkpoint
epoch boundary
scavenge pressure event
```

No hot malloc/free counter updates are allowed in speed lanes.

## Profile Family

HZ6 should start as a profile family:

```text
hz6-speed:
  thin FrontCache
  transfer-first
  batch-boundary fail-closed where safe

hz6-rss:
  smaller transfer caps
  stronger scavenge
  conservative source refill

hz6-remote:
  larger transfer windows
  class transfer prioritized

hz6-strict:
  immediate validation
  owner-inbox-first remote handoff

hz6-win-local2p:
  Windows exact 64K/a8192 sidecar route profile

hz6-linux-general:
  Linux ordinary malloc front-end family
```

## First Experiments

```text
R1 RouteLayer:
  common MISS / VALID / INVALID contract
  Windows sidecar route
  Linux descriptor route

T1 TransferClass128:
  Large 128K consumer-visible transfer cache
  ACTIVE -> TRANSFER_FREE -> ACTIVE

F1 FrontCacheMid:
  MidPage-style slab backend behind thin class bins
```

Success criteria:

```text
owned invalid never falls back
double-return is impossible
transfer is bounded
source refill does not bypass reusable transfer supply
RSS remains a reported first-class metric
```
