# S48 Question Pack: Pinned Segment Avoidance (S47 extension)

## Goal
Eliminate `alloc failed` under MT remote-heavy by avoiding pinned segments.
Target: T=32, R=50, size 16-32768, 5M iters, alloc failed == 0.

## Current Status
- S47 Segment Quarantine + S47-2 ArenaGateBox implemented.
- ArenaGate leader works, but compaction never frees a segment:
  - seg_freed=0, free_gen=0
  - follower_ok=0, follower_timeout>0
- Conclusion: pinned segment pattern (1 page left) prevents full 512-page free.

## Constraints
- No hot-path overhead.
- Event-only (arena alloc failure / pressure handler only).
- my_shard only when touching segment meta.
- A/B via compile-time flags.
- S45/S46 remain as fallback safety nets.

## Existing Mechanisms (S47)
- scan -> select target segment -> quarantine -> drain -> free segment
- quarantine skips allocating from draining_seg
- headroom-based soft/hard compaction
- arena gate for thundering herd

## Problem to Solve
How to avoid "pinned" segments (e.g., 1 page left) from being selected over and over,
so compaction can actually free a segment.

## Candidate Ideas
1) Avoid list (blacklist)
   - store N recent pinned segments (2-4 slots)
   - scan skips avoid list
2) Score by free_pages
   - prefer segments with higher meta->free_pages
   - e.g., score = sampled_pages + free_pages
3) TTL on draining_seg
   - if not freed after N epochs, release quarantine and reselect
4) Detect pinned pattern
   - if free_pages doesn't change across passes, mark pinned

## Questions
1) What is the cleanest event-only design to avoid pinned segments?
2) Should avoid list be per-shard, per-thread, or global?
3) Is free_pages-based scoring enough, or is an explicit pinned detector needed?
4) How to avoid pathological cycles (avoid list too small)?
5) Any known allocator patterns (mimalloc/tcmalloc) that help here without hot-path changes?

## Repro Command
```
LD_PRELOAD=./libhakozuna_hz3_scale.so \
./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 5000000 400 16 32768 50
```

## Source Bundle
- hz3_segment_quarantine.c/.h
- hz3_arena.c/.h
- hz3_tcache.c/.h
- hz3_mem_budget.c/.h
- hz3_config.h
- hz3_types.h
- Makefile
- PHASE_HZ3_S45_MEMORY_BUDGET_BOX.md
