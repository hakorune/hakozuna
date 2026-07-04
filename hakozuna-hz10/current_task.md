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
    LATEST RECHECK after HZ9 owner-flush fix and HZ10 boundary-stress
      classification, 20260704T201752Z/201823Z/201859Z, THREADS=4
      ITERS=500000 RUNS=10:
      logs:
        bench_results/20260704T201752Z_hz10_public_entry_vs_tcmalloc_same_run/combined.log
        bench_results/20260704T201823Z_hz10_public_entry_vs_hz8_same_run/combined.log
        bench_results/20260704T201859Z_hz10_full_followup_extra_tcmalloc/combined.log
      vs real tcmalloc:
        main_local0=0.564, main_r50=0.472, main_r90=0.492,
        medium_local0=0.611, small_remote50=0.479,
        small_remote90=0.539, slot_count1_r90=0.543
      vs HZ8 default preload:
        main_local0=1.896, main_r50=1.386, main_r90=1.763,
        medium_local0=2.137
      Read: HZ10 is consistently faster than HZ8 on these rows, but still
        ~0.47-0.61x of tcmalloc. The next performance box should isolate
        remote-free atomic/RMW cost directly; do not spend the next step on
        thread-exit/RSS unless a true continuous workload reproduces it.
      UPDATE: remote-free RMW attribution microbench added
        (bench/hz10_remote_rmw_microbench.c). THREADS=4 SLOT_COUNT=4096
        REPEAT=20000 RUNS=3 shows worker-side medians:
        pending_fetch_or ~3.3ns/op, treiber_push ~26.5ns/op,
        counter_plus_treiber ~40ns/op, pending_counter_treiber ~59.5ns/op.
        Owner-side drain/clear is small (~3.7ns/op for the full case).
        This confirms the remote-row gap is dominated by freeing-thread
        atomic publication cost, especially Treiber CAS plus the extra
        remote_push_count fetch_add, not by owner drain.
      UPDATE: remote_push_count is now debug-only
        (HZ10_ENABLE_DEBUG_STATS) so the release hot path no longer pays
        that successful-publish fetch_add. The same microbench now reports
        the release-equivalent pending_plus_treiber case at ~39ns/op versus
        ~62ns/op for pending_counter_treiber. Short public-entry checks
        (main_r50/r90 and small_remote50/90, RUNS=5) did not show a clear
        row-level win, so treat this as a justified hot-path cleanup, not a
        confirmed public-entry performance step.

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
      traffic keeps growing RSS with no ceiling in the current wiring.
      UPDATE: the Box 6 -> Box 4 pool wiring fix was built and initially
      looked like it mitigated this, but that result did not reproduce
      under matched before/after methodology, and the class-list
      diagnostic counters added afterward confirmed why: reclaim almost
      never fires in any row (see "class-list diagnostic counters + real-
      workload check done" further down for the full, corrected finding).
      This RSS growth is still open, not resolved, in every row tested

  Validation read:
    standalone check and normal smokes pass
    ASan/UBSan smokes pass
    TSan claims in older log entries should be treated cautiously here:
      this environment previously hit TSan runtime "unexpected memory mapping"
      before test execution. Reconfirm TSan in a clean environment before
      calling it a release gate.

  Next HZ10 action (re-prioritized after the locked-in table above):
    (0) CURRENT: resolve the main_r50/r90 RSS contradiction before designing
        any thread-lifecycle hook. True continuous steady-state
        (bench/hz10_public_entry_steady_state_bench.c) reports active_length
        stabilizing after ramp-up and retired_count staying exactly 0, while
        the thread-reuse segmented bench reports retired_count and RSS
        increasing segment by segment under a nominally identical main_r50
        shape. Treat the workloads as NOT equivalent until proven otherwise.
        Next diagnostic box: compare continuous periodic stats against
        segmented boundary stats, with special attention to segment-end
        drain/flush/barrier/live-set reset behavior. Do not start the
        larger thread-exit cleanup design until this contradiction is
        explained.
        UPDATE: the first diagnostic pass explains the contradiction as a
        bench-shape difference, not allocator steady-state growth. With
        THREADS=4 ITERS=500000 RUNS=5 MIN=16 MAX=32768 REMOTE_PCT=50,
        thread-reuse showed retired_count 319 -> 5790 and RSS 30MB ->
        118MB, but also boundary_inbox_count 2860 -> 8519: fast workers
        stop at the segment barrier while slower workers keep pushing
        remote frees into their inboxes, so the boundary itself creates
        a remote-free backlog and eventual retire pressure. The matching
        continuous 5s run completed ~63.3M ops with retired_count=0 and
        RSS ~25MB. Therefore thread-reuse is now classified as a
        boundary-stress diagnostic, not a true steady-state RSS proof.
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
    (5) DONE: implemented RetiredPartialReuse-L1 (promote partial-capacity
        retired pages back to active instead of only destroying fully-idle
        ones -- see "RetiredPartialReuse-L1 implemented and measured"
        below). Correctly implemented and verified (unit test, isolated
        mechanism test, and confirmed to fire given 6x the iteration
        budget), but measures as exactly zero effect on main_r50/main_r90
        at the established benchmark scale -- root-caused to a rate
        mismatch (harvest runs constantly but the specific pages it
        inspects resolve far too slowly), not a bug. main_r50/r90's RSS
        growth is STILL OPEN; see that section's "next concrete step"
    (6) DONE: wired an event-driven ready queue (HZ10RetiredReadyQueue-L0)
        alongside harvest -- see "HZ10RetiredReadyQueue-L0 wired into the
        real path" below. MAJOR REFRAMING: a new opt-in steady-state
        bench (not fixed ITERS) revealed main_r50/r90 stop evicting
        pages ENTIRELY after a short ramp-up in true steady state
        (retired_count stays at exactly 0) -- the "RSS climbs
        monotonically" finding this whole investigation chased was very
        likely an artifact of the locked-in bench's own RUNS=10
        structure (fresh threads every run, abandoned on exit -- HZ10
        has no thread-lifecycle hook), not genuine unbounded steady-
        state growth. The ready queue itself is a real, large,
        confirmed win where it CAN apply (slot_count=1: ~99.8% of
        retirements reclaimed via the queue vs 0% via polling alone,
        backlog bounded instead of unbounded) -- but neither mechanism
        was ever going to fix main_r50/r90, because that row's real
        open problem is thread-exit abandonment, a different, larger,
        not-yet-scoped question
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

CORRECTION to the RSS win claimed above -- it does not reproduce, and
new eviction_count/eviction_reclaimed_count counters (added below) show
exactly why: the claimed "265,984-424,064 KB, bounded" measurement used
6 SEPARATE process invocations (RUNS=1 each, via a shell for-loop), while
the pre-fix baseline it was compared against (61,568 -> 1,174,492 KB) used
ONE process doing RUNS=10 internally -- not a same-run comparison, an
apples-to-oranges one. Re-measured with the SAME RUNS=10-in-one-process
methodology on both sides: post_rss_kb for the slot_count=1 row still
climbs monotonically, 46,592 KB (run 1) -> 896,884 KB (run 10) -- the
same order of magnitude and shape as the ORIGINAL pre-fix numbers. The
Box 6 -> Box 4 wiring did not close this gap. Caught by adding real
instrumentation instead of continuing to infer from RSS deltas -- see
"class-list diagnostic counters + real-workload check" immediately below
for the root cause this surfaced, and current_task.md's own broader
lesson: an RSS number without matching run methodology on both sides of
a before/after comparison is not trustworthy, no matter how clean it
looks in isolation

class-list diagnostic counters + real-workload check done (src/
hz10_class_pages.{h,c}, src/hz10_public_entry.{h,c},
tests/hz10_class_pages_smoke.c, bench/hz10_public_entry_bench.c): Box 6
had zero instrumentation -- every claim about whether/how often eviction
and reclaim actually fire was inferred from RSS deltas, never measured
directly. This is what caught the correction above:
  added eviction_count and eviction_reclaimed_count to Hz10ClassPageList
  (plain fields, list mutation is single-writer -- see the header for the
  full reasoning), incremented in hz10_class_pages_add(); added
  hz10_public_entry_class_list_stats(class_id, ...) to read them from the
  calling thread's own state (out-of-range class_id is a documented
  no-op); added tests/hz10_class_pages_smoke.c (Box 6's first dedicated
  smoke, previously only exercised indirectly) covering: no eviction
  within the scan limit, an idle evicted tail gets reclaimed, a non-idle
  evicted tail is only dropped and stays safely freeable; added an opt-in
  HZ10_DUMP_CLASS_STATS=1 aggregation to bench/hz10_public_entry_bench.c
  (reads every worker thread's per-class stats right before it returns,
  since TLS state disappears at pthread_join -- zero cost when unset)
  measured against the real main_r50/main_r90 and slot_count=1 workloads
  (THREADS=4 ITERS=500000 RUNS=10, MODE=0), and the result overturns the
  "SCAN_LIMIT rarely reached" guess from the earlier main_r50/r90
  section: eviction fires CONSTANTLY, not rarely -- main_r50 saw
  100-2,563 evictions per run, main_r90 saw 645-5,768 per run, and
  slot_count=1 saw 4,932-19,973 per run. But eviction_reclaimed_count
  stayed EXACTLY 0 across all 10 runs of BOTH main_r50 and main_r90 (20
  runs total, not one lucky sample), and stayed at 0-4 per run for
  slot_count=1 -- a reclaim rate of essentially 0% in every row tested,
  as low as 4-out-of-19,973 (0.02%) even in the row this mechanism was
  built for
  root cause, now understood instead of guessed: the one-shot
  hz10_page_drain_remote() call at eviction time only catches a page that
  HAPPENS to be idle at that exact instant. Under real cross-thread churn
  (REMOTE_PCT=50/90), a page's last outstanding slot is freed by another
  thread on its own schedule -- if that free hasn't landed yet (or hasn't
  even been pushed) at the moment this page reaches the tail and gets
  evicted, it is permanently classified "not idle" and dropped, even
  though it might have become idle microseconds later. The mechanism
  built this session (destroy-if-idle-at-eviction) is correct and works
  exactly as designed (proven by the unit test and by the rare nonzero
  reclaim counts for slot_count=1) -- it is just structurally unable to
  catch pages whose idleness arrives asynchronously and late, which is
  the common case under remote pressure, not an edge case
  conclusion: the actual, now-measured state of HZ10's RSS story is worse
  than either the original ~40-50% ratio narrative or the corrected-then-
  retracted "slot_count=1 RSS win" suggested. Bounding list LENGTH (the
  eviction_count side) is real and working -- the per-class list is
  provably capped at SCAN_LIMIT nodes, per the earlier bench/
  hz10_class_pages_scan_bench.c result. But bounding RSS (the
  reclaimed_count side) is NOT working in any row tested, because the
  reclaim trigger (opportunistic, point-in-time) is fundamentally
  mismatched to how idleness actually arrives (asynchronous, delayed)
  under remote pressure
  next concrete step this points to: a point-in-time idle check at
  eviction cannot work for this access pattern -- a real fix needs either
  (a) a deferred/retry mechanism for dropped-non-idle pages (a bounded
  "second chance" list, itself needing its own eviction policy -- real
  complexity, not a quick patch), or (b) accepting that reclaim-on-evict
  is the wrong lever entirely and revisiting the alternative already
  named in the original "slot_count=1 isolation gap" investigation: a
  cheaper aligned-reservation primitive already closed the SYSCALL half
  of this (see "slot_count=1 gap CLOSED" above) -- the RSS half may need
  its own, different mechanism rather than reusing that one
  files: src/hz10_class_pages.{h,c}, src/hz10_public_entry.{h,c},
  tests/hz10_class_pages_smoke.c, bench/hz10_public_entry_bench.c

