// S267: large RSS plateau observation lane.
//
// Diagnostic-only. It keeps allocator behavior unchanged and emits per-class
// large scache peaks plus deferred-unmap inventory so RSS plateau rows can be
// explained without enabling S240 owner-front machinery.
#ifndef HZ3_S267_LARGE_RSS_PLATEAU_OBS
#define HZ3_S267_LARGE_RSS_PLATEAU_OBS 0
#endif

// S185: batch munmap on large-cache eviction path.
// 0=legacy per-victim unlock/munmap/relock, 1=batch victims then munmap outside lock.
#ifndef HZ3_S185_LARGE_EVICT_MUNMAP_BATCH
#define HZ3_S185_LARGE_EVICT_MUNMAP_BATCH 0
#endif

#ifndef HZ3_S185_EVICT_BATCH_MAX
#define HZ3_S185_EVICT_BATCH_MAX 8
#endif

#if HZ3_S185_EVICT_BATCH_MAX < 1
#error "HZ3_S185_EVICT_BATCH_MAX must be >= 1"
#endif

// S186: defer munmap from eviction path and drain by budget at free boundary.
// 0=off, 1=on.
#ifndef HZ3_S186_LARGE_UNMAP_DEFER
#define HZ3_S186_LARGE_UNMAP_DEFER 0
#endif

#ifndef HZ3_S186_UNMAP_DEFER_BUDGET
#define HZ3_S186_UNMAP_DEFER_BUDGET 4
#endif

#ifndef HZ3_S186_UNMAP_DEFER_MAX_BYTES
#define HZ3_S186_UNMAP_DEFER_MAX_BYTES (64ULL << 20)
#endif

#if HZ3_S186_UNMAP_DEFER_BUDGET < 1
#error "HZ3_S186_UNMAP_DEFER_BUDGET must be >= 1"
#endif

// S191: keep cache lock held on large_free fast path when madvise is not needed.
// 0=legacy unlock/relock sequence, 1=direct insert under first lock.
#ifndef HZ3_S191_LARGE_FREE_FAST_INSERT
#define HZ3_S191_LARGE_FREE_FAST_INSERT 0
#endif

// S192: soft purge advice mode for large-cache madvise path.
// 0=legacy behavior, 1=MADV_FREE, 2=MADV_DONTNEED.
#ifndef HZ3_S192_SOFT_PURGE_MODE
#define HZ3_S192_SOFT_PURGE_MODE 0
#endif

#if HZ3_S192_SOFT_PURGE_MODE < 0 || HZ3_S192_SOFT_PURGE_MODE > 2
#error "HZ3_S192_SOFT_PURGE_MODE must be 0..2"
#endif

// S193: demand-driven large scavenge on mmap miss.
// 0=off (legacy free-side purge), 1=on (purge only on miss, budgeted).
#ifndef HZ3_S193_DEMAND_SCAVENGE
#define HZ3_S193_DEMAND_SCAVENGE 1
#endif

// Run demand scavenge once per N mmap misses (>=1).
#ifndef HZ3_S193_SCAVENGE_MISS_STRIDE
#define HZ3_S193_SCAVENGE_MISS_STRIDE 16
#endif

// Maximum pages to purge in one demand-scavenge run.
#ifndef HZ3_S193_SCAVENGE_BUDGET_PAGES
#define HZ3_S193_SCAVENGE_BUDGET_PAGES 16
#endif

#if HZ3_S193_SCAVENGE_MISS_STRIDE < 1
#error "HZ3_S193_SCAVENGE_MISS_STRIDE must be >= 1"
#endif

#if HZ3_S193_SCAVENGE_BUDGET_PAGES < 1
#error "HZ3_S193_SCAVENGE_BUDGET_PAGES must be >= 1"
#endif

// S212: LargeUnmapDeferPlusBox
// Extend deferred-unmap usage from eviction-only to free-path fallback and
// add bounded demand-drain on mmap miss to avoid long defer backlog.
#ifndef HZ3_S212_LARGE_UNMAP_DEFER_PLUS
#define HZ3_S212_LARGE_UNMAP_DEFER_PLUS 1
#endif

// S212 deferred queue upper bound (bytes).
#ifndef HZ3_S212_UNMAP_DEFER_MAX_BYTES
#define HZ3_S212_UNMAP_DEFER_MAX_BYTES (64ULL << 20)
#endif

// S212 free-boundary drain budget.
#ifndef HZ3_S212_UNMAP_DEFER_FREE_BUDGET
#define HZ3_S212_UNMAP_DEFER_FREE_BUDGET 1
#endif

// If enabled, `do_munmap` path first tries to enqueue to S186 defer queue.
#ifndef HZ3_S212_DEFER_MUNMAP_ON_FREE_PATH
#define HZ3_S212_DEFER_MUNMAP_ON_FREE_PATH 1
#endif

// If enabled, alloc-miss path can drain part of deferred queue by bounded budget.
#ifndef HZ3_S212_DEFER_DRAIN_ON_ALLOC_MISS
#define HZ3_S212_DEFER_DRAIN_ON_ALLOC_MISS 1
#endif

// Run miss-triggered drain once per N alloc-cache misses.
#ifndef HZ3_S212_DEFER_DRAIN_MISS_STRIDE
#define HZ3_S212_DEFER_DRAIN_MISS_STRIDE 16
#endif

// Number of deferred victims to munmap in one miss-triggered drain.
#ifndef HZ3_S212_DEFER_DRAIN_MISS_BUDGET
#define HZ3_S212_DEFER_DRAIN_MISS_BUDGET 1
#endif

// Trigger alloc-miss drain only if deferred queue bytes exceed this threshold.
#ifndef HZ3_S212_DEFER_DRAIN_TRIGGER_BYTES
#define HZ3_S212_DEFER_DRAIN_TRIGGER_BYTES 33554432ULL
#endif

#if HZ3_S212_UNMAP_DEFER_FREE_BUDGET < 1
#error "HZ3_S212_UNMAP_DEFER_FREE_BUDGET must be >= 1"
#endif

#if HZ3_S212_UNMAP_DEFER_MAX_BYTES < (1ULL << 20)
#error "HZ3_S212_UNMAP_DEFER_MAX_BYTES must be >= 1MiB"
#endif

#if HZ3_S212_DEFER_DRAIN_MISS_STRIDE < 1
#error "HZ3_S212_DEFER_DRAIN_MISS_STRIDE must be >= 1"
#endif

#if HZ3_S212_DEFER_DRAIN_MISS_BUDGET < 1
#error "HZ3_S212_DEFER_DRAIN_MISS_BUDGET must be >= 1"
#endif
