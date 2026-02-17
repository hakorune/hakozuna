// hz4_tcache.c - HZ4 TCache Box (small-only allocator)
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdio.h>

#include "hz4_config.h"
#include "hz4_types.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_sizeclass.h"
#include "hz4_segment.h"
#include "hz4_os.h"
#include "hz4_tls_init.h"
#include "hz4_mid.h"
#include "hz4_large.h"
#include "hz4_tcache.h"
#if HZ4_PAGE_TAG_TABLE
#include "hz4_pagetag.h"
#endif

#if HZ4_FORCE_INLINE_MALLOC
#define HZ4_ALWAYS_INLINE __attribute__((always_inline)) inline
#define HZ4_ALWAYS_INLINE_EXTERN inline
#else
#define HZ4_ALWAYS_INLINE
#define HZ4_ALWAYS_INLINE_EXTERN
#endif

#if HZ4_TLS_SINGLE && (HZ4_ST_TCACHE_PREFETCH_LOCALITY >= 0)
#define HZ4_TCACHE_PREFETCH_LOCALITY_EFF HZ4_ST_TCACHE_PREFETCH_LOCALITY
#else
#define HZ4_TCACHE_PREFETCH_LOCALITY_EFF HZ4_TCACHE_PREFETCH_LOCALITY
#endif

#if HZ4_ST_STATS_B1
typedef struct {
    _Atomic uint64_t free_fast_path;
    _Atomic uint64_t free_fallback_path;
    _Atomic uint64_t refill_direct_calls;
    _Atomic uint64_t refill_direct_hit;
    _Atomic uint64_t refill_to_slow;
} hz4_st_stats_b1_counters_t;

static hz4_st_stats_b1_counters_t g_hz4_st_stats_b1;
static atomic_flag g_hz4_st_stats_b1_dump_once = ATOMIC_FLAG_INIT;

static inline void hz4_st_stats_b1_inc(_Atomic uint64_t* counter) {
    atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
}

static inline uint64_t hz4_st_stats_b1_load(_Atomic uint64_t* counter) {
    return atomic_load_explicit(counter, memory_order_relaxed);
}

static inline void hz4_st_stats_b1_free_fast_inc(void) {
    hz4_st_stats_b1_inc(&g_hz4_st_stats_b1.free_fast_path);
}
static inline void hz4_st_stats_b1_free_fallback_inc(void) {
    hz4_st_stats_b1_inc(&g_hz4_st_stats_b1.free_fallback_path);
}
static inline void hz4_st_stats_b1_refill_direct_calls_inc(void) {
    hz4_st_stats_b1_inc(&g_hz4_st_stats_b1.refill_direct_calls);
}
static inline void hz4_st_stats_b1_refill_direct_hit_inc(void) {
    hz4_st_stats_b1_inc(&g_hz4_st_stats_b1.refill_direct_hit);
}
static inline void hz4_st_stats_b1_refill_to_slow_inc(void) {
    hz4_st_stats_b1_inc(&g_hz4_st_stats_b1.refill_to_slow);
}

static void hz4_st_stats_b1_dump(void) {
    if (atomic_flag_test_and_set_explicit(&g_hz4_st_stats_b1_dump_once, memory_order_relaxed)) {
        return;
    }
    fprintf(stderr,
            "[HZ4_ST_STATS_B1] free_fast_path=%llu free_fallback_path=%llu "
            "refill_direct_calls=%llu refill_direct_hit=%llu refill_to_slow=%llu\n",
            (unsigned long long)hz4_st_stats_b1_load(&g_hz4_st_stats_b1.free_fast_path),
            (unsigned long long)hz4_st_stats_b1_load(&g_hz4_st_stats_b1.free_fallback_path),
            (unsigned long long)hz4_st_stats_b1_load(&g_hz4_st_stats_b1.refill_direct_calls),
            (unsigned long long)hz4_st_stats_b1_load(&g_hz4_st_stats_b1.refill_direct_hit),
            (unsigned long long)hz4_st_stats_b1_load(&g_hz4_st_stats_b1.refill_to_slow));
}

