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
  h8_bench_hz9staticlocalpage_shadow
  h8_bench_release_hz9staticlocalpage_shadow

flag:
  H9_STATIC_LOCAL_PAGE_SCAFFOLD_L0
  H9_STATIC_LOCAL_PAGE_SHADOW_L0
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

## Shadow Counters

`H9_STATIC_LOCAL_PAGE_SHADOW_L0` observes the existing medium path without
changing allocation behavior:

```text
alloc_seen:
  medium class allocation reached the shadow hook

alloc_hit_possible:
  a prior same-thread local free in the same class would have supplied a slot

local_free_seen:
  an exact medium local free reached the shadow hook

local_free_store_possible:
  the free could be stored into the static TLS page model

local_free_not_active:
  the free was not on the current active run shape used for the L0 estimate

local_free_store_blocked:
  duplicate/full shadow bit state blocked the store

remote_seen:
  remote free pressure reached the class

remote_after_local:
  remote pressure appeared while the class had local shadow state

remote_class_disable:
  first remote free disables the class in the admission-bound shadow model

alloc_disabled / local_free_disabled:
  work that would be skipped after class-level remote admission closes
```

Class arrays are reported for alloc/hit/free/remote distribution.

## Initial Debug Sanity Read

Command shape:

```bash
hakozuna-hz9/h8_bench_hz9staticlocalpage_shadow \
  --threads 2 --iters 5000 --min-size 4097 --max-size 65536 \
  --remote-pct 0 --interleaved 0

hakozuna-hz9/h8_bench_hz9staticlocalpage_shadow \
  --threads 2 --iters 5000 --min-size 4097 --max-size 65536 \
  --remote-pct 50 --interleaved 1
```

Read:

```text
medium_local0 debug:
  alloc_seen      100000
  hit_possible     99880
  local_free      100000
  store_possible  100000
  remote_seen          0
  hit_ratio        0.999
  store_ratio      1.000

medium_r50 debug:
  alloc_seen      100000
  hit_possible     49743
  local_free       49804
  store_possible   49804
  remote_seen      50196
  remote_after_local 50071
  hit_ratio        0.497
  store_ratio      1.000
```

Interpretation:

```text
local-only reuse potential:
  strong

remote contamination:
  immediate and class-wide in r50

next design implication:
  behavior must either be local/profile-only or add an explicit remote
  admission boundary before default consideration
```

## Remote Admission Shadow

The second shadow model is intentionally coarse:

```text
first remote free in class:
  disable static local page admission for that class

subsequent allocation/free in that class:
  counted as disabled and not stored/taken in the model
```

This does not change behavior. It answers one question before adding a real
mechanism:

```text
Is class-level remote admission too coarse for r50/mixed rows?
```

Expected reads:

```text
local0:
  remote_disable = 0
  alloc_disabled = 0
  free_disabled = 0

r50:
  if alloc_disabled/free_disabled dominate, class-level admission is too coarse
  and any behavior must use a narrower phase/thread/profile boundary
```

Short debug read after adding the admission shadow:

```text
medium_local0:
  remote_disable 0
  alloc_disabled 0
  free_disabled 0
  hit_ratio 0.999

medium_r50:
  remote_disable 120
  alloc_disabled 99720 / 100000
  free_disabled 49662 / 49804
  hit_ratio 0.001
```

Interpretation:

```text
class-level remote admission:
  too coarse for mixed r50

next viable boundary:
  narrower phase/thread/profile admission, not global class disable
```

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

Suggested short read:

```bash
make -C hakozuna-hz9 bench-hz9staticlocalpage-shadow
hakozuna-hz9/h8_bench_hz9staticlocalpage_shadow \
  --threads 2 --iters 30000 \
  --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0

RUNS=1 VARIANTS=baseline,staticlocal_shadow \
  ROWS=medium_local0,main_local0,medium_r50,main_r90 \
  scripts/run_hz9_candidate_gate.sh
```

The first command reads debug counters. The second checks release source-shape
movement; it is not expected to show a behavior win.
