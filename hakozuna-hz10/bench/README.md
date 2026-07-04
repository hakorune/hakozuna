# HZ10 Bench

Benchmarks must be public-shaped and honest:

```text
no fused-only promotion gates
no DCE-prone alloc/free loops
record THREADS / ITERS / RUNS
compare same-run HZ8 / HZ9 / HZ10 where possible
```

`HZ10ThreadLocalFreelistPage-L0` bench compares hz10_freelist_page against
plain libc malloc/free doing the identical fixed-size alloc/free/touch loop
(system malloc needs no external checkout, so this comparison is always
available; the opt-in HZ10_EXT_ROOT/HZ9 comparison from Box 1 remains the
path for a same-run HZ9 number). Covers both a LIFO (immediate alloc-then-free,
matching HZ9's "local0" bench pattern) and a non-LIFO (alloc a batch, free in
shuffled order) mode, since a LIFO-only bench cannot distinguish a real
freelist from a depth-1 last-pointer cache -- see current_task.md's box 2
note on HZ9 ProductEntry's free-cache mistake.

`HZ10RemoteStackDrain-L0` bench is genuinely multi-threaded (the first box
that is): THREADS producer threads concurrently remote-free disjoint slices
of a fully-allocated page, timed separately from the single owner-thread
drain that follows, repeated REPEAT cycles. A single-threaded `local_free`
row (Box 2's hz10_freelist_page_free, same total op count) runs alongside
in the same invocation as the "cost of staying local" reference point,
mirroring HZ8/HZ9's local-vs-remote-row comparisons.

`HZ10BoundedPagePool-L0` bench is the local/remote/RSS matrix: the same
create-alloc-free/remote-free-destroy page cycle is run with only the pool
cap changed (CAP vs. 0) so "does pooling help" isolates to that one
parameter, not a different code path (CAP=0 forces every release to really
munmap, i.e. what HZ10 would cost with no pool at all). Rows: `pooled_local`,
`unpooled_local`, `pooled_remote` (reusing Box 3's producer-thread remote
free), plus a `getrusage` `ru_maxrss` sample taken before/after the local
phases as the RSS half of the matrix, reported honestly rather than assumed.

Decommit/aging (hz10_page_pool_purge_idle, see current_task.md): filled
the pool to 1024 cached blocks (well above the real default cap of 64,
for a clean signal) and touched each block's first byte so it was
genuinely resident, then measured purge_idle(0) directly: ~3.8-4.4ms to
purge all 1024 (cheap, bounded by cap), and a real drop in *current* RSS
(via /proc/self/statm, not getrusage's ru_maxrss high-water mark, which
cannot show a decrease at all) from ~5.3MB to ~1.4-1.7MB -- confirms the
mechanism genuinely returns memory to the OS. Honestly scoped: this is
not yet exercised by hz10_public_entry_bench's rows, because Box 6
never calls hz10_pooled_page_destroy() today (a deliberate design
choice, not an oversight) -- see current_task.md for the open question
this leaves.

Multi-class public entry (src/hz10_public_entry.{h,c}) bench is the first
one shaped exactly like HZ8/HZ9/tcmalloc/mimalloc's medium_local0/
main_local0/medium_r50/main_r90 rows: a `MIN_SIZE..MAX_SIZE` random-size
alloc/touch/free loop across `THREADS` workers, `REMOTE_PCT` of frees handed
to a different thread's inbox instead of freed locally -- same row-naming
convention this whole project's benches already use (e.g. `MIN_SIZE=16
MAX_SIZE=32768 REMOTE_PCT=0` is a main_local0-style row, `REMOTE_PCT=90` a
main_r90-style one). `mech=system_malloc` is the always-available same-run
reference; a real HZ8/HZ9/tcmalloc/mimalloc row is `HZ10_EXT_ROOT` territory
(Box 1's same-run script pattern), not something this C binary links.

Two follow-up public-entry shapes exist for methodology checks:
`hz10_public_entry_steady_state_bench` is a true continuous, time-based
worker run with periodic allocator stats; use it to decide whether RSS or
retired-page growth exists during one live thread population. On the
20260705 main_r50/r90 recheck, retired_count stayed exactly 0 and RSS stayed
low, so the RUNS=10 fixed-iteration RSS climb is not currently treated as
proof of unbounded steady-state allocator growth.
`hz10_public_entry_thread_reuse_bench` intentionally keeps worker threads
alive across fixed segments, but segment barriers create remote-inbox
backlog; treat it as a boundary-stress diagnostic, not as the steady-state
RSS authority.

Measured findings, reported honestly rather than assumed:

```text
local rows (REMOTE_PCT=0): hz10 beats system_malloc, ~1.16x at
  main_local0-style (16-32768) and ~1.9x at medium_local0-style
  (4097-65536), THREADS=4 ITERS=50000. Against real tcmalloc (LD_PRELOAD
  swap of mech=system_malloc, no code changes; formalized in
  scripts/run_hz10_public_entry_vs_tcmalloc_same_run.sh, see below),
  hz10 local rows land at ~60-70% of tcmalloc at THREADS=4
  ITERS=500000-2000000 (main_local0 ~62-71%, medium_local0 ~47-67%
  across repeats) -- noticeably higher than an earlier, smaller-ITERS
  (50000) ad hoc reading of ~40-50%. Both readings are honest; the
  difference is real and understood: at ITERS=50000 hz10's one-time
  setup costs (first-touch pagemap leaf mmap, first page per class) are
  a larger fraction of the timed window than at 500K-2M, so the smaller
  run under-counts hz10's steady-state throughput. The 60-70% figure is
  the more representative one for sustained traffic; still inside the
  documented "good balanced target" band in
  docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md, closer to its top than
  first measured, not the (explicitly not-first-target) 70%+ bar.

remote rows (REMOTE_PCT=50/90) -- FIXED by src/hz10_class_pages.h (see
  current_task.md): previously hz10 was 15-17x SLOWER than system_malloc,
  root-caused to Box 5's abandon-on-exhaustion policy losing a page's
  capacity forever the moment it looked exhausted, even when a foreign
  remote free was sitting undrained in its Box 3 stack. After the fix
  (every page a thread ever created for a class is scanned, draining
  each candidate, before paying for a fresh one), THREADS=4 ITERS=500000
  MIN_SIZE=16 MAX_SIZE=32768:

    REMOTE_PCT=50: hz10 10.9M ops/s vs system_malloc 6.3M (hz10 ~1.7x
      FASTER, was slower before the fix)
    REMOTE_PCT=90: hz10 7.8M ops/s vs system_malloc 4.8M (hz10 ~1.5x
      FASTER)

  Against real tcmalloc (LD_PRELOAD, via
  scripts/run_hz10_public_entry_vs_tcmalloc_same_run.sh, repeated at
  THREADS=4 ITERS=1000000): main_r50 ~39-50% of tcmalloc, main_r90
  ~41-48% -- lower than the local rows' ~60-70% (see above), but still
  in the same "good balanced target" band, instead of being 15-17x off.
  The isolating fixed-size=65536/slot_count=1 case (the worst case
  identified during root-causing) went from 15-17x slower than
  system_malloc to only ~24% slower (4.6M vs 6.1M ops/s, REMOTE_PCT=90,
  THREADS=4) -- much closer, not yet fully closed. RSS is also lower
  than tcmalloc's on these rows (e.g. ~76MB vs ~129MB post_rss_kb at
  REMOTE_PCT=90), consistent with the RSS target.

