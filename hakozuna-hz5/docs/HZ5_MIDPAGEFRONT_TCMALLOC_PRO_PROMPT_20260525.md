# HZ5 MidPageFront tcmalloc Consultation Prompt

Please review the current HZ5 Linux MidPageFront design and suggest the next
highest-ROI design step to chase tcmalloc on ordinary mid-size malloc traffic.

Repository:

```text
hakorune/hakozuna
branch: codex/hz5-linux-p43-port
commit: 4bd404aad3c33fa47ba0e65f038f4f9be1ea736a
```

Context:

```text
HZ5 Linux general allocator now has:
  SmallFront-S1: <= 2048
  MidPageFront-M2: 2049..32768, 64K class-owned slabs
  MidFront-M1: fallback/control for remaining mid
  LargeFront-L1: larger ordinary malloc
  Local2P: exact 64K/a8192 special profile
```

Current goal:

```text
Catch tcmalloc on ordinary malloc workloads, especially mid_only_r0 and
mid_only_r90 from hakmem bench_random_mixed_mt_remote_malloc.
```

Important source files:

```text
hakozuna-hz5/midpagefront/hz5_midpagefront.c
hakozuna-hz5/midpagefront/hz5_midpagefront.h
hakozuna-hz5/preload/hz5_preload_full.c
linux/build_linux_hz5_standalone.sh
current_task.md
hakozuna-hz5/docs/HZ5_LINUX_STATUS.md
hakozuna-hz5/docs/HZ5_BENCH_RESULTS_INDEX.md
hakozuna-hz5/docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
```

## Current MidPageFront Shape

MidPageFront uses 64KiB slabs and one descriptor per slab.

```text
classes:
  3072, 4096, 8192, 16384, 32768

slab:
  64KiB

ownership:
  slab_base -> region descriptor -> Hz5MidPage
  descriptor has owner token, active_bits, optional remote_bits

free object:
  object memory stores Hz5MidPageNode { next, page }
```

Relevant hot-path functions:

```text
hz5_midpagefront_try_alloc
hz5_midpagefront_alloc_class
hz5_midpagefront_local_pop
hz5_midpagefront_local_push
hz5_midpagefront_slot_index
hz5_midpagefront_mark_active_local
hz5_midpagefront_mark_free_local
hz5_midpagefront_mark_free_remote
hz5_midpagefront_drain_remote_class
```

Current preload malloc dispatch:

```text
SmallFront first
MidPageFront try-alloc if BENCHLAB_HZ5_PRELOAD_MIDPAGE_ALLOC_FIRST
MidFront
LargeFront
generic HZ5 API
```

The try-alloc API was added to avoid duplicate class lookup:

```text
hz5_midpagefront_try_alloc(size, align, &ptr)
  -> UNSUPPORTED / OK / OOM
```

## Current Results

All performance runs below keep `HZ5_PRELOAD_STATS` unset. Attribution smoke is
run separately.

### Baseline focused read

From:

```text
private/raw-results/linux/tcmalloc_target_shadow_r3_20260525_033348
```

```text
main_r0:
  shadow    58.03M
  tcmalloc  53.11M

main_r50:
  shadow    31.85M
  tcmalloc  30.91M

main_r90:
  shadow    22.74M
  tcmalloc  21.64M

mid_only_r0:
  shadow    71.36M
  tcmalloc 139.58M

mid_only_r50:
  shadow    40.56M
  tcmalloc  77.68M

mid_only_r90:
  shadow    41.27M
  tcmalloc  48.23M
```

Interpretation:

```text
main rows can be tcmalloc-class.
mid_only_r0 is the largest remaining gap.
mid_only_r90 is closer but still behind.
```

### Diagnostics already tried

allocfirst:

```text
private/raw-results/linux/midpage_allocfirst_tryalloc_r3_20260525_054204

mid_only_r0:
  shadow     66.04M
  allocfirst 70.63M
  tcmalloc  141.00M

mid_only_r90:
  shadow     33.57M
  allocfirst 37.00M
  tcmalloc   48.94M
```

Decision:

```text
Useful. Duplicate MidPageFront class lookup in preload mattered.
```

slotswitch:

```text
private/raw-results/linux/midpage_slotswitch_r3_20260525_054521

mid_only_r0:
  allocfirst 66.17M
  slotswitch 65.72M
  tcmalloc  141.17M

mid_only_r90:
  allocfirst 40.66M
  slotswitch 37.04M
  tcmalloc   52.19M
```

Decision:

```text
No-go. Replacing variable slot-index division with fixed-class switch/shift did
not improve r0 and regressed r90.
```

hotslot:

```text
private/raw-results/linux/midpage_hotslot_smoke_r3_20260525_052514

mid_only_r0:
  shadow  53.86M
  hotslot 51.28M

mid_only_r90:
  shadow  29.88M
  hotslot 26.94M
```

Decision:

```text
No-go. One-entry TLS hot object cache does not fix the local object topology.
```

activetrust:

