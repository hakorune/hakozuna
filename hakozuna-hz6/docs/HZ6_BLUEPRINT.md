# HZ6 Blueprint

HZ6 is a route-safe, transfer-first, RSS-aware allocator family. This blueprint
is the implementation-facing design map. It is intentionally stricter than the
architecture draft: code should not start until the chosen prototype can satisfy
the contracts below.

## One-Screen Design

```text
malloc(size):
  class = SizeClass(size)
  p = FrontCache.pop(class)
  if p:
    return p
  return RefillSlow(class)

free(ptr):
  route = RouteLayer.lookup(ptr)
  if route == MISS:
    return RealAllocator.free(ptr)
  if route == INVALID:
    return FailClosed(ptr)
  return Front.free_tagged(ptr, route)

remote free:
  RouteLayer.lookup(ptr)
  ACTIVE -> TRANSFER_FREE or REMOTE_PENDING
  TransferLayer.publish(class, object)

alloc miss:
  FrontCache local refill
  TransferLayer consume
  OwnerLayer strict fallback
  SourceLayer allocate
  ScavengeLayer pressure handling
```

The key rule is:

```text
Reusable remote objects are consumed before new source refill.
```

HZ5 repeatedly showed that source refill and owner drain timing can dominate
throughput and RSS. HZ6 must make the reuse-before-source order explicit.

## What HZ6 Takes From Earlier Allocators

### HZ3 Lessons

```text
take:
  thin local/TLS front path
  simple size-class cache
  low branch count on cache hits

do not take:
  old large-cache implementation details
  route-specific special cases without a common ownership contract
```

### HZ4 Lessons

```text
take:
  remote grouping
  page/span-local reuse
  separated hot path and drain/control path

do not take:
  unsafe header shortcuts
  remote object lists that bypass descriptor validation
```

### HZ5 Lessons

```text
take:
  descriptor ownership
  fail-closed route behavior
  low-RSS profile discipline
  profile family instead of one universal default

do not take:
  owner inbox as the only fast remote path
  transfer cache as a late diagnostic patch
  hot-path counters or hot-path learning
```

## Contract Graph

```text
RouteLayer
  produces RouteResult
  consumed by FrontCache, Fronts, OwnerLayer

FrontCache
  consumes SizeClass and RouteResult
  produces cache hits or refill requests

TransferLayer
  consumes remote-free objects and refill requests
  produces reusable objects before source refill

OwnerLayer
  owns strict owner semantics, generation, orphaning
  used by strict profile and fallback paths

SourceLayer
  owns OS memory only
  called after local/transfer/owner reuse fails

ScavengeLayer
  receives empty/retained span events
  returns RSS on pressure and checkpoint

PolicyLayer
  reads slow-path observations only
  writes profile/cap choices only at slow-path boundaries
```

## Route Contract

RouteLayer must be the first safety gate.

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

Required behavior:

```text
MISS:
  pointer is not HZ6-owned
  may go to real allocator path

VALID:
  pointer is HZ6-owned and descriptor is safe to use

INVALID:
  pointer is HZ6-owned-looking but invalid
  must fail closed
  must not fall through to real allocator
```

Forbidden:

```text
VirtualQuery on hot Windows route
header direct dereference before route validation
foreign pointer probing that can fault
route lookup that allocates memory
```

## State Contract

Use a small shared state vocabulary even if each front stores it differently.

```text
ACTIVE:
  user owns the object

LOCAL_FREE:
  local front cache owns the object

TRANSFER_FREE:
  transfer cache owns the object

REMOTE_PENDING:
  strict owner inbox owns the object

ORPHAN:
  original owner died or generation mismatched

DEAD:
  retired / released / debug poison
```

Allowed transitions:

```text
local alloc:
  LOCAL_FREE -> ACTIVE

local free:
  ACTIVE -> LOCAL_FREE

fast remote free:
  ACTIVE -> TRANSFER_FREE

strict remote free:
  ACTIVE -> REMOTE_PENDING

transfer consume:
  TRANSFER_FREE -> ACTIVE

owner drain consume:
  REMOTE_PENDING -> ACTIVE or LOCAL_FREE

owner death:
  LOCAL_FREE / REMOTE_PENDING -> ORPHAN
```

No transition may allow the same object to be present in two caches at once.

## Transfer Contract

TransferLayer is the main difference between HZ5 and HZ6.

```text
HZ5:
  transfer was diagnostic and often layered after owner inbox

HZ6:
  transfer is the default remote reuse path for speed/remote profiles
```

Initial cache shape:

```text
small/mid:
  page/slab packets with slot bitsets

large:
  span pointer packets

exact Local2P:
  exact-object packets or bins
```

Transfer cache requirements:

