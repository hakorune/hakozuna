// hz3_tcache_stats.c - Statistics and atexit handlers
// Extracted from hz3_tcache.c for modularization
#define _GNU_SOURCE

#include "hz3_tcache_internal.h"
#include "hz3_arena.h"
#include "hz3_owner_stash.h"
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif
#if HZ3_S47_SEGMENT_QUARANTINE
#include "hz3_segment_quarantine.h"
#endif

// ============================================================================
// OOM Triage
// ============================================================================
#if HZ3_OOM_SHOT
_Atomic uint64_t g_medium_page_alloc_sc[HZ3_NUM_SC];
static _Atomic int g_medium_oom_dumped = 0;

void hz3_medium_oom_dump(int sc) {
    if (atomic_exchange_explicit(&g_medium_oom_dumped, 1, memory_order_relaxed) != 0) {
        return;
    }
    fprintf(stderr, "[HZ3_OOM_MEDIUM] sc=%d size=%zu\n",
            sc, hz3_sc_to_size(sc));
    for (int i = 0; i < HZ3_NUM_SC; i++) {
        uint64_t cnt = atomic_load_explicit(&g_medium_page_alloc_sc[i], memory_order_relaxed);
        if (cnt == 0) {
            continue;
        }
        fprintf(stderr, "  sc=%d size=%zu alloc=%lu\n",
                i, hz3_sc_to_size(i), cnt);
    }
}
#endif

// ============================================================================
// S15: Stats Dump
// ============================================================================
#if HZ3_STATS_DUMP && !HZ3_SHIM_FORWARD_ONLY
static _Atomic uint32_t g_stats_refill_calls[HZ3_NUM_SC] = {0};
static _Atomic uint32_t g_stats_central_pop_miss[HZ3_NUM_SC] = {0};
static _Atomic uint32_t g_stats_segment_new_count = 0;
pthread_once_t g_stats_dump_atexit_once = PTHREAD_ONCE_INIT;

void hz3_stats_aggregate_thread(void) {
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        atomic_fetch_add_explicit(&g_stats_refill_calls[sc],
            t_hz3_cache.stats.refill_calls[sc], memory_order_relaxed);
        atomic_fetch_add_explicit(&g_stats_central_pop_miss[sc],
            t_hz3_cache.stats.central_pop_miss[sc], memory_order_relaxed);
        t_hz3_cache.stats.refill_calls[sc] = 0;
        t_hz3_cache.stats.central_pop_miss[sc] = 0;
    }
    atomic_fetch_add_explicit(&g_stats_segment_new_count,
        t_hz3_cache.stats.segment_new_count, memory_order_relaxed);
    t_hz3_cache.stats.segment_new_count = 0;
}

static void hz3_stats_dump(void) {
    hz3_stats_aggregate_thread();
    fprintf(stderr, "[hz3] S15 stats: seg_new=%u\n",
            atomic_load(&g_stats_segment_new_count));
    fprintf(stderr, "[hz3] refill_calls   :");
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        fprintf(stderr, " [%d]=%u", sc, atomic_load(&g_stats_refill_calls[sc]));
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "[hz3] central_pop_miss:");
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        fprintf(stderr, " [%d]=%u", sc, atomic_load(&g_stats_central_pop_miss[sc]));
    }
    fprintf(stderr, "\n");
}

void hz3_stats_register_atexit(void) {
    atexit(hz3_stats_dump);
}
#endif

// ============================================================================
// S12-3C: V2 Stats
// ============================================================================
#if HZ3_S12_V2_STATS && !HZ3_SHIM_FORWARD_ONLY
static _Atomic uint64_t g_s12_v2_seg_from_ptr_calls = 0;
static _Atomic uint64_t g_s12_v2_seg_from_ptr_hit = 0;
static _Atomic uint64_t g_s12_v2_seg_from_ptr_miss = 0;
static _Atomic uint64_t g_s12_v2_small_v2_enter = 0;
pthread_once_t g_s12_v2_stats_atexit_once = PTHREAD_ONCE_INIT;

void hz3_s12_v2_stats_aggregate_thread(void) {
    atomic_fetch_add_explicit(&g_s12_v2_seg_from_ptr_calls,
        t_hz3_cache.stats.s12_v2_seg_from_ptr_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s12_v2_seg_from_ptr_hit,
        t_hz3_cache.stats.s12_v2_seg_from_ptr_hit, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s12_v2_seg_from_ptr_miss,
        t_hz3_cache.stats.s12_v2_seg_from_ptr_miss, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s12_v2_small_v2_enter,
        t_hz3_cache.stats.s12_v2_small_v2_enter, memory_order_relaxed);
}