same-run tcmalloc comparison script
(scripts/run_hz10_public_entry_vs_tcmalloc_same_run.sh): formalizes the
ad hoc LD_PRELOAD technique above into a real, repeatable script,
matching Box 1's scripts/run_hz10_pagemap_vs_hz9_same_run.sh pattern --
opt-in (skips the tcmalloc rows gracefully if no
libtcmalloc_minimal.so.4 is found; TCMALLOC_LIB overrides), always runs
hz10's own main_local0/main_r50/main_r90/medium_local0 rows, and prints
an average same-run ratio summary across the requested RUNS. `make -C hakozuna-hz10
bench-public-entry && hakozuna-hz10/scripts/run_hz10_public_entry_vs_tcmalloc_same_run.sh`
(or set THREADS/ITERS/RUNS) reproduces the numbers above.

  The remaining gap on the slot_count=1 isolation case, and the O(N^2)
  owner-drain duplicate-check cost noted in current_task.md's box 3
  section, are both still open -- this fix closed the dominant cost, not
  every cost.

drain-empty peek fast path (src/hz10_remote_stack.c, see current_task.md):
  investigating the slot_count=1 isolation gap above, instrumented
  hz10_class_pages_find_with_capacity and found each malloc call in that
  workload scans ~19 pages on average (max ~79, not unbounded), each
  paying a full atomic_exchange in hz10_page_drain_remote even when
  nothing is pending. Added a relaxed-load peek before the exchange.
  The full hz10_public_entry_bench isolating case is too noisy on this
  machine to isolate the effect (2-6x run-to-run variance, confirmed by
  re-running the unmodified baseline and seeing the same spread) -- a
  dedicated single-threaded microbenchmark (tight loop calling
  hz10_page_drain_remote on a page with nothing ever pushed) shows a
  clean, repeatable ~1.6x for that specific operation (520-530M calls/s
  baseline vs 830-844M calls/s with the peek), confirmed real. Whether
  this moves the noisy full-bench isolating-case number is not
  established either way -- reported honestly as "real at the
  micro level, not confirmed at the macro level" rather than assumed.

