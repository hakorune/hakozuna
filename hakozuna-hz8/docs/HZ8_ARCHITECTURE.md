# HZ8 Architecture

HZ8 is the SpanOwner Fusion design seed.  Its purpose is to keep the local
allocator as close as possible to a span-local fast path while putting safety at
the boundary instead of routing every object through global metadata.

## Architecture Choice

HZ8 uses the `B + D` direction:

```text
Backbone:
  HZ4-style span/page-local metadata and owner-stable remote free.

Local leaf:
  HZ3-style TLS active span, class lookup, bump allocation, and local free-list.

Pressure control:
  HZ5-style page/run policy, but only on refill, collect, empty transition, and
  maintenance slow paths.

Safety:
  HZ6-style MISS / VALID / INVALID contract at allocator boundaries.
```

HZ8 must not build separate local and remote allocators that are selected by a
route layer.  Local allocation and remote free are operations on the same span
metadata.

## High-Level Flow

```text
malloc/free API
  |
  v
Reserved Address Gate
  |-- outside HZ8 arenas --------------------> MISS / platform allocator
  |
  |-- Small Arena
  |     |
  |     +-- Local Leaf Path
  |     +-- Remote Span Inbox
  |     +-- Owner Collect
  |
  |-- Medium Run Arena       (v1)
  |     |
  |     +-- Run-local metadata
  |     +-- Remote Run Inbox
  |
  |-- Direct/Large Arena     (v1)
        |
        +-- Coarse range descriptor
```

Slow paths own segment refill, empty-span return/decommit, remote collection,
owner death/orphan adoption, RSS pressure control, and safety verification.

## Reserved Address Gate

HZ8 reserves a dedicated virtual address range for the small arena.

Initial v0 values:

```text
Small arena reservation:
  64GiB virtual address space

Segment:
  2MiB aligned

Small span:
  64KiB aligned within a segment
```

The first free boundary check is arithmetic:

```c
uintptr_t delta = (uintptr_t)ptr - h8_small_arena_base;

if (delta < h8_small_arena_size) {
    /* HZ8 small pointer candidate. */
} else {
    /* Medium/large directory or MISS. */
}
```

Inside the reserved small arena, an inactive or malformed pointer is
`INVALID`, not `MISS`.  Platform free is allowed only for pointers proven to be
outside all HZ8 arenas and registered HZ8 direct ranges.

## Small Object Identity

Small object identity is:

```text
segment + span + slot index
```

Small object owner is:

```text
span owner
```

Small object descriptor:

```text
none
```

Small object free must not need a global exact route lookup.  It may use only
arena range, segment state, span metadata, class, owner token, slot alignment,
and bitmaps.

## Small Span Metadata

Conceptual v0 metadata:

```c
typedef struct H8SpanMeta {
    uint16_t class_id;
    uint16_t owner_slot;
    uint32_t owner_generation;

    uint16_t slot_count;
    uint16_t bump_index;
    uint32_t local_free_head;
    uint32_t used_count;

    _Atomic(uintptr_t) remote_head;
    _Atomic(uint8_t) qstate; /* IDLE / QUEUED / DRAINING */

    /*
     * live_bitmap is owner-written.
     * remote_pending_bitmap is claimed by foreign producers and cleared by
     * owner collection.
     */
} H8SpanMeta;
```

The free-list is the speed structure.  Bitmaps are the validation and ownership
guard structures.  Same-owner local frees must not pay locked atomic RMW for
normal bitmap updates.

## Local Small Path

Allocation hit:

```text
size -> class lookup
TLS active_span[class]
local free-list pop
return
```

Bump hit:

```text
TLS active_span[class]
bump_index < slot_count
return next slot
```

Miss:

```text
noinline slow path
collect remote work if useful
find partial span or refill segment
```

Same-owner free:

```text
arena range check
segment/span metadata lookup
slot alignment check
owner token check
live bit check
clear live bit
push local free-list
decrement used_count
return
```

Forbidden in the same-owner local path:

```text
global route hash
per-object descriptor
global lock
owner rehome
OS query
diagnostic counter RMW
policy read
```

## Remote Free Plane

Remote free is a message to the span owner.  The owner of the object does not
change.

Foreign free:

```text
ptr -> segment/span/slot
validate slot alignment and span state
read owner token
atomic claim remote-pending bit
store next pointer into freed object payload
push object onto span remote_head
queue span to owner pending-span queue once
return
```

Remote producers do not drain, retire, rehome, unregister routes, mutate source
state, push frontcache, or decrement `used_count`.

The intrusive list uses the freed object as its node, so the per-span remote
queue has no fixed capacity and cannot report transfer-cache-full style
backpressure.

## Owner Collect

Owner collection runs on local slow paths:

```text
local free-list miss
span refill slow path
pending high-water
maintenance epoch
thread exit / owner lifecycle transition
```

The trigger order is part of the design, not an incidental tuning detail:

```text
1. local free-list miss:
   collect the current span/class first, because it can convert a miss into a
   local hit without broad scanning.

2. pending high-water:
   collect bounded batches from the owner pending-span queue before remote
   inventory turns into burst latency.

3. maintenance epoch:
   collect residual queued spans and run pressure control when the owner is
   already on a slow path.
```

Do not use maintenance epoch as the only collection trigger.  It keeps the hot
path clean but can leave remote inventory cold long enough to distort locality
and latency.  Do not collect the whole owner queue on every local miss either;
that moves remote pressure cost into unrelated local allocations.

Collection:

```text
head = atomic_exchange(span->remote_head, NULL)
for each object in head:
    verify live bit
    clear remote-pending bit
    clear live bit
    push local free-list
    decrement used_count
return qstate to IDLE, then recheck for missed remote work
```

Remote pending objects keep `used_count` nonzero until the owner collects them.
This prevents empty-span retirement while remote frees are still uncollected.

The pending-span notification state is:

```text
IDLE:
  no owner pending queue entry is active

QUEUED:
  span is on an owner pending queue

DRAINING:
  owner has popped the span and is collecting remote_head
```

Publishers transition only `IDLE -> QUEUED`.  Collectors transition
`QUEUED -> DRAINING -> IDLE`.  If a publisher adds work while the span is
`DRAINING`, the collector's finish recheck is responsible for requeueing.

## Medium and Large Direction

v0 does not implement medium or large retained allocation.  Requests above 4KiB
fall back explicitly to the platform allocator.

v1 adds:

```text
MediumRun:
  >4KiB..128KiB
  one run / one coarse registration
  run owner
  run class
  slot bitmap
  local free-list
  remote run inbox

DirectLarge:
  >128KiB
  direct OS allocation
  coarse range descriptor
```

HZ8 must not register one route entry per 4KiB page for medium/large runs.
Coarse run or region registration is the v1 boundary.

## Profiles

HZ8 starts with one balanced default.  v0 does not expose a public `H8Policy`
struct or profile selector.  Keep the first implementation on fixed internal
constants so benchmark pressure cannot turn into a profile family before the
layout proves itself.

v1 may introduce an immutable slow-path policy shape after the default passes
the v0 gates:

```c
typedef struct H8Policy {
    uint16_t remote_collect_budget;
    uint16_t remote_collect_high_water;
    uint16_t empty_span_cap;
    uint16_t decommit_delay_epochs;
    uint16_t segment_release_threshold;
} H8Policy;
```

Profiles must not change layout, queue type, route contract, owner model, or
size classes.  Hot paths must not read policy.
