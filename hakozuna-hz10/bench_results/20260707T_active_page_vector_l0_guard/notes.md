# HZ10MallocActivePageVector-L0

Status: NO-GO. Prototype reverted.

## Shape Tested

The prototype added an opt-in `HZ10_ENABLE_MALLOC_ACTIVE_PAGE_VECTOR` lane and
a sibling preload artifact:

```text
libhz10_active_vector.so
```

It extended `Hz10ThreadOwner` with:

```text
active_pages[HZ10_CLASS_COUNT]
```

Every public-entry write to `state->active` also updated the vector. The
malloc fast path loaded `owner->active_pages[class_id]` directly and fell back
to the existing page-layer slow path on NULL or exhaustion. `Hz10ClassState`
and the page lists remained authoritative.

## Correctness

Passed:

```text
make preload preload-active-vector smoke-public-entry smoke-shim-api smoke-shim-foreign
make smoke-public-entry-active-vector
```

The active-vector smoke used the same compile-time shape as the sibling preload:

```text
HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1
HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1
HZ10_ENABLE_FINE_SIZE_CLASSES=1
HZ10_ENABLE_MALLOC_ACTIVE_PAGE_VECTOR=1
```

## Codegen

The codegen target was achieved.

Default malloc active-page load:

```text
lea (%rax,%rax,2),%rcx
lea (%rax,%rcx,4),%rax
shl $0x5,%rax
mov 0x8(%rdx,%rax,1),%rax
```

Active-vector malloc active-page load:

```text
mov 0x8(%rax,%rdx,8),%rax
```

So this was a real test of the intended instruction-count reduction, not a
failed implementation.

## A/B Results

RUNS=5, `ALLOCATORS_CSV=hz10,hz10-active-vector`.

Focused sh6 pass:

```text
workload      hz10              hz10-active-vector
python_alloc  0.840s / 106668  0.840s / 106760
redis_setget  0.540s / 8064    0.540s / 8064
sh6bench      0.420s / 320000  0.410s / 320640
```

Guard pass with larson and mstress:

```text
workload      hz10              hz10-active-vector
larson        4.175s / 283520  4.178s / 283260
mstress       0.220s / 206776  0.210s / 204844
python_alloc  0.850s / 106760  0.840s / 106704
redis_setget  0.540s / 8064    0.540s / 8192
sh6bench      0.420s / 316672  0.420s / 319872
```

## Decision

NO-GO. The codegen win is visible, but the primary sh6bench gate does not hold
in the guard pass. Secondary row movement is small and partly rounded by the
macro harness. This is not enough to justify a permanent owner footprint
increase or another opt-in product lane.

The prototype was reverted. Reopen only with a same-build/same-layout variant
that avoids growing `Hz10ThreadOwner`, or with a lower-noise macro harness that
shows a durable >3% sh6bench win.
