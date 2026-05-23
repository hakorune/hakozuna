# HZ5 MidFront-M1 Design

MidFront-M1 is the next Linux full-preload performance front-end after
SmallFront-S1.

## Decision

4096-byte and larger ordinary malloc traffic should not be added to
SmallFront. SmallFront is a sub-page slot allocator. 4096 bytes is already a
page/run-sized object and belongs to MidFront.

The next performance target is:

```text
MidFront-M1:
  Linux full preload
  normal malloc alignment, align <= 16
  size classes: 4K, 8K, 16K, 32K, 64K
  one object per span in M1
  descriptor-owned metadata
  owner-aware remote inbox
  fail-closed state transitions
```

## Implementation Status

Implemented behind:

```text
--linux-midfront-m1
```

Current M1 implementation:

```text
files:
  midfront/hz5_midfront.c
  midfront/hz5_midfront.h

layout:
  descriptor/control prefix outside the user object
  user base starts one 4KiB page after the raw mapping
  one object per span

ownership:
  every user page in the span maps to the MidSpan descriptor
  free requires ptr == span->base
  malloc_usable_size returns class_bytes

remote:
  owner token per span
  owner-slot x class inbox
  sender-side remote batch
  ACTIVE -> REMOTE_PENDING CAS
```

Current smoke:

```text
/bin/true under full preload:
  OK

class usable smoke:
  3000  -> 4096
  4096  -> 4096
  5000  -> 8192
  8192  -> 8192
  16000 -> 16384
  32768 -> 32768
  65000 -> 65536

owner-death smoke:
  worker malloc(8192), worker exits, main free(ptr)
  OK, no crash

short hakmem, threads=2 iters=50000 ws=100 size=16..65536:
  r0 median:  about 36.8M ops/s
  r90 median: about 2.69M ops/s

attribution:
  malloc_hz5=5054
  malloc_real=0
  track_insert_fail=0
```

Interpretation:

```text
M1 is a coverage/control implementation, not the final fast MidFront. It makes
ordinary 4K..64K malloc HZ5-owned and descriptor-safe. The one-object source
path is intentionally simple and should be optimized after correctness and
paper-main coverage are stable.
```

## Allocator Shape

The Linux HZ5 general allocator should be split by responsibility:

```text
SmallFront:
  <= 2048 bytes ordinary malloc
  sub-page slot allocator

MidFront:
  4096..65536 bytes ordinary malloc
  page/run-sized one-object spans in M1

Local2P:
  exact 64K/a8192 appendix/special route

P43/P45:
  Windows exact/overaligned and control-plane research

LargeFallback:
  true large or unsupported allocations
```

There is intentionally no single catch-all HZ5 route yet. Each front-end must
own a narrow, testable domain.

## Scope

Covered by M1:

```text
malloc/free for normal malloc 2049..65536 rounded to MidFront classes
calloc via malloc + memset
realloc via same-class keep or allocate-copy-free
malloc_usable_size returns class bytes
remote free through owner-token inbox
```

Not covered by M1:

```text
posix_memalign/aligned_alloc with alignment > 16
exact 64K/a8192 Local2P replacement
multi-object slab packing inside a larger span
RSS trimming/release policy
perfect owner-death reclamation
Windows P43i/P45 changes
```

## Size Classes

```text
4096
8192
16384
32768
65536
```

Class mapping:

```text
2049..4096   -> 4096
4097..8192   -> 8192
8193..16384  -> 16384
16385..32768 -> 32768
32769..65536 -> 65536
```

## Metadata

M1 uses one descriptor per user object/span:

```c
typedef enum Hz5MidSpanState {
    HZ5_MIDSPAN_INVALID = 0,
    HZ5_MIDSPAN_ACTIVE = 1,
    HZ5_MIDSPAN_LOCAL_FREE = 2,
    HZ5_MIDSPAN_REMOTE_PENDING = 3,
    HZ5_MIDSPAN_ORPHAN = 4,
    HZ5_MIDSPAN_RETIRED = 5,
} Hz5MidSpanState;

typedef struct Hz5MidSpan {
    uint64_t magic;
    void* base;
    uint32_t class_bytes;
    uint16_t page_count;
    uint16_t class_index;
    Hz5OwnerToken owner;
    _Atomic uint8_t state;
    uint16_t generation;
    uint16_t flags;
    struct Hz5MidSpan* next;
} Hz5MidSpan;
```

