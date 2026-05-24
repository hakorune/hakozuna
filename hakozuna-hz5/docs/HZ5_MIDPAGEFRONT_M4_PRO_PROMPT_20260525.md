# HZ5 MidPageFront M4 Design Consultation Prompt

Please review the HZ5 Linux MidPageFront state and advise the next design.

Repository / branch:

```text
hakorune/hakozuna
branch: codex/hz5-linux-p43-port
latest local commit before this prompt:
  9659944 Clean up HZ5 Linux lane organization
```

Context:

```text
HZ5 Linux now has:
  SmallFront-S1: ordinary malloc <= 2048
  MidPageFront-M2: ordinary malloc 2049..32768
  MidFront-M1: fallback/control mid route
  LargeFront-L1: ordinary malloc >65536..1MiB
  Local2P: exact 64K/a8192 special profile

Current tcmalloc chase target:
  MidPageFront ordinary malloc, especially mid_only_r0 and mid_only_r90.
```

Current relevant lanes:

```text
--linux-hz5-general-midpage-region-shadow-allocfirst
  Current broad MidPage diagnostic lead.

--linux-hz5-general-midpage-region-shadow-nodeless
  M3 diagnostic: descriptor bitset + current_page/current_bits.

--linux-hz5-general-midpage-region-shadow-nodeless-ptrcache
  M3.2 diagnostic: per-class TLS pointer cache for owner-local frees.

--linux-hz5-general-midpage-region-shadow-localunsafe
  Unsafe r0 upper-bound diagnostic: skips owner-local MidPage bitmap checks.
```

Important measured results:

```text
mid_only_r0:
  allocfirst   ~70-90M ops/s depending run
  ptrcache     ~76M in r3 short run
  localunsafe  ~76.8M in r5 upper-bound run
  tcmalloc     ~140-147M

mid_only_r90:
  allocfirst   ~31-35M
  ptrcache     ~25-26M
  tcmalloc     ~42-47M

main_r90:
  allocfirst      ~25M
  allocfreefirst  ~29M
  ptrcache        ~27M
  tcmalloc        ~20-22M in these short runs

cross128_r90:
  allocfirst      ~14-22M depending run
  ptrcache        ~10M
  tcmalloc        ~7-8M
```

Size histogram for 16 threads, 80k iters/thread, ws=400:

```text
mid_only allocations:
  2049..3072      21,190
  3073..4096      21,396
  4097..8192      85,519
  8193..16384    170,952
  16385..32768   342,543

main allocations:
  <=2048          20,042
  2049..32768   301,179
  32769..65536  320,399

cross128 allocations:
  <=2048           9,992
  2049..32768    150,498
  32769..65536   160,700
  >65536         320,430
```

Perf / diagnostic read:

```text
1. nodeless reduced cache misses but increased branches/instructions.
2. nodeless stats showed huge partial/refill churn with single current_page.
3. ptrcache fixed most r0 partial/refill churn:
     mid_only_r0 ptrcache_hit=636934, refill=1569
   but remote-heavy still churned:
     mid_only_r90 refill=265631, remote_drained=556792
4. Unsafe local bitmap-check removal only improved mid_only_r0 ~5%.
5. alloc+free MidPage-first dispatch improved main_r90 but hurt mid_only.
6. perf record/report is noisy but shows HZ5 still paying preload ownership
   dispatch and MidPage try_alloc/free costs; perf stat previously showed HZ5
   instruction/branch count roughly ~2x tcmalloc in mid_only.
```

Question:

```text
Is the right next design MidPageFront-M4 magazine/front-cache?

Candidate M4:
  per-thread per-class pointer array cache
  alloc fast path: pop pointer from array, no page lookup, no per-object node
  free fast path: validate ptr -> page/slot, mark free, push pointer to array
  refill: batch from page/slab into pointer array
  drain: batch remote frees as page+bitmask packet, then refill local cache
  page/slab metadata touched mostly at refill/drain boundaries

Alternative:
  keep M2/M3 shape and add smaller fixes:
    free dispatch classifier
    class-specific functions
    larger current_page set
    remote page+bitmask packet only
```

Please answer:

```text
1. Does the data justify stopping M2/M3 tweak work and moving to M4?
2. Should M4 cache active-state as pointer-array entries and update page
   bitmaps only at refill/free, or is that unsafe for HZ5 fail-closed goals?
3. What is the minimum safe metadata model for M4?
4. Should remote use page+bitmask packets before or after local magazine work?
5. How should M4 handle the upper classes, since most mid_only allocations are
   16K..32K?
6. Is there a better design than M4 to approach tcmalloc while preserving
   fail-closed descriptor ownership?
```

Constraints:

```text
Do not weaken Windows P43i/P45.
Do not promote unsafe local nocheck.
Do not mix stats counters into speed lanes.
Keep unsupported/fallback/libc paths out of HZ5 wins.
Prefer Linux-specific candidate lanes until proven.
```