__attribute__((constructor))
static void hz4_st_stats_b1_register_atexit(void) {
    atexit(hz4_st_stats_b1_dump);
}
#else
static inline void hz4_st_stats_b1_free_fast_inc(void) {}
static inline void hz4_st_stats_b1_free_fallback_inc(void) {}
static inline void hz4_st_stats_b1_refill_direct_calls_inc(void) {}
static inline void hz4_st_stats_b1_refill_direct_hit_inc(void) {}
static inline void hz4_st_stats_b1_refill_to_slow_inc(void) {}
#endif

#if HZ4_SMALL_STATS_B19
typedef struct {
    _Atomic uint64_t malloc_calls;
    _Atomic uint64_t malloc_route_small;
    _Atomic uint64_t malloc_route_mid;
    _Atomic uint64_t malloc_route_large;
    _Atomic uint64_t malloc_bin_hit;
    _Atomic uint64_t malloc_bin_miss;
    _Atomic uint64_t malloc_refill_calls;
    _Atomic uint64_t malloc_refill_hit;
    _Atomic uint64_t malloc_refill_miss;
    _Atomic uint64_t free_calls;
    _Atomic uint64_t free_route_small;
    _Atomic uint64_t free_route_mid;
    _Atomic uint64_t free_route_large;
    _Atomic uint64_t free_route_unknown;
    _Atomic uint64_t free_pagetag_hit;
    _Atomic uint64_t free_pagetag_miss;
    _Atomic uint64_t small_free_local;
    _Atomic uint64_t small_free_remote;
    _Atomic uint64_t small_tcache_first;
    _Atomic uint64_t small_bump_meta;
    _Atomic uint64_t small_tcache_push;
    _Atomic uint64_t small_remote_keyed;
    _Atomic uint64_t small_remote_plain;
    // B76: SmallAllocPageInitLiteBox counters
    _Atomic uint64_t small_alloc_page_calls;
    _Atomic uint64_t small_alloc_page_init_lite_taken;
    _Atomic uint64_t small_alloc_page_init_lite_fallback_full;
    _Atomic uint64_t small_alloc_page_init_lite_verify_fail;
} hz4_small_stats_b19_counters_t;

static hz4_small_stats_b19_counters_t g_hz4_small_stats_b19;
static atomic_flag g_hz4_small_stats_b19_dump_once = ATOMIC_FLAG_INIT;

// Compatibility aliases: keep callsites unchanged while grouping counters.
#define g_hz4_small_stats_b19_malloc_calls g_hz4_small_stats_b19.malloc_calls
#define g_hz4_small_stats_b19_malloc_route_small g_hz4_small_stats_b19.malloc_route_small
#define g_hz4_small_stats_b19_malloc_route_mid g_hz4_small_stats_b19.malloc_route_mid
#define g_hz4_small_stats_b19_malloc_route_large g_hz4_small_stats_b19.malloc_route_large
#define g_hz4_small_stats_b19_malloc_bin_hit g_hz4_small_stats_b19.malloc_bin_hit
#define g_hz4_small_stats_b19_malloc_bin_miss g_hz4_small_stats_b19.malloc_bin_miss
#define g_hz4_small_stats_b19_malloc_refill_calls g_hz4_small_stats_b19.malloc_refill_calls
#define g_hz4_small_stats_b19_malloc_refill_hit g_hz4_small_stats_b19.malloc_refill_hit
#define g_hz4_small_stats_b19_malloc_refill_miss g_hz4_small_stats_b19.malloc_refill_miss
#define g_hz4_small_stats_b19_free_calls g_hz4_small_stats_b19.free_calls
#define g_hz4_small_stats_b19_free_route_small g_hz4_small_stats_b19.free_route_small
#define g_hz4_small_stats_b19_free_route_mid g_hz4_small_stats_b19.free_route_mid
#define g_hz4_small_stats_b19_free_route_large g_hz4_small_stats_b19.free_route_large
#define g_hz4_small_stats_b19_free_route_unknown g_hz4_small_stats_b19.free_route_unknown
#define g_hz4_small_stats_b19_free_pagetag_hit g_hz4_small_stats_b19.free_pagetag_hit
#define g_hz4_small_stats_b19_free_pagetag_miss g_hz4_small_stats_b19.free_pagetag_miss
#define g_hz4_small_stats_b19_small_free_local g_hz4_small_stats_b19.small_free_local
#define g_hz4_small_stats_b19_small_free_remote g_hz4_small_stats_b19.small_free_remote
#define g_hz4_small_stats_b19_small_tcache_first g_hz4_small_stats_b19.small_tcache_first
#define g_hz4_small_stats_b19_small_bump_meta g_hz4_small_stats_b19.small_bump_meta
#define g_hz4_small_stats_b19_small_tcache_push g_hz4_small_stats_b19.small_tcache_push
#define g_hz4_small_stats_b19_small_remote_keyed g_hz4_small_stats_b19.small_remote_keyed
#define g_hz4_small_stats_b19_small_remote_plain g_hz4_small_stats_b19.small_remote_plain
#define g_hz4_small_stats_b19_small_alloc_page_calls g_hz4_small_stats_b19.small_alloc_page_calls
#define g_hz4_small_stats_b19_small_alloc_page_init_lite_taken g_hz4_small_stats_b19.small_alloc_page_init_lite_taken
#define g_hz4_small_stats_b19_small_alloc_page_init_lite_fallback_full g_hz4_small_stats_b19.small_alloc_page_init_lite_fallback_full
#define g_hz4_small_stats_b19_small_alloc_page_init_lite_verify_fail g_hz4_small_stats_b19.small_alloc_page_init_lite_verify_fail

