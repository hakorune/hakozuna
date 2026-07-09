# HZ11RocksdbReadrandomCrashRootCause-L1

Status: **FIX-GO.** Root cause found and fixed. `hz11_malloc_usable_size` routed arena
pointers to libc, which read HZ11's arena slot data as a libc chunk header and
SEGFAULTED. The arena-aware fix closes the rocksdb correctness gap; rocksdb readrandom
now completes at ~tcmalloc parity. Perf evaluation is the next box.

## Context

`HZ11RealAppEvidenceRerun-L1`: every HZ11 lane SEGFAULTED on rocksdb db_bench
`readrandom` (8 threads); tcmalloc/jemalloc were clean. This box root-causes it.

## Root cause

`hz11_malloc_usable_size` (`src/hz11_public_entry.c:230`) blindly called
`hz11_sys_usable_size(ptr)` (libc `__malloc_usable_size`) on **any** pointer, including
HZ11-arena pointers. For an arena pointer (from HZ11 `malloc`), libc reads a "chunk
header" at the arena address — which is HZ11's slot data, not a libc chunk → it
dereferences garbage → **SIGSEGV**.

```text
rocksdb calls malloc()  -> HZ11 arena pointer
rocksdb calls malloc_usable_size(arena_ptr)  -> hz11_malloc_usable_size
  -> hz11_sys_usable_size (libc __malloc_usable_size)
  -> libc reads chunk header at arena address -> SIGSEGV
```

Refuted alternatives (investigated first):
- NOT allocator-API coverage: HZ11's shim intercepts every API rocksdb uses (malloc,
  free, calloc, realloc, posix_memalign, malloc_usable_size).
- NOT foreign-pointer classify: `hz11_span_classify` bound-checks before dereferencing
  the pagemap; non-arena pointers cleanly route to `hz11_sys_free`.
- NOT realloc: arena-ptr realloc is malloc+memcpy+free (sequential, no in-call race).

## Minimal repro (no rocksdb needed)

```c
void* p = malloc(48); malloc_usable_size(p); free(p);   // under LD_PRELOAD=fine128
```
Pre-fix this SEGFAULTS. gdb backtrace:
```text
SIGSEGV in __malloc_usable_size (m=0x7ffef7000000) at ./malloc/malloc.c:5143
#0 __malloc_usable_size  (libc, faulting on the arena pointer)
#1 main
```
glibc reference (no preload): `malloc_usable_size` returns sane sizes (malloc(16)->24,
malloc(32)->40, ...). The crash is HZ11-only, on arena pointers.

## The fix (small correctness fix)

`src/hz11_public_entry.c`:
```c
size_t hz11_malloc_usable_size(void* ptr) {
  if (!ptr) {
    return 0u;
  }
#if HZ11_CLASSIFY_SPAN
  uint8_t class_id;
  if (hz11_span_classify(ptr, &class_id)) {
    return hz11_class_slot_size(class_id);   /* arena ptr: real slot size */
  }
#endif
  return hz11_sys_usable_size(ptr);          /* non-arena (large/calloc/posix_memalign): libc */
}
```
NULL->0 (glibc-compatible); arena pointer -> slot size; non-arena -> libc. Behavior-
neutral for non-arena pointers and for workloads that never call malloc_usable_size on
arena pointers (the synthetic benches). Not a rocksdb-specific workaround.

## Verification (correctness only; perf is next box)

```text
malloc_usable_size smoke (post-fix):  malloc(16)->16, malloc(32)->32, ... SANE, rc=0
  (pre-fix: SIGSEGV).
rocksdb fillrandom,readrandom (8 threads, num=200000) under fine128, POST-FIX:
  rc=0 (no segfault).
  fillrandom : 9.926 micros/op, 1.986s, 89.1 MB/s   (tcmalloc: 9.753 / 1.952s)
  readrandom : 5.388 micros/op, 1.111s, 159.2 MB/s, 199957/200000 found
                                              (tcmalloc: 5.381 / 1.118s)
  -> ~tcmalloc PARITY (correctness restored; perf is a coincidence here, not claimed).
espresso + sqlite3 under fine128 (post-fix): rc=0 (no regression).
make hz11-standalone-check: ok.
```

## Decision

```text
FIX-GO. The rocksdb readrandom correctness gap is CLOSED.
- The crash root cause is identified (malloc_usable_size on arena pointers) and fixed.
- Minimal repro recorded (malloc + malloc_usable_size).
- rocksdb readrandom now completes (rc=0) at ~tcmalloc parity.
- No regression on espresso/sqlite3; standalone-check ok.
Perf evaluation (does cap1024 help real multi-thread now that it doesn't crash?) is the
NEXT box -- not claimed here.
```

## Claim boundary (updated)

```text
Allowed:
  - the rocksdb readrandom segfault is fixed; HZ11 (fine128) completes a multi-threaded
    real DB at ~tcmalloc wall on this one config (correctness restored).

Not allowed:
  - HZ11 generally beats tcmalloc, or rocksdb/real-app perf claims (perf is next box;
    one config, one machine).
  - the fix is a rocksdb-specific workaround (it fixes malloc_usable_size for ALL arena
    pointers).
```

## Files

```text
src/hz11_public_entry.c: arena-aware hz11_malloc_usable_size (the one fix; all lanes).
Rebuilt: span-transfer, fine128, cap768-bytes, cap1024-bytes.
```