active/retired two-list reclaim done (src/hz10_class_pages.{h,c}, src/
hz10_public_entry.{h,c}, tests/hz10_class_pages_smoke.c,
bench/hz10_public_entry_bench.c): the design proposed to fix the "one-
shot idle check misses async idle transitions" root cause -- implemented
and verified correct, but measured to NOT meaningfully move RSS in
either row it was tried against. Reported honestly as a real, working
mechanism whose real-world effect is too small to matter, not spun as a
fix:
  design (external review from two independent reviewers before
  implementation, both converged on this shape): Hz10ClassPageList split
  into two sublists, `active` (hz10_malloc's own bounded search
  structure, unchanged behavior, still capped at HZ10_CLASS_PAGES_
  SCAN_LIMIT=128) and `retired` (new, also capped, HZ10_CLASS_PAGES_
  RETIRED_LIMIT=128). An active-tail eviction that is NOT immediately
  idle moves to `retired` instead of being dropped; a new
  hz10_class_pages_sweep_retired() re-drains and re-checks up to
  HZ10_CLASS_PAGES_SWEEP_BUDGET=4 pages from retired's tail per call --
  called ONLY from hz10_public_entry.c's slow path (right after
  hz10_class_pages_find_with_capacity() fails and before a fresh page
  would be created -- a page reclaimed right there immediately feeds
  that same call's pool-acquire-first check), never the hot per-op alloc
  path. `retired` overflowing forces one final drain+check on its own
  tail before truly giving up (dropped, same fate as before this design
  existed). Both sublists reuse Hz10FreelistPage's existing prev/next_
  in_owner_list fields via small shared splice helpers (a page is in at
  most one sublist at a time). Counters split by WHICH mechanism
  reclaimed a page (eviction-instant / retired-overflow / budgeted
  sweep) plus retired_count and max_retired_length (high-water mark)
  deliberately deferred to a later phase, not implemented now: promoting
  a retired page back to `active` when a sweep drain finds it PARTIALLY
  (not fully) idle -- would also help ops/s, not just RSS, but risks
  active/retired thrashing if active is already full; correctly judged
  as needing destroy-only reclaim measured on its own first
  hz10_public_entry_class_list_stats() reshaped from four separate out-
  parameters into one Hz10ClassPageListStats out-struct (matches H10
  RouteResult's existing plain-struct-return pattern) since there were
  now nine fields worth reporting, not three
  tests/hz10_class_pages_smoke.c grew from 3 to 5 cases: the two new
  ones are the actual regression tests for this design -- a page that
  becomes idle AFTER entering retired (a late free, standing in for an
  async remote free) is reclaimed by the next sweep_retired() call (the
  old design could never catch this by construction); retired overflow
  drops its own oldest tail after one final failed check, staying
  bounded and reported via retired_dropped_count
  measured against the real workloads this was built for (THREADS=4
  ITERS=500000 RUNS=10, HZ10_DUMP_CLASS_STATS=1) -- decisive, not
  hopeful:
    slot_count=1 (the row this whole investigation started from): a
      real, nonzero improvement -- retired_reclaimed_by_sweep went from
      0 (old design, confirmed 0/0 across 20 runs) to 2-277 per run
      here. But eviction_count is 5,942-21,187 per run and
      retired_dropped_count is 5,556-19,638 per run in the SAME runs --
      the sweep is reclaiming roughly 1-2% of what gets dropped, not
      enough to matter. post_rss_kb across 10 runs in one process still
      climbed 75,904 -> 998,824 KB, essentially the same shape as before
      this design existed
    main_r50 (multi-slot classes, the row this was also meant to help):
      retired_reclaimed_by_sweep was EXACTLY 0 across all 10 runs.
      post_rss_kb still climbed 53,504 -> 830,056 KB across the same 10
      runs, again essentially unchanged
  root cause of the small effect, understood via a follow-up diagnostic,
  not guessed: raised HZ10_CLASS_PAGES_SWEEP_BUDGET to 16 and
  HZ10_CLASS_PAGES_RETIRED_LIMIT to 512 (4x both, as a probe, then
  reverted -- not kept) and re-measured slot_count=1: reclaim rate
  barely moved (525/12,283 evictions in the best run, ~4.3%, still
  dominated by drops). This rules out "just needs bigger constants" --
  the mechanism is structurally limited, not under-tuned. The real
  reason: idle requires free_count == slot_count, i.e. EVERY slot this
  page ever handed out has come back. For slot_count=1 that is one
  free -- readily achievable, if often just late (this is exactly the
  case the two-list design measurably helps, per the nonzero sweep
  counts above). For a multi-slot class (main_r50/r90's normal range,
  pages holding hundreds to thousands of slots), a page reaching find_
  with_capacity's scan already means it was JUST confirmed to have zero
  local AND zero remote capacity moments before eviction -- so at
  eviction time it typically still has many (not one) outstanding slots,
  and under continuous churn there is no guarantee all of them are ever
  freed within any bounded window, let alone within HZ10_CLASS_PAGES_
  RETIRED_LIMIT more eviction cycles. "Wait for 100% of a page's slots
  to come back" is a fundamentally harder target for high-slot-count
  classes than for low-slot-count ones -- not a timing problem sweep can
  fix by trying more often, a structural one
  conclusion: kept this design (it is correct, real, and does help
  slot_count=1 by a small, honestly-measured amount, plus it closes Box
  6's list-length-unbounded-growth gap the same way the earlier one-list
  version did) but does NOT claim it fixes the RSS gate. The real
  RSS problem for multi-slot classes remains open, and per the analysis
  above, likely needs an entirely different strategy than idle-page
  reclaim -- idle may just not be the right bar to wait for when a page
  holds hundreds of slots under sustained churn
  re-verified: all smokes (5 cases in hz10_class_pages_smoke.c, ASan/
  UBSan with ASAN_OPTIONS=detect_leaks=0 -- one of the 5 cases
  deliberately leaks a dropped page, same documented category as the
  existing TLS-resident-page noise), TSan (ASLR off),
  hz10-standalone-check, and the never-free regression bench
  (bench/hz10_class_pages_scan_bench.c, confirms the extra bounded sweep
  cost per slow-path call does not reintroduce O(n^2): last_over_first
  stayed ~1.03-1.15 across repeated runs) all green
  files: src/hz10_class_pages.{h,c}, src/hz10_public_entry.{h,c},
  tests/hz10_class_pages_smoke.c, bench/hz10_public_entry_bench.c

retired cursor fix done (src/hz10_class_pages.{h,c}, src/
hz10_public_entry.{h,c}, tests/hz10_class_pages_smoke.c, bench/
hz10_public_entry_bench.c): external review (a second reviewer, after
reading bench/hz10_public_entry_bench.c's actual loop body) identified a
sharper, more mechanistically precise critique of the active/retired
design above than the "multi-slot pages structurally can't reach idle"
conclusion this session had settled on. Result: BOTH turned out to be
real, each explaining a different row -- reported honestly, not spun
either direction:
  the critique: this bench has zero live set -- every allocated object
  IS eventually freed (locally or via the inbox simulation), so every
  page, if fully drained, WOULD become idle. Two concrete mechanisms
  were named: (1) a remote free only pushes onto the owning page's
  remote stack; nothing updates free_count until the OWNER thread later
  calls hz10_page_drain_remote() on that specific page, so a caller-side
  "free" completing is not the same event as this module observing it --
  real, sometimes long, delay between the two; (2) hz10_class_pages_
  sweep_retired() always restarted its budgeted walk from retired's
  CURRENT tail -- if the tail-most entries are slow to resolve, the tail
  barely advances, so newer entries arriving at head are never reached
  at all (classic head-of-line blocking), regardless of how many times
  sweep is called. Proposed fix: stop dropping retired's overflow tail
  (let retired grow unbounded -- it is never linearly scanned by
  hz10_malloc, only budget-swept, so this does not reintroduce the
  O(n^2) cost SCAN_LIMIT prevents on `active`) and replace the
  tail-restarting sweep with a persistent, resumable cursor that rotates
  through all of retired over time
  implemented as proposed: removed HZ10_CLASS_PAGES_RETIRED_LIMIT and
  its overflow-eviction/drop path entirely (retired_reclaimed_by_
  overflow_count and retired_dropped_count counters removed with it --
  nothing to count anymore). Added retired_sweep_cursor to
  Hz10ClassPageList: hz10_class_pages_sweep_retired() resumes from
  wherever the last call left off (wrapping back to retired's tail once
  a walk reaches head) instead of always restarting from tail
  verified the cursor itself is correct, isolated from any bench timing,
  before trusting it in the real workload (this project's own hard-won
  lesson about RSS deltas without direct verification, cited by the
  reviewer): built 1000 retired pages directly via hz10_class_pages_add,
  marked exactly 500 of them idle, then called sweep_retired() enough
  times to cover several full rotations (budget=4, so ~1250 calls for
  ~5 rotations of 1000 entries) -- all 500 idle ones were reclaimed,
  the other 500 were not, confirming the cursor rotates correctly and
  does not false-positive. This ruled out an implementation bug as the
  explanation for whatever the real-workload measurement showed next
  measured against the same real workloads, not assumed to have
  improved just because the mechanism is now correct:
    slot_count=1: a real but modest improvement over the tail-restarting
      version -- retired_reclaimed_by_sweep per run went from single
      digits (0-277, prior measurement) to a similar-to-somewhat-higher
      range (3-218 across 10 fresh runs), not the order-of-magnitude
      jump the mechanism's fix would suggest if head-of-line blocking
      were the whole story for this row. post_rss_kb still climbed
      51,968 -> 803,896 KB across 10 runs -- still not bounded
    main_r50: retired_reclaimed_by_sweep was EXACTLY 0 across all 10
      runs, same as before this fix, with retired.length growing into
      the thousands per thread within single runs (up to ~1803 pages)
      and never once reclaiming any of them via sweep. This is now
      doubly confirmed to not be a traversal-order bug (fixed, and
      proven correct in isolation above) or a premature-drop artifact
      (removed entirely, retired is unbounded) -- something about this
      row's pages genuinely does not reach free_count == slot_count
      within the sweep's reach, at all, in this session's testing
  conclusion, holding both findings at once rather than picking one:
  the reviewer's diagnosis was correct and the fix is real -- the sweep
  mechanism itself was measurably broken (proven via the isolated test)
  and is now fixed, and slot_count=1 (single-slot pages, where the
  reviewer's "just a delayed free" framing applies cleanly) shows a
  real, if smaller-than-hoped, improvement from it. But main_r50/r90's
  exact-zero result even under the fully corrected mechanism reinforces,
  rather than replaces, this session's earlier "many-slots-per-page"
  hypothesis: for those classes specifically, something beyond sweep
  traversal order or premature dropping is preventing free_count from
  ever reaching slot_count within a run's timeframe. Whether that is the
  bench's own inbox-drain-lag mechanism compounding at higher slot
  counts, or a genuinely different effect, is not resolved here -- flagged
  for whoever continues this, with the isolated-cursor-test methodology
  above as a template for verifying any next hypothesis before trusting
  a real-workload number
  honest safety note on unbounding retired: checked (not assumed) that
  this does not introduce new unbounded-memory risk in the adversarial
  never-free bench (bench/hz10_class_pages_scan_bench.c, 20M never-freed
  allocations): peak RSS ~1.26GB, reasoned through and consistent with
  what the OLD bounded+drop design would ALSO have resident in this
  exact scenario -- a genuinely-never-freed page's quantum stays mapped
  either way (tracked-but-unreachable in a growing retired list, or
  silently dropped-but-still-mapped), so unbounding retired changes
  bookkeeping reachability, not actual memory footprint, for this case
  re-verified: all 5 smokes (case 5 rewritten from "retired overflow
  drops oldest" to "sweep cursor rotates past a chronically stuck tail
  and reclaims something that became idle closer to head" -- the actual
  regression test for this fix), ASan/UBSan (no leak noise this time --
  nothing gets deliberately dropped anymore, so the smoke's teardown
  reaches everything), TSan (ASLR off), hz10-standalone-check, and the
  never-free regression bench (last_over_first ~1.05, flat) all green
  files: src/hz10_class_pages.{h,c}, src/hz10_public_entry.{h,c},
  tests/hz10_class_pages_smoke.c, bench/hz10_public_entry_bench.c

next (re-prioritized from this table, see "Next HZ10 action" above):
  (1) DONE (see above) -- perf stat + strace root-cause pass on
      main_r50/main_r90: fixed two real over-scoped locks, ratio did not
      move outside noise, real cost still unattributed past the
      still-open RMW hypothesis above
  (2) DONE, then CORRECTED (see above) -- wired Box 6's per-class page
      list to Box 4's pool. Initially reported as a real RSS win for
      slot_count=1; that comparison turned out to be methodologically
      flawed (6 separate processes vs. one 10-run process) and did NOT
      reproduce under matched methodology. No ops/s change either way
      (ratio stays ~0.48, corrected baseline)
  (3) DONE, then SUPERSEDED (see "class-list diagnostic counters" above)
      -- revisited RSS growth using real counters instead of inference:
      eviction fires constantly in every row (main_r50/r90 included, not
      just slot_count=1), but reclaim essentially never fires anywhere
      (~0% in all rows tested) -- the RSS problem is open in every row,
      not fixed in one and merely absent in others as previously believed
      (few distinct pages per class there, eviction rarely triggers)
  (3b) DONE, real but small (see "active/retired two-list reclaim"
      above) -- implemented the active/retired redesign this section's
      own analysis pointed at. Correct, verified, and a genuine (if
      ~1-2%) reclaim-rate improvement for slot_count=1; exactly 0 for
      main_r50/r90. RSS growth is essentially unchanged in both rows
      even at 4x the budget/limit constants -- ruled out as a tuning
      problem
  (3c) DONE, refines (3b) rather than replacing it (see "retired cursor
      fix" above) -- external review found and fixed a real bug (sweep
      always restarted from retired's tail, causing head-of-line
      blocking) and a real design flaw (retired's own overflow-drop),
      both verified correct via an isolated test before trusting real-
      workload numbers again. slot_count=1 improved somewhat further but
      still not by an order of magnitude; main_r50 stayed at EXACTLY 0
      reclaims even with both fixed. This doubly confirms (3b)'s
      "multi-slot classes may not reach 100% idle" hypothesis for
      main_r50/r90 specifically, while also confirming the reviewer's
      "sweep traversal was broken" diagnosis was independently real for
      slot_count=1 -- both true, for different rows, not a single cause
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
  run, which was assumed at the time to be because main_r50/r90's classes
  rarely reach HZ10_CLASS_PAGES_SCAN_LIMIT. CORRECTION (see "class-list
  diagnostic counters" above): that assumption was wrong -- real counters
  later showed eviction fires constantly in main_r50/r90 too (hundreds to
  thousands of times per run), just like slot_count=1. The real cause is
  that reclaim (not eviction) almost never fires in ANY row, because the
  one-shot idle check at eviction time structurally cannot catch a page
  whose last slot is freed by another thread moments too late -- see that
  section for the full, corrected root cause
  honest scope note: this is ONE comparison point (THREADS=4,
  ITERS=500000x10, this machine, HZ8's current default build). Not
  re-verified against a from-scratch HZ8 rebuild or a different thread
  count; treat as a real first data point for a bar that had never been
  measured before, not a final verdict
  next concrete step this points to (SUPERSEDED, see "class-list
  diagnostic counters" above for the corrected version): closing the RSS
  gate for main_r50/r90 does NOT just need the existing Box 6/Box 4
  reclaim mechanism reached more often -- that mechanism barely reclaims
  anything even in the row it was built for. A real fix needs a
  different trigger than a point-in-time idle check at eviction (a
  deferred/retry path for dropped-non-idle pages, or a different reclaim
  design entirely)
  files: scripts/run_hz10_public_entry_vs_hz8_same_run.sh (new),
  bench/lib/hz10_bench_common.sh

RetiredPartialReuse-L1 implemented and measured -- mechanism proven
correct, real-workload effect on main_r50/r90 is exactly zero, and the
reason why is now understood mechanistically instead of just observed
(src/hz10_class_pages.{h,c}, src/hz10_public_entry.{h,c},
tests/hz10_class_pages_smoke.c, bench/hz10_public_entry_bench.c):
  design (external review, ChatGPT Pro, after being shown the "retired
  cursor fix" section above and asked for a design given "main_r50/r90's
  multi-slot pages doubly confirmed to never reach free_count ==
  slot_count"): stop requiring 100% idle before a retired page can be
  reused. hz10_class_pages_sweep_retired() (void, destroy-only) replaced
  with hz10_class_pages_harvest_retired_capacity() (returns
  Hz10FreelistPage*): same budgeted, cursor-resumed walk as before, but
  now checks free_count==slot_count (destroy, as before) OR
  local_free_head != NULL (PARTIAL capacity -- promote straight back into
  `active` via the same prepend-with-eviction path hz10_class_pages_add()
  uses, and return this page directly so the caller can allocate from it
  immediately instead of paying for a fresh page). Idle takes priority
  over partial when both are true. New counter
  retired_promoted_by_sweep_count alongside the existing
  retired_reclaimed_by_sweep_count, split by which of the two happened
  verified the technical premise directly from source before implementing
  (this session's established discipline): hz10_page_drain_remote()
  (src/hz10_remote_stack.c) merges into local_free_head and increments
  free_count on ANY drain, not just one that reaches 100%; hz10_freelist_
  page_alloc() pops from local_free_head unconditionally -- so a
  "partial" page is genuinely, correctly allocatable, not a theoretical
  possibility
  smoke test rework required before trusting the new logic (caught by
  re-reading the existing fixtures against the new promote condition, not
  by a failing test): the existing "busy filler" helper allocated only 1
  of 16 slots per filler page, leaving the other 15 sitting on
  local_free_head from creation -- already "partial" by the new
  definition, silently breaking cases 3-5's "this filler must never be
  reclaimed/promoted" assumption. Fixed by fully exhausting every busy
  filler/target/stuck page (all slots allocated, zero capacity, not just
  fewer) before eviction in every case that needs a genuinely
  zero-capacity page, and adding a new case 6 that directly exercises the
  promote path (create, fully exhaust, evict to retired, free 5 of 16,
  harvest, assert: returned page is the candidate, now in `active`, not
  `retired`, retired_promoted_by_sweep_count==1, and is actually
  allocatable afterward)
  new diagnostic-only counter added while measuring, not part of the
  original design: harvest_call_count (Hz10ClassPageList, wired through
  hz10_public_entry_class_list_stats() and the bench's stats dump) --
  needed to distinguish "harvest is never reached for this workload
  shape" from "harvest runs constantly but finds nothing in its budget,"
  which look identical if you only have the destroy/promote counters
  measured against main_r50, main_r90, and slot_count=1 (THREADS=4
  ITERS=500000 RUNS=10 MODE=0, the same locked-in methodology as every
  other row in this file):
    main_r50: retired_reclaimed_by_sweep and retired_promoted_by_sweep
      BOTH exactly 0 across all 10 runs -- but harvest_call_count is
      1,011-3,918 per run, proving harvest IS being reached constantly
      (this sharpens, not just repeats, the earlier "doubly confirmed
      zero" finding: it is not a reachability problem). post_rss_kb still
      climbs monotonically, 28,928 -> 638,096 KB across the 10 runs
    main_r90: same pattern -- 0 and 0 across all 10 runs,
      harvest_call_count 1,931-7,725 per run, post_rss_kb climbing
      185,984 -> 1,120,440 KB
    slot_count=1: retired_reclaimed_by_sweep small but nonzero (3-69 per
      run vs 5,000-21,000+ evictions), consistent with the prior cursor-
      fix measurement; retired_promoted_by_sweep is EXACTLY 0 in every
      run -- expected and structurally correct, not a bug: a 1-slot page
      can only ever be fully idle (1/1 free) or fully exhausted (0/1
      free), never "partial" by definition. post_rss_kb still climbs
      monotonically, 49,664 -> 935,540 KB
  root-caused the exact-zero result before accepting it (this session's
  "measure, don't assume" rule applied one level deeper than usual, since
  the counters alone could not distinguish a bug from a genuine rate
  problem): added a temporary HZ10_DEBUG_HARVEST_PRINT build flag (not
  committed -- reverted after use) to print every node harvest actually
  inspected. First verified the drain+classify mechanism itself in
  complete isolation (a standalone 2-slot page, both slots remote-freed
  from a real second pthread, then drained) -- worked correctly on the
  first try, free_count went 0 -> 2 as expected, ruling out a mechanism
  bug. Then found, from the debug prints against the real bench, that
  main_r50/r90 (MIN_SIZE=16 MAX_SIZE=32768, sizes drawn UNIFORMLY BY BYTE
  VALUE) spend the large majority of allocations in the class nearest
  MAX_SIZE, because most integers in a uniform [16,32768] range are large
  -- for size=32768 specifically that is class 21, slot_count=2
  (65536/32768). Every single node harvest inspected in a THREADS=4
  ITERS=500000 REMOTE_PCT=50 run (2,548-4,852 checks logged) showed
  free_count=0, slot_count=2, remote_invalid_count=0, remote_duplicate_
  count=0 -- not rejected, simply never yet resolved. Re-ran the same
  single-class shape (MIN_SIZE=MAX_SIZE=32768) at 3,000,000 iters/thread
  (6x the locked-in budget) and DID see resolution: 84 destroys + 14
  promotes out of 40,830 checks (~0.24%) -- proving the mechanism does
  eventually work given enough time, just far too slowly to register at
  all within the established 500,000-iteration budget
  conclusion: RetiredPartialReuse-L1 is correctly implemented (unit-
  tested, isolated-mechanism-tested, and confirmed to fire given enough
  iterations) but provides no measurable benefit for main_r50/main_r90 at
  this project's established benchmark scale. This is not a traversal bug
  or a premature-drop artifact (both already ruled out in the cursor-fix
  entry above) and not a reachability problem either (harvest_call_count
  proves it runs constantly) -- it is a rate mismatch: the specific pages
  that reach `retired` are, by the very selection mechanism of eviction
  itself, precisely the ones whose outstanding objects are ALL still
  in-flight via cross-thread remote-free channels (a page that recovers
  even one slot locally gets reused immediately and rarely accumulates
  enough active-list pressure to be evicted at all), and resolving that
  requires both another thread's own periodic drain AND this thread's
  cursor revisiting that exact page again -- a combination that this
  measurement shows resolves at roughly 0.24% per check even after 6x the
  standard iteration budget. Waiting for partial capacity is a strictly
  weaker bar than waiting for full idle, and unambiguously fixes the
  design flaw the two external reviews correctly identified, but "weaker
  bar" and "reachable within this benchmark's timeframe" are different
  properties -- this measurement shows the row this was aimed at needs
  the latter, which this fix alone does not supply
  next concrete step for whoever continues this: the RSS problem for
  main_r50/r90 needs a trigger that does not depend on this specific
  thread revisiting this specific page fast enough relative to remote-
  free traffic -- e.g. reclaiming/decommitting at the OS-page (not
  Hz10FreelistPage-quantum) granularity so a page's memory can be
  returned even while some of its slots are still logically outstanding,
  or a fundamentally different ownership/recovery model for the remote-
  heavy multi-slot case. A larger HZ10_CLASS_PAGES_SWEEP_BUDGET was not
  tried this round (the 6x-iteration test above suggests the bottleneck
  is call COUNT over a long enough timeframe, not budget-per-call, but
  this is inference, not measurement -- verify before assuming)
  re-verified: all 6 smokes (case 6 new, cases 3-5 reworked for full
  slot exhaustion), ASan/UBSan (ASAN_OPTIONS=detect_leaks=0, both the
  smokes and bench/hz10_public_entry_bench.c under main_r50-shaped and
  slot_count=1-shaped real workloads), TSan (ASLR off via
  `setarch $(uname -m) -R`, same two workload shapes), hz10-standalone-
  check, and the never-free regression bench (last_over_first ~0.90-1.04
  across reruns, flat) all green
  files: src/hz10_class_pages.{h,c}, src/hz10_public_entry.{h,c},
  tests/hz10_class_pages_smoke.c, bench/hz10_public_entry_bench.c

polling-vs-event-driven diagnosis + design decision (external review,
Claude, given the RetiredPartialReuse-L1 measurement above): sharper
framing of the same root cause, confirmed against existing counters
without needing a new measurement, plus a decision to try a structurally
different reclaim trigger:
  diagnosis: hz10_malloc's slow path only calls harvest_retired_capacity()
  when find_with_capacity() returns NULL across the whole `active` scan
  window (src/hz10_public_entry.c) -- so harvest's outflow rate is
  (frequency active goes fully dry) x SWEEP_BUDGET=4, while retired's
  inflow rate (evictions) tracks total churn directly, with no shared
  ceiling between the two. Confirmed, not re-measured, from data already
  in the entry above: for every main_r50/r90 run, retired_count ==
  retired_length exactly (e.g. run3: 2041 == 2041) -- 100% of everything
  that ever entered `retired` was still sitting there, unmodified, at
  the end of the run. Outflow was not just slow, it was zero, for the
  entire run's duration, in every run measured. This is the same
  underlying fact as the "0.24% resolution rate" finding above, restated
  as a supply/demand mismatch rather than a timing-probability one -- not
  a new discovery, but a clarifying one: it rules out "the budget or
  sweep frequency just needs tuning higher" as a fix, since inflow has no
  ceiling that any fixed per-call budget can chase
  rejected direction: decommitting at OS-page (not Hz10FreelistPage-
  quantum) granularity instead of changing the trigger. Reasoning agreed
  with: destroy, decommit, and pool-return all still gate on the SAME
  fact (an owning thread observing a page's outstanding count reach
  zero) -- changing how memory is given back does not change WHEN the
  owner finds out it can be given back. The actual failure is in the
  discovery mechanism (owner-polls-eventually), not the reclaim
  mechanism (destroy-vs-decommit), so this does not address the root
  cause. Also independently a poor fit for this project's design: the
  intrusive freelist stores its next-pointer inside otherwise-free slots,
  so decommitting individual slots within a page would need to relocate
  that bookkeeping first
  accepted direction: event-driven reclaim instead of owner-polling.
  Structural argument for why this closes the gap that no amount of
  budget/frequency tuning can: today, ONLY the owning thread's own
  malloc-driven scan can ever discover that a retired page became usable
  again, and that scan's frequency is entirely decoupled from how fast
  pages actually become idle/partial. An event-driven design instead has
  whichever thread frees the LAST outstanding slot of a retired page
  self-register that page onto a lock-free "ready" queue at the moment
  it happens -- discovery latency becomes O(1) relative to churn instead
  of dependent on how often the owner happens to look, closing the
  supply/demand gap by construction rather than by degree
  design sketch, not yet built: a new atomic `outstanding` count per
  page, set to slot_count - free_count only at the moment a page is
  retired (active pages keep today's plain free_count path untouched,
  so the hot local-alloc/local-free path pays nothing new). A remote
  free against a page flagged `retired` atomically decrements
  `outstanding`; whichever thread's decrement reaches 0 pushes the page
  onto a new lock-free stack (same Treiber-stack shape as Box 3's
  remote_free_head, but keyed by page references, not individual slots).
  The owning thread drains that stack on its own next slow-path call --
  O(1) relative to queue depth, same as remote_free_head's existing
  drain. Open design questions, unresolved: only a REMOTE free can use
  the ready-queue push (a foreign thread cannot safely unlink from
  `list->retired`'s doubly-linked sublist itself -- that mutation stays
  owner-only, per the existing threading model); an owner-thread local
  free of its own retired page's last slot can instead react inline,
  without needing the queue at all. Cost to the existing remote-free hot
  path (src/hz10_remote_stack.c's hz10_page_remote_free(), already 2
  atomics per call: pending-bit fetch_or + Treiber CAS) needs to be
  measured, not assumed, once built -- expected to be a flag check plus
  one conditional atomic decrement, opt-in only for pages already
  flagged retired, but "expected" is exactly the kind of claim this
  project does not accept without a bench number
  plan agreed: do not build this against main_r50/r90 directly first.
  Build an isolated prototype (single class or slot_count=1, matching
  this session's own precedent of proving the cursor mechanism correct
  in isolation before trusting it against a real workload) and confirm
  RSS actually bounds there before wiring it into the real multi-class
  public entry path. Not yet started as of this entry.

HZ10RetiredReadyQueue-L0 prototype built and measured in isolation --
hypothesis confirmed decisively, plus one real (pre-existing, documented)
correctness gap found and worked around at the bench level, not in
production code (src/hz10_retired_ready.{h,c} (new), src/
hz10_freelist_page.h, tests/hz10_retired_ready_smoke.c (new), bench/
hz10_retired_ready_bench.c (new)):
  design landed as a HINT layer, not a harvest replacement (a deliberate,
  load-bearing scope decision, not a shortcut): making the event-driven
  outstanding count itself provably race-free against the exact instant
  a page transitions from Box 3's push+drain tracking to this counter
  would need real synchronization machinery (something like a
  quiescence/epoch scheme) to rule out a double-count -- the one failure
  mode that would be genuinely unsafe (a page's count reaching zero
  early, before every slot is actually back, risks treating a still-live
  slot as reclaimable). Rather than build that, this module never lets
  its own count directly authorize a destroy: hz10_retired_ready_pop()
  only ever returns a CANDIDATE, and the caller is required to re-run
  the exact same free_count == slot_count check harvest already uses
  before trusting it. A false positive (raced, not actually fully idle
  yet) just fails that re-check and is left for harvest's own cursor to
  find later -- a wasted queue entry, not a correctness bug. A false
  negative (resolved but never pushed) likewise just falls back to
  harvest eventually finding it. Since nothing downstream trusts this
  module's count on its own, only its effect on DISCOVERY LATENCY needed
  measuring, not its exactness
  mechanism: Hz10FreelistPage gained retired_ready_flag/_outstanding/
  _next/_stack (zero cost for a page never retired this way -- Box 2/3's
  existing alloc/free/drain paths never read or write them).
  hz10_retired_ready_mark(page, stack, outstanding) is owner-only, called
  once at retirement. hz10_retired_ready_note_remote_free(page) is safe
  from any thread, meant to be called immediately after a
  hz10_page_remote_free() call returns 1 on the same page -- pairing
  matters, since Box 3 already guarantees at most one success per slot
  ever, so this never double-counts a single slot against itself.
  Whichever call's atomic decrement reaches what it believes is zero
  pushes the page onto a Treiber stack (same shape as Box 3's
  remote_free_head) for the owner to pop
  correctness verified before measuring performance (this session's
  standing rule): tests/hz10_retired_ready_smoke.c -- single-page
  countdown fires exactly on the Nth note, an unmarked page is inert,
  two pages on one stack resolve independently and pop LIFO, and a
  64-page/4-thread concurrent-note stress case confirms no duplicate
  pops and no lost pages under real concurrent pressure. ASan/UBSan and
  TSan (ASLR off) both clean
  bench/hz10_retired_ready_bench.c: two independent page sets (slot_
  count=2, matching main_r50's dominant pathological class -- see
  the "RetiredPartialReuse-L1" entry above for why size=32768 speci-
  fically), one driven by a miniature reimplementation of harvest's
  budgeted cursor walk, one by this module, both aged by the same
  producer threads under identical conditions in the same process.
  Measured (PAGES_PER_SET=50000, POLL_BUDGET=4, matching HZ10_CLASS_
  PAGES_SWEEP_BUDGET): polling needed 12,500 owner check-ins to reach
  zero unresolved (exactly pages_per_set / budget, a hard floor from
  budget-limited scanning); event-driven needed 1. Reproduced at
  PAGES_PER_SET=200,000 (polling: 50,000 check-ins, event-driven: 1) and
  with 8 producer threads instead of 4 (no change to either) -- polling's
  cost scales linearly with population regardless of producer count or
  total population, event-driven's does not scale with population at
  all, exactly as the diagnosis above predicted
  a real bug found and fixed, but in the BENCH, not in Box 2/3 (important
  to state precisely): TSan caught a genuine data race the first time
  this bench ran unguarded -- the owner destroying a page while a
  producer thread was still inside its own hz10_page_remote_free() call
  for that same page's last slot (a use-after-free of the page struct's
  own remote_push_count field). This is NOT a new bug: tests/README.md
  already documents this exact gap ("a foreign thread starting a new
  push concurrently with destroy is not defended against either way").
  It had simply never been exercised this aggressively before -- every
  existing test/bench checks in far less often than this bench's
  unthrottled busy-loop check-in does, so the nanosecond-wide window
  between a push's CAS landing and hz10_page_remote_free()'s own
  trailing bookkeeping finishing had never been hit. Fixed at the bench
  level (not by touching src/hz10_remote_stack.c or hz10_freelist_page.c
  -- out of scope for this prototype and not owned by this change): each
  page's producer thread sets a per-page "producer done" flag (release)
  only after its LAST hz10_page_remote_free() call for that page has
  fully returned, and both checkers require that flag before trusting
  free_count == slot_count. Re-verified clean under ASan/UBSan and TSan
  (ASLR off) after the fix -- TSan needed a smaller PAGES_PER_SET
  (2,000 instead of 50,000) to avoid its own shadow-memory/mmap-tracking
  resource limit unrelated to correctness (a `ThreadSanitizer failed to
  deallocate` internal-allocator error, not a reported race)
  conclusion: the isolated hypothesis is confirmed, decisively and
  reproducibly -- an event-driven ready queue eliminates the search-cost
  floor that makes polling's outflow scale with retired population
  instead of staying flat. This is a strong enough result to justify
  wiring it into the real multi-class public entry path next, per the
  agreed plan, rather than a reason to skip straight there
  next concrete step: wire hz10_retired_ready into src/hz10_class_pages.c
  /hz10_public_entry.c for real -- hz10_class_pages_add()'s eviction path
  calls hz10_retired_ready_mark() instead of (or alongside) moving a
  non-idle page to plain `retired`; hz10_free()'s remote-free dispatch
  calls note_remote_free() after a successful hz10_page_remote_free();
  hz10_malloc's slow path pops from the ready stack before falling back
  to harvest's cursor walk. Measure against main_r50, main_r90, and
  slot_count=1 with the same locked-in methodology as every other row in
  this file once wired -- this isolated bench's dramatic gap is
  motivation to build the integration, not a substitute for measuring it
  files: src/hz10_retired_ready.{h,c} (new), src/hz10_freelist_page.h,
  tests/hz10_retired_ready_smoke.c (new), bench/hz10_retired_ready_bench.c
  (new)

HZ10RetiredReadyQueue-L0 wired into the real path -- two real bugs found
and fixed via a new opt-in steady-state bench, and a major reframing of
the whole main_r50/r90 RSS investigation (src/hz10_class_pages.{h,c},
src/hz10_public_entry.{h,c}, src/hz10_retired_ready.{h,c}, src/
hz10_freelist_page.h, bench/hz10_public_entry_bench.c, bench/
hz10_public_entry_steady_state_bench.c (new)):
  wiring: hz10_class_pages_add()'s eviction path calls hz10_retired_
  ready_mark() (outstanding = slot_count - free_count) alongside moving a
  non-idle evicted page to `retired`, unchanged otherwise. hz10_free()'s
  remote-free branch calls hz10_retired_ready_note_remote_free() after
  every accepted hz10_page_remote_free() (unconditional -- a cheap flag
  check, no-op for a page never retired this way). hz10_class_pages_
  harvest_retired_capacity() now drains list->ready to empty FIRST (each
  candidate re-verified against the same idle/partial check before being
  trusted, exactly as the isolated prototype's design specified), only
  falling through to the existing budgeted cursor walk if the ready
  queue yields nothing promotable. New diagnostic counters (retired_
  reclaimed/promoted_by_ready_count, ready_false_positive_count, and
  Hz10RetiredReadyStack's own push_count) threaded through Hz10ClassPage-
  ListStats and both public-entry benches, matching every prior
  diagnostic counter added this session
  claude君's new-bench design (relayed, agreed with, and built): a
  SEPARATE, opt-in bench (bench/hz10_public_entry_steady_state_bench.c),
  never named main_rNN so it can't be confused with the locked-in
  ITERS=500000 RUNS=10 table -- workers run for RUN_SECONDS of wall
  time (steady state) instead of a fixed iteration count, removing the
  old bench's "pthread_barrier_wait + two unconditional full inbox
  drains, only THEN collect stats" structure entirely. Stats are
  captured twice (the instant each worker's own time-bounded loop ends,
  before any flush, and again after the same barrier+flush the other
  bench always did) plus at N approximate mid-run checkpoints (each
  worker self-reports against its own elapsed-time clock -- no
  synchronizing barrier, since a barrier here would distort the very
  steady state being measured)
  bug #1, found immediately by this new bench under ASan (heap-use-
  after-free in hz10_class_pages_harvest_retired_capacity, reading
  node->prev_in_owner_list on a freed page): the ready-queue draining
  loop's hz10_page_sublist_remove() calls did not know about list-
  >retired_sweep_cursor -- if the cursor (left over from a PRIOR
  harvest() call) happened to point at exactly the page the ready-queue
  loop destroyed or promoted, the very next budgeted-walk read of that
  stale cursor was a use-after-free. Fixed with a new hz10_class_pages_
  retired_remove() wrapper (redirects the cursor to the removed node's
  prev first, then does the plain unlink) used at all four `retired`
  removal sites, not just the new ones -- cheap and correct regardless
  of which mechanism is doing the removing
  bug #2, found next (same bench, longer run, same category of error):
  a page could be sitting on the ready stack (pushed, not yet popped)
  AND simultaneously still linked into `retired`'s doubly-linked list --
  the budgeted cursor walk, which has no idea a page might be ready-
  queue-tracked, could independently find it idle via its OWN free_
  count check and destroy it while hz10_retired_ready_pop() might still
  reach it via retired_ready_next, corrupting the stack. retired_ready_
  flag could not detect this: it is already cleared before the push (so
  it cannot double as "currently on the stack"). Fixed with a THIRD,
  separate field (retired_ready_on_stack, set right before the push,
  cleared right after a successful pop) that the budgeted walk now
  checks and skips over -- leaving any such node for the ready-queue
  path to finish instead of touching it itself. Both bugs are new,
  specific to this integration (not the pre-existing, already-documented
  "foreign push vs. destroy" gap tests/README.md accepts) -- re-verified
  clean under ASan/UBSan and TSan (ASLR off) at up to 30 seconds/265M
  ops after each fix, plus the full existing smoke suite and the never-
  free regression bench (last_over_first ~1.05-1.23 across rebuilds)
  the major finding, from the fixed bench, that reframes this whole
  investigation: main_r50/main_r90-shaped churn (MIN_SIZE=16 MAX_SIZE=
  32768, REMOTE_PCT=50/90), run as a genuine steady state instead of a
  bounded iteration count, converges to a small, STABLE working set
  (active_length_sum plateaus around 500-900 across 4 threads within a
  few seconds) and DOES NOT EVICT A SINGLE PAGE AGAIN afterward --
  retired_count stayed at EXACTLY 0 for the ENTIRE run in every trial
  (up to 265M ops over 20 seconds, both REMOTE_PCT=50 and 90). Once
  find_with_capacity()'s bounded scan has enough real churn to draw
  from, it keeps finding capacity within its own SCAN_LIMIT=128 window
  every time, so hz10_malloc's genuine-miss slow path -- the ONLY place
  eviction/retirement is ever triggered -- simply stops firing. Neither
  harvest's polling NOR this session's new ready queue can matter for a
  row that never evicts in the first place
  what this means for the "RSS climbs monotonically across RUNS=10"
  finding this whole investigation was chasing: that finding almost
  certainly was NOT observing genuine unbounded steady-state growth --
  it was observing bench/hz10_public_entry_bench.c's own RUNS=10
  structure spinning up a BRAND NEW set of 4 threads every run, each of
  which abandons its entire active+retired page set when its threads
  exit (HZ10 has no thread-exit/thread-lifecycle hook -- an already-
  documented, separate gap, see "abandoned/TLS-resident pages" in tests/
  README.md), accumulating across 10 runs. This is a real, still-open
  RSS problem, but a DIFFERENT one than RetiredPartialReuse-L1 or this
  ready-queue were ever built to address -- neither a smarter retired-
  list reclaim mechanism nor a faster discovery queue helps memory that
  is abandoned whole-thread, not stuck-page-by-stuck-page
  a genuine, measured win for the ready queue, on the row shape where it
  actually applies: slot_count=1 (MIN_SIZE=MAX_SIZE=65536, REMOTE_PCT=90)
  DOES keep evicting/retiring continuously even in steady state (a
  1-slot page's active-list churn does not stabilize the way multi-slot
  classes do), and here the ready queue delivers exactly the effect
  predicted: over a 30-second steady-state run, retired_reclaimed_by_
  ready climbed to 14,120 against retired_count=14,153 (~99.8% coverage)
  while retired_reclaimed_by_sweep (the old polling path) stayed at
  EXACTLY 0 the entire time -- the unresolved backlog (retired_length)
  fluctuated in a bounded ~20-300 range instead of growing without limit.
  ready_false_positive_count was 0 throughout every trial run (the race
  the design accepts as possible was not observed in practice at this
  scale, though it remains structurally possible and is not assumed
  impossible just because unseen)
  conclusion: the ready queue works exactly as designed and delivers a
  large, real win for the row shape that keeps evicting under steady
  state (slot_count=1). For main_r50/r90 specifically, it cannot matter
  because -- newly discovered here, not previously known -- steady-state
  churn for those classes does not keep evicting at all past an initial
  ramp-up, so this investigation's original framing ("main_r50/r90 RSS
  climbs monotonically, needs a better retired-page reclaim mechanism")
  was aimed at the wrong layer for that specific row. The real open
  problem for main_r50/r90 is thread-exit page abandonment, a Box-6-
  external, thread-lifecycle question neither RetiredPartialReuse-L1 nor
  this ready queue were positioned to solve
  next concrete step: (1) measure whether bench/hz10_public_entry_bench.c
  itself should change its RUNS=10 methodology (reuse the SAME threads
  across runs instead of spawning fresh ones each time) to separate "RSS
  growth within one thread population's lifetime" from "RSS growth from
  abandoning thread populations" -- these are two different bars, and
  the locked-in table's current RUNS=10 number conflates them; (2) if
  thread-exit abandonment is confirmed as the real main_r50/r90 driver,
  the fix is a thread-lifecycle/cleanup hook (HZ10 has none today, by
  design, see current_task.md's rules) -- a materially different, larger
  design question than anything this session has built so far, not to
  be started without discussing scope first
  files: src/hz10_class_pages.{h,c}, src/hz10_public_entry.{h,c}, src/
  hz10_retired_ready.{h,c}, src/hz10_freelist_page.h, bench/
  hz10_public_entry_bench.c, bench/hz10_public_entry_steady_state_bench.c
  (new), Makefile

Thread-reuse bench finds two more real bugs in HZ10RetiredReadyQueue-L0's
wiring (a genuine cross-thread race and a same-thread stale-tracking bug),
both fixed (src/hz10_remote_stack.{h,c}, src/hz10_public_entry.c, src/
hz10_retired_ready.{h,c}, src/hz10_class_pages.{h,c}, src/
hz10_freelist_page.h, bench/hz10_public_entry_thread_reuse_bench.c (new)):
  motivation: the previous entry's "RSS climbs monotonically across
  RUNS=10" reframing (fresh-threads-per-run artifact, not genuine
  unbounded growth) needed direct testing, not just inference -- built a
  new bench that reuses the SAME worker threads across "logical run"
  segments instead of spawning fresh ones each time (matching the
  locked-in bench's THREADS=4/ITERS=500000/RUNS=10 shape otherwise, no
  unconditional flush between segments, a barrier used only to keep
  printed per-segment stats clean, never gating the work loop itself)
  bug #3 (a genuine data race, not a hint-vs-authoritative gap): the OLD,
  unsplit hz10_page_remote_free() made a slot visible to the owner's
  drain (its Treiber-stack CAS) BEFORE returning, so hz10_retired_ready_
  note_remote_free() -- called after that function returned -- could
  still be reading/writing `page` while the owner concurrently observed
  the page as idle and destroyed it. TSan confirmed this immediately
  under sustained, continuously-checking-in load (never exercised long
  enough to hit by any earlier bench). Fixed by splitting hz10_page_
  remote_free() into hz10_page_remote_free_claim() + _publish() (src/
  hz10_remote_stack.{h,c}) and running note_remote_free() strictly
  BETWEEN them (src/hz10_public_entry.c's hz10_free()): a claimed-but-
  not-yet-published slot is unreachable from remote_free_head, so it
  cannot be merged, so free_count cannot reach slot_count, so the owner
  cannot destroy/recycle the page while a claim is outstanding. Also
  proves the design's original "could this double-count a slot" worry
  was never possible in the first place: a slot's note() call can only
  run while retired_ready_flag is already 1 (set by mark(), which already
  folded that slot's own prior resolution, if any, into the initial
  outstanding snapshot via drain) -- the initial snapshot and a later
  note() decrement are provably disjoint sets, never the same slot twice
  bug #4 (an ABA hazard bug #3's fix does not cover): even with claim/
  publish, a `ready` stack ENTRY can go stale between push and pop if the
  page is destroyed and its address recycled for an unrelated brand-new
  page in between (the budgeted cursor walk never has this problem --
  it only ever walks its own thread-local `retired` list, never a
  reference a foreign thread handed it). Fixed with retired_ready_
  generation (src/hz10_freelist_page.h, set at mark() time): the owner
  must compare a popped candidate's CURRENT page->generation against the
  value captured at mark() time before touching any other field (src/
  hz10_class_pages.c's ready-queue drain loop) -- a mismatch means drop
  the reference, do not process it
  bug #5 (the mirror image of #4, found via extensive debug-instrumented
  list-consistency assertions under the new bench, confirmed by an
  abort() showing a ready candidate no longer in list->retired's chain):
  no address recycling needed -- the SAME live page is the problem. The
  budgeted cursor walk decides to destroy/promote a retired page using
  only its own free_count/local_free_head check, with zero knowledge of
  whether hz10_retired_ready_mark() still considers that page tracked
  (flag == 1, outstanding > 0, not yet pushed). If the walk took the page
  anyway (e.g. promoting it into `active`), a LATER note_remote_free()
  call for one of that page's still-genuinely-outstanding slots would
  still see stale flag == 1, decrement/push a page no longer in `retired`
  at all, and corrupt list->retired's own linkage when that stale entry
  is later popped and unlinked. Closed with hz10_retired_ready_cancel()
  (src/hz10_retired_ready.{h,c}): the walk must win a CAS on flag's 1->0
  transition -- the SAME transition note_remote_free() itself performs
  when its decrement reaches zero -- before it is allowed to actually
  remove the page (both destroy and promote branches); note_remote_free()
  was changed from an unconditional store to the same CAS so exactly one
  side ever wins, and the loser backs off (a lost cancel() leaves the
  node in `retired` for the ready path to finish, counted in the new
  sweep_cancel_lost_race_count diagnostic -- expected rare, worth
  measuring rather than assuming; on_stack's existing skip check in the
  walk is a strict subset of what cancel() now covers and was kept as a
  cheap fast-path, not removed)
  reasoning explicitly ruled out a simpler fix: a plain (non-CAS) flag
  clear in cancel() was considered first and rejected -- it leaves a
  check-then-act window where a concurrent note_remote_free() that
  already read flag==1 could still push a page the walk is simultaneously
  taking out of `retired`, reopening bug #5 through a narrower door. The
  CAS makes the two paths genuinely mutually exclusive instead of merely
  making the window rare
  verification: full smoke suite, ASan/UBSan (detect_leaks=0), TSan
  (ASLR off via setarch -R), hz10-standalone-check, and the never-free
  regression bench (last_over_first=0.996) all clean after every fix.
  The new thread-reuse bench itself (previously crashing on a
  substantial fraction of runs) re-run 20x plain release, 20x with the
  debug list-consistency assertions still compiled in, 8x under ASan,
  and 5x under TSan (ASLR off) -- zero failures across all four. Re-
  measured main_r50/main_r90/main_local0/medium_local0 against tcmalloc
  with the locked-in THREADS=4/ITERS=500000/RUNS=10 methodology
  (ratio~0.50/0.46/0.50/0.52) and slot_count=1 against the steady-state
  bench (MIN_SIZE=MAX_SIZE=65536 REMOTE_PCT=90, 15s) -- reclaimed_by_
  ready still dominates over reclaimed_by_sweep and retired_length stays
  bounded, consistent with the pre-fix win, no regression on either axis
  files: src/hz10_remote_stack.{h,c}, src/hz10_public_entry.{h,c}, src/
  hz10_retired_ready.{h,c}, src/hz10_class_pages.{h,c}, src/
  hz10_freelist_page.h, bench/hz10_public_entry_thread_reuse_bench.c (new)

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