static inline void hz4_small_stats_b19_malloc_calls_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_malloc_calls, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_malloc_route_small_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_malloc_route_small, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_malloc_route_mid_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_malloc_route_mid, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_malloc_route_large_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_malloc_route_large, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_malloc_bin_hit_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_malloc_bin_hit, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_malloc_bin_miss_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_malloc_bin_miss, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_malloc_refill_calls_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_malloc_refill_calls, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_malloc_refill_hit_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_malloc_refill_hit, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_malloc_refill_miss_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_malloc_refill_miss, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_free_calls_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_free_calls, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_free_route_small_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_free_route_small, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_free_route_mid_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_free_route_mid, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_free_route_large_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_free_route_large, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_free_route_unknown_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_free_route_unknown, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_free_pagetag_hit_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_free_pagetag_hit, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_free_pagetag_miss_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_free_pagetag_miss, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_free_local_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_free_local, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_free_remote_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_free_remote, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_tcache_first_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_tcache_first, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_bump_meta_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_bump_meta, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_tcache_push_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_tcache_push, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_remote_keyed_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_remote_keyed, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_remote_plain_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_remote_plain, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_alloc_page_calls_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_alloc_page_calls, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_alloc_page_init_lite_taken_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_alloc_page_init_lite_taken, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_alloc_page_init_lite_fallback_full_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_alloc_page_init_lite_fallback_full, 1u, memory_order_relaxed);
}
static inline void hz4_small_stats_b19_small_alloc_page_init_lite_verify_fail_inc(void) {
    atomic_fetch_add_explicit(&g_hz4_small_stats_b19_small_alloc_page_init_lite_verify_fail, 1u, memory_order_relaxed);
}

static void hz4_small_stats_b19_dump(void) {
    if (atomic_flag_test_and_set_explicit(&g_hz4_small_stats_b19_dump_once, memory_order_relaxed)) {
        return;
    }
    fprintf(stderr,
            "[HZ4_SMALL_STATS_B19] malloc_calls=%llu malloc_route_small=%llu malloc_route_mid=%llu "
            "malloc_route_large=%llu malloc_bin_hit=%llu malloc_bin_miss=%llu malloc_refill_calls=%llu "
            "malloc_refill_hit=%llu malloc_refill_miss=%llu free_calls=%llu free_route_small=%llu "
            "free_route_mid=%llu free_route_large=%llu free_route_unknown=%llu free_pagetag_hit=%llu "
            "free_pagetag_miss=%llu small_free_local=%llu small_free_remote=%llu small_tcache_first=%llu "
            "small_bump_meta=%llu small_tcache_push=%llu small_remote_keyed=%llu small_remote_plain=%llu "
            "small_alloc_page_calls=%llu small_alloc_page_init_lite_taken=%llu "
            "small_alloc_page_init_lite_fallback_full=%llu small_alloc_page_init_lite_verify_fail=%llu\n",
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_malloc_calls,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_malloc_route_small,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_malloc_route_mid,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_malloc_route_large,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_malloc_bin_hit,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_malloc_bin_miss,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_malloc_refill_calls,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_malloc_refill_hit,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_malloc_refill_miss,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_free_calls,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_free_route_small,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_free_route_mid,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_free_route_large,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_free_route_unknown,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_free_pagetag_hit,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_free_pagetag_miss,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_free_local,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_free_remote,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_tcache_first,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_bump_meta,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_tcache_push,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_remote_keyed,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_remote_plain,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_alloc_page_calls,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_alloc_page_init_lite_taken,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_alloc_page_init_lite_fallback_full,
                                                      memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hz4_small_stats_b19_small_alloc_page_init_lite_verify_fail,
                                                      memory_order_relaxed));
}

