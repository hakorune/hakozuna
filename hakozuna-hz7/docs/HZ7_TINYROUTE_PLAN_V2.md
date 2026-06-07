# HZ7 TinyRoute Plan V2

This is the implementation-ready TinyRoute plan. It keeps the original HZ7
goal intact:

```text
HZ7 is HZ6-minimal.
TinyRoute-0 proves smallness.
TinyRoute-1 reintroduces route safety without becoming HZ6-lite.
```

## TinyRoute-0 Scope

```text
Files:
  hz7.h
  hz7.c

API:
  h7_malloc(size)
  h7_calloc(count, size)
  h7_free(ptr)
  h7_stats()

Threading:
  single-threaded only
  callers may externally synchronize

Non-goals:
  h7_realloc
  h7_aligned_alloc
  libc interpose
  foreign pointer fallback
  route table
  descriptor table
  medium retained pool
```

## Benchmark Integration

TinyRoute-0 is intentionally integrated only where its direct API is honest.

```text
Windows random_mixed:
  row name: hz7-tinyroute
  adapter: h7_malloc / h7_free
  scope: direct API, single-process, benchmark-owned pointers

Not yet:
  libc interpose
  cross-thread ownership benchmark
  Redis/memcached real app rows
  mt-remote rows
```

The integration point is the benchmark adapter, not an HZ6 profile lane. HZ7
should stay a single-shape allocator until TinyRoute-1 route safety is added.

## Two Layers

TinyRoute-0 has only two allocation layers.

```text
small:
  16B, 32B, 64B, 128B, 256B, 512B, 1KiB, 2KiB, 4KiB
  64KiB aligned region
  bitmap + free list

big:
  >4KiB
  direct OS allocation
  64KiB aligned region
  freed immediately
```

Use named boundaries, but do not implement a medium layer yet.

```c
#define H7_SMALL_MAX 4096u
#define H7_DIRECT_MIN (H7_SMALL_MAX + 1u)
```

## Region Header Invariant

TinyRoute-0 uses a common 64KiB-aligned region header for both small spans and
big direct allocations.

```text
Every HZ7 allocation belongs to a 64KiB-aligned region.
The first bytes of that region are H7RegionHeader.
h7_free(ptr) classifies small versus big by masking ptr to the region base.
```

```c
typedef enum H7RegionKind {
  H7_REGION_SMALL_SPAN = 1,
  H7_REGION_DIRECT = 2
} H7RegionKind;

typedef struct H7RegionHeader {
  uint32_t magic;
  uint32_t cookie;
  uint16_t kind;
  uint16_t flags;
  uint32_t reserved;
  size_t region_size;
} H7RegionHeader;
```

Small spans and direct regions both begin with `H7RegionHeader`.

```text
small:
  H7RegionHeader + H7Span metadata + bitmap + slots

big:
  H7RegionHeader + H7Direct metadata + user area
```

TinyRoute-0 still does not accept foreign pointers. TinyRoute-1 adds a segment
table so pointer-shaped input can be classified as `MISS`, `VALID`, or
`INVALID` before trusting the region header.

## Span Mask Invariant

```text
Every small span is H7_SPAN_BYTES aligned.
The span header lives at the aligned base.
ptr -> span is one mask operation.
```

```c
#define H7_SPAN_BYTES (64u * 1024u)

_Static_assert((H7_SPAN_BYTES & (H7_SPAN_BYTES - 1u)) == 0,
               "H7_SPAN_BYTES must be a power of two");

static inline H7Span* h7_span_from_ptr(const void* ptr) {
  return (H7Span*)((uintptr_t)ptr & ~((uintptr_t)H7_SPAN_BYTES - 1u));
}
```

This invariant is the reason TinyRoute-0 can avoid a route table.

## Alignment Contract

TinyRoute-0 does not expose `h7_aligned_alloc()`, but the internal layout must
preserve natural slot alignment.

```text
slot 16B  -> 16B aligned
slot 32B  -> 32B aligned
slot 64B  -> 64B aligned
slot >= N -> N-byte alignment when N is the slot size
```

The first slot area is aligned to the class slot size. Alignment must not
become a later span-layout rewrite.

## Span Metadata

```c
typedef struct H7Span {
  H7RegionHeader region;
  uint16_t class_id;
  uint16_t span_flags;
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t used_count;
  uint32_t free_head; /* slot index, UINT32_MAX means none */
  struct H7Span* next;
  /* bitmap words follow */
  /* slot storage follows */
} H7Span;
```

Free slots store the next free index in the first word of the slot.

```text
free list:
  allocation speed

bitmap:
  active-slot truth
  debug double-free / invalid-slot detection
```

TinyRoute-0 class lists should stay small.

```c
typedef struct H7Class {
  uint32_t slot_size;
  H7Span* partial;
  H7Span* empty;
  uint32_t empty_count;
} H7Class;
```

There is no `full` list in TinyRoute-0. Full spans are not on an allocation
list. Freeing from a full span returns it to the partial list.

## Empty Span Policy

TinyRoute-0 must not retain empty spans without a cap.

