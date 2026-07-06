# HZ10Sh6PostNoGoAttribution-L0

Status: COMPLETE.

## Goal

Re-profile the clean default `libhz10.so` after these reverted NO-GO boxes:

- `HZ10SizeClassSmallLookup-L0`
- `HZ10OwnerLocalPageIndex-L0`

The purpose was to avoid stacking another speed box on stale attribution.

## Setup

```text
binary: /mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/sh6bench
args:   100000 1 1000 4
build:  make preload
host:   Linux x86_64 perf
```

Artifacts in this directory:

- `perf_stat_hz10.csv`
- `perf_stat_tcmalloc.csv`
- `perf_stat_mimalloc.csv`
- `perf_report_flat.txt`
- `perf_annotate_hz10_malloc.txt`
- `perf_annotate_hz10_free.txt`
- `objdump_malloc_free.txt`

The raw `perf_hz10.data` is intentionally not part of the committed record.

## Stat Snapshot

```text
allocator  cycles        instructions   IPC   cache-misses
hz10       12.917B       26.910B        2.08  78.2M
tcmalloc    9.270B       18.337B        1.98  133.7M
mimalloc    8.820B       14.067B        1.59  88.5M
```

Ratios:

```text
hz10 / tcmalloc: cycles 1.39x, instructions 1.47x
hz10 / mimalloc: cycles 1.46x, instructions 1.91x
```

The IPC remains high and branch misses are low. The remaining gap is still
mostly instruction volume, not a visible memory-stall or branch-mispredict
problem.

## Flat Profile

Top `libhz10.so` symbols:

```text
30.77% hz10_free
21.56% hz10_malloc
 9.67% free shim wrapper
 6.80% malloc shim wrapper
 1.50% hz10_page_drain_remote
```

The entry points are still the right area. The tail symbols are too small to
justify another wrapper/linker box first.

## Annotate Reading

`hz10_free` still shows the pagemap route and local push sequence. However,
the direct owner-local page index prototype had a high hit rate and still
regressed macro rows, so free-side owner indexing is closed unless a new shape
removes the added owner-table overhead.

`hz10_malloc` now shows a clean structural block after size-class selection:

```text
class_id -> owner->classes[class_id] address:
  mov class_id
  lea
  lea
  shl
  load state->active
  test state->active
```

This is a better next candidate than another size-class table: sh6bench's
small-size branch path is already predictable, and the table prototype was
measured as NO-GO.

## Decision

Do not reopen:

- stack-protector removal
- route div skip / reciprocal based only on annotate samples
- size-class small lookup table
- owner-local free page index

Recommended next design box:

- `HZ10MallocActivePageVector-L0`: opt-in owner-local hot active-page vector.

The intended L0 is a cache of `state->active`, not a replacement for
`Hz10ClassState`. It should update at the same boundaries that assign
`state->active`, keep the class-state list as authority, and fall back to the
existing page-layer slow path on NULL/exhaustion. The primary risk is owner
footprint and extra update work, so the first artifact must be a separate
build lane with a full macro A/B.