__attribute__((constructor))
static void hz4_small_stats_b19_register_atexit(void) {
    atexit(hz4_small_stats_b19_dump);
}
#else
static inline void hz4_small_stats_b19_malloc_calls_inc(void) {}
static inline void hz4_small_stats_b19_malloc_route_small_inc(void) {}
static inline void hz4_small_stats_b19_malloc_route_mid_inc(void) {}
static inline void hz4_small_stats_b19_malloc_route_large_inc(void) {}
static inline void hz4_small_stats_b19_malloc_bin_hit_inc(void) {}
static inline void hz4_small_stats_b19_malloc_bin_miss_inc(void) {}
static inline void hz4_small_stats_b19_malloc_refill_calls_inc(void) {}
static inline void hz4_small_stats_b19_malloc_refill_hit_inc(void) {}
static inline void hz4_small_stats_b19_malloc_refill_miss_inc(void) {}
static inline void hz4_small_stats_b19_free_calls_inc(void) {}
static inline void hz4_small_stats_b19_free_route_small_inc(void) {}
static inline void hz4_small_stats_b19_free_route_mid_inc(void) {}
static inline void hz4_small_stats_b19_free_route_large_inc(void) {}
static inline void hz4_small_stats_b19_free_route_unknown_inc(void) {}
static inline void hz4_small_stats_b19_free_pagetag_hit_inc(void) {}
static inline void hz4_small_stats_b19_free_pagetag_miss_inc(void) {}
static inline void hz4_small_stats_b19_small_free_local_inc(void) {}
static inline void hz4_small_stats_b19_small_free_remote_inc(void) {}
static inline void hz4_small_stats_b19_small_tcache_first_inc(void) {}
static inline void hz4_small_stats_b19_small_bump_meta_inc(void) {}
static inline void hz4_small_stats_b19_small_tcache_push_inc(void) {}
static inline void hz4_small_stats_b19_small_remote_keyed_inc(void) {}
static inline void hz4_small_stats_b19_small_remote_plain_inc(void) {}
static inline void hz4_small_stats_b19_small_alloc_page_calls_inc(void) {}
static inline void hz4_small_stats_b19_small_alloc_page_init_lite_taken_inc(void) {}
static inline void hz4_small_stats_b19_small_alloc_page_init_lite_fallback_full_inc(void) {}
static inline void hz4_small_stats_b19_small_alloc_page_init_lite_verify_fail_inc(void) {}
#endif

#if HZ4_FREE_ROUTE_STATS_B26 || HZ4_FREE_ROUTE_ORDER_GATE_STATS_B27
static inline void hz4_free_stats_atomic_inc(_Atomic uint64_t* counter) {
    atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
}

static inline uint64_t hz4_free_stats_atomic_load(_Atomic uint64_t* counter) {
    return atomic_load_explicit(counter, memory_order_relaxed);
}
#endif

#if HZ4_FREE_ROUTE_STATS_B26
typedef struct {
    _Atomic uint64_t calls;
    _Atomic uint64_t seg_checks;
    _Atomic uint64_t seg_hits;
    _Atomic uint64_t seg_misses;
    _Atomic uint64_t large_validate_calls;
    _Atomic uint64_t large_validate_hits;
    _Atomic uint64_t mid_magic_hits;
    _Atomic uint64_t large_magic_hits;
    _Atomic uint64_t small_page_valid_hits;
    _Atomic uint64_t unknown_hits;
} hz4_free_b26_counters_t;

