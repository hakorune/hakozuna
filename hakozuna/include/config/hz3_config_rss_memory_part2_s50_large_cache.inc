
// ============================================================================
// S50: Large Cache Size-Class Box
// ============================================================================

#ifndef HZ3_S50_LARGE_SCACHE
#define HZ3_S50_LARGE_SCACHE 1
#endif

#ifndef HZ3_S50_LARGE_SCACHE_EVICT
#define HZ3_S50_LARGE_SCACHE_EVICT 0
#endif

// S51: madvise(MADV_DONTNEED) on cache push
#ifndef HZ3_S51_LARGE_MADVISE
#define HZ3_S51_LARGE_MADVISE 0
#endif

// S52: Best-fit fallback for large cache
#ifndef HZ3_S52_LARGE_BESTFIT
#define HZ3_S52_LARGE_BESTFIT 0
#endif

#ifndef HZ3_S52_BESTFIT_RANGE
#define HZ3_S52_BESTFIT_RANGE 2
#endif

// S181: lock striping for active-large hash map (insert/take/find-size path)
// 1 = legacy single lock, N(power-of-2) = striped locks.
#ifndef HZ3_S181_LARGE_MAP_LOCK_STRIPES
#define HZ3_S181_LARGE_MAP_LOCK_STRIPES 64
#endif

#if HZ3_S181_LARGE_MAP_LOCK_STRIPES < 1
#error "HZ3_S181_LARGE_MAP_LOCK_STRIPES must be >= 1"
#endif

#if (HZ3_S181_LARGE_MAP_LOCK_STRIPES & (HZ3_S181_LARGE_MAP_LOCK_STRIPES - 1)) != 0
#error "HZ3_S181_LARGE_MAP_LOCK_STRIPES must be power-of-2"
#endif

// S238: LargeDirectHeaderLookupBox (opt-in)
// 0=legacy hash-only take, 1=direct-first + safe fallback to hash walk.
#ifndef HZ3_S238_LARGE_DIRECT_LOOKUP
#define HZ3_S238_LARGE_DIRECT_LOOKUP 0
#endif

// S238 one-shot counters at exit.
#ifndef HZ3_S238_STATS
#define HZ3_S238_STATS 0
#endif

// S182: use userspace spin lock for large-cache global lock to reduce futex/kernel time
// on MT large alloc/free hot path. 0=mutex(hz3_lock_t), 1=atomic_flag spin.
#ifndef HZ3_S182_LARGE_CACHE_SPINLOCK
#define HZ3_S182_LARGE_CACHE_SPINLOCK 0
#endif

// S183: split large cache lock by size class (S50 scache path only).
// 0=single cache lock, 1=per-class list lock + global budget lock.
#ifndef HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#define HZ3_S183_LARGE_CLASS_LOCK_SPLIT 0
#endif

// S184: free-path budget precheck with atomic cached-bytes hint.
// 0=off (legacy), 1=on (skip first budget lock section when likely cacheable).
#ifndef HZ3_S184_LARGE_FREE_PRECHECK
#define HZ3_S184_LARGE_FREE_PRECHECK 0
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
