# HZ11 Thread Cache Fast Path L0

Status: design scoped, implementation not started.

## Purpose

Build the smallest HZ11 prototype that can test whether a Hakozuna
speed-first line can reach tcmalloc-class local throughput.

The box should measure front-end cache potential only. It must not pull in
HZ8/HZ10 owner-return, reclaim, or diagnostic machinery.

## Shape

```text
malloc:
  class = h11_size_class(size)
  cache = tls_cache[class]
  if cache.count != 0:
    return cache.items[--cache.count]
  return h11_refill_slow(class)

free:
  if token exact hit:
    class = token.class_id
  else:
    class = h11_route_fallback(ptr)
  push ptr into current thread cache[class]
  if overflow:
    h11_flush_slow(class)
```

## Components

### Size Class Table

```text
class_map:
  uint8 table indexed by 16B quanta
  target coverage 16B..64KiB first
  large sizes go to direct/large fallback later
```

The table should be generated or hand-written once. Do not put the HZ10 fine
class branch ladder on the HZ11 hot path unless a measured box proves it wins.

### Thread Cache

```c
typedef struct H11ClassCache {
    void **items;
    unsigned count;
    unsigned capacity;
    unsigned max_capacity;
} H11ClassCache;

typedef struct H11ThreadCache {
    H11ClassCache class_cache[H11_CLASS_COUNT];
    size_t cached_bytes;
    size_t max_cached_bytes;
} H11ThreadCache;
```

Initial implementation may use fixed arrays to avoid allocator recursion.
Dynamic growth is a later box.

### Pointer Token Table

The token table is a speed cache, not the authority.

```text
direct-mapped TLS table:
  key: exact pointer
  value: class_id

hit:
  exact pointer match
  route skipped

miss/collision:
  route fallback
```

Token collision must never silently classify a different pointer. Exact pointer
comparison is mandatory.

### Route Fallback

Fallback route exists for:

```text
token miss
token collision
unknown pointer
realloc/calloc/usable-size support
checked-mode validation
large/direct allocations
```

Fallback route is not the local fast path. If the first prototype routes most
local frees, the token design failed.

## Explicit Non-Goals

```text
no transfer cache yet
no per-CPU/rseq cache yet
no HZ8 slot_state authority
no HZ10 owner-drain remote stack
no per-op generation validation
no full double-free detection in speed mode
no low-RSS claim
```

## Counters

Counters are opt-in and must not be always-on hot-path work in release builds.

```text
thread_cache_malloc_hit
thread_cache_malloc_miss
thread_cache_free_token_hit
thread_cache_free_route_fallback
thread_cache_overflow
thread_cache_flush
token_collision
```

## Bench Ladder

### L0 Body Microbench

```text
fixed16_local0
fixed64_local0
fixed256_local0
fixed4096_local0
medium_fixed64k_local0
```

Record:

```text
ops/s
instructions/op
branches/op
branch misses
L1D misses
post RSS
```

### L1 Public Local Rows

```text
guard_local0
main_local0
medium_local0
random_size_local0
```

### L2 Mixed Rows

Only after transfer cache exists:

```text
small_remote50/90
main_r50/r90
medium_r50/r90
producer_consumer_free
```

## GO / NO-GO

L0 first gate:

```text
fixed64_local0 >= tcmalloc * 0.95
guard_local0  >= tcmalloc * 0.85
main_local0   >= tcmalloc * 0.80
```

NO-GO if:

```text
token hit rate is low on local rows
route fallback remains common on valid local workloads
fixed64_local0 misses the 0.95x gate by a large margin
post RSS exceeds tcmalloc * 1.50 in local rows before transfer cache exists
```

## Risks

```text
R1:
  token cache can hide correctness assumptions. Keep exact pointer compare and
  route fallback mandatory.

R2:
  speed mode can drift into unsafe public claims. Keep checked-mode language
  separate from speed-mode language.

R3:
  per-thread cache may not beat modern tcmalloc per-CPU mode. If local rows are
  close but not ahead, the next real box is per-CPU/rseq, not more HZ10-style
  metadata.

R4:
  RSS can grow quickly without transfer cache caps. Fixed byte caps are part of
  L0, even if release-to-OS is later.
```

## Review Questions

```text
Q1:
  Should the first token table key use raw pointer bits or page/object index?

Q2:
  What is the smallest route fallback sufficient for speed-mode smoke without
  importing HZ10 route semantics?

Q3:
  Should class cache storage be fixed static arrays in L0 or metadata-slab
  allocated from mmap?

Q4:
  What is the first fair tcmalloc comparator: system tcmalloc package or the
  same local source build used in HZ10 matrices?
```
