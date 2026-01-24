#define _GNU_SOURCE

#include "hz3_s64_tcache_dump.h"

#if HZ3_S64_TCACHE_DUMP

#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_dtor_stats.h"

#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
#include "hz3_small_v2.h"
#endif

// S64-1G: TCacheDumpGuardBox (debug-only)
// tcache dump walks next pointers; it will segfault quickly if local bins contain a bad pointer.
// Guard validates (cur/next) before following, and fails fast with (owner, sc, ptr).
#ifndef HZ3_S64_TDUMP_GUARD
#define HZ3_S64_TDUMP_GUARD 0
#endif

#ifndef HZ3_S64_TDUMP_GUARD_FAILFAST
#define HZ3_S64_TDUMP_GUARD_FAILFAST HZ3_S64_TDUMP_GUARD
#endif

#ifndef HZ3_S64_TDUMP_GUARD_SHOT
#define HZ3_S64_TDUMP_GUARD_SHOT 1
#endif

#if HZ3_S64_TDUMP_GUARD
#if HZ3_S64_TDUMP_GUARD_SHOT
static _Atomic int g_s64_tdump_guard_shot = 0;
#else
static _Atomic int g_s64_tdump_guard_shot __attribute__((unused)) = 0;
#endif
static inline int hz3_s64_tdump_guard_should_log(void) {
#if HZ3_S64_TDUMP_GUARD_SHOT
    return (atomic_exchange_explicit(&g_s64_tdump_guard_shot, 1, memory_order_relaxed) == 0);
#else
    return 1;
#endif
}

static inline int hz3_s64_tdump_guard_ptr_ok(uint8_t owner, int sc, void* ptr,
                                             const char* where, void* cur, void* next) {
    uint32_t seg_idx = 0;
    void* seg_base_ptr = NULL;
    if (!hz3_arena_contains_fast(ptr, &seg_idx, &seg_base_ptr)) {
        if (hz3_s64_tdump_guard_should_log()) {
            fprintf(stderr,
                    "[HZ3_S64_TDUMP_GUARD] where=%s why=seg_unused_or_out_of_arena owner=%u sc=%d ptr=%p cur=%p next=%p\n",
                    where, (unsigned)owner, sc, ptr, cur, next);
        }
#if HZ3_S64_TDUMP_GUARD_FAILFAST
        abort();
#endif
        return 0;
    }
#if HZ3_PTAG_DSTBIN_ENABLE
    if (g_hz3_page_tag32) {
        uint32_t page_idx = 0;
        if (!hz3_arena_page_index_fast(ptr, &page_idx)) {
            if (hz3_s64_tdump_guard_should_log()) {
                fprintf(stderr,
                        "[HZ3_S64_TDUMP_GUARD] where=%s why=page_idx_fail owner=%u sc=%d ptr=%p cur=%p next=%p\n",
                        where, (unsigned)owner, sc, ptr, cur, next);
            }
#if HZ3_S64_TDUMP_GUARD_FAILFAST
            abort();
#endif
            return 0;
        }
        uint32_t tag32 = hz3_pagetag32_load(page_idx);
        if (tag32 == 0) {
            if (hz3_s64_tdump_guard_should_log()) {
                fprintf(stderr,
                        "[HZ3_S64_TDUMP_GUARD] where=%s why=ptag32_zero owner=%u sc=%d ptr=%p page_idx=%u cur=%p next=%p\n",
                        where, (unsigned)owner, sc, ptr, (unsigned)page_idx, cur, next);
            }
#if HZ3_S64_TDUMP_GUARD_FAILFAST
            abort();
#endif
            return 0;
        }
        uint32_t bin = hz3_pagetag32_bin(tag32);
        uint8_t dst = hz3_pagetag32_dst(tag32);
        if (bin != (uint32_t)hz3_bin_index_small(sc) || dst != owner) {
            if (hz3_s64_tdump_guard_should_log()) {
                fprintf(stderr,
                        "[HZ3_S64_TDUMP_GUARD] where=%s why=ptag32_mismatch owner=%u sc=%d ptr=%p page_idx=%u want_bin=%u tag32=0x%x bin=%u dst=%u cur=%p next=%p\n",
                        where, (unsigned)owner, sc, ptr, (unsigned)page_idx,
                        (unsigned)hz3_bin_index_small(sc), tag32, (unsigned)bin, (unsigned)dst,
                        cur, next);
            }
#if HZ3_S64_TDUMP_GUARD_FAILFAST
            abort();
#endif
            return 0;
        }
    }
#endif
    (void)seg_idx;
    (void)seg_base_ptr;
    return 1;
}
#endif  // HZ3_S64_TDUMP_GUARD

// ============================================================================
// S64-1: TCacheDumpBox - budgeted dump from tcache to central
// ============================================================================
//
// Dumps tcache local bins (small_v2 only) to central during epoch tick.
// This supplies objects for S64_RetireScanBox to find empty pages and retire.
//
// Unlike hz3_tcache_flush_small_to_central() which does full flush,
// this does budgeted partial flush controlled by knobs.s64_tdump_budget_objs.

extern HZ3_TLS Hz3TCache t_hz3_cache;

// Forward declaration (small_v2 central push)
#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
void hz3_small_v2_central_push_list(uint8_t shard, int sc, void* head, void* tail, uint32_t n);
#endif

