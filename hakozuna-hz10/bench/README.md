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
  (4097-65536), THREADS=4 ITERS=50000

remote rows (REMOTE_PCT=50/90): hz10 is 15-17x SLOWER than system_malloc,
  not faster -- isolated to size classes with small slot_count (e.g. a
  fixed 65536-byte class has slot_count=1): under remote pressure a
  1-slot page is exhausted almost every allocation, and each exhaustion
  that a drain cannot satisfy pays a fresh page acquisition (plus this
  module's own double pagemap registration for the owner tag, see
  src/hz10_public_entry.c) instead of a cheap freelist pop. A fixed
  16-byte class (slot_count=4096) under the same REMOTE_PCT=90 stays
  fast (~7M ops/s) because one page absorbs far more remote churn before
  needing replacement. This is layered on top of, and currently harder
  to isolate from, the O(N^2) owner-drain regression noted in
  current_task.md's box 3 section -- both need addressing before remote
  rows are a fair comparison against HZ8/HZ9.
```

