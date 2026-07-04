# HZ10 Current Task

HZ10 is a standalone next-substrate research line. Keep this file short.

## Active Direction

```text
status:
  Box 1-6 and the follow-on fixes are implemented, verified, and committed.
  Latest commits through de4a0715:
    Box1 pagemap route
    Box2 thread-local intrusive freelist page
    Box3 remote stack + owner drain
    Box4 bounded page pool + explicit purge_idle()
    Box5 multi-class public entry
    Box6 per-class page tracking / remote recovery
    follow-ons: in-slot local-free marker, remote-free striping, finer
      size classes, large-object path, pool decommit, batched quantum
      reservation

  Current performance read:
    LOCKED-IN final table, 20260704T030633Z, THREADS=4 ITERS=500000
      RUNS=10, real tcmalloc via LD_PRELOAD (not system_malloc-only):
      logs at bench_results/20260704T030633Z_hz10_public_entry_vs_tcmalloc_same_run/combined.log
      and bench_results/20260704T030633Z_hz10_full_followup/
        main_local0:    ratio=0.562 (hz10 184.4M vs tcmalloc 328.0M ops/s)
        main_r50:       ratio=0.475 (hz10  13.4M vs tcmalloc  28.3M ops/s)
        main_r90:       ratio=0.488 (hz10   8.7M vs tcmalloc  17.8M ops/s)
        medium_local0:  ratio=0.553 (hz10 183.3M vs tcmalloc 331.6M ops/s)
        small_remote50: ratio=0.694 (hz10  18.76M vs tcmalloc 27.05M ops/s)
        small_remote90: ratio=0.700 (hz10  12.97M vs tcmalloc 18.52M ops/s)
        slot_count=1 (65536/65536, REMOTE_PCT=90):
                        ratio=0.628 (hz10   6.72M vs tcmalloc 10.71M ops/s)
      RSS: small_remote50/90 sees HZ10 noticeably lighter than tcmalloc;
        slot_count=1 RSS stays high for HZ10 (see RSS growth note below)
    Box6 flipped prior remote rows from 15-17x SLOWER than system malloc
      to ~1.5-1.7x FASTER than system malloc
    small_remoteNN O(N^2) drain regression is fixed by in-slot marker:
      short post-fix check puts small_remote50/90 around system_malloc
      parity instead of 3-4x slower; against real tcmalloc the locked-in
      table above still shows a real ~30% gap, attributed (not yet
      further reducible) to the inherent cost of the 2 atomic RMWs every
      remote free pays: pending-bit fetch_or (hz10_page_remote_free) and
      the Treiber-stack CAS push, both in src/hz10_remote_stack.c -- box3's
      striping notes already found stripe count plateaus around 4-8, so
      this is not expected to move further without a design change
    slot_count=1 gap is closed FOR THE RAW SYSCALL COST by batched quantum
      reservation (strace mmap calls dropped ~152,490 -> 874, munmap
      ~152,479 -> 864, roughly system_malloc parity) but the locked-in
      table above shows a real ~37% gap against actual tcmalloc still
      remains. Root cause not yet fixed: Box4's page pool is created
      (hz10_pooled_page_create_with_owner is called from
      src/hz10_public_entry.c:67) but never destroyed back
      (hz10_pooled_page_destroy is never called from the public-entry
      path) -- so every slot_count=1 exhaustion event still pays a fresh
      Hz10FreelistPage struct alloc + full pagemap registration, because a
      1-slot page is exhausted on literally every allocation (nothing
      amortizes this bookkeeping the way a multi-slot page does). This is
      the still-open "Box6/Box4 pool wiring" question below, now
      re-prioritized as the concrete next action for this row specifically
    post RSS is generally below tcmalloc in measured public-entry rows;
      HZ10 still aims for "closer to HZ8 RSS than tcmalloc RSS"
    RSS growth observation (new, from the locked-in 20260704T030633Z run):
      post_rss_kb in combined.log increases monotonically run 1->10 within
      the SAME process for main_local0/main_r50/main_r90 (e.g. main_r90
      165248 -> 883808 KB across 10 runs of the same 2M-op workload). This
      is consistent with, not contradicting, Box6's deliberate "never
      destroy a page, keep it forever in the per-class list" design (see
      box 6 notes above) -- not a bug, but it does mean sustained/long-running
      traffic keeps growing RSS with no ceiling in the current wiring. Not
      separately root-caused yet; likely mitigated by the same Box4 pool
      wiring fix as the slot_count=1 gap above, since actually destroying
      genuinely-idle pages is a prerequisite for both. Revisit after that
      fix lands and report honestly whether it helps or not

  Validation read:
    standalone check and normal smokes pass
    ASan/UBSan smokes pass
    TSan claims in older log entries should be treated cautiously here:
      this environment previously hit TSan runtime "unexpected memory mapping"
      before test execution. Reconfirm TSan in a clean environment before
      calling it a release gate.

  Next HZ10 action (re-prioritized after the locked-in table above):
    (1) DONE: perf stat + strace pass on main_r50/main_r90 found and fixed
        two real over-scoped locks (src/hz10_freelist_page.c's
        quantum-region lock, src/hz10_page_pool.c's pool lock -- see
        "main_r50/main_r90 perf stat + strace pass done" below), but the
        row ratio stayed within this machine's documented noise band
        (not a confirmed win). The real ~1.8x futex differential vs
        tcmalloc, after subtracting the bench harness's own shared inbox
        cost, is still not attributed to a specific line -- open
        hypothesis: compounded remote-free atomic RMW cost across all 24
        size classes, unconfirmed
    (2) DONE: wired Box6's per-class page list to Box4's pool (see
        "Box 6 -> Box 4 pool wiring done" below) -- real, measured RSS win
        for slot_count=1-shaped classes (bounded instead of unbounded
        growth), no ops/s change. Also found and corrected a measurement
        error in the original slot_count=1 ratio (0.628 -> 0.485, see
        "CORRECTION" note below) while re-measuring this
    (3) DONE: revisited RSS growth -- the Box6/Box4 wiring fix helps
        slot_count=1 specifically, does not meaningfully change
        main_r50/r90's RSS growth (see below for why)
    bench/README.md update still deferred: the real ops/s gap for
    main_r50/r90 and slot_count=1 (~0.48 both, after correction) remains
    open and unattributed to a specific line -- see "next" below for the
    concrete follow-up (a dedicated remote-free RMW microbenchmark)

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

decommit/aging policy done (src/hz10_page_pool.{h,c}):
  the RSS Contract's "cap overflow returns/decommits pages" line only
  ever covered the OVERFLOW half -- cached blocks under the cap sat
  resident forever with no expiry. Added
  hz10_page_pool_purge_idle(max_idle_ns): each cached block now stores
  its own release() timestamp (hz10_platform_now_ns(), CLOCK_MONOTONIC)
  in its own memory (same "write into otherwise-unused cached-block
  bytes" technique Box 3's local-free marker already uses), and
  purge_idle walks the cache (bounded by the cap, so cheap even called
  often) releasing any block idle longer than the given threshold
  deliberately an explicit, caller-invoked sweep (like glibc's
  malloc_trim()), not automatic: HZ10 has no background-thread/timer
  infrastructure, and building one now would be new, untested
  infrastructure well beyond this task's scope -- an explicit API gives
  the capability without inventing a scheduler
  smoke test avoids real-time sleep (flaky/slow) by testing both
  unambiguous ends of the threshold instead: a huge max_idle_ns purges
  nothing this fresh, a zero max_idle_ns purges everything currently
  cached (idle_ns >= 0 always holds) -- together these prove the
  threshold comparison itself without depending on wall-clock timing
  measured, not just asserted: filled the pool to 1024 cached blocks (a
  cap far above the real default of 64, to get a clean signal), touched
  each block's first byte so it was genuinely resident, then measured
  purge_idle(0) directly: ~3.8-4.4ms to purge all 1024 (cheap, bounded
  by cap as designed) and a real, current-RSS drop (via /proc/self/statm,
  not getrusage's ru_maxrss high-water mark, which cannot show a
  decrease at all) from ~5.3MB to ~1.4-1.7MB -- confirms the mechanism
  actually returns memory to the OS, not just removes bookkeeping
  IMPORTANT, honestly-scoped caveat: this feature has zero effect on
  hz10_public_entry_bench's rows today. hz10_public_entry.c (Box 6)
  never calls hz10_pooled_page_destroy() -- it keeps every page
  reachable via the class list forever instead of destroying any of
  them (a deliberate Box 6 decision, not an oversight) -- so Box 4's
  pool, this decommit policy included, is currently populated by
  NOTHING in the real hz10_malloc/hz10_free path; only Box 4's own
  standalone smoke/bench exercise create/destroy cycles directly. This
  is not swept under the rug: whether it is worth wiring Box 6 to
  actually destroy genuinely-idle pages and use Box 4's pool (trading
  some of Box 6's "never destroy, always findable" guarantee for real
  RSS give-back) is an open design question for whoever picks this up
  next, not resolved here
  re-verified all existing Box 4 smokes, ASan/UBSan/TSan: no regression
  files: src/hz10_page_pool.{h,c}, tests/hz10_bounded_page_pool_smoke.c

slot_count=1 gap CLOSED (src/hz10_freelist_page.c, option (b) from the
three candidates listed above -- a cheaper aligned-reservation
primitive, not (a) wiring Box 6 to Box 4's pool or (c) a different
reuse structure):
  root cause (already found via strace, see above): every fresh single
  quantum paid its own mmap(2x)+trim(1-2 munmap) round trip.
  hz10_freelist_reserve_aligned_quantum() now reserves
  HZ10_QUANTUM_REGION_COUNT (256) quanta at once with the same
  over-reserve-then-trim technique, then bump-allocates individual
  quanta from that region with no further syscalls until it's
  exhausted -- amortizing the trim cost by ~256x. POSIX only (the
  Windows branch is unchanged: VirtualAlloc's allocation granularity
  already matches HZ10_PAGE_QUANTUM with no trim needed, and Windows
  cannot MEM_RELEASE a sub-range of a larger reservation the way POSIX
  munmap can). Individual quanta still release independently when a
  page is destroyed -- munmapping a sub-range of a larger mapping is
  valid POSIX usage, and the bump cursor only ever moves forward so
  nothing is ever double-handed-out. Chose option (b) over (a): (a)
  needed a way to find idle pages beyond the scan window without
  reintroducing unbounded scan cost (no clean solution found when
  reconsidered), while (b) is a pure Box 2 implementation detail,
  touches nothing in Box 3-6, and directly attacks the exact syscalls
  strace already identified as the cost
  did NOT decide the open Box 6/Box 4 wiring question from the
  decommit-policy box -- this fix makes it moot for the slot_count=1
  case specifically (see below), so it stays open for whatever else
  might motivate it later
  measured, not assumed: re-ran the exact strace diagnostic from the
  original root-cause investigation (THREADS=4 MIN_SIZE=MAX_SIZE=65536
  REMOTE_PCT=90, 8M ops) -- mmap dropped from ~152,490 to 874 calls
  (~175x fewer), munmap from ~152,479 to 864. hz10_public_entry_bench's
  isolating-case numbers, run 4 times with both mechs in the same
  invocation: hz10 5.5-6.9M ops/s vs system_malloc 5.4-7.1M ops/s --
  essentially at parity (hz10 wins some runs), up from the previously
  measured ~24% slower (4.6M vs 6.1M). Numbers are also far more stable
  run-to-run than before (the original 2-6x variance was largely the
  syscall cost itself, not scheduler noise as first suspected)
  re-verified all existing smokes, ASan/UBSan/TSan (this touches shared
  mutable state -- a global bump cursor behind a mutex -- so this
  mattered more than usual), and every established row (main_local0/
  r50/r90, medium_local0, small_remote50/90): no regression, Box 2's own
  bench unaffected
  files: src/hz10_freelist_page.c only

locked-in final table collected (20260704T030633Z, THREADS=4 ITERS=500000
RUNS=10, real tcmalloc via LD_PRELOAD): main_local0=0.562, main_r50=0.475,
main_r90=0.488, medium_local0=0.553, small_remote50=0.694,
small_remote90=0.700, slot_count=1=0.628 -- see "Current performance read"
above for the full breakdown and per-row root-cause status. Also surfaced
a new RSS monotonic-growth observation from this run (see above)

CORRECTION to the slot_count=1 figure above, found while re-measuring for
the Box 6/Box 4 pool-wiring work below: 0.628 (6.72M/10.71M) does not
reproduce from bench_results/20260704T030633Z_hz10_full_followup's own raw
data. Recomputing directly from slot_count1_tcmalloc.log alone (the one
genuine same-run pairing: mech=hz10 and mech=system_malloc in the SAME
LD_PRELOAD=libtcmalloc_minimal.so.4 process) gives hz10_avg=6.99M,
tcmalloc_avg=14.42M, ratio=0.485 -- both this session's independent re-run
(THREADS=4 ITERS=500000 RUNS=10, fresh process) and the original log's own
numbers agree on this. The 10.71M figure lands almost exactly on the
average of slot_count1_tcmalloc.log's real-tcmalloc numbers (14.42M) AND
slot_count1_native.log's plain-glibc numbers (6.68M, no LD_PRELOAD at
all) pooled together -- (14.42+6.68)/2 = 10.55M, within rounding of
10.71M. Whatever produced the original figure blended two different
mechanisms (real tcmalloc and plain glibc malloc) into one "tcmalloc"
average, which is not a valid same-run comparison. Corrected: slot_count=1
ratio against REAL tcmalloc is ~0.485, a bigger gap than previously
stated, not 0.628. Treat any other cross-session-reported ratio the same
way going forward -- recompute from the actual paired same-process log
before trusting it

main_r50/main_r90 perf stat + strace pass done -- found and fixed two
real over-scoped locks, but the row ratio did NOT move outside this
machine's already-documented noise band; gap stays open, not closed:
  perf stat on the isolating workload (THREADS=4 MIN_SIZE=16 MAX_SIZE=32768
  REMOTE_PCT=50, MODE=0/hz10-only, 8M ops) showed sys time (0.985s)
  exceeding user time (0.767s) -- a mutex/futex signature, not a CPU-bound
  one. strace -c confirmed: 98.7-98.9% of syscall time in futex (132,803
  calls for r50, 209,231 for r90), versus near-zero (122 total syscalls,
  no futex) at REMOTE_PCT=0 on the identical size range -- the cost is
  real and specific to REMOTE_PCT>0, not a general main_local0-shaped cost
  found bug #1: src/hz10_freelist_page.c's hz10_quantum_region_lock (the
  slot_count=1 fix's own mutex) wrapped the ENTIRE
  hz10_freelist_reserve_aligned_quantum() body, including the common
  per-quantum bump-allocate, not just the rare mmap+trim refill -- every
  single fresh-quantum handout across all 24 size classes and all threads
  serialized on one lock. Fixed: common path is now a lock-free CAS bump
  on _Atomic(char*) cursor/end; the mutex now guards only the actual
  refill (~1-in-256 calls), matching what the surrounding comment already
  claimed it did
  found bug #2 (bigger effect on paper): src/hz10_page_pool.c's
  hz10_page_pool_try_acquire() takes hz10_pool_lock unconditionally on
  EVERY new-page creation. Per this file's own earlier finding (see the
  decommit/aging policy box above), the pool is PROVABLY always empty on
  the real hz10_public_entry.c path (hz10_pooled_page_destroy is never
  called there) -- so every call was paying a full lock/unlock pair to
  find nothing. Fixed with the same relaxed-peek-before-lock pattern
  already used in src/hz10_remote_stack.c's drain fast path: hz10_pool_head
  is now _Atomic(Hz10PagePoolNode*), and try_acquire returns NULL
  immediately on a relaxed NULL peek, skipping the mutex entirely on the
  (currently: always) empty case. hz10_page_pool_purge_idle() updated to
  walk a local snapshot under the lock and publish the kept list back
  once, since it can no longer take `&hz10_pool_head` as a plain pointer
  measured, not assumed, that these were NOT the dominant cost: re-ran the
  identical strace -c after both fixes -- futex count for main_r50/r90 was
  unchanged (135,587 / 212,721, statistically the same as before). Root
  cause of that non-result, found by comparing MODE=0 (hz10) vs MODE=1
  with real tcmalloc vs MODE=1 with plain glibc, same bench binary, same
  row: bench/hz10_public_entry_bench.c's OWN Hz10BenchInbox (the
  mutex-protected per-thread queue the harness itself uses to simulate
  "a different thread ends up calling free()", see
  hz10_bench_inbox_push/drain) contributes a large futex cost EQUALLY to
  every mechanism under test -- plain glibc malloc alone (no LD_PRELOAD)
  hit 1.1M futex calls on this same row from arena-lock contention on top
  of that shared inbox cost, while real tcmalloc (LD_PRELOAD, MODE=1)
  showed 73,627. HZ10 (MODE=0) showing 135,587 sits between those two,
  meaning strace on the full bench binary conflates bench-harness
  simulation cost with allocator-internal cost -- a methodological finding
  for whoever profiles this bench again: compare mode=0 against a
  known-good real mechanism in the SAME run before attributing 100% of
  syscalls to the thing under test
  confirmed hz10_pagemap_route() (the per-free lookup used by every single
  op, unlike register/release) is lock-free by design already (see
  src/hz10_pagemap.c, "Lock-free read" comment) -- not a hidden contention
  source
  net result, reported honestly: re-ran the same-run tcmalloc script
  (THREADS=4 ITERS=500000 RUNS=6) after both fixes: main_local0=0.540,
  main_r50=0.484, main_r90=0.434, medium_local0=0.574 -- all within this
  machine's already-documented 2-6x run-to-run noise band around the
  locked-in table above (0.562/0.475/0.488/0.553), not a confirmed
  improvement OR regression. Both fixes are kept anyway: they are real,
  correctness-preserving removals of unnecessary lock/unlock pairs
  (verified via strace that the pool lock truly is skippable on this path,
  and the quantum lock truly was over-scoped), re-verified against all
  smokes, ASan/UBSan, TSan (ASLR off), and hz10-standalone-check, all
  green -- just not the row's dominant cost, so the ~0.48 gap stays open
  hypothesis at the time this was written: the same "inherent" per-op
  cost already documented in the small_remoteNN investigation above (2
  atomic RMWs per remote free: pending-bit fetch_or + Treiber CAS push,
  src/hz10_remote_stack.c), now compounded across all 24 size classes
  simultaneously instead of the 1-3 small classes that row exercised --
  UPDATE: tested directly with a dedicated microbenchmark (see "remote-
  free class-count sweep done" below) and REFUTED for the push/RMW side
  specifically; a real but different compounding cost was found instead,
  on the drain/scan side. See that entry for the full result
  files: src/hz10_freelist_page.c, src/hz10_page_pool.c

Box 6 -> Box 4 pool wiring done (src/hz10_class_pages.{h,c}, src/
hz10_freelist_page.h): a real RSS win, but NOT an ops/s win --
reported honestly as both:
  root cause (see "slot_count=1 isolation gap" above, option (a), and the
  corrected ratio above): Box 6 never destroys a page, so Box 4's pool is
  provably always empty on the real hz10_malloc/hz10_free path. For a
  slot_count=1 class specifically, a page is exhausted on literally every
  allocation, so the number of distinct pages ever created for that class
  vastly exceeds HZ10_CLASS_PAGES_SCAN_LIMIT (128) under sustained churn --
  confirmed the fix target is real: widening SCAN_LIMIT to 1024 was
  already tested earlier (see above) and did NOT help, meaning "keep
  everything forever and hope a wider scan finds it" cannot work at this
  churn rate; proactive reclaim is the only lever left
  design: Hz10FreelistPage gained a second list pointer
  (prev_in_owner_list, same "inert storage Box 2 never touches" rule as
  next_in_owner_list) so Hz10ClassPageList is now doubly-linked with a
  tail pointer and a length counter. hz10_class_pages_add() is still O(1)
  amortized -- prepend at head, and if length now exceeds SCAN_LIMIT,
  evict exactly the tail (oldest) node: one hz10_page_drain_remote() call
  (a last chance to notice a pending remote free) then a free_count ==
  slot_count check. Idle -> hz10_pooled_page_destroy() (real reclaim to
  Box 4's pool). Not idle -> just unlink and drop, identical fate to
  today's already-accepted out-of-window behavior (list membership was
  never load-bearing for hz10_free's correctness, see hz10_class_pages.h)
  this also closes Box 6's other long-standing accepted gap for free, as
  a side effect: the per-class list can no longer grow unbounded -- it is
  now permanently capped at SCAN_LIMIT nodes
  correctness reasoning, not just hope: destroying a page requires
  free_count == slot_count, which means every slot ever handed out has
  already come back -- no application-held live pointer can reference any
  of its slots, so reclaiming it is memory-safe. List mutation only ever
  happens from the owning thread's own hz10_malloc call (single-writer,
  matches Box 6's existing threading model), so no new synchronization
  was needed for the doubly-linked bookkeeping itself
  measured, not assumed: re-ran the slot_count=1 isolating workload
  (THREADS=4 MIN_SIZE=MAX_SIZE=65536 REMOTE_PCT=90) 6x in the same
  process, MODE=0 -- post_rss_kb stayed in the 265,984-424,064 KB range
  across all 6 runs, versus the pre-fix combined.log showing the same
  workload climb monotonically from 61,568 KB (run 1) to 1,174,492 KB
  (run 10) in ONE 10-run process. This is the real, intended win: RSS is
  now bounded instead of growing without limit under sustained slot_count=1
  churn. Checked the OTHER rows too, not just the one this was designed
  for: main_r90's RSS growth across 10 runs was NOT meaningfully
  different before vs after (149K->1.01M KB post-fix vs 165K->884K KB
  pre-fix, same order of growth) -- expected and reported honestly: main's
  24 classes mostly hold many slots per page, so few distinct pages get
  created per class within one run, meaning the per-class list rarely
  reaches the 128-node eviction boundary at all. This fix is a real win
  specifically for slot_count=1-shaped (or any high-distinct-page-count)
  classes, not a general RSS fix for every row
  ops/s: re-measured the slot_count=1 row against real tcmalloc after this
  fix (THREADS=4 ITERS=500000 RUNS=10, same methodology as the correction
  above) -- hz10_avg=6.91M, tcmalloc_avg=14.46M, ratio=0.478, statistically
  unchanged from the corrected 0.485 baseline. Reported honestly, not
  spun: this fix solves the RSS/memory-retention problem it targeted, but
  does NOT move the throughput ratio -- makes sense in hindsight, since
  the earlier batched-quantum-reservation fix had already made a *fresh*
  quantum cheap (bump-allocate, no per-call mmap), so pool-reuse vs
  fresh-allocation are now close in CPU cost; the value here is bounding
  RSS, not raising ops/s
  re-verified: all smokes (including a new-shape bounded_page_pool/
  public_entry pairing), ASan/UBSan (ASAN_OPTIONS=detect_leaks=0 to
  isolate real errors from the already-documented TLS-resident-page
  LeakSanitizer noise, per tests/README.md), TSan (ASLR off),
  hz10-standalone-check, and the checked-in never-free regression bench
  (bench/hz10_class_pages_scan_bench.c) all green -- that bench's
  last_over_first stayed ~1.02-1.09 (flat, no O(n^2) reintroduced),
  though its absolute ops/s dropped from the pre-fix ~32-34M to ~27-29M:
  a real, expected, honestly-reported cost (every add() in a pure
  never-free workload now pays one extra drain-peek + free_count compare
  for the eviction check, since that workload hits the eviction path on
  almost every call)
  files: src/hz10_class_pages.{h,c}, src/hz10_freelist_page.h

next (re-prioritized from this table, see "Next HZ10 action" above):
  (1) DONE (see above) -- perf stat + strace root-cause pass on
      main_r50/main_r90: fixed two real over-scoped locks, ratio did not
      move outside noise, real cost still unattributed past the
      still-open RMW hypothesis above
  (2) DONE (see above) -- wired Box 6's per-class page list to Box 4's
      pool: real, measured RSS win for slot_count=1-shaped classes
      specifically (bounded instead of unbounded growth), but no ops/s
      change (ratio stays ~0.48, corrected baseline)
  (3) DONE (see above) -- revisited RSS growth: fix helps slot_count=1
      specifically, does not meaningfully change main_r50/r90's RSS growth
      (few distinct pages per class there, eviction rarely triggers)
  (4) DONE -- built the dedicated microbenchmark this section used to
      call for (see "remote-free class-count sweep" below): the RMW-cost-
      compounds-across-classes hypothesis is REFUTED for the push side,
      CONFIRMED for the drain side, refining rather than just closing this
      question
  deprioritized, unchanged: the box 3 O(N^2) contains-walk fallback (not
  confirmed to matter given the marker fix already handles the common case)

remote-free class-count sweep done (bench/hz10_multiclass_remote_bench.c,
new file, `make bench-multiclass-remote`): decisive answer to the
still-open "does remote-free RMW cost compound across classes" question,
refining rather than confirming or refuting it outright:
  design: same proven shape as bench/hz10_remote_stack_drain_bench.c
  (pre-allocate every slot up front, split ownership statically across
  persistent producer threads, time a pure parallel remote-free burst then
  a single owner drain pass) -- deliberately reused instead of
  reinvented, since that shape already avoids the two confounds the
  main_r50/r90 investigation identified: bench/hz10_public_entry_bench.c's
  own Hz10BenchInbox mutex, and fresh-page creation/eviction churn. New
  here: CLASSES pages (the first N of the 24 real hz10_size_class shapes,
  THREADS=4 matching main_r50/r90) instead of one, all their slots pooled
  into one flat array and Fisher-Yates shuffled (fixed seed) before being
  split across producer threads, so each thread's slice is a realistic
  cross-class mix rather than one class per thread. Swept CLASSES = 1, 2,
  4, 8, 16, 24
  measured, reproduced twice (REPEAT=200 and REPEAT=500, numbers agree
  within a few percent both times -- unusually low noise for this
  environment, unlike the slot_count=1 investigation's syscall-bound
  numbers): remote_push (the actual atomic RMW cost: pending-bit
  fetch_or + Treiber CAS) stayed FLAT to slightly IMPROVING across the
  sweep -- ~25-27M ops/s at classes=1, ~29-31M ops/s at classes=24, no
  degradation. owner_drain (the per-class hz10_page_drain_remote() calls
  a scan makes) showed a clear, reproducible, monotonic DECLINE instead:
  ~131-134M ops/s at classes=1 down to ~73-74M ops/s at classes=8, flat
  from there to classes=24 -- roughly 1.8x slower, plateauing rather than
  growing further past ~8 distinct pages
  conclusion: the original hypothesis ("compounded remote-free RMW cost
  across 24 classes") is REFUTED as stated -- the RMW push itself does not
  get more expensive with more classes live. What IS real and reproducible
  is a cache/TLB-pressure cost on the DRAIN/scan side from touching more
  distinct Hz10FreelistPage structs and their backing quanta, unrelated to
  the atomic-RMW mechanism itself. This is a genuinely different, more
  specific finding than the RMW guess it replaces: it points at
  hz10_class_pages_find_with_capacity()'s per-candidate
  hz10_page_drain_remote() calls (src/hz10_class_pages.c) as the
  compounding cost, not src/hz10_remote_stack.c's push path
  honest scope limit, stated up front: this bench measures N DIFFERENT
  pages touched in one timed drain round, which is a reasonable proxy for
  cache/TLB pressure from a larger live working set, but is NOT a literal
  reproduction of hz10_class_pages_find_with_capacity()'s real bound (a
  single hz10_malloc call only ever drains within its OWN class's list,
  up to HZ10_CLASS_PAGES_SCAN_LIMIT=128 pages of the SAME shape -- never
  across classes in one call). main_r50/r90's real working set could be
  up to 24 classes x 128 pages each, far larger than this sweep's max of
  24 total pages -- so the real effect in the full bench could plausibly
  be larger than what's measured here, not smaller. Not further chased
  in this session; flagged for whoever picks up the ops/s gap next
  re-verified: ASan/UBSan clean (no bugs in the new shuffle/thread-split
  logic), ops/s numbers reproduced across two separate REPEAT values,
  hz10-standalone-check green
  next concrete step for whoever continues this: profile
  hz10_class_pages_find_with_capacity()'s scan specifically (not the full
  bench binary) with perf stat/cachegrind at realistic SCAN_LIMIT-bounded
  depths per class, now that the drain-side cache-pressure mechanism is
  the confirmed lead instead of the RMW guess
  files: bench/hz10_multiclass_remote_bench.c (new), Makefile, .gitignore

first GO measured against real HZ8, not just guessed at -- mixed result,
reported honestly (throughput mostly clears the bar, RSS does not):
  this bar was stated in current_task.md from the start but had never
  actually been measured: every prior comparison in this file was against
  tcmalloc, not HZ8. New script scripts/run_hz10_public_entry_vs_hz8_same_run.sh
  (same technique as the tcmalloc same-run script: LD_PRELOAD HZ8's own
  default preload build, libhakozuna_hz8_preload.so from HZ8's own `make
  preload` -- HZ8-v2/KeepRefill baked in, over hz10_public_entry_bench's
  mech=system_malloc path) plus a new hz10_bench_find_hz8_preload_lib()
  helper in bench/lib/hz10_bench_common.sh, opt-in via HZ8_PRELOAD_LIB or
  HZ10_EXT_ROOT same as the existing HZ9 comparison
  measured (THREADS=4 ITERS=500000 RUNS=10, reproduced twice, numbers
  agree within ~1%):
    throughput ratio (hz10/hz8):
      main_local0:    1.971-1.972 (bar: >=2.0x -- just under, ~1.5% short,
                       not a clean pass but very close)
      main_r50:       1.462-1.580 (bar: >=1.2x remote -- clears easily)
      main_r90:       1.587-1.610 (bar: >=1.2x remote -- clears easily)
      medium_local0:  2.262-2.375 (not one of the three named gate rows,
                       but clears the 2.0x local bar comfortably)
    RSS ratio (hz10 post_rss_kb / hz8 post_rss_kb, run 10/10 of a 10-run
    same-process series, so this is steady-state after real accumulation,
    not a cold first sample):
      main_local0:    48,832 / 64,600 = 0.756 (bar: <=2x -- clears, hz10
                       actually lighter than HZ8 here)
      medium_local0:   9,196 / 64,564 = 0.142 (clears easily)
      main_r50:       791,024 / 179,272 = 4.41x HZ8's RSS (bar: <=2x --
                       FAILS, badly)
      main_r90:       953,860 / 253,124 = 3.77x HZ8's RSS (FAILS, badly)
  conclusion: the throughput half of "first GO" is essentially met (local
  just barely short, remote comfortably clear), directly consistent with
  hz3/hz4's own established pattern of winning specific lanes rather than
  sweeping every row. The RSS half is NOT met, and specifically fails on
  the remote rows -- directly consistent with everything else this
  session found: Box 6's per-class list only bounds/reclaims once a
  class's list actually hits HZ10_CLASS_PAGES_SCAN_LIMIT (128) within the
  run, which the earlier RSS-revisit work already showed does not
  meaningfully happen for main_r50/r90's classes at this op count (many
  slots per page, few distinct pages created) -- so RSS keeps
  accumulating there with nothing to trigger reclaim, exactly the
  mechanism already documented, now shown to matter enough to fail an
  actual named gate, not just look untidy in a chart
  honest scope note: this is ONE comparison point (THREADS=4,
  ITERS=500000x10, this machine, HZ8's current default build). Not
  re-verified against a from-scratch HZ8 rebuild or a different thread
  count; treat as a real first data point for a bar that had never been
  measured before, not a final verdict
  next concrete step this points to: closing the RSS gate for main_r50/r90
  needs the SAME Box 6/Box 4 reclaim mechanism built this session, just
  triggered by something other than the SCAN_LIMIT boundary (e.g. an
  explicit idle-page sweep call, or a much smaller effective window for
  high-churn classes) -- the slot_count=1 fix's mechanism already proves
  the reclaim path works, it just isn't reached by these rows' access
  pattern
  files: scripts/run_hz10_public_entry_vs_hz8_same_run.sh (new),
  bench/lib/hz10_bench_common.sh

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