```text
per-class empty span cap:
  1 by default
  2 is an optional experiment

when used_count becomes 0:
  if empty_count < H7_EMPTY_SPAN_CAP:
    keep span on class empty list
  else:
    return span to the OS

allocation order:
  partial span
  empty span
  OS allocation
```

## Big Direct Region

```c
typedef struct H7Direct {
  H7RegionHeader region;
  size_t requested_size;
} H7Direct;
```

Big allocations also use 64KiB-aligned regions. This keeps `h7_free(ptr)`
classification trivial and route-mode-ready. TinyRoute-2 may add a
page-granular medium path if medium RSS matters.

## Stats Contract

Always-on stats describe current state. They are not per-operation diagnostic
counters.

```c
typedef struct H7Stats {
  size_t active_bytes;
  size_t reserved_bytes;
  size_t span_count;
  size_t direct_count;
} H7Stats;
```

Optional debug stats may include:

```text
alloc_count
free_count
os_alloc_bytes
os_free_bytes
double_free_detected
invalid_free_detected
span_reuse_count
bitmap_check_count
```

No-go: if always-on stats exceed 20 fields, stop and redesign.

## OS Abstraction

Keep OS calls inside `hz7.c` behind three helpers.

```text
h7_os_alloc(size)
h7_os_free(ptr, size)
h7_os_page_size()
```

If a fourth OS helper becomes necessary, that is the trigger to split
`hz7_os.h` / `hz7_os.c`.

## TinyRoute-1 Route Mode

TinyRoute-1 adds a fixed segment table.

```c
typedef enum H7RouteKind {
  H7_ROUTE_MISS = 0,
  H7_ROUTE_VALID = 1,
  H7_ROUTE_INVALID = 2
} H7RouteKind;
```

```text
MISS:
  HZ7 does not know this pointer

VALID:
  HZ7 owns this active object

INVALID:
  HZ7-owned-looking pointer, stale pointer, interior pointer, inactive slot,
  or double free
```

Segment table policy:

```text
fixed capacity
64KiB region-base hash first
append insert
swap-remove active entries
no heap allocation
no arbitrary pointer dereference before segment lookup
```

TinyRoute-1 may fall back to a bounded range scan only when the base-hash
lookup misses. This preserves `INVALID` semantics for interior pointers inside
large direct regions without making the common small-span path a full table
scan.

TinyRoute-1 may keep a bounded released-segment quarantine.

```text
active or retained segment + valid object:
  VALID

active, retained, or quarantined segment + stale/interior/double-free pointer:
  INVALID

released segment after quarantine eviction:
  MISS
```

Do not claim permanent stale-pointer detection.

## After Route Safety

Once TinyRoute-1 smoke and random_mixed direct-API rows are stable, the next
steps are intentionally narrow.

```text
TinyRoute-2:
  add a coarse global lock build/smoke
  prove multithread safety before proving multithread speed
  keep direct API only

TinyRoute-3:
  replace the coarse lock bottleneck with thread-local small spans/front cache
  add owner/remote handoff only after same-thread TLS is clean

TinyRoute-4:
  add a retained medium layer for >4KiB workloads
  do not add it before route and multithread safety are stable
```

Acceptance for TinyRoute-2 is safety, not speed. If global-lock multithread
smoke is clean, HZ7 can be included in multithread benchmark harnesses as a
clearly labeled safety baseline while TinyRoute-3 works on throughput.

## Invalid Free Action

```text
TinyRoute-0:
  foreign or invalid pointer is an API contract violation
  debug builds may assert or abort
  release builds do not promise safe foreign-pointer handling

TinyRoute-1:
  MISS:
    no fallback in the direct API prototype

  INVALID:
    never fallback
    debug may assert or abort
    release should fail closed
```

## Acceptance

TinyRoute-0 hard acceptance:

```text
source:
  allocator core is hz7.h / hz7.c

binary:
  at least 10x smaller than HZ6 in the same toolchain
  no profile matrix

small:
  16B..4KiB smoke passes
  reuse works
  debug double-free detection works

big:
  >4KiB direct allocation works
  free releases payload

calloc:
  count * size overflow returns NULL
  zero-size behavior is defined
  returned memory is zero-filled

RSS:
  1M active 64B objects stays under 80 MiB peak RSS target

speed:
  single-thread 64B alloc/free cycle > 50M ops/s on the dev machine
```

Stretch target:

```text
core no-debug .text + .rdata < 8 KiB
```

TinyRoute-1 acceptance:

```text
foreign pointer -> MISS
valid owned pointer -> VALID
interior owned pointer while segment metadata is active/quarantined -> INVALID
double free while segment metadata is active/quarantined -> INVALID
stale inactive slot in retained span -> INVALID
released segment within bounded quarantine -> INVALID
INVALID never falls back
segment metadata stays fixed-capacity
```

## No-Go

```text
HZ7 needs many compile flags to explain itself
span metadata becomes a descriptor table
route-safe mode becomes the HZ6 route layer
foreign pointer check dereferences arbitrary memory
invalid owned pointer becomes MISS
medium retained cache appears before TinyRoute-0 is proven
production hot-path diagnostics appear
OS query is used on every free
binary size approaches HZ6
source grows beyond readable single-file too early
```