class_pages bounded scan (src/hz10_class_pages.{h,c}, bench/hz10_class_pages_scan_bench.c):
  investigated whether Box 6's per-class page list actually grows
  unbounded (item (1) from the box-6 follow-up list) with a targeted,
  realistic workload: a single thread doing nothing but hz10_malloc(64)
  in a loop, never freeing (a growing cache or long-lived arena, not an
  adversarial pattern). Confirmed real, not hypothetical: before a scan
  bound existed, throughput degraded from ~26M ops/s in the first
  million calls to ~8.5M ops/s by 30 million -- a clear O(n^2) shape
  (each failed scan walks the entire, ever-growing list). Fixed with
  HZ10_CLASS_PAGES_SCAN_LIMIT (128 pages) bounding the search per
  hz10_malloc call regardless of total list length. After the fix, the
  same 30M-call run stays flat at ~26-28M ops/s throughout
  (bench/hz10_class_pages_scan_bench.c, run with `make
  bench-class-pages-scan`, reports first-segment-vs-last-segment ratio
  so a future regression here is directly visible: last_over_first
  should stay near 1.0, not shrink toward 0). Re-ran the established
  main_r50/main_r90 rows and the slot_count=1 isolating case afterward
  to confirm no regression from the added bound -- both stayed
  consistent with the pre-fix numbers above (main_r50/r90 still clearly
  beat system_malloc; the isolating case's run-to-run noise on this
  machine is unchanged, no new signal either way).

