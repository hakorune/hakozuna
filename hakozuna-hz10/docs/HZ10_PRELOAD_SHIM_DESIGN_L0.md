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
