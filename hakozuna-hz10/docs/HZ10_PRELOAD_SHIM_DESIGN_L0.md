# HZ10 Preload Shim Design L0

Design memo for HZ10PreloadShim-L0: a real LD_PRELOAD `libhz10.so` so HZ10
can run foreign programs and macro benchmarks. Review document, not an
implementation patch.

## Why this is the next frontier (state of the micro campaign)

As of 20260706 every named micro box is done or closed with evidence:

```text
local:   LTO + E1/E2a/E3 landed; E2b NO-GO; E4 parked (its bundle,
         front default-ON, measured NO-GO: main_r50/r90 +8-9% but
         small_local0 -5.2%, main_local0 -2.2%); front cache stays
         opt-in; front-array opt-in evidence lane; address coloring
         BLOCKED (zero tail slack on 32768x2/65536x1).
remote:  attribution done twice (mutex + spin lanes); F1 NO-GO;
         F2 limited GO; StripeSpread narrowed GO; F3 (contract change)
         closed -- residuals are instruction diet + inherent
         producer->owner->slot round trips, neither worth a contract.
harness: spin inbox lane exists; candidate E measured and closed
         (mutex inbox was inflating main remote gaps ~2x).
```

The micro rows now say: local bulk 1.45-1.7x tcmalloc (was 5-9x), remote
main 0.7-0.85 allocator-only, RSS strong, ahead of mimalloc 2.1.7 on many
rows. The next claim to earn -- "comparable allocator" -- cannot come from
these self-made rows; it needs real programs. That requires a shim, and
the shim forces the real API/lifecycle decisions this repo has deferred.

## Scope

v0 GOAL: `LD_PRELOAD=libhz10.so <real program>` runs correctly, and a
mimalloc-bench-style subset runs to completion with comparable numbers
recorded against glibc/tcmalloc/mimalloc(source build -- NOT the broken
Ubuntu package, see the 20260705 mimalloc probe notes).

v0 NON-GOALS: automatic thread-exit reclaim (contract forbids it, see
below), fork-heavy correctness beyond atfork lock handling, Windows,
C++ operator new interposition (libc malloc covers it via default
operator new), exotic APIs (pvalloc/valloc get trivial wrappers).

## Interposition surface and semantics decisions

```text
malloc(size)         size==0 -> treat as 16 (glibc-parity unique ptr).
                     The LIBRARY hz10_malloc keeps returning NULL for 0
                     (documented L0 scope); the mapping lives in the
                     shim layer only.
free(ptr)            hz10_free(ptr); if it returns 0 (unknown pointer):
                     default = abort with a diagnostic (fail-closed,
                     catches interposition holes early);
                     HZ10_SHIM_TOLERATE_FOREIGN=1 = count + leak, for
                     triaging programs that hand us pre-main pointers.
realloc(ptr,size)    ptr==NULL -> malloc; size==0 -> free, return NULL
                     (glibc behavior); else route(ptr) -> slot_size:
                     new size <= slot_size -> return ptr in place
                     (class rounding gives natural headroom), else
                     malloc+memcpy(min)+free. Route failure -> abort
                     (same fail-closed rule as free).
calloc(n,size)       overflow-checked multiply, malloc, memset. Always
                     memset in v0: recycled slots are dirty; "fresh
                     quantum pages are already zero" is a later
                     optimization with a freshness bit, not v0.
malloc_usable_size   route -> slot_size (large: registered span size).
posix_memalign /     alignment <= 16 (HZ10_MIN_ALIGN): plain malloc.
aligned_alloc /      16 < alignment <= HZ10_PAGE_QUANTUM: v0 satisfies
memalign             it through the large path (quantum-aligned base,
                     64KiB alignment covers every smaller power of two)
                     -- wasteful for mid alignments, correct, and
                     measured before optimized. > quantum: ENOMEM.
                     Rationale: 1.5x-class slot offsets do not give
                     power-of-two alignment guarantees, so classes
                     cannot serve alignment > 16 safely.
strdup/etc.          not interposed; libc implementations call malloc,
                     which resolves to the shim.
```

