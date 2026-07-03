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

slot_count=1 isolation gap root cause (see current_task.md, not fixed):
  wall-clock benching of this case is too noisy on this machine (2-6x
  run-to-run) to read a ~24% gap directly, so switched to `perf stat`
  and `strace -c` on THREADS=4 MIN_SIZE=MAX_SIZE=65536 REMOTE_PCT=90 (8M
  total ops). Found: sys time exceeded user time (2.6s vs 1.47s);
  152,490 mmap + 152,479 munmap + 253,273 futex calls -- roughly 1 in 53
  allocations pays a full fresh-page reservation (each miss costs 1
  mmap + up to 2 munmap from src/hz10_freelist_page.c's over-reserve-
  then-trim alignment technique). This contradicts an earlier, wrong
  same-session RSS-delta estimate of "~500 pages per 800K ops,
  negligible" -- strace is the trustworthy number, that estimate was
  not. Tested the obvious next hypothesis directly (widen
  HZ10_CLASS_PAGES_SCAN_LIMIT so more late-arriving remote frees get
  caught before falling out of the window): SCAN_LIMIT=128 vs 1024, 6
  repeats each, averaged 5.36M vs 5.55M ops/s -- ~4% apart, inside the
  noise band, not a real fix. Also confirmed Box 4's page pool is never
  populated in this path (hz10_page_pool_reuse_count/release_count stay
  0 throughout) because hz10_public_entry.c never destroys a page, so
  every genuine miss pays the full syscall cost with nothing to soften
  it. Real, understood, not closed -- a fix needs bigger surgery
  (destroyable-and-pool-returnable pages, a cheaper aligned-reservation
  primitive, or a different reuse structure), deprioritized since this
  is a pathological combination (the single largest size class at
  REMOTE_PCT=90) the established rows above do not hit.

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
```
