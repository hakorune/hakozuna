# HZ10 Current Task

HZ10 is a standalone next-substrate research line. Keep this file short.

## Active Direction

```text
status:
  Box 1-5 implemented and verified, uncommitted
  Box 5 (multi-class public entry) local rows genuinely beat system
  malloc (~1.2-1.9x); remote rows are 15-17x SLOWER, root-caused to
  small-slot_count classes churning fresh pages under remote pressure
  plus this module's own double pagemap registration -- see box 5 notes,
  this is the priority before any HZ8/HZ9 same-run comparison means
  anything

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
  remote-free of a slot already returned to the owner's local freelist is
  rejected at owner-drain time (drain_invalid_count); this preserves the
  planned delayed foreign double-free contract without putting validation on
  the same-thread fast path
  classification pipeline (tail-slack/misaligned/interior/generation) is
  the same as Box 1's, scoped to a page the caller already resolved
  smoke green: basic push/drain, duplicate rejection, stale-generation/
  invalid-pointer rejection with counters, delayed foreign double-free
  rejection at drain, and an 8-thread concurrent stress case (disjoint slots
  per thread) proving no lost pushes under real CAS contention -- not just
  single-threaded API shape
  standalone check green; ASan/UBSan clean. TSan is not concluded in this
  environment: the TSan runtime aborts with "unexpected memory mapping"
  before the smoke body runs, so it is an environment/toolchain blocker, not
  a passing result
  bench: genuinely multi-threaded (first box that is). 4 producer threads
  remote-freeing concurrently costs ~25x a single-threaded local free
  (CAS contention on one shared stack head, as expected -- same
  local-vs-remote asymmetry HZ8/HZ9 always showed)
  measured regression from the delayed-double-free fix: owner_drain's
  duplicate check (hz10_page_local_freelist_contains) walks the current
  local_free_head chain per drained slot, so a full drain is O(merged x
  current_local_len), not O(merged) -- at slot_count=1024 this measured
  217.4M ops/s before the fix, 1.83M ops/s after (~119x). Correctness
  (hard zero gate) kept as-is for now; making this O(1) would need a
  per-slot "currently allocated" bit that Box 2's alloc()/free() hot path
  would have to maintain, which is exactly the one-directional-dependency/
  minimal-hot-path-state line Box 2 has held since Box 3 -- not changed
  without deciding that trade-off first. Revisit before any class's
  slot_count grows enough for this to dominate a remote-heavy row
  files: src/hz10_remote_stack.{h,c}, tests/hz10_remote_stack_drain_smoke.c,
  bench/hz10_remote_stack_drain_bench.c

box 4 done (HZ10BoundedPagePool-L0):
  two modules, deliberately kept independent: src/hz10_page_pool.{h,c} is
  a generic bounded cache of raw quantum blocks (mutex-protected, not
  lock-free -- acquire+release both happen from arbitrary threads here,
  unlike Box 3's push-only remote_free_head, and that is exactly the
  classic Treiber-stack ABA hazard, so a mutex was the deliberate,
  correct choice over cleverness); src/hz10_pooled_page.{h,c} is the only
  module that knows about both the pool and Box 2's freelist page
  Box 2 stayed pool-agnostic: added hz10_freelist_page_create_with_base()
  (accepts an externally-supplied aligned block, or NULL for a fresh
  mmap) and hz10_freelist_page_destroy_reclaim_base() (returns the base
  instead of unmapping it) -- Box 2 still has zero dependency on pooling,
  hz10_pooled_page is what composes them
  a block recycled from the pool is re-registered with Box 1's pagemap,
  bumping generation the same way any re-registration does, so a stale
  pre-recycle generation is correctly rejected -- verified directly
  smoke green: pool cap/reuse counters, pooled_page basic correctness,
  generation-bump-on-reuse, and a deterministic "sustained churn bounds
  the cache" case (more pages created+kept-alive than the cap, then all
  destroyed, forcing real releases for the excess) -- counter-based proof
  of release pressure, since a raw OS RSS sample is too noisy run to run
  standalone check green; ASan/UBSan clean. TSan is not concluded in this
  environment for the same runtime mapping reason noted above
  bench (local/remote/RSS matrix): pooled_local vs. unpooled_local (same
  code path, only the cap differs: CAP vs. 0) shows pooling is ~55-60x
  faster per create/destroy cycle -- avoiding mmap+munmap+pagemap
  re-registration on every cycle is a large, unsurprising win; pooled_remote
  reuses Box 3's producer-thread remote free on a pool-backed page;
  getrusage ru_maxrss sampled before/after, reported honestly (modest
  growth observed, not claimed as zero)
  scope note: single size class only, matching every box's L0 discipline
  so far. Does NOT add multi-class dispatch or a public malloc()/free()
  entry point -- so it does not yet produce a medium_local0/main_local0/
  medium_r50/main_r90 row directly comparable to HZ8/HZ9/tcmalloc/mimalloc.
  That wiring is explicitly future work, not silently claimed here
  files: src/hz10_page_pool.{h,c}, src/hz10_pooled_page.{h,c},
  tests/hz10_bounded_page_pool_smoke.c, bench/hz10_bounded_page_pool_bench.c

box 5 done (multi-class public entry, name TBD):
  src/hz10_size_class.{h,c}: 13 classes, doubling 16B..64KiB, each an
  exact single quantum (slot_size * slot_count == HZ10_PAGE_QUANTUM);
  covers both main_local0 (16-32768) and medium_local0 (4097-65536)
  bench ranges
  src/hz10_public_entry.{h,c}: TLS active-page-per-class array + a
  per-thread token (&TLS-variable address, a standard per-thread-unique
  void*); hz10_malloc dispatches by class, hz10_free routes via Box 1's
  pagemap and its new owner tag (H10RouteResult.owner, additive: Box
  1-4 callers that pass NULL are unaffected) to recover the exact
  Hz10FreelistPage*, then compares owner_thread_token to decide local
  free (Box 2) vs. remote free (Box 3)
  owner tag required a second pagemap registration per fresh page
  (hz10_pooled_page_create only knows Box 2's owner-less
  create_with_base) -- costs once per page, not per op, but see the
  remote-row finding below for where that stops being true
  known, honestly-scoped gaps (see tests/README.md and
  src/hz10_public_entry.h): size > HZ10_PAGE_QUANTUM or size == 0
  rejected (no multi-quantum spanning yet); an exhausted page is
  abandoned rather than tracked for future reuse (still correctly
  freeable via the owner tag, just never handed out again); free()
  cannot carry a generation (real free(void*) has none), so the
  generation-mismatch contract is not reachable through this API by
  design, not a regression from Box 1-4's own directly-tested contract
  smoke green: multi-class basic + LIFO reuse, rejected inputs (oversized/
  zero/NULL/interior/unknown), abandoned-page-still-freeable, and a real
  cross-thread free (alloc on one thread, free on another, recovered by
  the allocating thread's own later traffic) -- standalone check green,
  ASan/UBSan clean (LeakSanitizer flags intentional still-reachable
  abandoned/TLS-resident pages, see tests/README.md; run with
  ASAN_OPTIONS=detect_leaks=0 to isolate real memory-safety checks), TSan
  clean in this environment with ASLR off (same environment caveat as
  Box 3/4)
  bench: the first row shaped like HZ8/HZ9/tcmalloc/mimalloc's
  medium_local0/main_local0/medium_r50/main_r90. Local rows
  (REMOTE_PCT=0) genuinely beat system malloc: ~1.16x at a
  main_local0-style row (16-32768), ~1.9x at a medium_local0-style row
  (4097-65536), THREADS=4 ITERS=50000 -- a real, positive, first
  same-shape signal
  remote rows (REMOTE_PCT=50/90) are 15-17x SLOWER than system malloc.
  Isolated by testing fixed sizes: a 65536-byte class (slot_count=1)
  craters to ~99K ops/s under REMOTE_PCT=90, while a 16-byte class
  (slot_count=4096) under the identical REMOTE_PCT=90 stays at ~7M
  ops/s. Root cause: small-slot_count classes exhaust almost every
  allocation under remote pressure, and each exhaustion a drain can't
  satisfy pays a fresh page acquisition (plus this module's own double
  registration) instead of a cheap freelist pop -- NOT confirmed to be
  the box 3 O(N^2) drain regression specifically (that would show up
  worse for LARGE slot_count, the opposite pattern measured here); both
  costs are real and layered, but this one dominates at current scale
  files: src/hz10_size_class.{h,c}, src/hz10_public_entry.{h,c},
  tests/hz10_public_entry_smoke.c, bench/hz10_public_entry_bench.c

next box not yet named:
  fix the remote-row regression before any HZ8/HZ9 same-run comparison
  is meaningful -- likely needs: (a) per-class free-page tracking so an
  exhausted page's capacity is recovered instead of abandoned, (b) a
  cheaper (or skippable) owner-tag registration path, (c) revisiting the
  box 3 O(N^2) drain cost once (a) changes how often large-slot_count
  classes actually drain
  then: same-run HZ8/HZ9/tcmalloc/mimalloc comparison via HZ10_EXT_ROOT,
  matching Box 1's scripts/run_hz10_pagemap_vs_hz9_same_run.sh pattern

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
