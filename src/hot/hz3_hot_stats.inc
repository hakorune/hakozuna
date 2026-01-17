// ============================================================================
// hz3_hot_stats.inc - S110/S113 SegHdr PerPage FreeMeta stats
// This file is included by hz3_hot.c (single TU)
// ============================================================================

// ============================================================================
// S110: SegHdr PerPage FreeMeta stats (observation + optional free fast path)
// ============================================================================

#if HZ3_S110_STATS
// Observation stats (S110-0) + optional fast path stats (S110-1).
static _Atomic(uint64_t) g_s110_seg_hit = 0;
static _Atomic(uint64_t) g_s110_seg_miss = 0;
static _Atomic(uint64_t) g_s110_meta_hit = 0;
static _Atomic(uint64_t) g_s110_meta_zero = 0;
static _Atomic(uint64_t) g_s110_fast_taken = 0;
static _Atomic(uint64_t) g_s110_fast_local = 0;
static _Atomic(uint64_t) g_s110_fast_remote = 0;
static _Atomic(uint64_t) g_s110_fallback_ptag = 0;
static _Atomic(uint64_t) g_s110_page0_skip = 0;
static _Atomic(uint64_t) g_s110_bin_oob = 0;
static _Atomic(int) g_s110_atexit_registered = 0;

static void hz3_s110_atexit_dump(void) {
    uint64_t seg_hit = atomic_load_explicit(&g_s110_seg_hit, memory_order_relaxed);
    uint64_t seg_miss = atomic_load_explicit(&g_s110_seg_miss, memory_order_relaxed);
    uint64_t meta_hit = atomic_load_explicit(&g_s110_meta_hit, memory_order_relaxed);
    uint64_t meta_zero = atomic_load_explicit(&g_s110_meta_zero, memory_order_relaxed);
    uint64_t fast_taken = atomic_load_explicit(&g_s110_fast_taken, memory_order_relaxed);
    uint64_t fast_local = atomic_load_explicit(&g_s110_fast_local, memory_order_relaxed);
    uint64_t fast_remote = atomic_load_explicit(&g_s110_fast_remote, memory_order_relaxed);
    uint64_t fallback_ptag = atomic_load_explicit(&g_s110_fallback_ptag, memory_order_relaxed);
    uint64_t page0_skip = atomic_load_explicit(&g_s110_page0_skip, memory_order_relaxed);
    uint64_t bin_oob = atomic_load_explicit(&g_s110_bin_oob, memory_order_relaxed);
    uint64_t total_calls = seg_hit + seg_miss;
    fprintf(stderr,
            "[HZ3_S110_FREE] calls=%lu seg_hit=%lu meta_hit=%lu fast_taken=%lu "
            "fast_local=%lu fast_remote=%lu fallback_ptag=%lu meta_zero=%lu "
            "page0_skip=%lu bin_oob=%lu\n",
            (unsigned long)total_calls,
            (unsigned long)seg_hit,
            (unsigned long)meta_hit,
            (unsigned long)fast_taken,
            (unsigned long)fast_local,
            (unsigned long)fast_remote,
            (unsigned long)fallback_ptag,
            (unsigned long)meta_zero,
            (unsigned long)page0_skip,
            (unsigned long)bin_oob);
}
#endif

// ============================================================================
// S113: SegMath + SegHdr FreeMeta stats (observation only for Phase 0)
// ============================================================================

#if HZ3_S113_STATS
static _Atomic(uint64_t) g_s113_calls = 0;
static _Atomic(uint64_t) g_s113_seg_math_hit = 0;     // ptr in arena && used[slot]==1
static _Atomic(uint64_t) g_s113_seg_math_miss = 0;    // ptr outside arena or used[slot]==0
static _Atomic(uint64_t) g_s113_meta_hit = 0;         // page_bin_plus1 != 0
static _Atomic(uint64_t) g_s113_meta_zero = 0;        // page_bin_plus1 == 0
static _Atomic(uint64_t) g_s113_page0_skip = 0;       // page_idx == 0 (header page)
static _Atomic(uint64_t) g_s113_fallback_ptag = 0;    // falls through to PTAG path
static _Atomic(int) g_s113_atexit_registered = 0;

static void hz3_s113_atexit_dump(void) {
    uint64_t calls = atomic_load_explicit(&g_s113_calls, memory_order_relaxed);
    uint64_t seg_math_hit = atomic_load_explicit(&g_s113_seg_math_hit, memory_order_relaxed);
    uint64_t seg_math_miss = atomic_load_explicit(&g_s113_seg_math_miss, memory_order_relaxed);
    uint64_t meta_hit = atomic_load_explicit(&g_s113_meta_hit, memory_order_relaxed);
    uint64_t meta_zero = atomic_load_explicit(&g_s113_meta_zero, memory_order_relaxed);
    uint64_t page0_skip = atomic_load_explicit(&g_s113_page0_skip, memory_order_relaxed);
    uint64_t fallback_ptag = atomic_load_explicit(&g_s113_fallback_ptag, memory_order_relaxed);
    double hit_rate = (calls > 0) ? (100.0 * meta_hit / calls) : 0.0;
    fprintf(stderr,
            "[HZ3_S113] calls=%lu seg_math_hit=%lu seg_math_miss=%lu "
            "meta_hit=%lu (%.1f%%) meta_zero=%lu page0_skip=%lu fallback_ptag=%lu\n",
            (unsigned long)calls,
            (unsigned long)seg_math_hit,
            (unsigned long)seg_math_miss,
            (unsigned long)meta_hit,
            hit_rate,
            (unsigned long)meta_zero,
            (unsigned long)page0_skip,
            (unsigned long)fallback_ptag);
}
#endif
