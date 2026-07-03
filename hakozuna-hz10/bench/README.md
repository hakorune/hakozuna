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
```