## The two real design items

### 1. Metadata self-hosting (recursion break)

The allocator itself calls libc heap in exactly three places, all in
hz10_freelist_page.c's create/destroy (page struct calloc, pending-array
calloc for slot_count>64, stripe-spread array calloc): under a shim these
recurse into hz10_malloc mid-page-create. Fix: a private, mmap-backed
metadata slab (hz10_platform_reserve; intrusive freelist of
page-struct-sized nodes; the pending/spread side arrays fold into the
same node by sizing it for the worst case both need -- pending for
slot_count<=4096 is <=64 u64s, spread is 4 padded lines, so a fixed
~1KiB metadata node covers every shape with one freelist). Zeroing is
the slab's job (create relies on calloc semantics today). This change
benefits the library standalone too (page create/destroy stops paying
glibc malloc) and must land FIRST, gated on existing smokes + the
public-entry matrix staying flat.

### 2. Thread exit policy (the honest gap)

hz10_public_entry_flush_thread_cache_quiescent()'s contract explicitly
forbids running it from an automatic destructor (a foreign thread can
still be remote-freeing into the dying thread's pages). v0 policy:
NO destructor. An exiting thread's pages are orphaned-but-safe exactly
as today's benches document: remote frees into them still land in
stripes/pending correctly, nothing dangles, and the RSS cost is bounded
by the thread's working set at exit. This is recorded as the shim's
known RSS liability; the ownership-handoff design (already named in
current_task.md as the real fix) is deliberately NOT bundled into v0.
Macro benches with thread churn (larson) will price this liability for
us -- that number decides whether the handoff design gets opened next.

Also required, small: pthread_atfork(lock, unlock, unlock-reinit) around
the pagemap mutex and quantum-region lock so forked children do not
inherit a held lock (git, shells, and redis BGSAVE fork).

## Deliverables and gates

```text
D1 metadata self-hosting commit (library-only, no shim yet):
   gates: full smokes + sanitizers + TSan lane + standalone check +
   public-entry matrix flat (no row moves > noise) + rss-guard.
D2 shim: src/hz10_shim.c -> libhz10.so (new make target `preload`),
   plus tests/hz10_shim_api_smoke.c (the semantics matrix above,
   including realloc in-place/copy edges, calloc overflow, memalign
   alignment checks, usable_size).
D3 foreign-program smoke script (scripts/run_hz10_shim_smoke.sh):
   LD_PRELOAD over ls/grep/python3 -c/git status, checked for exit
   status; abort-on-foreign-pointer active. This is the fail-closed
   proof the interposition surface is complete enough.
D4 macro bench lane: mimalloc-bench subset (larson, mstress,
   xmalloc-test, cache-scratch; redis-benchmark if available) x
   {glibc, tcmalloc, mimalloc-source, hz10} with wall time + RSS
   columns; results become the NEW headline table, with the existing
   micro rows demoted to regression guards.
Rollback: the shim is additive (new .so); D1 is the only library
   change and is judged independently.
```

## D1 implementation record

DONE 20260706: `hz10_freelist_page.c` no longer calls libc
`calloc/free` for page metadata. Page struct, multi-word pending bits,
and tiny-class spread stripes now live in a private mmap-backed metadata
node freelist. Public page layout and pending/spread semantics are
unchanged; `pending_bits != &pending_inline_word` still means
out-of-line storage, but that storage is allocator-private metadata
rather than libc heap.

Gates:

```text
smokes: pagemap route/diff, freelist page, remote drain, bounded pool,
        class pages, size class, public entry, front, front-array,
        retired-ready: green
sanitizers: ASan/UBSan public page path smokes: green
TSan: smoke-tsan-aslr-off: green
standalone check: green
rss guard: green
flatness: alternating base/current n=30, THREADS=4 ITERS=200000,
          all 9 public-entry rows within +/-2.4% median ops/s
log: bench_results/20260705T194922Z_hz10_metadata_self_host_d1_alternating/
```

## D2/D3 implementation record

DONE 20260706: `src/hz10_shim.c` builds `libhz10.so` via the new
`preload` target. It interposes `malloc/free/calloc/realloc`,
`malloc_usable_size`, `posix_memalign`, `aligned_alloc`, and `memalign`.
Internal shim calls use private helpers rather than symbol lookup, which
matters for the direct `dlopen()` API smoke and for real LD_PRELOAD
startup. The shim registers `pthread_atfork()` hooks for the pagemap lock
and the freelist metadata/quantum reservoirs.

API semantics are covered by `tests/hz10_shim_api_smoke.c`:

```text
malloc(0) -> non-NULL shim allocation
calloc zero-fill and overflow ENOMEM
realloc in-place when new size <= malloc_usable_size, copy+free when grown
realloc(NULL,0) follows malloc(0); realloc(ptr,0) frees and returns NULL
posix_memalign/aligned_alloc/memalign alignment checks
malloc_usable_size route -> slot_size
unknown free aborts by default; HZ10_SHIM_TOLERATE_FOREIGN=1 counts+returns
```

D3 foreign-program smoke is `scripts/run_hz10_shim_smoke.sh` and the
`smoke-shim-foreign` make target. It currently runs fail-closed
`LD_PRELOAD=libhz10.so` over `/bin/true`, `/bin/ls`, `/bin/grep`,
`/usr/bin/python3`, and `git status`. A direct fork smoke
(`/bin/sh -c 'true; echo ...'`) also passed after atfork hooks landed.

Gates:

```text
smoke-shim-api: green
smoke-shim-foreign: green
ASan/UBSan shim build + smoke-shim-api: green
public-entry/freelist/pagemap smokes: green
standalone check: green
```

## D4 initial macro lane record

DONE 20260706: `scripts/run_hz10_macro_preload_matrix.sh` defines the
first macro preload matrix and `bench-macro-preload` runs it. This is not
the final mimalloc-bench suite yet; it is the always-available lane that
proves HZ10 can produce real LD_PRELOAD wall/RSS rows against other
allocators.

Current workloads:

```text
python_alloc: PYTHONMALLOC=malloc python allocation churn, measured with
              /usr/bin/time wall_sec + max_rss_kb.
redis_setget: starts redis-server under each allocator and runs
              redis-benchmark SET/GET; records client wall/max RSS and
              server RSS.
allocators:   glibc, hz10, tcmalloc if found, mimalloc only when an
              explicit/source-build candidate is found (do not use the
              known-bad Ubuntu libmimalloc2.0 package).
```

Initial run:
`bench_results/20260705T201126Z_hz10_macro_preload_matrix/`
with `RUNS=3 REDIS_OPS=20000 REDIS_CLIENTS=32 PYTHON_LOOPS=80`.

Median result:

```text
workload      allocator  wall_sec  max_rss_kb  wall/glibc
python_alloc  glibc        1.230       92400      1.000
python_alloc  hz10         0.900      116904      0.732
python_alloc  tcmalloc     0.840      104576      0.683
python_alloc  mimalloc     0.810      100224      0.659
redis_setget  glibc        0.540        8192      1.000
redis_setget  hz10         0.540        8192      1.000
redis_setget  tcmalloc     0.540        8192      1.000
redis_setget  mimalloc     0.570        8320      1.056
redis server RSS: glibc 6824K, hz10 7580K, tcmalloc 10916K, mimalloc 6884K
```

Read: D4 is operational. HZ10 is clearly faster than glibc on the Python
malloc-forced churn, but still behind tcmalloc/mimalloc and with higher
RSS on that row. The short Redis SET/GET lane is too flat to rank speed,
but it does show HZ10 server RSS below tcmalloc and modestly above
glibc/mimalloc. Next D4 work is to add the real mimalloc-bench subset or
longer macro rows; do not promote this two-workload smoke as the final
headline table.

## Python RSS attribution and class-granularity result

DONE 20260706: `python_alloc` RSS was decomposed in
`bench_results/20260706T190000Z_hz10_macro_rss_attribution/notes.md`.
Two probes established that front-cache is not the fix for this row, while
class-boundary fit is the main driver: worst-fit sizes just above class
edges made HZ10 ~49% heavier than glibc, while best-fit sizes just below
edges reduced the gap to ~11%.

HZ10ClassGranularity-L1 then added quarter-step classes in the 64..8192
band. The attribution was confirmed: the `python_alloc` shim row improved
from ~116.9MB to ~106.7MB median RSS in the same session. However, making
the finer table the default failed the public-entry gate, with large
local/interleaved regressions (main_local0 ~0.75x in the full A/B, still
~0.77x after lookup trim). The result is recorded in
`docs/HZ10_NO_GO_LEDGER.md` as `NO-GO as default`.

The fine table remains as `HZ10_ENABLE_FINE_SIZE_CLASSES=1`, an opt-in
macro/RSS probe lane. The next RSS box is not retention tuning by guesswork:
add `HZ10ShimExitStats-L0`, an env-gated atexit per-class/pool dump, to
split the remaining best-fit gap into live class occupancy, pool retention,
and metadata before changing policy.

## HZ10ShimExitStats-L0 implementation record

DONE 20260706: `HZ10_SHIM_EXIT_STATS=1` on `libhz10.so` registers an
atexit dump. Output is stderr-only and line-oriented:

```text
hz10_shim_exit_stats summary ...
hz10_shim_exit_stats class=...
hz10_shim_exit_stats class_totals ...
```

The summary line reports tolerated foreign frees, page-pool cached/reuse/
release/purge counters, and private metadata slab capacity/live/free node
counts. The per-class lines report the exiting thread's TLS page-list
snapshot: active/retired page counts, eviction/retirement/reclaim counters,
and find counters.

This is deliberately a measurement box, not a retention policy change. Its
main caveat is also explicit: `hz10_public_entry_class_list_stats()` is
TLS-local, so the dump observes the thread running the exit handler. It is
valid for the single-thread `python_alloc` attribution row, but it cannot
account for pages abandoned by already-exited worker threads. That remains
a separate ownership/thread-exit design.

Probe log:
`bench_results/20260706T210000Z_hz10_shim_exit_stats_l0/`.
Small Python probe excerpt:

```text
metadata_slabs=2 metadata_capacity=132 metadata_live=82 metadata_free=50
class_totals active_pages=82 retired_pages=0 page_bytes=5373952
```

Next macro step: rerun the `python_alloc` and best-fit residual probes with
`HZ10_SHIM_EXIT_STATS=1`, then split the remaining RSS gap into live page
footprint, metadata slab footprint, and page-pool retention before opening
any retention-policy box.

## HZ10RetiredLocalIdleReclaim-L0 prototype record

PROTOTYPE 20260706: exit-stats showed a local-churn-specific retention
hole. The ready queue is remote-free-driven, so a single-thread local free
that makes a retired page fully idle does not push any event. The budgeted
sweep can eventually find such a page, but in `python_alloc` it does not
keep up.

The opt-in lane `HZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM=1` adds exactly one
owner-local hook: after `hz10_freelist_page_free()` updates `free_count`, if
the page is fully idle, scan that class's retired list for the exact page.
Only if the page is actually retired does the hook run the same
load-bearing guards as harvest (`retired_ready_on_stack` skip plus
`hz10_retired_ready_cancel()`), then unlink and destroy it. Active pages are
not destroyed by this hook.

Same-session `python_alloc` probe, `RUNS=3`, default vs opt-in:

```text
default:   retired_pages=933 page_bytes=105971712 pool_reuse=230
           reclaimed_sweep=419 reclaimed_local_free=0 max_rss ~= 116.8MB
opt-in:    retired_pages=2   page_bytes=44957696  pool_reuse=696
           reclaimed_sweep=0   reclaimed_local_free=1807 max_rss ~= 113.1MB
```

Read: the structural win is real; the retired backlog collapses. The short
probe's maxrss win is smaller than the exit footprint delta because maxrss
records earlier fresh mapping churn. Keep this lane opt-in until it clears
the full public-entry micro matrix, rss-guard, and the best-fit/uniform
macro rows. Combine with `HZ10_ENABLE_FINE_SIZE_CLASSES=1` only as a
separate shim/RSS experiment, not as a default allocator change.

Follow-up gate result: `NO-GO as default`. The hook was tightened with an
owner-list-state guard and bounded to the 80..4096 slot-size band, then
rechecked. It still carried local-path risk in selected micro rows, while
the macro result did not beat fine classes alone:

```text
default:       wall 0.910s, maxrss 116.8MB, retired_pages 933
retired-local: wall 0.900s, maxrss 113.1MB, retired_pages 2
fine:          wall 0.870s, maxrss 106.7MB, retired_pages 574
fine+retired:  wall 0.920s, maxrss 108.2MB, retired_pages 1
```

Decision: keep retired-local as an opt-in diagnostic/footprint reducer, not
a default or recommended shim lane. For the next macro step, fine classes
remain the stronger measured RSS lever; expand the macro matrix and price
thread-churn/no-destructor behavior before opening another local-free
retention tweak.

## HZ10MacroMatrixExpand-L0 scoped next box

SCOPED 20260707: see `docs/HZ10_MACRO_MATRIX_EXPAND_L0.md`.

The next D4 box is not another allocator micro-optimization. It expands the
macro matrix so one run can answer two open decisions:

```text
1. Add an `hz10+fine` allocator column backed by a non-clobbering
   `libhz10_fine.so` sibling. The existing convenience preload targets all
   write `libhz10.so`, which is fine for manual probes but not for a matrix
   that compares default HZ10 and fine HZ10 side by side.
2. Add a larson row from local artifacts, SKIP if no executable is present.
   This is the first macro row aimed directly at thread churn and the v0
   no-destructor retained-page liability.
```

Use current RSS, not only `ru_maxrss`, when pricing retained/orphan debt.
`ru_maxrss` remains useful for churn peak, but it cannot tell whether pages
remain resident after worker exit. Keep thread-exit ownership and automatic
destructor ideas closed until the larson/current-RSS matrix produces a
material median delta.

## Open questions for reviewers

```text
1. free() of a pointer from a DIFFERENT thread's dead owner: already
   safe (remote path), but should the shim ever drain orphan pages
   opportunistically (any thread may NOT: drain is owner-only by
   contract)? v0 says no; confirm nobody expects otherwise.
2. Is abort-on-foreign-free too aggressive for D3's program set?
   (dlopen'd libraries with static ctors predating the shim shouldn't
   exist under LD_PRELOAD, but PYTHONMALLOC / interpreter arenas may
   surprise; the tolerate env exists for exactly this triage.)
3. aligned_alloc via the large path: acceptable v0 waste, or do mid
   alignments (32..4096) deserve a class-side story before macro runs
   (some workloads memalign heavily)? Recommendation: measure in D4
   first.
4. Does D1's fixed-size metadata node (~1KiB for the worst-case
   pending+spread) waste too much for tiny-slot_count pages, or is
   page-count times 1KiB trivially acceptable (it is ~1.6% of a
   64KiB quantum)? Recommendation: accept, revisit only if D4's RSS
   columns say otherwise.
```
