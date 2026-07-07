# HZ11StaticConstSizeClassTable-L1

Status: NO-GO.

## Motivation

`HZ11SizeTableStaticInit-L1` proved that the runtime-filled BSS table cannot drop
the lazy-init guard: `ld.so` can call `malloc` before constructors run. A true
compile-time const table in `.rodata` avoids that loader-order problem because
the table is populated as soon as the shared object is mapped.

## Attempt

The mutable table and lazy readiness flag were replaced with a checked-in
4096-entry `const uint8_t hz11_size_table[]`. `hz11_size_class()` then read the
table directly with no `hz11_size_table_ready` check.

## Result

Correctness passed:

```text
all HZ11 smoke lanes green
exhaustive 1..65536 size-class check green
LD_PRELOAD loader-time class-0 corruption avoided for the optimized lanes
```

Performance regressed on the main speed-ceiling lane:

```text
fixed64 tcmalloc        about 186.5M ops/s
fixed64 hz11-token-soa  about 145.6M ops/s  (down from about 157-159M)
```

## Read

The readiness guard was not the hot cost it appeared to be. With the mutable
non-volatile table, LTO already made the steady-state guard cheap enough that
removing it did not help. Moving the table to `.rodata` changed code generation
and address selection enough to hurt throughput.

## Verdict

NO-GO. Revert the const table and keep the prior mutable table with the
load-bearing lazy guard. The useful exhaustive class-map smoke test remains from
`HZ11SizeTableStaticInit-L1`.

## Next

The fixed-local gap is now better treated as structural. Further attempts should
be design boxes such as direct-entry / no-shim entry, per-CPU/rseq front-end, or
another larger front-end shape change rather than another tiny table cleanup.