slot_count=1 isolation gap: root cause found via strace, then CLOSED
(src/hz10_freelist_page.c, see current_task.md):
  wall-clock benching of this case is too noisy on this machine (2-6x
  run-to-run) to read a ~24% gap directly, so switched to `perf stat`
  and `strace -c` on THREADS=4 MIN_SIZE=MAX_SIZE=65536 REMOTE_PCT=90 (8M
  total ops). Found: sys time exceeded user time (2.6s vs 1.47s);
  152,490 mmap + 152,479 munmap + 253,273 futex calls -- roughly 1 in 53
  allocations pays a full fresh-page reservation (each miss costs 1
  mmap + up to 2 munmap from src/hz10_freelist_page.c's over-reserve-
  then-trim alignment technique). Tested widening
  HZ10_CLASS_PAGES_SCAN_LIMIT (128 vs 1024): ~4% apart, inside the noise
  band, not a real fix. Confirmed Box 4's page pool was never populated
  in this path either, so every genuine miss paid the full syscall cost
  with nothing to soften it.

  Fixed by reserving quanta in batches of HZ10_QUANTUM_REGION_COUNT
  (256) instead of one at a time: one big over-reserve-then-trim per
  256 quanta, then a bump pointer hands out individual quanta with no
  further syscalls until the region is exhausted -- amortizes the trim
  cost ~256x. Re-ran the exact strace diagnostic: mmap dropped from
  152,490 to 874 calls (~175x fewer), munmap from 152,479 to 864.
  hz10_public_entry_bench's isolating case, 4 runs with both mechs in
  the same invocation: hz10 5.5-6.9M ops/s vs system_malloc 5.4-7.1M --
  essentially at parity (hz10 wins some runs), up from ~24% slower, and
  far more stable run-to-run than before (most of the original 2-6x
  variance was the syscall cost itself, not scheduler noise). No
  regression on any established row (main_local0/r50/r90, medium_local0,
  small_remote50/90) or Box 2's own bench.

small_remoteNN row -- box 3's O(n^2) drain cost CONFIRMED real, marker fix
implemented (see current_task.md):
  every row this project had benched so far (main_local0/r50/r90,
  medium_local0) only ever lands in classes with slot_count<=16, so
  Box 3's owner-drain duplicate-check cost (O(merged x
  current_local_len), first measured in isolation back in box 3) had
  never actually been exercised by a real hz10_public_entry row. Tested
  a small_remoteNN row (MIN_SIZE=16 MAX_SIZE=64, hits slot_count
  1024-4096 classes, matching this project's own established row-naming
  convention) at THREADS=4 ITERS=500000, low noise (RSS flat, tight
  spread unlike the slot_count=1 case):

    REMOTE_PCT=0:  hz10 ~200-208M ops/s, parity with system_malloc
    REMOTE_PCT=50: hz10 ~5.7-6.2M vs system_malloc ~19.4M (~3.2x SLOWER)
    REMOTE_PCT=90: hz10 ~3.0-3.3M vs system_malloc ~12.9M (~4.2x SLOWER)

  Confirmed via perf stat + strace this is a pure CPU cost (opposite
  signature from the slot_count=1 case): user time 5.26s vs sys time
  0.015s, 606 page-faults, ~4880 cycles/op -- consistent with
  hz10_page_local_freelist_contains (src/hz10_remote_stack.c) walking
  long local_free_head chains (up to 4096 nodes) on every drained slot.
  Selected fix: a glibc-tcache-style in-slot marker, not a separate bit
  array. Box 2 writes a second-word local-free marker on same-thread free
  and clears it on alloc; Box 3 rejects marked slots before remote publish
  and keeps the old O(N) contains walk only as a marked fallback. This should
  make normal remote drain O(merged). Short post-fix check
  (THREADS=4 ITERS=200000 RUNS=3, same MIN/MAX): REMOTE_PCT=50 hz10
  ~17.0-18.8M vs system_malloc ~18.1-19.5M; REMOTE_PCT=90 hz10
  ~12.7-13.8M vs system_malloc ~12.0-13.1M. The old 3-4x-slower numbers are
  retained above as the pre-fix evidence, not the current state.

