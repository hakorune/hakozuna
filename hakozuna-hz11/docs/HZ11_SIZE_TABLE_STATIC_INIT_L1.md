# HZ11SizeTableStaticInit-L1

Status: NO-GO for removing the hot-path lazy-init guard.

## Motivation

After `HZ11CacheLayout-L1`, one remaining attributed cost was the
`hz11_size_class()` runtime readiness check:

```text
if (!hz11_size_table_ready) hz11_size_class_init();
```

The candidate optimization was to initialize the table before first allocator
use and remove that check from the malloc hot path.

## Attempts

```text
1. Remove the ready check entirely.
2. Add an early priority constructor for the table.
3. Run table init earlier inside the shim constructor.
4. Change the ready flag from volatile to non-volatile.
```

Attempts 1 and 2 corrupted the process heap during preload/perf runs. The
failure mode was glibc allocator assertions such as `sysmalloc` and
`free(): invalid next size`.

## Root Cause

`ld.so` can call `malloc` while loading an `LD_PRELOAD` library, before any
constructor in that library has run. At that point the size-class table is still
BSS-zeroed. If the lazy guard is removed, every cached allocation size is
classified as class 0, so allocations larger than 16 bytes receive undersized
slots and corrupt the heap.

The shim constructor is still useful for normal user-code startup, but it is not
early enough to protect loader-time malloc. Therefore the lazy-init check is a
load-bearing correctness guard.

## Kept Changes

The useful byproducts are kept:

```text
tests/hz11_thread_cache_smoke.c:
  exhaustive 1..65536 size-class table check

src/hz11_size_class.{c,h}:
  comments documenting why the lazy-init guard cannot be removed
  ready flag changed from volatile to non-volatile
```

The non-volatile flag did not move throughput measurably, but the table is
initialized idempotently and normally before user code through the shim
constructor. The lazy path remains for loader-time use.

## Measurement

No stable instruction-count win was observed after restoring the guard.
Throughput stayed in the prior range:

```text
fixed64 tcmalloc        about 187.8M ops/s
fixed64 hz11-token-soa  about 156.4M ops/s
fixed64 hz11-span-soa   about 162.0M ops/s
```

## Verdict

NO-GO. Do not retry removing the readiness check unless the table becomes a
true compile-time `static const` object with no runtime fill requirement.

## Next

The remaining small candidate is `HZ11RefillTailCall-L1`, but fixed-local
tcmalloc parity may require a larger structural change rather than another
single-instruction cleanup.
