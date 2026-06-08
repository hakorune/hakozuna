# HZ7 TinyRoute Plan

HZ7 TinyRoute is a tiny-binary allocator line distilled from HZ6. The point is
not to beat HZ6 by adding another profile family. The point is to make one
small allocator shape that is readable, predictable, and easy to audit.

## Design Thesis

```text
HZ7 is HZ6-minimal.

TinyRoute-0:
  prove smallness

TinyRoute-1:
  reintroduce route safety without becoming HZ6-lite
```

The first implementation must stay a direct API allocator. It does not
interpose `malloc/free`, does not accept foreign-pointer fallback as a feature,
and does not carry HZ6's lane matrix.

## TinyRoute-0 Contract

```text
Files:
  hz7.h
  hz7.c

Threading:
  single-threaded only
  callers may externally synchronize if they need multi-thread use

API:
  void* h7_malloc(size_t size)
  void* h7_calloc(size_t count, size_t size)
  void  h7_free(void* ptr)
  H7Stats h7_stats(void)

Non-goal API:
  h7_realloc
  h7_aligned_alloc
  libc interpose
```

`h7_calloc()` is allowed because it is a small wrapper around `h7_malloc()` with
overflow checking and zero-fill. `h7_realloc()` and `h7_aligned_alloc()` wait
until the base span shape is proven.

## Two Allocation Layers

TinyRoute-0 has only two layers.

```text
small:
  16B, 32B, 64B, 128B, 256B, 512B, 1KiB, 2KiB, 4KiB
  64KiB aligned spans
  bitmap for active-slot validation
  free list for allocation speed

big:
  >4KiB
  direct OS allocation
  H7Direct header
  released immediately on free
```

Use named boundaries even though the implementation is two-layer:

```c
#define H7_SMALL_MAX 4096u
#define H7_DIRECT_MIN (H7_SMALL_MAX + 1u)
```

A future TinyRoute-2 may add `H7_MEDIUM_MAX`, but TinyRoute-0 must not carry a
medium retained pool.

## Span Invariant

TinyRoute-0 depends on one core invariant.

```text
Every small span is H7_SPAN_BYTES aligned.
The span header lives at the aligned base.
ptr -> span is one mask operation.
```

```c
#define H7_SPAN_BYTES (64u * 1024u)

static inline H7Span* h7_span_from_ptr(const void* ptr) {
  return (H7Span*)((uintptr_t)ptr & ~((uintptr_t)H7_SPAN_BYTES - 1u));
}
```

This is why TinyRoute-0 can avoid a route table. Windows `VirtualAlloc`
naturally uses 64KiB allocation granularity. Linux support may use aligned
over-allocation and trimming if `mmap()` does not return a suitably aligned
span.

## Alignment Contract

TinyRoute-0 does not expose `h7_aligned_alloc()`, but the internal span layout
must preserve natural slot alignment.

```text
slot 16B  -> 16B aligned
slot 32B  -> 32B aligned
slot 64B  -> 64B aligned
slot >= N -> N-byte alignment when N is the slot size
```

The first slot area should be aligned to the class slot size. This keeps
alignment from becoming a later layout rewrite.

## Span Metadata

The intended shape is simple.

```c
typedef struct H7Span {
  uint32_t magic;
  uint32_t cookie;
  uint16_t class_id;
  uint16_t flags;
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t used_count;
  uint32_t free_head; /* slot index, UINT32_MAX means none */
  struct H7Span* next;
  /* bitmap words follow */
  /* slot storage follows */
} H7Span;
```

Free slots store the next free index in the first word of the slot. The bitmap
is the source of truth for active versus free; the free list exists for speed.

```text
free list:
  allocation speed

bitmap:
  double-free and invalid-slot detection
```

## Empty Span Policy

TinyRoute-0 must not retain empty spans without a cap.

```text
per-class empty span cap:
  1 by default
  2 is an optional compile-time experiment

when used_count becomes 0:
  if empty_count < H7_EMPTY_SPAN_CAP:
    keep span on the class empty list
  else:
    return span to the OS

allocation order:
  partial span
  empty span
  OS allocation
```

This keeps RSS predictable and prevents the retained-span policy from becoming
an HZ6-style control plane.

## Big Allocation Header

Big allocations use a small direct header.

```c
typedef struct H7Direct {
  uint32_t magic;
  uint32_t cookie;
  size_t requested_size;
  size_t reserved_size;
} H7Direct;
```

TinyRoute-0 can free big allocations by reading this header because the direct
API contract does not promise foreign-pointer safety. TinyRoute-1 should use
the segment table before trusting any pointer-shaped input.

## OS Abstraction

HZ7 stays single-file, but OS calls should be isolated behind three internal
functions.

```text
h7_os_alloc(size)
h7_os_free(ptr, size)
h7_os_page_size()
```

If a fourth OS helper becomes necessary, that is the trigger to consider
splitting `hz7_os.h` / `hz7_os.c`.

## Stats Contract

HZ7 must not repeat HZ6's diagnostic counter growth.

Always-available stats should stay small.

```c
typedef struct H7Stats {
  size_t alloc_count;
  size_t free_count;
  size_t active_bytes;
  size_t span_count;
  size_t direct_count;
  size_t os_alloc_bytes;
  size_t os_free_bytes;
} H7Stats;
```

Debug-only stats may exist behind `H7_DEBUG_STATS`.

```text
double_free_detected
invalid_free_detected
span_reuse_count
bitmap_check_count
route_miss / route_valid / route_invalid in TinyRoute-1
```

No-go: if always-on stats exceed 20 fields, stop and redesign.

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

TinyRoute-1 route table:

```text
fixed capacity
linear scan first
no heap allocation
no arbitrary pointer dereference before segment lookup
```

Do not start with a hashed page map. If linear scan is too slow, that belongs
to TinyRoute-2.

## Acceptance

TinyRoute-0 acceptance:

```text
source:
  hz7.h / hz7.c only for the allocator

binary:
  .text + .rdata < 8 KiB on Windows x64 /O2 target
  at least 10x smaller than HZ6 in the same toolchain

small:
  16B..4KiB smoke passes
  reuse works
  debug double-free detection works

big:
  >4KiB direct allocation works
  free releases payload

RSS:
  1M active 64B objects stays under 80 MiB peak RSS target

speed:
  single-thread 64B alloc/free cycle > 50M ops/s on the dev machine

build:
  cl.exe and gcc/clang clean builds
  warning-clean under strict warnings where practical
```

TinyRoute-1 acceptance:

```text
foreign pointer -> MISS
valid owned pointer -> VALID
interior owned pointer -> INVALID
double free -> INVALID
stale inactive slot -> INVALID
INVALID never falls back
segment metadata stays fixed-capacity
```

## No-Go

```text
no-go:
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

