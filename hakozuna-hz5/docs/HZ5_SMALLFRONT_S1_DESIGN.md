# HZ5 SmallFront-S1 Design

This document defines the next Linux HZ5 allocator target after the full
preload control lane.

## Decision

The full preload lane fixed attribution: paper-main malloc traffic can now be
routed through HZ5 instead of silently passing through libc. It did not solve
performance because ordinary small and mid-sized malloc traffic currently
falls into the no-HZ3 fallback wrapped-mmap path.

The next target is therefore a HZ5-native small object front-end:

```text
HZ5-SmallFront-S1
  malloc/free <= 2048 bytes
  normal malloc alignment
  Linux full-preload first
  fail-closed descriptor ownership
```

This is not a direct hz3 port. It should reuse the design lessons:

```text
from hz3:
  size classes
  TLS fast path
  low branch and metadata cost

from hz4:
  owner-aware remote free
  grouped/batched drain
  page/run-local reuse

from HZ5:
  fail-closed ownership
  descriptor as source of truth
  invalid or double-free does not fall back to libc/HZ3
```

## General Allocator Shape

HZ5 Linux should be split by responsibility:

```text
SmallFront     <= 2KiB or 4KiB ordinary malloc
MidRun         4KiB..64KiB normal aligned/page-ish allocations
Local2P        exact 64K/a8192 Linux appendix/special route
P43/P45        Windows exact/overaligned and control-plane research
LargeFallback  true large or unsupported allocations
```

Local2P remains the exact 64K/a8192 research profile. SmallFront is the start
of the general allocator path.

## S1 Scope

Covered by S1:

```text
malloc(size <= 2048)
free(ptr from SmallFront)
calloc via malloc + memset
realloc via allocate-copy-free
malloc_usable_size from the small-page descriptor
normal malloc alignment, 16 bytes
Linux full preload lane
```

Not covered by S1:

```text
posix_memalign/aligned_alloc with alignment > 16
exact 4K/8K a8192 guard rows
thread-death hardening
RSS trimming
huge page policy
replacing Local2P
Windows P43i/P45 changes
```

## Size Classes

Initial S1 classes:

```text
16, 32, 48, 64, 96, 128, 192, 256,
384, 512, 768, 1024, 1536, 2048
```

The minimal fallback set for early bring-up is:

```text
16, 32, 64, 128, 256, 512, 1024, 2048
```

Use the full table for paper-main work unless the shorter table is needed to
isolate a correctness issue.

## Page Model

S1 uses 4KiB single-class small pages.

The first implementation uses an 8KiB raw mapping:

```text
raw + 0KiB:
  descriptor/control page

raw + 4KiB:
  user slot page
```

Free does not dereference foreign pointer-adjacent memory. It computes the
4KiB page base and looks that page base up in a HZ5 SmallFront page map. The map
is write-locked only on page insertion; ownership checks are lock-free reads.

```c
typedef struct Hz5SmallPage {
    void* page_base;
    uint16_t size_class;
    uint16_t slot_count;
    uint16_t free_count;
    uint16_t owner_id;
    uint32_t generation;
    uint64_t active_bits[4];
    void* local_free_head;
    void* remote_free_head;
    struct Hz5SmallPage* next_partial;
} Hz5SmallPage;
```

The exact layout may change during implementation, but the invariants should
not:

```text
descriptor is outside user object
each page has one size class
object pointers are slot-aligned inside page_base
active bitmap is the double-free guard
freed objects carry the intrusive next pointer
active objects do not have a per-object wrapper header
```

Slot counts for 4KiB pages:

```text
16B   -> 256 slots
32B   -> 128 slots
64B   ->  64 slots
128B  ->  32 slots
256B  ->  16 slots
512B  ->   8 slots
1024B ->   4 slots
2048B ->   2 slots
```

## Ownership Lookup

SmallFront free should not depend on the full-preload pointer table for
HZ5-owned small pointers.

Required ownership path:

```text
free(ptr)
  -> derive or look up page_base
  -> load small-page descriptor
  -> check descriptor kind == SMALL
  -> check ptr is on a slot boundary
  -> check active bit
  -> owner-local free or remote inbox
```

The full-preload pointer table remains a bootstrap compatibility mechanism for
real-libc allocations, not the primary HZ5 ownership mechanism.

Current implementation rule:

```text
SmallFront-owned malloc results are not inserted into the full-preload pointer
table. free, realloc, and malloc_usable_size identify them through
SmallFront descriptor ownership.
```

## Fast Paths

### malloc