remote-free striping (Box 3, see current_task.md for the reasoning behind
picking this over literal batching):
  remote_free_head is now 4 independent Treiber stacks per page
  (HZ10_REMOTE_STRIPE_COUNT), keyed by a hash of the freeing thread, to
  spread the CAS traffic across cache lines instead of contending one.
  hz10_remote_stack_drain_bench's own remote_push mech showed no visible
  change -- that bench creates a new OS thread per REPEAT cycle (2000
  times), so its number is dominated by thread-lifecycle cost, not CAS
  contention (a pre-existing bench limitation, not something this change
  introduced). A dedicated microbenchmark with persistent threads (no
  per-cycle thread creation) showed a real, modest effect: baseline (1
  stripe) ~15.0M ops/s stable; 4 stripes ~16.3-16.5M (~8-9% faster); 8
  stripes ~15.2-21.0M (noisier, ~15% on average); 16 stripes no further
  gain -- plateaus once stripe count matches thread count, as expected.
  Settled on 4 stripes. Re-verified small_remote50/90 and main_r50/r90
  afterward: no regression from the added per-page memory or per-stripe
  drain peek.

remote-free RMW attribution microbench:
  `make -C hakozuna-hz10 bench-remote-rmw-micro` builds
  `bench/hz10_remote_rmw_microbench.c`, a synthetic persistent-worker
  microbench that isolates the remote-free atomic publication pieces after
  routing has already found a page. Standard run:

    THREADS=4 SLOT_COUNT=4096 REPEAT=20000 RUNS=3 \
      make -C hakozuna-hz10 bench-remote-rmw-micro

  On 2026-07-04, worker-side medians were approximately:

    pending_fetch_or:         3.3 ns/op
    treiber_push:            26.5 ns/op
    counter_plus_treiber:    40.0 ns/op
    pending_counter_treiber: 59.5 ns/op

  Owner-side drain/clear stayed small (~3.7 ns/op for the full
  pending+counter+Treiber case). This confirms the remaining remote-row
  gap is dominated by freeing-thread publication cost, especially the
  Treiber CAS plus the extra remote_push_count fetch_add, not by owner
  drain.

  Follow-up: remote_push_count is debug-only after this measurement, so
  the release hot path matches the new `pending_acqrel_treiber` mode rather
  than `pending_counter_treiber`. On the same standard setting, that
  release-equivalent mode measured around 39-40 ns/op, while the debug-style
  full counter mode measured around 62 ns/op. A short public-entry recheck
  did not show a clear row-level win, so record this as hot-path cleanup
  and attribution evidence, not a confirmed public-entry throughput gain.

  Pending-bit ordering/ceiling matrix: `pending_acqrel_treiber` and
  `pending_relaxed_treiber` were effectively tied (~40.5 ns/op), so the
  safe acq_rel -> relaxed change is not a useful lever on this x86_64
  Linux machine. Do not generalize this to ARM/AArch64 without measuring:
  relaxed RMW may have a different ordering cost there. The unsafe
  no-pending ceiling (`treiber_no_pending_unsafe`) was much lower
  (~24.9 ns/op), which means the remaining possible win is a
  correctness-design problem, not a trivial memory-order cleanup. That box
  is intentionally not opened here: the pending bit is the remote-remote
  duplicate-free guard, and weakening/replacing it would be a contract
  change that needs a full-bench gate.

Next measurement box after the RMW ceiling: active-list scan/drain cost in
`hz10_class_pages_find_with_capacity()`. The RMW microbench isolates the
freeing-thread publication cost, but it does not cover alloc-side page scans,
remote-stack drain attempts, non-empty drain rate, or scan depth across the
multi-class public-entry rows. Any next remote-row tuning should first report
pages visited, drain calls, non-empty drains, slots merged, active-fast hits,
and scan-depth histograms for main_r50/r90 and small_remote50/90.
`bench-public-entry-active-scan` is the opt-in lane for this:

```text
HZ10_DUMP_CLASS_STATS=1 MODE=0 THREADS=4 ITERS=50000 RUNS=1 \
  MIN_SIZE=16 MAX_SIZE=32768 REMOTE_PCT=50 \
  make -C hakozuna-hz10 bench-public-entry-active-scan
```