static hz4_free_b26_counters_t g_hz4_free_b26;
static atomic_flag g_hz4_free_b26_dump_once = ATOMIC_FLAG_INIT;

static inline void hz4_free_stats_b26_calls_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b26.calls);
}
static inline void hz4_free_stats_b26_seg_check_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b26.seg_checks);
}
static inline void hz4_free_stats_b26_seg_hit_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b26.seg_hits);
}
static inline void hz4_free_stats_b26_seg_miss_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b26.seg_misses);
}
static inline void hz4_free_stats_b26_large_validate_call_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b26.large_validate_calls);
}
static inline void hz4_free_stats_b26_large_validate_hit_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b26.large_validate_hits);
}
static inline void hz4_free_stats_b26_mid_magic_hit_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b26.mid_magic_hits);
}
static inline void hz4_free_stats_b26_large_magic_hit_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b26.large_magic_hits);
}
static inline void hz4_free_stats_b26_small_page_valid_hit_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b26.small_page_valid_hits);
}
static inline void hz4_free_stats_b26_unknown_hit_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b26.unknown_hits);
}

static void hz4_free_stats_b26_dump(void) {
    if (atomic_flag_test_and_set_explicit(&g_hz4_free_b26_dump_once, memory_order_relaxed)) {
        return;
    }
    fprintf(stderr,
            "[HZ4_FREE_ROUTE_STATS_B26] free_calls=%llu seg_checks=%llu seg_hits=%llu "
            "seg_misses=%llu large_validate_calls=%llu large_validate_hits=%llu "
            "mid_magic_hits=%llu large_magic_hits=%llu small_page_valid_hits=%llu "
            "unknown_hits=%llu\n",
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b26.calls),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b26.seg_checks),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b26.seg_hits),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b26.seg_misses),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b26.large_validate_calls),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b26.large_validate_hits),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b26.mid_magic_hits),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b26.large_magic_hits),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b26.small_page_valid_hits),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b26.unknown_hits));
}

__attribute__((constructor))
static void hz4_free_stats_b26_register_atexit(void) {
    atexit(hz4_free_stats_b26_dump);
}
#else
static inline void hz4_free_stats_b26_calls_inc(void) {}
static inline void hz4_free_stats_b26_seg_check_inc(void) {}
static inline void hz4_free_stats_b26_seg_hit_inc(void) {}
static inline void hz4_free_stats_b26_seg_miss_inc(void) {}
static inline void hz4_free_stats_b26_large_validate_call_inc(void) {}
static inline void hz4_free_stats_b26_large_validate_hit_inc(void) {}
static inline void hz4_free_stats_b26_mid_magic_hit_inc(void) {}
static inline void hz4_free_stats_b26_large_magic_hit_inc(void) {}
static inline void hz4_free_stats_b26_small_page_valid_hit_inc(void) {}
static inline void hz4_free_stats_b26_unknown_hit_inc(void) {}
#endif

#if HZ4_FREE_ROUTE_ORDER_GATE_STATS_B27
typedef struct {
    _Atomic uint64_t windows_large_first;
    _Atomic uint64_t windows_small_first;
    _Atomic uint64_t switch_to_large_first;
    _Atomic uint64_t switch_to_small_first;
    _Atomic uint64_t freeze_large_first;
    _Atomic uint64_t freeze_small_first;
} hz4_free_b27_counters_t;

static hz4_free_b27_counters_t g_hz4_free_b27;
static atomic_flag g_hz4_free_b27_dump_once = ATOMIC_FLAG_INIT;

static inline void hz4_free_stats_b27_window_large_first_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b27.windows_large_first);
}
static inline void hz4_free_stats_b27_window_small_first_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b27.windows_small_first);
}
static inline void hz4_free_stats_b27_switch_to_large_first_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b27.switch_to_large_first);
}
static inline void hz4_free_stats_b27_switch_to_small_first_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b27.switch_to_small_first);
}
static inline void hz4_free_stats_b27_freeze_large_first_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b27.freeze_large_first);
}
static inline void hz4_free_stats_b27_freeze_small_first_inc(void) {
    hz4_free_stats_atomic_inc(&g_hz4_free_b27.freeze_small_first);
}

