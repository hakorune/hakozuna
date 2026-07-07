# HZ11 Span-Backed Classify L1

Status: design for the box opened by HZ11ThreadCacheFastPath-L0's NO-GO.

## Why this box (L0 -> L1)

HZ11ThreadCacheFastPath-L0 measured NO-GO for the tcmalloc-tier gate
(fixed16/64/256/4096 at 0.71-0.82 of tcmalloc). The front-end works (cache hit
100%, token hit 100%, beats glibc/mimalloc/hz10), but the free-side **token hash**
is the gap: tcmalloc classifies a free by a direct-indexed page table (~1-2
cycles), while L0 must hash the pointer because its objects come from `sys_malloc`
at arbitrary addresses -- there is no contiguous region to index directly.

L1 tests the lever the diagnosis named: drop the system-malloc backing for cached
objects, give HZ11 its own mmap arena + 64KiB spans, and classify free by
**direct index** (one table load, no hash). The cache layer and the inlined hot
path from L0 are kept; only the **refill source** and the **free classifier**
change, behind a build flag so the L0 token lane stays as the A/B comparator.

This is still a speed-first research probe, not the HZ11 allocator body and not
an "HZ8 replacement." No checked/fail-closed diagnostics, no transfer cache, no
per-CPU/rseq.

## Shape (decisions)

```text
span size      : 64 KiB (HZ11_SPAN_SHIFT 16). Matches HZ8/HZ10.
arena          : 4 GiB virtual, mmap RW | MAP_NORESERVE, fault-on-access.
                 (NOT PROT_NONE+mprotect: HZ10BumpInit showed fault-on-first-write
                 is the right lazy pattern, and skipping mprotect removes a
                 syscall per carve. 4 GiB, not 64 GiB, keeps the classify table
                 small for L1; a constant, trivial to bump later.)
classify table : uint8_t hz11_span_class[ARENA>>16]  (65536 bytes for 4 GiB),
                 static BSS (loader zero-fill = uncarved). Encoding: 0 = uncarved
                 / foreign, class_id+1 = carved. The mmap zero-fill IS the
                 uncarved state -- no init memset.
refill         : returned-list pop FIRST; else bump from the per-thread current
                 span; else carve a fresh 64 KiB span from the arena bump-cursor
                 (atomic CAS), stamp span_class[id]=class+1, bump.
free           : if ptr in arena: c1 = span_class[(ptr-base)>>16]; if c1:
                   push to cache[c1-1]; return.
                 else: sys_free(ptr).  (one load, no hash, no collision)
malloc         : cache pop (unchanged); on miss, refill as above. Large (>64KiB)
                 still sys_malloc; its free misses the arena -> sys_free.
realloc(arena) : malloc(new) + memcpy(min) + free(old). sys_realloc is NOT valid
                 on an arena pointer (it is not a system pointer).
```

## Overflow sink (the L1 correctness fix -- do NOT sys_free arena pointers)

L0 overflow did `sys_free`, which was safe ONLY because L0 objects were
system-malloc'd. Arena/span pointers are NOT system pointers, so the span lane
MUST NOT sys_free them on cache overflow or `max_cached_bytes` overflow.

```text
sink           : hz11_returned[HZ11_CLASS_COUNT], each a mutex-guarded intrusive
                 stack (void* head; the object's first word is the link).
overflow path  : hz11_thread_cache_push_overflow_slow (span lane) moves the
                 class's cache items to hz11_returned[class] (under the lock),
                 then caches the new object. Arena pointers are never sys_free'd,
                 never dropped, never leaked.
refill         : drains hz11_returned[class] before bumping a fresh span.
scope          : a minimal SAFE overflow sink, NOT a tuned transfer cache (no
                 batch policy, no cap that could force a drop). The LIFO fixed
                 rows do not overflow, so it adds near-zero measurement noise.
sys_free       : still used for arena-OUTSIDE pointers (large/foreign), which are
                 real system pointers.
RSS            : the returned list is unbounded by design (dropping forbidden);
                 sustained overflow grows RSS until span reclaim lands in a later
                 box -- the same liability L0 documented for the cache cap.
```

## Speed-mode safety (valid-C assumption, with the span caveat)

```text
- assumes a valid C program; no double-free / interior / stale detection.
- the span-class table is write-once at carve (read-only after), so the free
  classify read is race-free.
- unknown pointers (outside the arena) fall to sys_free.
- CAVEAT: because classify is span_class[(ptr-base)>>16], an INTERIOR pointer
  (mid-object), a STALE pointer, or a DOUBLE-FREE of an in-arena address would
  still classify to that span's class and be (wrongly) pushed to the cache.
  Speed mode accepts this under the valid-C assumption; this is exactly what
  checked mode exists to catch later. No checked diagnostics on the hot path.
```

## A/B and files

Build flag `HZ11_CLASSIFY_SPAN` (default 0). `libhz11.so` = L0 token lane
(unchanged); `libhz11_span.so` = L1 span lane (`-DHZ11_CLASSIFY_SPAN=1`). The
gate compares BOTH against tcmalloc so the token-hash cost is isolated.