```text
private/raw-results/linux/midpage_activetrust_smoke_r3_20260525_052755

mid_only_r0:
  shadow      51.73M
  activetrust 55.62M

mid_only_r90:
  shadow      33.50M
  activetrust 24.44M
```

Decision:

```text
Diagnostic only. Skipping alloc-side remote-bit check helps r0 slightly but
hurts r90.
```

tls/link diagnostics:

```text
private/raw-results/linux/midpage_tls_split_r3_20260525_055155

mid_only_r0:
  allocfirst 69.59M
  tlsie      80.03M
  linkonly   77.63M
  tlslink    78.16M

mid_only_r90:
  allocfirst 26.57M
  tlsie      30.68M
  linkonly   35.73M
  tlslink    20.22M

cross128_r90:
  allocfirst 12.66M
  tlsie      11.83M
  linkonly   13.89M
  tlslink    10.40M
```

Higher-run linkonly verify:

```text
private/raw-results/linux/midpage_linkonly_verify_r7_20260525_055344

main_r90:
  allocfirst 38.71M
  linkonly   29.83M
  tcmalloc   48.97M

mid_only_r90:
  allocfirst 37.52M
  linkonly   33.50M
  tcmalloc   44.69M

cross128_r90:
  allocfirst 10.87M
  linkonly   10.92M
  tcmalloc    7.83M
```

Decision:

```text
TLS/linkage overhead is real, but neither initial-exec TLS nor link flags are a
broad lead yet. initial-exec TLS helps r0, linkonly can help short remote runs,
but r7 does not promote it.
```

Attribution smoke for HZ5 full preload:

```text
malloc_hz5 high
malloc_real=0
track_insert_fail=0
```

## Current Read

What seems sufficiently observed:

```text
1. MidPageFront route attribution is working; libc fallback is not the current
   issue in these rows.
2. Dispatch/class lookup mattered but is not enough.
3. Region lookup cache is not the main gap.
4. Slot-index division is not the main gap.
5. One-entry hot object cache is not enough.
6. Weakening remote shadow / active checks helps local slightly but hurts
   remote stability.
7. TLS/linkage cost is real but not a standalone fix.
```

Remaining main gap:

```text
mid_only_r0:
  HZ5 roughly 70M to 80M depending diagnostic
  tcmalloc roughly 140M

This is still about a 1.8x to 2.0x gap.
```

Suspected current hot-path costs:

```text
alloc:
  size -> class dispatch
  TLS owner/current setup
  local free_head pop
  page pointer from free node
  slot_index validation
  active_bits load/check/store
  remote_bits check when remote-shadow enabled

free local:
  ptr -> page_for_ptr
  region lookup
  slot_index validation
  owner compare
  active_bits load/check/store
  remote_bits check
  write node->page and node->next
  push to TLS free_head

remote:
  remote_bits CAS
  sender-side batch
  owner inbox exchange/drain
```

## Design Constraints

Please do not suggest:

```text
1. Removing fail-closed ownership entirely.
2. Sending owned-looking invalid/double-free pointers to libc fallback.
3. Changing Windows P43i/P45 behavior.
4. Making Local2P exact 64K/a8192 the ordinary malloc path.
5. Benchmark-special SPSC as default allocator behavior.
```

Safety that should remain:

```text
foreign pointer: not HZ5-owned
wrong route: fail closed
double-free-before-reuse: detected if practical
remote-free: no corruption / no unbounded backlog
```

## Questions

1. What is the next best structural change for MidPageFront to close the
   `mid_only_r0` gap to tcmalloc?

2. Should MidPageFront keep 64KiB slabs with `Hz5MidPageNode { next, page }` in
   every free object, or should local free objects become a slimmer tcmalloc-like
   freelist node with page/descriptor found only on free?

3. Is `node->page` worth keeping in the free object for alloc-side speed, or is
   it causing too much payload/cache write overhead on free?

4. Would a per-page local freelist with `current_partial[class]` reduce the need
   for `node->page` on pop while preserving fail-closed free?

5. Should active state be changed from `active_bits` checked on every local
   alloc/free into a cheaper local epoch or per-page free-count model, with
   atomic/bitmap only on remote frees?

6. Is it better to split local-only and remote-safe MidPageFront profiles, or
   should the default profile keep one unified remote-shadow design?

7. Would using class-specific functions for 4096/8192/16384/32768 be better than
   the tested slot-switch approach, or did the no-go result already rule that
   direction out?

8. Is the 64KiB unified slab still right for 3K/4K/8K/16K/32K, or should HZ5
   copy more of tcmalloc/HZ4 by using different slab spans per class?

9. What should be measured next before implementing a bigger rewrite?

10. If you had to choose one experiment, which one should it be?

Please answer with:

```text
Recommended design:
  ...

Why previous diagnostics did/did not rule it out:
  ...

Minimal implementation step:
  ...

Expected performance effect:
  local r0:
  remote r90:
  cross128:

No-go criteria:
  ...
```