static void hz3_s12_v2_stats_dump(void) {
    fprintf(stderr, "[hz3] S12_V2 stats: calls=%lu hit=%lu miss=%lu enter=%lu\n",
            (unsigned long)atomic_load(&g_s12_v2_seg_from_ptr_calls),
            (unsigned long)atomic_load(&g_s12_v2_seg_from_ptr_hit),
            (unsigned long)atomic_load(&g_s12_v2_seg_from_ptr_miss),
            (unsigned long)atomic_load(&g_s12_v2_small_v2_enter));
}

void hz3_s12_v2_stats_register_atexit(void) {
    atexit(hz3_s12_v2_stats_dump);
}
#endif

// ============================================================================
// S54: Segment Scavenge Observe
// ============================================================================
#if HZ3_SEG_SCAVENGE_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
static _Atomic uint32_t g_scavenge_obs_calls = 0;
static _Atomic uint32_t g_scavenge_obs_max_segments = 0;
static _Atomic uint32_t g_scavenge_obs_max_free_pages = 0;
static _Atomic uint32_t g_scavenge_obs_max_candidate_pages = 0;
static _Atomic size_t g_scavenge_obs_max_potential_bytes = 0;
// pthread_once_t g_scavenge_obs_atexit_once defined in hz3_tcache.c