It builds a separate `hz10_public_entry_active_scan_bench` binary with
`HZ10_ENABLE_ACTIVE_SCAN_STATS=1`, leaving the default public-entry bench
unchanged. A smoke-sized main_r50 probe on this machine reported
active_cache_hit=158821, active_cache_drain_calls=41092,
find_calls=26745, find_pages_visited=95668, find_drain_calls=95668,
find_nonempty_drains=26038, and depth buckets h0/h1/h2_4/h5_16/h17_64/
h65_128 = 87/7349/14049/4860/304/96. Treat that as a wiring check only,
not a performance decision; run the intended RUNS=10 row set before
tuning.

RUNS=10 measurement, 20260705:
`bench_results/20260704T214123Z_hz10_active_scan_cost_l0/combined.log`.
Median read: main_r50 sees ~260.9k find calls and ~886.8k pages visited
(~3.40 pages/find); main_r90 sees ~509.3k find calls and ~2.44M pages
visited (~4.80 pages/find). small_remote50/90 barely enter
find_with_capacity after warm-up (median find_calls 20/24); their useful
remote recovery is almost entirely current-active-page drain
(~99.5-99.6% of active-cache drains nonempty). So active-list scan/drain
is a main_r50/r90 question, not the explanation for the small_remote rows.

Follow-up hit-depth/per-class measurement:
`bench_results/20260704T220236Z_hz10_active_hit_depth_by_class_l0/combined.log`.
For main_r50/r90, classes 20 and 21 (24KiB and 32KiB, both 2-slot pages)
account for roughly 93% of scan pages visited. Successful hits are shallow
(~2.9 pages/hit at r50, ~3.9 at r90), while misses are rare (~1.0% r50,
~0.6% r90) but deep (~85-89 pages). This points at a recency/order A/B
such as move-to-front before a broader class-local cache design.

Move-to-front A/B:
`bench_results/20260704T220424Z_hz10_active_mtf_ab_l0/combined.log`.
The opt-in `bench-public-entry-active-mtf` lane (`HZ10_ENABLE_ACTIVE_MTF=1`)
is a NO-GO: main_r50 regressed 13.97M -> 13.20M ops/s (-5.5%) and scan
pages rose ~27%; main_r90 was only +0.95% while scan pages still rose ~15%.
Keep MTF default OFF. The next active-list question is narrower: whether
the dominant 2-slot classes need a class-local second-active/recent page,
not list-wide move-to-front.

Two-slot active-pattern probe (HZ10TwoSlotActivePattern-L0), also NO-GO:
`bench_results/20260704T221256Z_hz10_two_slot_active_pattern_l0/combined.log`.
Before building a second-active-page cache for class 20/21 (the two 2-slot
classes that dominate scan cost), the opt-in `bench-public-entry-two-slot`
lane (`HZ10_ENABLE_TWO_SLOT_STATS=1`, on top of the active-scan lane) tested
the premise directly: does the active page ping-pong between exactly two
physical pages? THREADS=4 ITERS=500000 RUNS=10 median for class 20/21:
main_r50 sees ~4.22 allocs served per activation (only ~9.5% of activations
exhaust within 1 op) and a single remembered "prior active" page would have
caught only ~14% of the next find_with_capacity() miss; main_r90 sees ~2.24
allocs served (~18.1% immediate-exhaust) and only ~5.8% second-active hit
rate. Both numbers are far below what a real two-page cycle would show
(which would read close to 100%), so remote-free traffic for these classes
is spread across more than two live pages per thread. Decision: do not open
HZ10SecondActiveByClass-AB-L0 -- a single remembered page would not avoid
most full-list scans, so an A/B here is not worth its implementation cost.
This concludes the HZ10ActiveScanCost-L0 line of investigation without a
cache-policy win; see current_task.md for what remains open (pending-bit
redesign, thread-exit abandonment).

