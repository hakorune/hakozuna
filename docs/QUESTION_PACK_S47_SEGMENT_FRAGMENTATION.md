# S47 Question Pack: Segment Fragmentation / alloc failed (R=50)

## Goal
Eliminate `alloc failed` under remote-heavy MT without slowing hot paths.
Target: T=32, R=50, size 16-32768, 5M iters, alloc failed == 0, perf stable.

## Current Symptoms
- R=50 @ 5M iters: 7-9 alloc failed per run (16GB arena).
- 32GB arena improves to 0-5 alloc failed, but does not eliminate.
- R=0 has 0 alloc failed and high throughput.
- Perf is stable (approx 130M+ ops/s in R=50); failures are rare but persistent.

## Why we think this is fragmentation
- Segment is 2MB = 512 pages.
- A segment can be returned only when all 512 pages are free.
- MT remote-heavy scatters free runs across many segments.
- Central often has free runs, but not enough to make any single segment reach free_pages == 512.
- Arena slot exhaustion happens even though RSS is not maxed.

## Constraints (Box Theory)
- No hot-path overhead.
- Event-only boundaries (arena allocation failure / slow path entry).
- Compile-time A/B switches, avoid runtime env controls unless required.
- my_shard only when touching segment meta (no cross-shard mutation).

## Existing Mitigations (S45/S46)
- S45 Memory Budget Box:
  - emergency flush: local medium bins + inbox -> central
  - focus reclaim: sample from central, choose best segment, return runs
  - medium reclaim: pop central runs and return to segments
  - segment reclaim: free fully empty segments
- S46 Global Pressure Box:
  - arena pressure epoch broadcast
  - fast path drops to slow path when epoch advances
  - pressure handler flushes: remote stash, owner stash, inbox, local bins
  - aggressive focus reclaim with yield loops on allocation failure

## Current Implementation Notes
- Pressure broadcast in `hz3_arena_alloc()` when slot allocation fails.
- Focus reclaim runs both in pressure loop and mem_budget fallback.
- Emergency flush is event-only and per-thread.

## Hypothesis
We need a mechanism that *concentrates* free pages into a segment so one segment
becomes fully free, without touching the hot path.

## Candidate Directions
1) Focused reclaim improvements
   - Better grouping strategy to avoid scatter.
   - Push-back policy that avoids re-fragmentation.

2) Free list sharding or segment ownership tightening
   - Increase locality so frees cluster to fewer segments.

3) Partial segment reuse / page-level decommit
   - Keep arena slots but return physical pages (madvise/DONTNEED).
   - Could reduce RSS but does not increase slots; may not fix alloc failed.

4) Segment-level relocation in event-only path
   - Move runs from many segments into a selected target segment.
   - Only during pressure (avoid hot path).

5) Central policy changes
   - Maintain per-segment buckets to enable fast "fill one segment" strategy.

## Questions for Design Feedback
1) What is the minimal event-only design to *guarantee* one segment becomes fully free?
2) Is there a known approach from mimalloc/tcmalloc to reduce fragmentation in this pattern
   without hot-path changes?
3) Is it worth adding a small per-segment accounting table in central to
   pick a target segment deterministically?
4) Are there low-risk heuristics to reduce scattering when remote frees dominate?
5) If arena slots are the limiting factor, is there a clean way to decouple
   segment slots from size-class usage (e.g., cross-class reuse under pressure)?

## Repro Command (scale lane)
```
LD_PRELOAD=./libhakozuna_hz3_scale.so \
./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 5000000 400 16 32768 50
```

## Source Bundle (files included in zip)
- hz3 arena, mem_budget, tcache pressure, owner stash, inbox, segment/segmap,
  small_v2, hot path, config/types, Makefile, and this question pack.
