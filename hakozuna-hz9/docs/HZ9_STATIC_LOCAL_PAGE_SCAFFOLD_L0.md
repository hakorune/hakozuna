# HZ9 Static Local Page Scaffold L0

## Purpose

```text
HZ9StaticLocalPageScaffold-L0

behavior:
  none

goal:
  start the next substrate shape without adding another OwnerPage/SlabPage flag
  prove a static TLS state body can exist without dynamic state ensure
  prove the local free-bit body can use owner-local plain bits in the scaffold
```

This is a source-shape box after the OwnerPage closure. It is not a default
candidate and does not route allocations.

## Why This Shape

Closed evidence:

```text
OwnerPage:
  clean route/lifetime API
  blocked by per-allocation TLS state ensure and local_free_bits RMW/body cost

SlabPage / DirectSlabUse:
  strong remote/profile wins
  local rows still regress

LocalArena:
  local wins exist
  mixed local/remote page mutation collapses remote-heavy rows
```

The next substrate therefore starts from the two narrow requirements that remain
useful:

```text
static TLS state:
  no h8_sys_calloc state ensure on the allocation path
  no H8ThreadCtx / H8OwnerRecord field additions

owner-local plain bits:
  local scaffold uses plain uint64_t bit transitions
  remote authority is not introduced in L0
  no atomic local_free_bits RMW in the local body scaffold
```

## Implemented L0

```text
source:
  src/h8_hz9_static_local_page.c

test:
  tests/h8_hz9_static_local_page_smoke.c

build:
  smoke-hz9staticlocalpage
  h8_bench_release_hz9staticlocalpage_scaffold

flag:
  H9_STATIC_LOCAL_PAGE_SCAFFOLD_L0
```

The L0 state is `_Thread_local` and static:

```text
H9StaticLocalPageState:
  classes[class].local_free_bits
  classes[class].local_alloc_bits
  touched_class_bits
```

No allocator entry point calls it yet. This is intentional: the first gate is
source shape and deterministic scaffold invariants, not performance.

## Smoke Contract

```text
put:
  class/slot must be valid
  duplicate free bit is rejected

take:
  returns the lowest local free bit
  moves it to local_alloc_bits
  empty take is rejected

free_allocated:
  allocated bit returns to local_free_bits
  double free is rejected

invalid:
  invalid class and slot are rejected
```

## Next Step

Only after this scaffold stays clean:

```text
add shadow observation:
  count where a static TLS page would have hit
  count where remote traffic would force fallback

then consider behavior:
  local-only static page for a narrow class/profile
  remote frees still use HZ8 pending/qstate boundary
```

Do not connect this to allocation/free behavior until the shadow read identifies
a class/row where the local body can repay the entry cost.