void hz3_seg_scavenge_observe(void) {
    if (!t_hz3_cache.initialized) return;

#if HZ3_SHARD_COLLISION_FAILFAST
    // Collision detection enabled - safe to observe
#endif

    uint8_t my_shard = (uint8_t)t_hz3_cache.my_shard;
    uint32_t segments = 0;
    uint32_t free_pages = 0;
    uint32_t candidate_pages = 0;

    // PackBox API for observation
    hz3_pack_observe_owner(my_shard, &segments, &free_pages, &candidate_pages);

    atomic_fetch_add_explicit(&g_scavenge_obs_calls, 1, memory_order_relaxed);

    // Max tracking (CAS loop)
    uint32_t current_max;

    // Max segments
    current_max = atomic_load_explicit(&g_scavenge_obs_max_segments, memory_order_relaxed);
    while (segments > current_max) {
        if (atomic_compare_exchange_weak_explicit(&g_scavenge_obs_max_segments,
                &current_max, segments, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }

    // Max free_pages
    current_max = atomic_load_explicit(&g_scavenge_obs_max_free_pages, memory_order_relaxed);
    while (free_pages > current_max) {
        if (atomic_compare_exchange_weak_explicit(&g_scavenge_obs_max_free_pages,
                &current_max, free_pages, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }

    // Max candidate_pages
    current_max = atomic_load_explicit(&g_scavenge_obs_max_candidate_pages, memory_order_relaxed);
    while (candidate_pages > current_max) {
        if (atomic_compare_exchange_weak_explicit(&g_scavenge_obs_max_candidate_pages,
                &current_max, candidate_pages, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }

    // Max potential_bytes
    size_t potential_bytes = (size_t)candidate_pages * HZ3_PAGE_SIZE;
    size_t current_max_bytes = atomic_load_explicit(&g_scavenge_obs_max_potential_bytes, memory_order_relaxed);
    while (potential_bytes > current_max_bytes) {
        if (atomic_compare_exchange_weak_explicit(&g_scavenge_obs_max_potential_bytes,
                &current_max_bytes, potential_bytes, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

static void hz3_seg_scavenge_obs_dump(void) {
    static _Atomic int g_scavenge_obs_dumped = 0;
    if (atomic_exchange_explicit(&g_scavenge_obs_dumped, 1, memory_order_acquire) == 0) {
        fprintf(stderr, "[HZ3_SEG_SCAVENGE_OBS] calls=%u max_segments=%u max_free_pages=%u "
                "max_candidate_pages=%u max_potential_bytes=%zu\n",
                atomic_load_explicit(&g_scavenge_obs_calls, memory_order_relaxed),
                atomic_load_explicit(&g_scavenge_obs_max_segments, memory_order_relaxed),
                atomic_load_explicit(&g_scavenge_obs_max_free_pages, memory_order_relaxed),
                atomic_load_explicit(&g_scavenge_obs_max_candidate_pages, memory_order_relaxed),
                atomic_load_explicit(&g_scavenge_obs_max_potential_bytes, memory_order_relaxed));
    }
}

void hz3_seg_scavenge_obs_register_atexit(void) {
    atexit(hz3_seg_scavenge_obs_dump);
}
#endif  // HZ3_SEG_SCAVENGE_OBSERVE

// ============================================================================
// S55: Retention Observe
// ============================================================================
#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
static _Atomic uint32_t g_retention_obs_calls = 0;
static _Atomic size_t g_retention_obs_max_tls_small_bytes = 0;
static _Atomic size_t g_retention_obs_max_tls_medium_bytes = 0;
static _Atomic size_t g_retention_obs_max_owner_stash_bytes = 0;
static _Atomic size_t g_retention_obs_max_central_medium_bytes = 0;
static _Atomic size_t g_retention_obs_max_arena_used_bytes = 0;
static _Atomic size_t g_retention_obs_max_pack_pool_free_bytes = 0;
#if HZ3_S55_RETENTION_OBSERVE_MINCORE
static _Atomic size_t g_retention_obs_max_ptag16_resident_bytes = 0;
static _Atomic size_t g_retention_obs_max_ptag32_resident_bytes = 0;
#endif
static _Atomic uint32_t g_retention_obs_suspicious_bins = 0;
static _Atomic uint32_t g_retention_obs_first_susp_bin = 0;
static _Atomic uint32_t g_retention_obs_first_susp_count = 0;
static _Atomic size_t g_retention_obs_first_susp_usable = 0;
// pthread_once_t g_retention_obs_atexit_once defined in hz3_tcache.c

#if HZ3_S55_RETENTION_OBSERVE_MINCORE
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

static size_t hz3_mincore_resident_bytes(const void* addr, size_t len) {
    if (!addr || len == 0) {
        return 0;
    }
    long page_sz = sysconf(_SC_PAGESIZE);
    if (page_sz <= 0) {
        return 0;
    }
    uintptr_t start = (uintptr_t)addr & ~((uintptr_t)page_sz - 1u);
    uintptr_t end = ((uintptr_t)addr + len + (uintptr_t)page_sz - 1u) & ~((uintptr_t)page_sz - 1u);
    size_t aligned_len = (size_t)(end - start);
    size_t pages = aligned_len / (size_t)page_sz;
    if (pages == 0) {
        return 0;
    }

    // Best-effort snapshot: allocate on stack for typical sizes, fall back to heap if huge.
    // For scale lane (64GB arena): ptag32=64MB => 16384 pages => 16KB vec, still stack-safe.
    unsigned char vec_stack[32768];
    unsigned char* vec = NULL;
    if (pages <= sizeof(vec_stack)) {
        vec = vec_stack;
    } else {
        vec = (unsigned char*)malloc(pages);
        if (!vec) {
            return 0;
        }
    }

    if (mincore((void*)start, aligned_len, vec) != 0) {
        if (vec != vec_stack) {
            free(vec);
        }
        return 0;
    }

    size_t resident_pages = 0;
    for (size_t i = 0; i < pages; i++) {
        if (vec[i] & 1u) {
            resident_pages++;
        }
    }

    if (vec != vec_stack) {
        free(vec);
    }
    return resident_pages * (size_t)page_sz;
}
#endif

void hz3_retention_observe_event(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

    uint8_t my_shard = (uint8_t)t_hz3_cache.my_shard;

    size_t tls_small_bytes = 0;
    size_t tls_medium_bytes = 0;
    size_t central_medium_bytes = 0;
    size_t owner_stash_bytes = 0;
    size_t arena_used_bytes = 0;
    size_t pack_pool_free_bytes = 0;
#if HZ3_S55_RETENTION_OBSERVE_MINCORE
    size_t ptag16_resident_bytes = 0;
    size_t ptag32_resident_bytes = 0;
#endif

    // 1) TLS local bins (capacity = count * usable_size)
#if HZ3_TCACHE_SOA_LOCAL
    for (uint32_t bin = 0; bin < HZ3_BIN_TOTAL; bin++) {
        Hz3BinCount count = t_hz3_cache.local_count[bin];
        if (count == 0) {
            continue;
        }
        size_t usable = hz3_bin_to_usable_size(bin);
        if (usable == 0) {
            continue;
        }
        size_t bytes = (size_t)count * usable;
        // Sanity: bin counts are expected to be small (tcache caps). Record if clearly abnormal.
        if (__builtin_expect(bytes > (1ull << 30), 0)) {  // >1GiB from a single TLS bin is suspicious
            atomic_fetch_add_explicit(&g_retention_obs_suspicious_bins, 1, memory_order_relaxed);
            if (atomic_exchange_explicit(&g_retention_obs_first_susp_bin, bin, memory_order_relaxed) == 0) {
                atomic_store_explicit(&g_retention_obs_first_susp_count, (uint32_t)count, memory_order_relaxed);
                atomic_store_explicit(&g_retention_obs_first_susp_usable, usable, memory_order_relaxed);
            }
        }
        if (bin < HZ3_MEDIUM_BIN_BASE) {
            tls_small_bytes += bytes;
        } else {
            tls_medium_bytes += bytes;
        }
    }
#else
    // AoS: counts may be disabled (lazy-count). Use conservative approximation.
    for (uint32_t bin = 0; bin < HZ3_BIN_TOTAL; bin++) {
        size_t usable = hz3_bin_to_usable_size(bin);
        if (usable == 0) continue;
        Hz3Bin* b = NULL;
        if (bin < HZ3_SMALL_NUM_SC) {
            b = hz3_tcache_get_small_bin((int)bin);
        } else if (bin < HZ3_MEDIUM_BIN_BASE) {
#if HZ3_SUB4K_ENABLE
            b = hz3_tcache_get_sub4k_bin((int)(bin - HZ3_SUB4K_BIN_BASE));
#endif
        } else {
            b = hz3_tcache_get_bin((int)(bin - HZ3_MEDIUM_BIN_BASE));
        }
        if (!b || !b->head) continue;
        Hz3BinCount count = hz3_bin_count_get(b);  // may be 0 when lazy-count
        if (count == 0) continue;
        size_t bytes = (size_t)count * usable;
        if (bin < HZ3_MEDIUM_BIN_BASE) tls_small_bytes += bytes;
        else tls_medium_bytes += bytes;
    }
#endif

    // 2) Central medium (my_shard only). Use trylock to avoid data race / contention.
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        Hz3CentralBin* cb = &g_hz3_central[my_shard][sc];
        if (pthread_mutex_trylock(&cb->lock) != 0) {
            continue;
        }
        uint32_t count = cb->count;
        pthread_mutex_unlock(&cb->lock);

        if (count == 0) {
            continue;
        }
        central_medium_bytes += (size_t)count * hz3_sc_to_size(sc);
    }

    // 3) Owner stash (approximate count; may be disabled)
#if HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE && HZ3_S44_OWNER_STASH_COUNT
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        uint32_t count = hz3_owner_stash_count(my_shard, sc);
        if (count == 0) continue;
        owner_stash_bytes += (size_t)count * hz3_small_sc_to_size(sc);
    }
#endif

    // 4) Arena used slots (segment count proxy)
#if HZ3_S47_SEGMENT_QUARANTINE
    arena_used_bytes = (size_t)hz3_arena_used_slots() * (size_t)HZ3_SEG_SIZE;
#endif

    // 5) Pack pool free pages (S54 PackBox API)
#if HZ3_S49_SEGMENT_PACKING
    uint32_t segs = 0, free_pages = 0, candidate_pages = 0;
    hz3_pack_observe_owner((uint8_t)t_hz3_cache.my_shard, &segs, &free_pages, &candidate_pages);
    (void)segs;
    (void)candidate_pages;
    pack_pool_free_bytes = (size_t)free_pages * (size_t)HZ3_PAGE_SIZE;
#endif

#if HZ3_S55_RETENTION_OBSERVE_MINCORE
    // 6) PTAG tables (resident bytes via mincore). This is often a fixed-cost RSS component.
    // NOTE: This is a best-effort snapshot; failures return 0 (do not fail-fast in OBSERVE).
#if HZ3_SMALL_V2_PTAG_ENABLE
    if (g_hz3_page_tag) {
        size_t bytes = (size_t)HZ3_ARENA_MAX_PAGES * sizeof(_Atomic(uint16_t));
        ptag16_resident_bytes = hz3_mincore_resident_bytes((const void*)g_hz3_page_tag, bytes);
    }
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
    if (g_hz3_page_tag32) {
        size_t bytes = (size_t)HZ3_ARENA_MAX_PAGES * sizeof(_Atomic(uint32_t));
        ptag32_resident_bytes = hz3_mincore_resident_bytes((const void*)g_hz3_page_tag32, bytes);
    }
#endif
#endif

    atomic_fetch_add_explicit(&g_retention_obs_calls, 1, memory_order_relaxed);
    hz3_atomic_max_size(&g_retention_obs_max_tls_small_bytes, tls_small_bytes);
    hz3_atomic_max_size(&g_retention_obs_max_tls_medium_bytes, tls_medium_bytes);
    hz3_atomic_max_size(&g_retention_obs_max_owner_stash_bytes, owner_stash_bytes);
    hz3_atomic_max_size(&g_retention_obs_max_central_medium_bytes, central_medium_bytes);
    hz3_atomic_max_size(&g_retention_obs_max_arena_used_bytes, arena_used_bytes);
    hz3_atomic_max_size(&g_retention_obs_max_pack_pool_free_bytes, pack_pool_free_bytes);
#if HZ3_S55_RETENTION_OBSERVE_MINCORE
    hz3_atomic_max_size(&g_retention_obs_max_ptag16_resident_bytes, ptag16_resident_bytes);
    hz3_atomic_max_size(&g_retention_obs_max_ptag32_resident_bytes, ptag32_resident_bytes);
#endif
}

static void hz3_retention_obs_dump(void) {
    static _Atomic int g_retention_obs_dumped = 0;
    if (atomic_exchange_explicit(&g_retention_obs_dumped, 1, memory_order_acquire) != 0) {
        return;
    }
    // If no observe event ran (e.g., epoch/pressure did not trigger during a short run),
    // take one best-effort snapshot at exit so the SSOT line is still informative.
    if (atomic_load_explicit(&g_retention_obs_calls, memory_order_relaxed) == 0) {
        hz3_retention_observe_event();
    }
    fprintf(stderr,
            "[HZ3_RETENTION_OBS] calls=%u tls_small_bytes=%zu tls_medium_bytes=%zu "
            "owner_stash_bytes=%zu central_medium_bytes=%zu arena_used_bytes=%zu pack_pool_free_bytes=%zu "
#if HZ3_S55_RETENTION_OBSERVE_MINCORE
            "ptag16_resident_bytes=%zu ptag32_resident_bytes=%zu "
#endif
            "suspicious_bins=%u first_susp_bin=%u first_susp_count=%u first_susp_usable=%zu\n",
            atomic_load_explicit(&g_retention_obs_calls, memory_order_relaxed),
            atomic_load_explicit(&g_retention_obs_max_tls_small_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_retention_obs_max_tls_medium_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_retention_obs_max_owner_stash_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_retention_obs_max_central_medium_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_retention_obs_max_arena_used_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_retention_obs_max_pack_pool_free_bytes, memory_order_relaxed),
#if HZ3_S55_RETENTION_OBSERVE_MINCORE
            atomic_load_explicit(&g_retention_obs_max_ptag16_resident_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_retention_obs_max_ptag32_resident_bytes, memory_order_relaxed),
#endif
            atomic_load_explicit(&g_retention_obs_suspicious_bins, memory_order_relaxed),
            atomic_load_explicit(&g_retention_obs_first_susp_bin, memory_order_relaxed),
            atomic_load_explicit(&g_retention_obs_first_susp_count, memory_order_relaxed),
            atomic_load_explicit(&g_retention_obs_first_susp_usable, memory_order_relaxed));
}

void hz3_retention_obs_register_atexit(void) {
    atexit(hz3_retention_obs_dump);
}
#endif  // HZ3_S55_RETENTION_OBSERVE

// ============================================================================
// S56: Active Set Stats
// ============================================================================
#if HZ3_S56_ACTIVE_SET && HZ3_S56_ACTIVE_SET_STATS && !HZ3_SHIM_FORWARD_ONLY
_Atomic uint64_t g_s56_active_choose_alt = 0;
static pthread_once_t g_s56_active_atexit_once = PTHREAD_ONCE_INIT;

static void hz3_s56_active_set_dump(void) {
    fprintf(stderr, "[HZ3_S56_ACTIVE_SET] choose_alt=%lu\n",
            (unsigned long)atomic_load_explicit(&g_s56_active_choose_alt, memory_order_relaxed));
}

static void hz3_s56_active_set_register_atexit_once(void) {
    atexit(hz3_s56_active_set_dump);
}

void hz3_s56_active_set_register_atexit(void) {
    pthread_once(&g_s56_active_atexit_once, hz3_s56_active_set_register_atexit_once);
}
#endif