```text
NEW  src/hz11_span.{c,h}        arena, span bump-cursor, span_class table,
                                carve, arena-range/classify helpers (inline),
                                hz11_returned[] sink + push/pop.
MOD  src/hz11_thread_cache.h    H11SpanCurrent + current[CLASS_COUNT];
                                HZ11_CLASSIFY_SPAN default 0; gate token table;
                                span counters.
MOD  src/hz11_thread_cache.c    refill (returned/bump/carve); push_overflow_slow
                                (span lane: arena->returned, not sys_free).
MOD  src/hz11_public_entry.c    free direct-index classify; realloc arena-ptr
                                malloc+copy+free.
MOD  include/hz11.h + shim      span_create_count, direct_hit_count,
                                direct_miss_count; HZ11_DUMP_STATS prints them.
MOD  Makefile                   libhz11_span.so + preload-span; smoke both lanes.
MOD  scripts/run_hz11_fixed_local_gate.sh  add hz11-span column.
MOD  tests/hz11_thread_cache_smoke.c       span-lane correctness.
```

All under `hakozuna-hz11/`. HZ8/HZ10/HZ9 untouched. L0 token lane preserved by
the flag.

## Measurement + GO/NO-GO

```text
rows (single-thread LIFO): fixed16/64/256/4096_local0
compare: glibc, tcmalloc, mimalloc, hz10, hz11-token (L0), hz11-span (L1)
RUNS>=5 median. metrics: ops/s, hz11 counters (cache hit, direct-hit vs
token-hit, refill, span_create, direct_miss), post RSS.

GO (all):
  fixed64_local0  hz11-span >= tcmalloc * 0.95      (decisive)
  fixed16/256/4096 hz11-span clearly improves over hz11-token
  post RSS        hz11-span <= tcmalloc * 1.5
  direct-classify hit >= 0.99 on fixed rows
NO-GO:
  fixed64 hz11-span does not improve much over hz11-token
  direct classify costs ~ the token hash (table load no faster)
  RSS hz11-span > tcmalloc * 1.5
  span carve/refill heavier than sys_malloc and worsens fixed rows
```

## Risk

The bet is that a flat `uint8` direct-index load beats the token hash
(mix+load+compare) by enough to reach tc*0.95. It should: one dependent load vs
hash+load+compare, and no collision. Secondary risk: span carve/refill cost on a
refill that exhausts a span; for the LIFO bench the cache absorbs reuse after
warmup, so carves are cold. If fixed64 still misses, the next lever is refill
batch size (many slots per carve) or per-CPU, NOT more metadata.

## Measurement + verdict (RUNS=5 median)

Status: implemented + measured; **fixed-gate NO-GO, and the L0 diagnosis was
too strong.** The span/direct-index lane works correctly (direct-classify hit
100%, direct_miss 0, span_create 2/run, overflow sink in place). It slightly
improves over the token lane after the unsafe pointer-range comparisons were
fixed to use integer address arithmetic, but the gain is far too small to close
the tcmalloc gap:

```text
row          tcmalloc   hz11-token  hz11-span   span/tc   token/tc   gate(>=0.95)
fixed16      187.49M    136.04M     138.64M     0.739     0.726      MISS
fixed64      188.65M    135.79M     139.46M     0.739     0.720      MISS
fixed256     189.39M    135.16M     138.69M     0.732     0.714      MISS
fixed4096    166.22M    135.16M     138.02M     0.830     0.813      MISS
```

hz11-span improves over hz11-token by only about 2-3% on these rows. Both
plateau at tc*0.73-0.83.

Corrected diagnosis: the token hash is not the main bottleneck. Replacing it
with direct-index classify moves fixed64 from 0.720 to 0.739 of tcmalloc, which
is useful evidence but not the missing 20-25%. The remaining gap is the OVERALL
front-end per-op cost (shim call overhead, TLS-get, size-class table load,
pop/push), not the classification method alone.

Verdict: NO-GO for the L1 gate (fixed64 span/tcmalloc 0.739, gate 0.95). The
box succeeded at its purpose: it ran the A/B the L0 diagnosis named, and showed
classify replacement is not enough. Do not open another classify-method box
without new evidence.

Implementation audit note: the first review found a correctness issue in the
range check. Comparing arbitrary object pointers with `char*` relational
operators is undefined in C, so `hz11_span_classify()` and
`hz11_arena_contains()` now use `uintptr_t` arithmetic. The global
`span_create_count` counter is also relaxed-atomic to avoid a stats data race
under multithreaded span carve.

Next box (the lever the corrected diagnosis points at): attack the OVERALL
front-end, not the classify. Candidates: (a) measure/attribute the HZ11 malloc/
free instruction count vs tcmalloc's tc_malloc/tc_free on the SAME fixed64 row
(perf instructions/op), (b) the LD_PRELOAD shim call frame (malloc@plt ->
hz11_malloc) vs tcmalloc's entry, (c) the TLS-get + size-class cost. Do NOT open
another classify-method box without new evidence -- this A/B showed that
classification alone is not enough.
