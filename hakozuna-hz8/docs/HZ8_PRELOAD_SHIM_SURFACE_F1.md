# HZ8PreloadShimSurface-F1

Status: implemented + measured; GO for the Redis allocator-split crash.

## Purpose

`HZ8MacroFailureAttribution-L0` classified the Redis macro-lane SIGSEGV as an
LD_PRELOAD interposition surface mismatch:

```text
malloc/free/calloc/realloc -> HZ8
malloc_usable_size         -> Redis-linked jemalloc
```

Redis calls `malloc_usable_size()` on a pointer returned by the active malloc.
When HZ8 owns the allocation but jemalloc answers the usable-size query,
jemalloc reads foreign metadata and crashes.

This box fixes that narrow mismatch. It does not attempt to solve HZ8's
python retention, larson commit failure, or xmalloc livelock findings.

## Scope

Add the preload symbols needed to avoid allocator splitting in common
LD_PRELOAD programs:

```text
malloc_usable_size
posix_memalign
aligned_alloc
memalign
```

`malloc_usable_size()` is the main Redis fix. The alignment symbols are added
as surface-completeness wrappers, but they deliberately allocate through the
next allocator rather than manufacturing an adjusted HZ8 pointer. HZ8's free
path requires the exact base pointer for HZ8-owned objects, so overallocating
and returning an interior aligned pointer would violate the fail-closed
boundary.

## Contract

For `malloc_usable_size(ptr)`:

```text
if ptr is NULL:
  return 0
if ptr is a valid HZ8 small/medium/direct-large allocation:
  return the HZ8 slot/request usable size
if ptr is HZ8-owned but invalid/interior/freed:
  return 0
otherwise:
  delegate to RTLD_NEXT malloc_usable_size if available, else return 0
```

The HZ8-owned invalid case must not delegate. Delegating an HZ8 interior or
freed pointer to jemalloc/glibc would recreate the allocator-split crash in a
different shape.

For `posix_memalign`, `aligned_alloc`, and `memalign`:

```text
delegate to RTLD_NEXT when available
fallback to a conservative local implementation only if there is no next symbol
```

The fallback may use HZ8 `malloc()` only when the requested alignment is no
stricter than the platform's default pointer alignment. Stricter alignment
without a real aligned allocator returns `ENOMEM`/`NULL` rather than creating
an interior pointer that HZ8 cannot later free.

## Implementation Shape

Add one internal query helper:

```c
bool h8_usable_size_inner(void* ptr, size_t* usable_out, bool* owned_out);
```

It mirrors the existing `realloc` ownership detection:

```text
small arena exact slot -> h8_class_size(span->class_id)
medium exact slot      -> h8_medium_usable_size_inner()
direct-large exact     -> h8_direct_large_usable_size_*()
miss                   -> owned_out=false
invalid HZ8-owned      -> owned_out=true, return false
```

The preload boundary then exports `malloc_usable_size()` and uses the helper to
decide whether to answer locally or delegate.

## Gates

Required:

```text
make -C hakozuna-hz8 smoke preload-smoke
./hakozuna-hz8/h8_smoke
```

New preload-smoke coverage:

```text
malloc_usable_size(HZ8 small ptr) >= requested size
malloc_usable_size(HZ8 medium ptr) >= requested size
malloc_usable_size(interior HZ8 ptr) == 0
posix_memalign returns aligned memory and free() accepts it
aligned_alloc returns aligned memory and free() accepts it
memalign returns aligned memory and free() accepts it
```

Redis gate when available:

```text
LD_PRELOAD=libhakozuna_hz8_preload.so redis-server --daemonize no
```

The minimum acceptable result is that Redis startup no longer crashes in
jemalloc `malloc_usable_size`. If Redis still fails for a later HZ8 limitation,
that is a separate row classification.

## Verdict Rule

GO if the preload smoke passes and the Redis allocator-split crash disappears.

NO-GO if `malloc_usable_size` must delegate HZ8-owned pointers to a foreign
allocator, or if the alignment wrappers require returning non-base HZ8
pointers.

## Result

Implemented:

```text
malloc_usable_size
posix_memalign
aligned_alloc
memalign
```

The preload library now exports the expanded surface:

```text
malloc/calloc/realloc/free
malloc_usable_size
posix_memalign/aligned_alloc/memalign
h8_route
```

Verification:

```text
make -C hakozuna-hz8 smoke preload-smoke
./hakozuna-hz8/h8_smoke
nm -D --defined-only libhakozuna_hz8_preload.so
```

Redis startup gate:

```text
timeout 3s env LD_PRELOAD=libhakozuna_hz8_preload.so \
  redis-server --port 0 --unixsocket "$tmpdir/redis.sock" \
  --save '' --appendonly no --dir "$tmpdir" --daemonize no
```

Result: Redis initialized and reached "ready to accept connections" until the
timeout sent SIGTERM. The previous startup SIGSEGV in jemalloc
`malloc_usable_size()` did not reproduce.

Verdict: GO. This closes the Redis allocator-split crash. Any later Redis row
failure should be classified separately from the missing usable-size symbol.