static void hz4_free_stats_b27_dump(void) {
    if (atomic_flag_test_and_set_explicit(&g_hz4_free_b27_dump_once, memory_order_relaxed)) {
        return;
    }
    fprintf(stderr,
            "[HZ4_FREE_ROUTE_ORDER_STATS_B27] windows_large_first=%llu "
            "windows_small_first=%llu switch_to_large_first=%llu "
            "switch_to_small_first=%llu freeze_large_first=%llu "
            "freeze_small_first=%llu\n",
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b27.windows_large_first),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b27.windows_small_first),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b27.switch_to_large_first),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b27.switch_to_small_first),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b27.freeze_large_first),
            (unsigned long long)hz4_free_stats_atomic_load(&g_hz4_free_b27.freeze_small_first));
}

__attribute__((constructor))
static void hz4_free_stats_b27_register_atexit(void) {
    atexit(hz4_free_stats_b27_dump);
}
#else
static inline void hz4_free_stats_b27_window_large_first_inc(void) {}
static inline void hz4_free_stats_b27_window_small_first_inc(void) {}
static inline void hz4_free_stats_b27_switch_to_large_first_inc(void) {}
static inline void hz4_free_stats_b27_switch_to_small_first_inc(void) {}
static inline void hz4_free_stats_b27_freeze_large_first_inc(void) {}
static inline void hz4_free_stats_b27_freeze_small_first_inc(void) {}
#endif

// Include-order boundary:
// route helper includes stay above tcbox/malloc/free boundaries.
#if HZ4_XFER_CACHE
#include "hz4_xfer.inc"
#endif

#if HZ4_TCACHE_PURGE_BEFORE_DECOMMIT
static inline bool hz4_tcache_has_page_for_sc(hz4_tls_t* tls,
                                               uint8_t sc,
                                               hz4_page_t* target_page);
#endif

// Boundary boxes (single TU ownership)
#include "hz4_segq.inc"
#include "hz4_remote_flush.inc"
#include "hz4_collect.inc"
#if HZ4_CENTRAL_PAGEHEAP
#include "hz4_central_pageheap.h"
#endif

#if !HZ4_TLS_MERGE
typedef struct hz4_tcache_bin {
    void* head;
    uint32_t count;
#if HZ4_POPULATE_BATCH
    hz4_page_t* bump_page;  // lazy populate target
#endif
#if HZ4_TCACHE_OBJ_CACHE_ON
    void* st_cache[HZ4_TCACHE_OBJ_CACHE_SLOTS_ON];
    uint8_t st_cache_n;
#endif
} hz4_tcache_bin_t;

typedef struct hz4_alloc_tls {
    hz4_tcache_bin_t bins[HZ4_SC_MAX];
    hz4_seg_t* cur_seg;
    uint32_t next_page_idx;
    uint8_t owner;
    uint8_t inited;
} hz4_alloc_tls_t;

static __thread hz4_alloc_tls_t g_hz4_alloc_tls;

// Forward declaration (defined later in this file)
static hz4_alloc_tls_t* hz4_alloc_tls_get(hz4_tls_t* tls);
#endif

#include "hz4_tcache_fast.inc"
#if HZ4_TRANSFERCACHE
#include "hz4_tcbox.inc"
#endif

// Terminal tcache boundary includes:
// keep order stable (slow -> init -> refill helpers -> malloc -> free).
#include "hz4_tcache_slow.inc"

#include "hz4_tcache_init.inc"
#include "hz4_tcache_refill_helpers.inc"
#include "hz4_tcache_malloc.inc"
#include "hz4_tcache_free.inc"
size_t hz4_small_usable_size(void* ptr) {
    if (!ptr) {
        return 0;
    }
    hz4_page_t* page = hz4_page_from_ptr(ptr);
    if (!hz4_page_valid(page)) {
        HZ4_FAIL("hz4_small_usable_size: invalid page");
        abort();
    }
#if HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta = hz4_page_meta(page);
    return hz4_sc_to_size(meta->sc);
#else
    return hz4_sc_to_size(page->sc);
#endif
}