```text
bounded by object count and bytes
consumer-visible
source refill checks transfer first
overflow has a safe fallback
owner-dead or generation-mismatch objects cannot be published as reusable
```

Preferred shard model:

```text
push:
  producer batches to shard by CPU/thread/round-robin

pop:
  consumer checks home shard
  then bounded steal from nonempty mask

fallback:
  strict owner inbox or source path
```

The HZ5 no-go variants are explicitly not default models:

```text
single global transfer:
  possible diagnostic, not default

producer TLS retention:
  can starve consumers

old-owner shard:
  often misses next consumer
```

## FrontCache Contract

FrontCache hit path must be thin.

```text
malloc hit:
  class lookup
  bin pop
  optional compact state transition
  return pointer

free local hit:
  route lookup
  compact state transition
  bin push
```

FrontCache miss path may:

```text
drain local partial pages
consume transfer cache
call owner strict fallback
call source refill
trigger scavenge pressure checks
```

FrontCache hit path must not:

```text
read policy state
update learning counters
walk owner inbox
call source refill directly
call OS APIs
```

## Source And Scavenge Contract

SourceLayer must not decide allocator policy. It only supplies memory.

```text
source_refill(class):
  assert FrontCache miss
  assert TransferLayer was checked first
  reserve/commit memory
  register route descriptors
  return span/slab to front
```

ScavengeLayer owns memory return:

```text
empty slab/span:
  enters retention list

pressure:
  release payload or whole region if safe

checkpoint:
  optional release according to profile
```

RSS must be measured for every profile claim.

## Policy Contract

Policy is allowed only on slow paths.

Allowed update points:

```text
source refill
transfer flush
owner drain
epoch/checkpoint
scavenge event
thread exit
```

Forbidden:

```text
malloc/free hit-path counters
atomic statistics in speed lanes
per-allocation adaptive decisions
```

First policy should be rule-based, not bandit/ML:

```text
if RSS pressure high:
  reduce transfer/source caps

if refill pressure high and RSS low:
  allow larger transfer/source caps

else:
  stay conservative
```

## Prototype Plan

### HZ6-R1: Contract Skeleton

Files:

```text
include/hz6_contract.h
route/hz6_route.h
transfer/hz6_transfer.h
source/hz6_source.h
owner/hz6_owner.h
```

Goal:

```text
compile shared contracts
no allocator behavior
no OS-specific hot path yet
```

Acceptance:

```text
Linux build passes
Windows build passes
headers do not include platform-only APIs
```

### HZ6-R2: Route Smoke

Goal:

```text
RouteLayer returns MISS / VALID / INVALID
Windows sidecar route and Linux descriptor route share the same result type
```

Acceptance:

```text
foreign pointer -> MISS
owned pointer -> VALID
owned-looking invalid pointer -> INVALID
INVALID never falls through to real free
```

### HZ6-R3: Transfer Smoke

Goal:

```text
bounded transfer cache can publish and consume one class
```

Acceptance:

```text
ACTIVE -> TRANSFER_FREE -> ACTIVE
double publish rejected
double consume impossible
overflow bounded and safe
```

### HZ6-R4: First Front

Choose one:

```text
Windows Local2P exact:
  best if starting from Windows sidecar numbers

Linux Large 128K:
  best if testing transfer-first reuse directly

Linux MidPage:
  best if testing general malloc front cache
```

Default recommendation:

```text
Start with Route Smoke, then Windows Local2P exact.
Use Linux Large 128K as the first transfer stress test.
```

## Acceptance Matrix

HZ6 cannot claim success with throughput alone.

```text
safety:
  owned invalid fails closed
  double-free cannot become reusable
  stale owner generation cannot publish reusable object

performance:
  hot cache hits have no policy counter updates
  transfer is consumed before source refill
  no unbounded drain on alloc hit

RSS:
  transfer and local caches are byte-bounded
  empty spans/slabs have a scavenge path
  peak and final RSS are reported

portability:
  Linux and Windows share contracts
  OS memory operations stay behind SourceLayer
  route implementations may differ without changing Front/Transfer contracts
```

## No-Go Rules

Stop and redesign if any prototype requires:

```text
VirtualQuery or equivalent OS query on hot free
header direct dereference before route validation
unbounded transfer cache
source refill before transfer lookup
hot-path learning counters
owned-invalid fallback to real allocator
silent double-free acceptance
single global transfer cache as the only remote mechanism
```

## Documentation Discipline

Keep HZ6 docs small and role-based.

```text
HZ6_BLUEPRINT.md:
  implementation-facing design map

HZ6_ARCHITECTURE_DRAFT.md:
  higher-level rationale

HZ6_FOLDER_LAYOUT.md:
  file ownership and boundaries

HZ6_MIGRATION_FROM_HZ5.md:
  what to promote and what to leave behind

current_task.md:
  current work only, no long logs
```