Run-lifecycle RSS diagnostic (HZ10RunLifecycleRSS-L0):
`bench_results/20260704T221855Z_hz10_run_lifecycle_rss_l0/combined.log`.
Compared public-entry RUNS=10 fresh worker threads, thread-reuse segments,
and true 8s steady-state for main_r50/r90. Fresh-thread public-entry RSS
still climbs hard (main_r50 42,880 -> 542,256 KB; main_r90 123,776 ->
781,996 KB), while true steady-state keeps retired_count at exactly 0 and
RSS low (main_r50 20,992 KB, main_r90 31,488 KB after 8s). Thread-reuse
plateaus lower but shows large boundary inbox backlogs, so it remains a
boundary-stress workload. Read: the RSS issue is thread/run lifecycle page-
set abandonment, not continuous main-class fragmentation.

Thread-exit reclaim diagnostic (HZ10ThreadExitReclaim-L0):
`bench_results/20260704T223610Z_hz10_thread_exit_reclaim_l0/combined.log`.
`HZ10_THREAD_EXIT_RECLAIM=1` is an opt-in public-entry bench env that calls
`hz10_public_entry_reclaim_thread_idle_pages()` only after the workers' two
final inbox-drain barriers. It is a diagnostic quiescent hook, not an
automatic pthread destructor. On THREADS=4 ITERS=500000 RUNS=10, main_r50
baseline RSS climbed 55,296 -> 612,200 KB, while reclaim bounded it at
109,056 -> 127,720 KB; main_r90 baseline climbed 57,216 -> 858,352 KB, while
reclaim bounded it at 108,416 -> 227,804 KB. Reclaim saw and destroyed every
page at that quiescent point (r50: 29,581/29,581, busy=0; r90:
57,522/57,522, busy=0). This confirms the fresh-thread RSS growth is idle
lifecycle retention. A real fix should define an explicit safe-call
lifecycle/ownership contract before moving this behavior out of the bench.
Follow-up fix after review: retired pages now use the same ready-queue
guards as the normal harvest path before destruction. A ready-stacked page is
deferred (`pages_deferred_ready`) and an idle retired page that loses
`hz10_retired_ready_cancel()` is deferred (`pages_deferred_cancel`), leaving
both registered for the owner path instead of destroying them from the
lifecycle hook.
Guarded re-measure:
`bench_results/20260704T224951Z_hz10_thread_exit_reclaim_guard_fix_l0/combined.log`.
main_r50 now bounds RSS at 58,112 -> 311,580 KB with 13,197 pages reclaimed
and 12,021 deferred_ready; main_r90 bounds RSS at 102,400 -> 505,200 KB with
14,302 reclaimed and 21,436 deferred_ready. This is safe but not full RSS
closure; the L1 design needs a ready-drain phase before the direct retired
walk.

large-object path (src/hz10_large_alloc.{h,c}): size > HZ10_PAGE_QUANTUM
  now succeeds instead of returning NULL, via a dedicated direct-mmap
  path (see current_task.md for the design and a real Box 1 bug this
  surfaced and fixed). Measured the known "no pooling" gap's real cost
  rather than just asserting it: a tight alloc/touch/free loop of one
  fixed large size, no other threads, single-threaded -- hz10 ~70K ops/s
  vs system_malloc ~53-56M ops/s at 200KB and 1MB (roughly 750x slower).
  Not apples-to-apples: glibc's dynamic mmap-threshold adjustment serves
  this exact repeated-same-size pattern from a reused heap region after
  the first few iterations (no real syscalls after warmup), while every
  hz10 call pays a genuine mmap+munmap round trip by design (no large-
  object pooling yet, an explicitly stated gap, not a surprise). Not
  expected to matter for the workload shapes this project's rows
  actually stress (large allocations are typically few and long-lived in
  real programs, unlike the small/medium classes main_local0/medium_local0/
  small_remoteNN exercise) -- a future box should add a size-bucketed
  large-object cache if a real workload shows otherwise.
  Re-verified main_local0/medium_local0 (which cap at 32768/65536, never
  reaching this new path): no regression.
```
