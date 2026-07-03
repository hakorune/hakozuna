# HZ10 Current Task

HZ10 is a standalone next-substrate research line. Keep this file short.

## Active Direction

```text
status:
  Box 1, Box 2, and Box 3 implemented and verified, uncommitted
  Box 4 (HZ10BoundedPagePool-L0) is the active next box

design:
  thread-local intrusive freelist pages
  O(1) pagemap route
  remote stack + owner drain
  bounded RSS page pool

box 1 done (HZ10PageMapRoute-L0):
  2-level radix pagemap: root 2048 entries always resident (16KiB), leaf
  2^20 entries lazily mmap'd per touched range (~24MiB, kernel-demand-paged)
  smoke green: exact/interior/misaligned/generation/miss + tail-slack bonus
  standalone check green (hz10-standalone-check)
  route-cost bench: pagemap beats an in-process hash-probe baseline by
  ~20-23% across PAGES=1024..7500, once bench is built with -flto
  earlier measurement without -flto showed pagemap ~3% SLOWER -- root
  cause was a cross-TU inlining asymmetry (hash baseline is a static
  function in the same TU as the hot loop, hz10_pagemap_route() is a real
  call across TUs without LTO), not a flaw in the radix design; fixed via
  Makefile BENCH_CFLAGS += -flto
  files: src/hz10_pagemap.{h,c}, src/hz10_platform.{h,c} (renamed copy of
  HZ9's platform wrappers, no HZ8/HZ9 entanglement), tests/hz10_pagemap_route_smoke.c,
  bench/hz10_pagemap_route_bench.c

box 2 done (HZ10ThreadLocalFreelistPage-L0):
  one-class intrusive local freelist page (Layer 0); alloc()/free() are
  `static inline` in the header (not .c functions) so any caller gets them
  inlined without relying on a build flag -- same reason mimalloc/tcmalloc
  ship their hot path header-only; create()/destroy() are regular
  functions and are the only calls into Box 1's hz10_pagemap_register/
  release, exactly once each, never per-op
  smoke green: exhaustion, LIFO reuse, non-LIFO/shuffled reuse (no
  duplicates, no drops), pagemap integration at create/destroy boundary
  standalone check green
  bench: hz10_freelist beats plain libc malloc/free on the identical
  fixed-size alloc/free/touch loop by ~5x (lifo) and ~2.6x (batch/
  shuffled) -- expected, since this is a single-class/single-page
  mechanism with none of glibc's size-class/tcache overhead yet; not a
  same-run HZ8/HZ9 comparison (no public malloc() entry point exists yet,
  that is Box 4's job)
  known accepted gap: same-thread double-free is not rejected at this
  layer, by design (see tests/README.md) -- that is the route boundary's
  job (Layer 1), not Layer 0's trusted fast path
  files: src/hz10_freelist_page.{h,c}, tests/hz10_freelist_page_smoke.c,
  bench/hz10_freelist_page_bench.c

box 3 done (HZ10RemoteStackDrain-L0):
  separate module (src/hz10_remote_stack.{h,c}) operating on Box 2's
  Hz10FreelistPage; deliberately one-directional dependency (remote_stack
  depends on freelist_page's struct, freelist_page depends on nothing from
  remote_stack) -- destroy() does NOT auto-drain, the caller must drain
  before destroy if the page was ever exposed to remote_free, see
  tests/README.md
  remote_free_head is a Treiber stack (single CAS push); a per-slot
  pending-bit array (atomic fetch_or claim / fetch_and clear) rejects a
  duplicate remote free of the same slot before it drains, so a slot is
  never merged into local_free_head twice
  classification pipeline (tail-slack/misaligned/interior/generation) is
  the same as Box 1's, scoped to a page the caller already resolved
  smoke green: basic push/drain, duplicate rejection, stale-generation/
  invalid-pointer rejection with counters, and an 8-thread concurrent
  stress case (disjoint slots per thread) proving no lost pushes under
  real CAS contention -- not just single-threaded API shape
  standalone check green; ASan/UBSan and TSan both clean (TSan needed
  ASLR off in this environment -- `setarch $(uname -m) -R`, an env
  quirk, not a code issue)
  bench: genuinely multi-threaded (first box that is). 4 producer threads
  remote-freeing concurrently costs ~25x a single-threaded local free
  (CAS contention on one shared stack head, as expected -- same
  local-vs-remote asymmetry HZ8/HZ9 always showed); owner_drain itself is
  cheap (single-threaded O(n) walk)
  files: src/hz10_remote_stack.{h,c}, tests/hz10_remote_stack_drain_smoke.c,
  bench/hz10_remote_stack_drain_bench.c

box 4 next (HZ10BoundedPagePool-L0):
  page cache caps, release pressure, local/remote/RSS matrix
  this box is what the first GO / good target gates below are judged on

first GO:
  >=2.0x HZ8 local or 250M+ local0
  remote >=1.2x HZ8
  post RSS <=2x HZ8
```

## Rules

```text
Keep active source files under 800 lines.
Do not copy HZ9 history wholesale.
Do not treat tcmalloc 70% local as the first gate.
Do not weaken fail-closed route without writing the contract first.
```

## Read First

```text
README.md
docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md
```
