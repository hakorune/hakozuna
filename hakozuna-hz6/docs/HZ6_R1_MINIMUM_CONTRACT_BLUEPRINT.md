# HZ6 R1 Minimum Contract Blueprint

HZ6-R1 is the smallest implementation milestone. It must prove the contracts
compose before any broad allocator behavior is attempted.

## R1 Scope

R1 should implement one narrow target first.

Preferred order:

```text
1. Route contract smoke
2. Windows Local2P exact route proof
3. Linux Large 128K transfer proof
```

Do not attempt all fronts in R1.

## Public Operation Flow

### `malloc`

```text
size -> size class
FrontCache pop
if hit:
  return pointer

RefillSlow:
  local partial/cache
  TransferLayer consume
  OwnerLayer strict fallback
  SourceLayer refill
```

### `free`

```text
RouteLayer.lookup(ptr)

MISS:
  real allocator / foreign path

VALID:
  front-specific free_tagged(ptr, route)

INVALID:
  fail closed
  never real allocator fallback
```

## RouteResult Contract

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

### Trusted Fields

```text
MISS:
  no fields except kind are trusted

VALID:
  front_id, class_id, generation, descriptor are trusted
  descriptor may be used by the selected front

INVALID:
  front_id/class_id may be diagnostic only
  descriptor must not be used unless route backend explicitly marks it safe
```

### Route Lookup Must Not

```text
allocate
block on source refill
call VirtualQuery or equivalent hot OS query
dereference user-adjacent headers before route validation
return MISS for owned-looking invalid pointers
```

Interior pointers are front-specific. R1 default:

```text
interior pointer into HZ6-owned object:
  INVALID

exact base pointer:
  VALID
```

## Object State Table

R1 uses the shared state vocabulary even if storage differs per front.

| State | Meaning | Reusable by malloc? |
| --- | --- | --- |
| `ACTIVE` | User owns object | No |
| `LOCAL_FREE` | Owner/local cache owns object | Yes, local only |
| `TRANSFER_FREE` | Transfer cache owns object | Yes, through transfer |
| `REMOTE_PENDING` | Strict owner inbox owns object | Yes, after owner drain |
| `ORPHAN` | Owner generation/liveness mismatch | No until repaired |
| `DEAD` | Retired/released/debug poisoned | No |

Allowed R1 transitions:

```text
LOCAL_FREE -> ACTIVE
ACTIVE -> LOCAL_FREE
ACTIVE -> TRANSFER_FREE
TRANSFER_FREE -> ACTIVE
ACTIVE -> REMOTE_PENDING
REMOTE_PENDING -> ACTIVE
REMOTE_PENDING -> LOCAL_FREE
LOCAL_FREE / REMOTE_PENDING -> ORPHAN
```

Forbidden:

```text
TRANSFER_FREE -> LOCAL_FREE without transfer ownership proof
ORPHAN -> ACTIVE on hot path
DEAD -> ACTIVE
any state -> two caches simultaneously
```

## Transfer Contract

R1 transfer cache is bounded and class-scoped.

### Push

```text
remote free validates route
state ACTIVE -> TRANSFER_FREE
object enters bounded transfer cache
```

### Pop

```text
alloc miss checks transfer before source
state TRANSFER_FREE -> ACTIVE
consumer becomes current owner if owner is tracked
```

### Overflow

Overflow must be explicit:

```text
preferred:
  fallback to strict owner inbox

allowed:
  fallback to global/source-controlled free list

forbidden:
  drop object
  unbounded queue growth
  silent source refill while reusable transfer objects exist
```

### Stale Owner

If owner generation/liveness is relevant and stale:

```text
do not publish as reusable
move to ORPHAN or strict repair path
```

## Source Order Contract

Source refill order is part of correctness and RSS behavior.

```text
1. FrontCache local
2. TransferLayer consume
3. OwnerLayer fallback
4. SourceLayer allocate
5. ScavengeLayer pressure actions
```

SourceLayer must not allocate before TransferLayer has had a chance to satisfy
the request.

## Memory-Ordering Minimum

R1 should keep this simple.

```text
cross-thread publish:
  release store/CAS before making object visible

cross-thread consume:
  acquire load/CAS before returning object

owner-local cache:
  may use plain stores only when no other thread can observe the object

state transitions that detect double-free:
  CAS or exchange-based check
```

R1 should document every transition that uses plain stores.

## Acceptance Smokes

R1 route smoke:

```text
foreign pointer -> MISS
owned exact pointer -> VALID
owned interior pointer -> INVALID
owned stale/corrupt route -> INVALID
INVALID does not call real free
```

R1 state smoke:

```text
alloc/free local
double-free before reuse
remote free
remote consume
transfer overflow
stale owner generation
thread exit/orphan path
```

R1 source/RSS smoke:

```text
transfer object consumed before source allocation
transfer cap enforced
empty object/span has a scavenge path
peak and final RSS recorded in benchmark output
```

## No-Go

R1 is rejected if it requires:

```text
owned invalid -> MISS
unbounded transfer storage
OS route query on hot path
header direct read before route validation
source refill before transfer check
double-free becoming reusable
hot-path policy counters
```