```text
malloc(size):
  sc = size_to_class(size)
  if sc <= S1_MAX:
      obj = tls_class[sc].free_head
      if obj:
          pop
          mark active
          return obj

      page = tls_class[sc].partial
      if page has free slots:
          pop slot
          mark active
          return obj

      page = acquire_new_small_page(sc)
      return first slot

  else:
      MidRun / P2 / Local2P / LargeFallback
```

### free

```text
free(ptr):
  page = small_page_lookup(ptr)
  if not small:
      continue to other HZ5 routes or bootstrap-real handling

  if owner == current:
      if active bit is clear:
          fail closed
      clear active bit
      push object to owner-local free list
      return

  remote:
      claim or clear active bit safely
      push object to owner inbox
      return
```

## Remote-Free Policy

S1 should include owner-aware remote-free from the start, but it should stay
simple.

Initial policy:

```text
remote thread:
  validates the small pointer
  detects double-free
  pushes object to owner inbox

owner thread:
  drains inbox on class miss or periodic slow path
  batches objects into local free lists
```

Thread death may be handled later through orphan ownership or global transfer.
For S1, benchmark smoke may assume owner threads remain alive.

## Safety Contract

SmallFront must fail closed for:

```text
foreign pointer
wrong page kind
non-slot pointer
double free
corrupted descriptor
owned-looking invalid pointer
```

SmallFront must not:

```text
send HZ5-owned invalid small pointers to libc
send HZ5-owned invalid small pointers to HZ3 fallback
require per-object wrapper headers
require a global lock on the common hot path
require the preload pointer table for HZ5-owned small free
```

## calloc, realloc, usable Size

S1 compatibility rules:

```text
calloc:
  malloc + memset to zero

realloc:
  allocate-copy-free
  preserve min(old_usable, new_size)

malloc_usable_size:
  return size_class for SmallFront pointers
  use existing Local2P/wrapper logic for other HZ5 routes
  use real malloc_usable_size only for bootstrap-real pointers
```

## Build and Lane Naming

Planned lane name:

```text
hz5-smallfront-s1
```

Planned role:

```text
Linux general allocator candidate
```

It should be separate from:

```text
hz5-preload-full:
  attribution/control adapter

hz5-local2p-*:
  exact 64K/a8192 appendix profiles

hz5-p25:
  exact lowpage64 control
```

## Validation Plan

Bring-up order:

1. Build-disabled skeleton and source-map entry.
2. Size-class mapping tests.
3. Single-thread malloc/free smoke for every S1 class.
4. calloc zero check.
5. realloc grow/shrink smoke.
6. malloc_usable_size smoke.
7. double-free and foreign-pointer fail-closed smoke.
8. remote-free smoke with owner alive.
9. short hakmem paper-main MT with `HZ5_PRELOAD_STATS=1`.
10. hz3/hz4/mimalloc/tcmalloc comparison only after safety and attribution pass.

Required early comparisons:

```text
full preload without SmallFront
hz5-smallfront-s1
hz3
hz4
mimalloc
tcmalloc
system libc
```

## Stop Rules

Immediate no-go:

```text
crash
unexpected malloc_real in benchmark body
track_insert_fail > 0
double-free goes to real libc
foreign pointer corrupts HZ5
remote-free crash
malloc_usable_size breaks app compatibility
```

Performance no-go:

```text
SmallFront < 5x current full-preload wrapped-mmap path
paper-main guard/main/cross128 remain more than 2x slower than hz3/hz4
small malloc/free < 50% of hz4 in the first microbench
```

Complexity no-go:

```text
per-object wrapper header is required
global lock is required on the common fast path
preload pointer table is required for HZ5-owned small free
too many build knobs are needed before the first clean S1 result
```

## Implementation Rule

SmallFront-S1 should make HZ5 preload honest and competitive for ordinary
small malloc traffic. It should not weaken Local2P, P25, or Windows P43i/P45.

## First Implementation Note

The first S1 implementation is active behind:

```text
--linux-smallfront-s1
```

Current short smoke status:

```text
malloc/calloc/realloc/malloc_usable_size:
  pass

remote free:
  pass with owner-page remote stack

HZ5 API double-free:
  second free returns HZ5_FREE_INVALID

preload attribution:
  malloc_real=0
  track_insert_fail=0
```

Short hakmem guard smoke, `threads=2 iters=50000 ws=100`:

```text
r0:
  about 28-30M ops/s

r90:
  about 17M ops/s
```

This is enough to continue development, but not enough to promote SmallFront as
the paper-main HZ5 allocator row.