// Stats (atomic for multi-thread safety)
#if HZ3_S64_STATS
HZ3_DTOR_STAT(g_s64d_tick_calls);
HZ3_DTOR_STAT(g_s64d_objs_dumped);
HZ3_DTOR_STAT(g_s64d_budget_hits);  // budget exhausted count

HZ3_DTOR_ATEXIT_FLAG(g_s64d);

static void hz3_s64d_atexit_dump(void) {
    uint32_t ticks = HZ3_DTOR_STAT_LOAD(g_s64d_tick_calls);
    uint32_t dumped = HZ3_DTOR_STAT_LOAD(g_s64d_objs_dumped);
    uint32_t budget_hits = HZ3_DTOR_STAT_LOAD(g_s64d_budget_hits);
    if (ticks > 0 || dumped > 0) {
        fprintf(stderr, "[HZ3_S64_TCACHE_DUMP] ticks=%u objs_dumped=%u budget_hits=%u\n",
                ticks, dumped, budget_hits);
    }
}
#endif

void hz3_s64_tcache_dump_tick(void) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    // small_v2 disabled: no-op
    return;
#else
    // TLS initialization guard (critical: epoch may be called before tcache init)
    if (!t_hz3_cache.initialized) {
        return;
    }

#if HZ3_S64_STATS
    HZ3_DTOR_STAT_INC(g_s64d_tick_calls);
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s64d, hz3_s64d_atexit_dump);
#endif

    uint8_t my_shard = t_hz3_cache.my_shard;
    uint32_t budget = t_hz3_cache.knobs.s64_tdump_budget_objs;
    uint32_t total_dumped = 0;

    for (int sc = 0; sc < HZ3_SMALL_NUM_SC && budget > 0; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        // SoA path: directly access local_head array
        uint32_t bin_idx = hz3_bin_index_small(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (!head) continue;

        // Budgeted walk: find tail and count up to budget
        void* tail = head;
        uint32_t n = 1;
        while (n < budget) {
#if HZ3_S64_TDUMP_GUARD
            if (!hz3_s64_tdump_guard_ptr_ok(my_shard, sc, tail, "tdump_walk_cur", tail, NULL)) {
                break;
            }
#endif
            void* next = hz3_obj_get_next(tail);
            if (!next) {
                break;
            }
#if HZ3_S64_TDUMP_GUARD
            if (!hz3_s64_tdump_guard_ptr_ok(my_shard, sc, next, "tdump_walk_next", tail, next)) {
                break;
            }
#endif
            tail = next;
            n++;
        }

        // Cut: save remaining, terminate dumped list
        void* remaining = hz3_obj_get_next(tail);
        hz3_obj_set_next(tail, NULL);

        // Push to central
        hz3_small_v2_central_push_list(my_shard, sc, head, tail, n);

        // Update local bin
#if HZ3_BIN_SPLIT_COUNT
        // S122: Get current count, subtract n, re-pack
        Hz3BinRef ref = hz3_tcache_get_local_binref(bin_idx);
        uint32_t cur = hz3_binref_count_get(ref);
        uint32_t new_count = (cur > n) ? (cur - n) : 0;
        uint32_t new_low = new_count & (uint32_t)HZ3_SPLIT_CNT_MASK;
        *ref.count = (Hz3BinCount)(new_count >> HZ3_SPLIT_CNT_SHIFT);
        *ref.head = hz3_split_pack(remaining, new_low);
#else
        t_hz3_cache.local_head[bin_idx] = remaining;
#if !HZ3_BIN_LAZY_COUNT
        t_hz3_cache.local_count[bin_idx] -= (Hz3BinCount)n;
#endif
#endif

#else  // AoS path
        Hz3Bin* bin = hz3_tcache_get_small_bin(sc);
        if (!bin->head) continue;

        // Budgeted walk: find tail and count up to budget
        void* head = bin->head;
        void* tail = head;
        uint32_t n = 1;
        while (n < budget) {
#if HZ3_S64_TDUMP_GUARD
            if (!hz3_s64_tdump_guard_ptr_ok(my_shard, sc, tail, "tdump_walk_cur", tail, NULL)) {
                break;
            }
#endif
            void* next = hz3_obj_get_next(tail);
            if (!next) {
                break;
            }
#if HZ3_S64_TDUMP_GUARD
            if (!hz3_s64_tdump_guard_ptr_ok(my_shard, sc, next, "tdump_walk_next", tail, next)) {
                break;
            }
#endif
            tail = next;
            n++;
        }

        // Cut: save remaining, terminate dumped list
        void* remaining = hz3_obj_get_next(tail);
        hz3_obj_set_next(tail, NULL);

        // Push to central
        hz3_small_v2_central_push_list(my_shard, sc, head, tail, n);

        // Update local bin
        bin->head = remaining;
#if !HZ3_BIN_LAZY_COUNT
        bin->count -= (Hz3BinCount)n;
#endif
#endif  // HZ3_TCACHE_SOA_LOCAL

        budget -= n;
        total_dumped += n;
    }

#if HZ3_S64_STATS
    if (total_dumped > 0) {
        HZ3_DTOR_STAT_ADD(g_s64d_objs_dumped, total_dumped);
    }
    if (budget == 0) {
        HZ3_DTOR_STAT_INC(g_s64d_budget_hits);
    }
#endif
#endif  // HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
}

#endif  // HZ3_S64_TCACHE_DUMP