M1 should keep descriptor metadata outside the user object. It should not add
per-object wrapper headers to MidFront user memory.

## Ownership Lookup

MidFront should have its own descriptor/page map at first, similar to
SmallFront:

```text
ptr -> page base or span base
lookup MidSpan descriptor
check magic/kind
check ptr == desc->base
check state transition
```

Reason:

```text
P2 segment/page metadata is useful as a source/provider, but it should not be
the hot ownership lookup or remote policy body for MidFront-M1.
```

A direct experiment routing ordinary 2049..65536 malloc to
`hz5_p2_alloc_aligned(size, 4096)` was not a clean next step. It could still
fall back to wrapped mmap and caused early benchmark setup allocation failures.

## Hot Path

Allocation:

```text
malloc(size):
  if size <= 2048:
      SmallFront
  else if size <= 65536 and align <= 16:
      MidFront
  else:
      LargeFallback or unsupported path
```

MidFront alloc:

```text
class = round_mid_class(size)
tls = current owner TLS

span = tls.mid[class].head
if span:
    pop
    state LOCAL_FREE -> ACTIVE
    return span->base

drain owner inbox for class
span = tls.mid[class].head
if span:
    pop
    state LOCAL_FREE -> ACTIVE
    return span->base

span = source_alloc_span(class)
init descriptor
state = ACTIVE
return span->base
```

Free:

```text
desc = lookup(ptr)
if not MidFront:
    continue to other HZ5/real paths

if ptr != desc->base:
    fail closed

if owner == current:
    CAS ACTIVE -> LOCAL_FREE
    push to owner-local TLS cache
else:
    CAS ACTIVE -> REMOTE_PENDING
    sender-batch to owner/class inbox
```

## Remote-Free Policy

MidFront should mirror the SmallFront-S1 ownership hardening:

```text
remote free:
  descriptor lookup
  ACTIVE -> REMOTE_PENDING CAS
  sender TLS batch keyed by (owner, mid_class)
  publish list to owner-slot x class inbox

owner alloc miss:
  drain owner inbox for class
  REMOTE_PENDING -> LOCAL_FREE
  push spans into owner-local cache
```

O1 owner lifetime checks should be shared:

```text
if owner alive and generation matches:
    publish to owner inbox
else:
    publish to orphan/global queue
```

## Source Policy

Do not start with slab packing.

M1 source options, in priority order:

```text
1. simple mmap/posix source for one-object spans, for correctness bring-up
2. P2/segment as slow source provider only, after M1 ownership is stable
3. packed multi-object slab or region allocator, after paper-main coverage is measured
```

P2 can provide memory later, but MidFront should own:

```text
descriptor lookup
state transitions
owner local cache
remote inbox
usable size
fail-closed invalid handling
```

## calloc, realloc, usable Size

```text
calloc:
  MidFront alloc + memset

realloc same class:
  keep pointer

realloc class change:
  allocate new
  memcpy min(old_usable, new_size)
  free old

malloc_usable_size:
  return class_bytes
```

Exact requested size is not required in M1. Usable size is class size.

## Validation

Smoke:

```text
/bin/true under full preload
malloc/free each MidFront class
calloc zero check
realloc same class
realloc grow/shrink
malloc_usable_size
foreign pointer
double free
remote free
thread exit smoke after OwnerLifetime-O1
```

Benchmarks:

```text
size microbench:
  4096, 8192, 16384, 32768, 65536

random mix:
  16..2048 SmallFront
  2049..65536 SmallFront+MidFront

paper-main:
  hakmem guard/main/cross128
  remote = 0,50,90
  threads = 1,2,4,8,16
  runs = 10 when stable
```

Comparisons:

```text
hz3
hz4
mimalloc
tcmalloc
glibc/system
HZ5 SmallFront-only
HZ5 SmallFront+MidFront
```

## Stop Rules

Stop and redesign if:

```text
MidFront local 4K/8K is < 50% of hz4/tcmalloc
paper-main remains dominated by wrapped mmap path
remote-heavy r90 regresses materially versus SmallFront-only
malloc_real appears in benchmark body
track_insert_fail > 0
double-free or foreign pointer reaches real libc incorrectly
owner death can crash remote free
```

Keep if:

```text
paper-main guard/main throughput improves by >= 3x over current full preload
malloc_hz5 remains high
malloc_real remains 0 in benchmark body
SmallFront <=2KiB performance remains stable
Local2P exact profile remains unchanged
```
