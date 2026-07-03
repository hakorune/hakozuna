# HZ10 Current Task

HZ10 is a standalone next-substrate research line. Keep this file short.

## Active Direction

```text
status:
  Box 1-6 implemented and verified, uncommitted; class_pages bounded
  scan and drain-empty peek follow-ups done too (see below)
  NEW, IMPORTANT: a "small_remoteNN" row (MIN_SIZE=16 MAX_SIZE=64, never
  tested before) showed hz10 3.2-4.2x SLOWER than system_malloc due to
  Box 3's O(N^2) drain-time duplicate check on large-slot_count classes.
  Fix lane selected: in-slot local-free marker, remote publish pre-reject,
  and O(N) verification only on marker fallback
  Box 6 (src/hz10_class_pages.{h,c}) fixed Box 5's remote-row regression:
  remote rows flipped from 15-17x SLOWER than system malloc to ~1.5-1.7x
  FASTER -- see box 6 notes
  real tcmalloc comparison, now a formal script
  (scripts/run_hz10_public_entry_vs_tcmalloc_same_run.sh, LD_PRELOAD on
  hz10_public_entry_bench's MODE=1 system_malloc path, opt-in/skips
  gracefully if no libtcmalloc found): local rows land at ~60-70% of
  tcmalloc at THREADS=4 ITERS=500K-2M (main_local0 ~62-71%,
  medium_local0 ~47-67% across repeats) -- higher than an earlier,
  smaller-ITERS (50000) ad hoc reading of ~38-50%; the earlier reading
  under-counted steady-state throughput because hz10's one-time setup
  costs are a larger fraction of a shorter timed window. Remote rows
  land lower, ~39-50% (main_r50) and ~41-48% (main_r90) -- still the
  same "good balanced target" band, just not as close to the top as
  local rows. post RSS is consistently lower than tcmalloc's in these
  runs, consistent with the "closer to HZ8 than tcmalloc" RSS goal

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
  rejected before remote publish using the slot-local marker; this avoids
  corrupting the local freelist next pointer with a Treiber-stack push
  classification pipeline (tail-slack/misaligned/interior/generation) is
  the same as Box 1's, scoped to a page the caller already resolved
  smoke green: basic push/drain, duplicate rejection, stale-generation/
  invalid-pointer rejection with counters, foreign double-free rejection
  before push, and an 8-thread concurrent stress case (disjoint slots per
  thread) proving no lost pushes under real CAS contention -- not just
  single-threaded API shape
  standalone check green; ASan/UBSan clean. TSan is not concluded in this
  environment: the TSan runtime aborts with "unexpected memory mapping"
  before the smoke body runs, so it is an environment/toolchain blocker, not
  a passing result
  bench: genuinely multi-threaded (first box that is). 4 producer threads
  remote-freeing concurrently costs ~25x a single-threaded local free
  (CAS contention on one shared stack head, as expected -- same
  local-vs-remote asymmetry HZ8/HZ9 always showed)
  measured regression from the original delayed-double-free fix:
  owner_drain's duplicate check (hz10_page_local_freelist_contains) walked
  the current local_free_head chain per drained slot, so a full drain was
  O(merged x current_local_len), not O(merged). Marker lane replaces that:
  Box 2 writes a second-word local-free marker on same-thread free and
  clears it on alloc; Box 3 rejects marked slots before remote publish and
  only falls back to the O(N) contains walk if a marked slot somehow reaches
  drain. Normal remote drain is therefore O(merged)
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
  remote rows (REMOTE_PCT=50/90) were 15-17x SLOWER than system malloc
  before the Box 6 fix below -- see box 6 notes for the fixed numbers
  files: src/hz10_size_class.{h,c}, src/hz10_public_entry.{h,c},
  tests/hz10_public_entry_smoke.c, bench/hz10_public_entry_bench.c

box 6 done (HZ10PerClassPageList-L0):
  src/hz10_class_pages.{h,c}: per-class linked list of every page a
  thread has ever created (Hz10FreelistPage.next_in_owner_list, an inert
  storage field Box 2 itself never reads/writes -- mirrors the
  owner-tag-field pattern from Box 5); hz10_class_pages_find_with_capacity()
  scans the list, draining each candidate's remote stack before giving
  up on it, so a page that looked exhausted but has since received a
  remote free is recovered instead of abandoned forever. Mirrors the
  shape of HZ8/HZ9's owner-scan-list-then-detached-list cascade
  (h8_medium_alloc.inc's h8_medium_malloc_class_inner), scoped to one
  thread's one-class page set
  also eliminated the double pagemap registration Box 5 paid for the
  owner tag on every fresh page: hz10_freelist_page_create_with_base_and_owner()
  (Box 2, via a shared hz10_freelist_page_create_common() helper -- the
  struct/pending_bits are now allocated before registration so the
  self-referential owner=page tag has a valid pointer) and
  hz10_pooled_page_create_with_owner() (Box 4 glue) register the owner
  tag in the same call a fresh page already needed, additive only (the
  owner-less _with_base/_create paths are unchanged)
  known, accepted L0 gap: the per-class list only grows, never pruned or
  capped -- a thread under sustained per-class churn keeps every page it
  ever made findable (correct) but growing without bound (not yet
  addressed; a future box should cap it and return genuinely-idle pages
  to Box 4's pool)
  smoke green (new case: an exhausted page remote-freed from a foreign
  thread comes back as the *exact same address* on the next malloc for
  that class, not merely a fresh page -- the actual regression test for
  this fix), all prior Box 1-5 smokes still pass unchanged, standalone
  check green, ASan/UBSan clean, TSan clean in this environment (ASLR off)
  bench, THREADS=4 ITERS=500000 MIN_SIZE=16 MAX_SIZE=32768: REMOTE_PCT=50
  went from system-malloc-relative SLOWER to hz10 10.9M ops/s vs
  system_malloc 6.3M (~1.7x FASTER); REMOTE_PCT=90 hz10 7.8M vs 4.8M
  (~1.5x FASTER). Against real tcmalloc (LD_PRELOAD, same technique as
  the local-row comparison): REMOTE_PCT=50 ~43% of tcmalloc, REMOTE_PCT=90
  ~45% -- both now in the same "good balanced target" band as local rows,
  down from 15-17x off. The isolating fixed-size=65536/slot_count=1 case
  (the worst case from root-causing Box 5's regression) improved from
  15-17x slower than system_malloc to ~24% slower (4.6M vs 6.1M ops/s) --
  much closer, not fully closed. post_rss_kb stayed lower than
  tcmalloc's on every remote row measured
  files: src/hz10_class_pages.{h,c}; also touched
  src/hz10_freelist_page.{h,c} (new field + _with_base_and_owner),
  src/hz10_pooled_page.{h,c} (_with_owner), src/hz10_public_entry.{h,c}
  (rewritten to use the per-class list instead of a single active page)

box 6 follow-up done (drain-empty peek fast path):
  investigated item (1) from the box-6 follow-up list first: instrumented
  hz10_class_pages_find_with_capacity with a temporary probe build on the
  slot_count=1/REMOTE_PCT=90 isolating case (200K iters) -- the per-class
  list does NOT grow unbounded in practice for this workload (avg ~19
  scanned nodes, max ~79, stable across the run); the original "unbounded
  list growth is the remaining cost" hypothesis was wrong for this shape.
  A longer run (2M iters) does show real RSS growth over time, but a
  stashed before/after comparison showed the same growth on the
  unmodified baseline too -- not something this fix introduced or fixes,
  likely Box 4 pool/page churn at sustained scale, left as-is
  actual finding: each of those ~19 scanned nodes pays a full
  atomic_exchange in hz10_page_drain_remote (src/hz10_remote_stack.c)
  even when remote_free_head is NULL (nothing pending) -- an atomic RMW
  costs more than a plain load on this architecture. Fixed with a
  relaxed atomic_load peek before the exchange, returning 0 immediately
  when nothing is pending; correctness-neutral (a stale NULL read just
  means this call skips a drain the next call will catch)
  measured honestly: the full hz10_public_entry_bench isolating case is
  too noisy on this shared machine to isolate the effect (run-to-run
  variance of 2-6x dwarfs the change, confirmed by re-running the
  unmodified baseline and seeing the same spread). Wrote a dedicated
  single-threaded microbenchmark instead (tight loop calling
  hz10_page_drain_remote on a page with nothing ever pushed): baseline
  ~520-530M calls/s, with the peek fast path ~830-844M calls/s -- a
  clean, repeatable ~1.6x for this specific operation, confirmed real
  and not noise. All smokes/ASan/UBSan/TSan/standalone check stayed
  green
  files: src/hz10_remote_stack.c only

box 6 follow-up done (class_pages bounded scan):
  investigated item (1) from the prior follow-up list ("cap/bound the
  per-class list") with a targeted, realistic probe first: a single
  thread doing nothing but hz10_malloc(64) in a loop, never freeing (a
  growing cache or long-lived arena, not adversarial). Confirmed real,
  not hypothetical: throughput degraded from ~26M ops/s in the first
  million calls to ~8.5M ops/s by 30 million -- textbook O(n^2), since
  every failed scan walks the entire, ever-growing list (nothing to
  find, nothing to drain, so the unbounded scan never short-circuits)
  fixed with HZ10_CLASS_PAGES_SCAN_LIMIT (128 pages,
  src/hz10_class_pages.h): bounds search cost per hz10_malloc call to
  O(1) regardless of total list length. Accepted, explicit tradeoff: a
  page whose capacity would only be found past the scan window is never
  found again by hz10_malloc (permanently invisible to reuse), though it
  remains correctly freeable via Box 1's route either way -- list
  membership here is purely an allocation-side search structure, hz10_free
  never consults it. Did NOT implement "return idle pages to Box 4's
  pool" from the original follow-up wording -- on inspection a fully
  idle page (free_count == slot_count) is already found and reused by
  the existing local_free_head check before ever needing eviction, so
  there was no real problem there to fix; proactively trimming an idle
  class's RSS is a different feature (allocator "trim"/release), out of
  scope here, not silently implemented
  same 30M-call never-free run stays flat ~26-28M ops/s after the fix
  (last_over_first=1.036, see bench/hz10_class_pages_scan_bench.c,
  `make bench-class-pages-scan` -- checked into the repo as a permanent
  regression bench, not left as a throwaway probe). Re-ran main_r50/
  main_r90 and the slot_count=1 isolating case afterward: both
  consistent with pre-fix numbers, no regression from the added bound.
  All smokes/ASan/UBSan/TSan/standalone check green
  files: src/hz10_class_pages.{h,c}, src/hz10_public_entry.h (doc only),
  bench/hz10_class_pages_scan_bench.c, Makefile, .gitignore

tcmalloc same-run script done
(scripts/run_hz10_public_entry_vs_tcmalloc_same_run.sh):
  formalized the ad hoc LD_PRELOAD tcmalloc comparison into a real,
  repeatable script, matching Box 1's
  scripts/run_hz10_pagemap_vs_hz9_same_run.sh opt-in pattern (skips the
  tcmalloc rows gracefully if no libtcmalloc_minimal.so.4 is found;
  TCMALLOC_LIB overrides; a new hz10_bench_find_tcmalloc_lib() helper in
  bench/lib/hz10_bench_common.sh). Always runs hz10's own
  main_local0/main_r50/main_r90/medium_local0 rows and prints an average
  same-run ratio summary across the requested RUNS. Running it with larger,
  steadier ITERS (500K-2M vs the original ad hoc 50000) surfaced a real,
  positive correction: local
  rows land at ~60-70% of tcmalloc, not ~38-50% as first measured -- see
  status above and bench/README.md for the honest explanation (shorter
  runs under-count hz10's steady-state throughput relative to its
  one-time setup cost)
  files: scripts/run_hz10_public_entry_vs_tcmalloc_same_run.sh,
  bench/lib/hz10_bench_common.sh

slot_count=1 isolation gap: root cause found via strace, not further
fixed (documented, deprioritized):
  measurement noise on this machine (2-6x run-to-run) made the ~24% gap
  unreadable via wall-clock benching alone, so switched to `perf stat`
  and `strace -c` on the exact isolating workload (THREADS=4
  MIN_SIZE=MAX_SIZE=65536 REMOTE_PCT=90, 8M total ops). Real numbers,
  not guesses: sys time (2.6s) exceeded user time (1.47s); strace showed
  152,490 mmap + 152,479 munmap + 253,273 futex calls over 8M ops --
  roughly 1 in 53 allocations pays a full fresh-page reservation
  (src/hz10_freelist_page.c's hz10_freelist_reserve_aligned_quantum()
  over-reserves 2x and munmaps the unaligned head/tail every time,
  so each miss costs 1 mmap + up to 2 munmap). This directly
  contradicted an earlier same-session RSS-delta-based estimate of
  "~500 pages per 800K ops, negligible" -- that estimate was wrong;
  strace is the trustworthy number here, RSS deltas were not.
  The reasonable next hypothesis (widen HZ10_CLASS_PAGES_SCAN_LIMIT so
  more late-arriving remote frees get caught before falling out of the
  window) was tested directly: SCAN_LIMIT=128 vs 1024, 6 repeats each,
  same workload -- averages were 5.36M and 5.55M ops/s, ~4% apart, well
  inside the run-to-run noise band. Widening the window does NOT
  meaningfully help; the misses are not primarily a window-size problem
  also confirmed separately: hz10_page_pool_reuse_count() and
  hz10_page_pool_release_count() both stay 0 throughout every one of
  these runs -- Box 4's pool is never populated because
  hz10_public_entry.c never calls hz10_pooled_page_destroy() (Box 6
  keeps every page reachable via the class list instead of destroying
  any of them). This is consistent with Box 6's design (not a bug), but
  it does mean every genuine miss pays a full syscall-level reservation
  with no pool to soften it
  conclusion: real, understood, not closed. A real fix would need either
  (a) making pages that fall permanently out of the scan window and are
  later confirmed idle destroyable and returned to Box 4's pool, or (b)
  a cheaper aligned-reservation primitive than reserve-2x-then-trim, or
  (c) a fundamentally different reuse structure. All three are bigger
  surgery than this investigation's scope, and this gap is isolated to
  a pathological combination (slot_count=1, the single largest size
  class, at REMOTE_PCT=90) that the established main_local0/main_r50/
  main_r90/medium_local0 rows do not hit -- deprioritized accordingly

box 3 O(N^2) drain cost CONFIRMED real and significant -- marker fix lane
selected and implemented:
  the box 3 O(N^2) owner-drain duplicate-check was earlier deprioritized
  as "not clearly the isolating case's bottleneck" -- true for
  slot_count=1, but that was the wrong row to test it against. Small
  size classes (large slot_count: 16B->4096, 32B->2048, 64B->1024) are
  never hit by the main_local0/r50/r90/medium_local0 rows this project
  had been using (their size ranges only ever land in slot_count<=16
  classes), so this cost went completely untested until now
  new row tested (MIN_SIZE=16 MAX_SIZE=64, matching this project's own
  established "small_remoteNN" naming from the root current_task.md's
  HZ9 notes), THREADS=4 ITERS=500000, low noise (unlike the slot_count=1
  case -- RSS stays flat, run-to-run spread is tight):
    REMOTE_PCT=0:  hz10 ~200-208M ops/s, parity with system_malloc
    REMOTE_PCT=50: hz10 ~5.7-6.2M vs system_malloc ~19.4M (~3.2x SLOWER)
    REMOTE_PCT=90: hz10 ~3.0-3.3M vs system_malloc ~12.9M (~4.2x SLOWER)
  confirmed via perf stat + strace this is a pure CPU cost, not a
  syscall one (opposite signature from the slot_count=1 investigation
  above): user time 5.26s vs sys time 0.015s, 606 page-faults (trivial),
  ~4880 cycles per op -- consistent with hz10_page_local_freelist_contains
  (src/hz10_remote_stack.c) walking long local_free_head chains
  (up to 4096 nodes) on every drained slot, exactly the O(merged x
  current_local_len) cost already measured in isolation back in box 3
  (217.4M ops/s before the user's delayed-double-free fix, 1.83M after)
  selected fix: glibc-tcache-style in-slot marker. Box 2's minimum public
  slot_size is 16B, so the first word remains the intrusive next pointer and
  the second word can carry a local-free marker. Same-thread free writes the
  marker, alloc clears it, remote publish rejects marked slots before it can
  overwrite the local next pointer, and drain only pays the old O(N) contains
  walk on a marked fallback. This preserves the fail-closed foreign
  double-free check while avoiding a separate per-slot bit array
  preliminary post-fix check (THREADS=4 ITERS=200000 RUNS=3,
  MIN_SIZE=16 MAX_SIZE=64): REMOTE_PCT=50 hz10 ~17.0-18.8M vs system_malloc
  ~18.1-19.5M; REMOTE_PCT=90 hz10 ~12.7-13.8M vs system_malloc ~12.0-13.1M.
  The old 3-4x slower small_remote regression is gone in this short run.
  Main/medium local short checks stayed strong (main_local0 ~171-178M vs
  system_malloc ~105-106M; medium_local0 ~189-195M vs ~98-100M)
  this is now the clearer priority over the slot_count=1 gap (that one
  needs a quiet machine or bigger surgery either way; this one has a
  clean, low-noise, reproducible 3-4x signal and a concrete, bounded fix
  shape already sketched, just gated on the Box 2 hot-path decision)

remote-free striping done (Box 3, not literal snmalloc-style batching --
see reasoning below):
  ChatGPT-collaborator plan (independently reviewed, mostly agreed with)
  proposed snmalloc-style batching to attack the ~25x local-vs-remote
  contention cost measured back in box 3. Considered literal batching
  (stage several frees locally, flush as one push) and rejected it for
  this iteration: it needs a guaranteed eventual flush, and a thread that
  remote-frees exactly once and then does nothing else could leave that
  free stuck in a thread-local staging buffer forever -- silently
  breaking the "accepted means at least drainable" contract every box
  and smoke test relies on. HZ10 has no thread-lifecycle/timer hook to
  guarantee a flush yet, so this would be a real, if narrow, correctness
  regression, not a safe drop-in
  implemented instead: remote_free_head is now HZ10_REMOTE_STRIPE_COUNT
  (4) independent Treiber stacks per page
  (src/hz10_freelist_page.h/.c), keyed by a hash of the freeing thread's
  own private thread-local token (hz10_remote_stack.c owns this identity
  concept itself -- does not reach into Box 5's owner_thread_token, kept
  one-directional). Every remote free still does exactly one CAS (same
  count as before), just onto one of several cache lines instead of one
  shared one -- spreads contention without any flush-guarantee gap
  measured honestly: the EXISTING hz10_remote_stack_drain_bench's
  remote_push mech showed no visible change from striping -- turned out
  its own methodology creates a new OS thread via pthread_create for
  EVERY one of REPEAT (2000) cycles, so its "remote_push" number is
  dominated by thread-lifecycle overhead, not CAS contention (a real,
  pre-existing bench limitation, not something this change introduced;
  worth fixing in that bench file later but out of scope here). Wrote a
  dedicated microbenchmark instead (persistent producer threads via
  pthread_barrier, no per-cycle thread creation) and swept stripe counts
  1/2/4/8/16, 5-6 repeats each: baseline (1 stripe) ~15.0M ops/s stable;
  4 stripes ~16.3-16.5M (~8-9% faster); 8 stripes ~15.2-21.0M (noisier,
  ~15% faster on average); 16 stripes no further gain (~16.3M) -- matches
  the "spreads contention across N cores, plateaus once stripes >=
  thread count" intuition. Settled on 4 stripes: modest, real,
  reproducible improvement, not the dramatic win the original ~25x
  number might have suggested (a meaningful chunk of that 25x is the
  inherent cost of an atomic RMW vs a plain store, which no amount of
  striping removes)
  re-verified small_remote50/90 and main_r50/r90 rows afterward: no
  regression from the added per-page memory (remote_free_head grew from
  1 pointer to 4) or the extra per-stripe peek during drain
  files: src/hz10_freelist_page.h (struct field, HZ10_REMOTE_STRIPE_COUNT),
  src/hz10_remote_stack.c (stripe picker, push/drain loop over stripes)

finer size-class table done (src/hz10_size_class.{h,c}):
  24 classes instead of 13: 16, 32, then a (1.5x, 2x) pair per doubling
  from 32 up to 65536 (48/64, 96/128, ... 24576/32768, 49152/65536),
  jemalloc-style. The very first doubling (16..32) keeps no 1.5x member
  since 1.5*16=24 is not a multiple of HZ10_MIN_ALIGN (16) -- Box 1
  requires every slot offset on a 16-byte boundary, so slot_size itself
  must stay a 16-multiple; every 1.5x class from 48 up is naturally a
  16-multiple (1.5*2^e for 2^e>=32)
  classify function is still closed-form O(1), no table scan: same
  "how many bits does size-1 need" trick as before to find the octave,
  then one comparison against the octave's 1.5x midpoint to pick which
  of its 2 classes. Verified exhaustively (not sampled) against a
  from-the-table linear-scan oracle for every one of the 65536 possible
  byte sizes -- new tests/hz10_size_class_smoke.c, `make
  smoke-size-class`, checked into the repo as a permanent regression
  test for this notoriously off-by-one-prone kind of code
  measured the actual fragmentation improvement directly (a Python
  script simulating 2M uniform-random sizes against both tables, not
  assumed): average rounding waste roughly halved (33.4% of requested
  bytes wasted with the old 13-class table vs 16.7% with the new
  24-class one). Does not help the very smallest requests (1-32 bytes,
  the un-splittable first doubling) -- benefit is real for everything
  from 33 bytes up
  known, accepted tradeoff: 1.5x classes do not evenly divide
  HZ10_PAGE_QUANTUM, so their pages waste a little tail space (negligible
  for small classes like 48 -- 16 of 65536 bytes; up to 16384 of 65536
  bytes, 25%, for the largest 1.5x class, 49152, whose page holds only
  1 slot anyway) -- still a net win for the requesting application
  (see hz10_size_class.h for the full writeup), and Box 1's route
  already rejects any pointer in that tail slack as out-of-range, so
  it's a space cost, not a correctness one
  re-verified all existing smokes, ASan/UBSan/TSan, main/medium local
  and remote rows: no regression (class boundaries for existing tests'
  fixed sizes -- 32768, 65536 -- are unchanged, both still exact classes)
  files: src/hz10_size_class.{h,c}, tests/hz10_size_class_smoke.c, Makefile

large-object path done (src/hz10_large_alloc.{h,c}):
  size > HZ10_PAGE_QUANTUM now routes to a dedicated direct-mmap path
  instead of hz10_malloc returning NULL. Design: register the allocation
  with Box 1 as a single-slot "page" (slot_size == the request rounded up
  to the next HZ10_PAGE_QUANTUM multiple, slot_count == 1) -- reuses Box
  1's exact classification pipeline (tail-slack/misaligned/interior/
  generation) as-is instead of inventing a second one. Only the
  allocation's own base quantum is ever registered; any pointer landing
  in a later quantum of the same reservation looks up a different,
  unregistered pagemap slot and correctly MISSes -- sufficient because a
  large allocation only ever hands out its own base pointer, never an
  interior offset, so nothing ever needs to validate an address in a
  later quantum
  hit a real bug during implementation, not just theory: Box 1's
  register function rejected ANY span exceeding one quantum
  unconditionally, including slot_count == 1, so the first attempt
  failed every call. Fixed by relaxing the check to apply only when
  slot_count > 1 -- the "fits in one quantum" requirement only exists
  because a multi-slot page's later slots need to be addressable through
  the SAME single leaf entry as the first, which a single-slot
  registration (only ever slot index 0) never needed anyway. This was
  exactly the extension the original Box 1 design doc anticipated
  ("multi-quantum span registration is a natural Box 2+ extension, not
  needed for the routing proof itself"). Verified the relaxation is
  scoped correctly with a dedicated new Box 1 smoke case: a 3-quantum
  single-slot registration succeeds and classifies correctly (exact/
  later-quantum-MISS/past-end), while a multi-slot registration spanning
  more than one quantum is still correctly rejected
  added a second opaque field to Box 1, H10RouteResult.flags (mirrors
  owner, Box 1 never interprets it), so hz10_free() can tell a large
  allocation's route apart from a small-class Hz10FreelistPage* before
  ever casting route.owner -- HZ10_PAGEMAP_FLAG_LARGE lives in
  hz10_large_alloc.h, not Box 1, keeping Box 1 agnostic about what the
  flag means
  known, accepted L0 gaps (stated up front in hz10_large_alloc.h, not
  discovered after the fact): no pooling/reuse of large blocks (every
  hz10_large_alloc() is a fresh mmap, every hz10_large_free() a real
  munmap) and no cross-thread remote-free batching for large objects
  (works correctly either way, just not measured as a local-vs-remote
  row the way small classes are)
  measured the pooling gap's real cost, not just asserted it: a tight
  alloc/touch/free loop (single object size, no other threads) is ~70K
  ops/s for hz10's large path vs ~53-56M ops/s for system_malloc at the
  same sizes (200KB and 1MB) -- roughly 750x slower. This is not
  apples-to-apples: glibc's dynamic mmap-threshold adjustment serves
  this exact repeated-same-size pattern from a reused heap region after
  the first few iterations (no real syscalls), while every hz10 call
  pays a genuine mmap+munmap round trip by design (no pooling yet). The
  gap is real, expected, and already explained by a documented gap, not
  a surprise -- a future box should add a size-bucketed large-object
  cache if this matters for a workload that actually churns large
  objects (most do not: large allocations are typically few and
  long-lived, unlike the small/medium classes this project's rows
  actually stress)
  re-verified all existing smokes, ASan/UBSan/TSan, and that
  main_local0/medium_local0 (which cap at 32768/65536, never reaching
  this new path) show no regression
  files: src/hz10_large_alloc.{h,c}, src/hz10_pagemap.{h,c} (flags field,
  relaxed span check), src/hz10_public_entry.{h,c} (dispatch),
  tests/hz10_pagemap_route_smoke.c, tests/hz10_public_entry_smoke.c,
  Makefile

next:
  rerun small_remote50/90 and main/medium rows at larger ITERS/RUNS for the
  final table. The slot_count=1 gap still needs one of its three
  bigger-surgery options (see above), not another small parameter tweak
  then proceed to task #45 (decommit/aging policy for Box 4's page pool)

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
