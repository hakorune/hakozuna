#define _GNU_SOURCE

#include "hz3_s62_observe.h"

#if HZ3_S62_OBSERVE

#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_arena.h"
#include "hz3_seg_hdr.h"
#include "hz3_dtor_stats.h"
#include "hz3_small_v2.h"  // S135-1B: for hz3_small_v2_page_try_get_sc, hz3_s69_live_count_read, hz3_small_v2_page_capacity

// ============================================================================
// S62-0: OBSERVE - count potential purge candidates from free_bits
// ============================================================================
//
// This is a stats-only pass: scan arena for small segments owned by my_shard,
// count pages marked free in free_bits. Does NOT call madvise.
//
// Purpose: measure current free_bits purge potential.
// Expected result: free_pages ≈ 0 (all pages allocated), confirming need for S62-1 PageRetireBox.

// TLS access
extern HZ3_TLS Hz3TCache t_hz3_cache;

// ============================================================================
// Stats (global, atomic for multi-thread safety)
// ============================================================================
HZ3_DTOR_STAT(g_s62_dtor_calls);
HZ3_DTOR_STAT(g_s62_scanned_segs);
HZ3_DTOR_STAT(g_s62_candidate_segs);
HZ3_DTOR_STAT(g_s62_total_pages);
HZ3_DTOR_STAT(g_s62_free_pages);
HZ3_DTOR_STAT(g_s62_used_pages);

HZ3_DTOR_ATEXIT_FLAG(g_s62);

#if HZ3_S135_PARTIAL_SC_OBS
// S135-1B: Per-sizeclass partial page stats (global, atomic)
static _Atomic uint32_t g_s135_partial_count[HZ3_SMALL_NUM_SC];
static _Atomic uint32_t g_s135_total_count[HZ3_SMALL_NUM_SC];
static _Atomic uint32_t g_s135_empty_count[HZ3_SMALL_NUM_SC];
static _Atomic uint32_t g_s135_full_count[HZ3_SMALL_NUM_SC];
// Occupancy histogram (5 buckets per SC)
static _Atomic uint32_t g_s135_occ_buckets[HZ3_SMALL_NUM_SC][5];
#endif

#if HZ3_S135_FULL_SC_OBS
// S135-1C: Per-sizeclass waste stats (global, atomic)
static _Atomic uint64_t g_s135c_tail_waste_sum[HZ3_SMALL_NUM_SC];    // Total tail waste (all pages)
static _Atomic uint32_t g_s135c_tail_waste_count[HZ3_SMALL_NUM_SC];  // Count of pages
static _Atomic uint64_t g_s135c_full_waste_sum[HZ3_SMALL_NUM_SC];    // Tail waste on full pages
static _Atomic uint32_t g_s135c_full_count[HZ3_SMALL_NUM_SC];        // Count of full pages
#endif

static void hz3_s62_snapshot_best_effort(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

    uint8_t my_shard = t_hz3_cache.my_shard;

    // Get arena info
    void* arena_base = hz3_arena_get_base();
    if (!arena_base) {
        return;  // Arena not initialized
    }
    uint32_t slots = hz3_arena_slots();

    // Local stats for this snapshot
    uint32_t scanned_segs = 0;
    uint32_t candidate_segs = 0;
    uint32_t total_pages = 0;
    uint32_t free_pages = 0;
    uint32_t used_pages = 0;

    // Scan arena slots (small segments only; skip page 0 which is header)
    for (uint32_t i = 0; i < slots; i++) {
        if (!hz3_arena_slot_used(i)) {
            continue;
        }
        scanned_segs++;

        void* slot_base = (char*)arena_base + ((size_t)i << HZ3_SEG_SHIFT);
        Hz3SegHdr* hdr = (Hz3SegHdr*)slot_base;

        if (hdr->magic != HZ3_SEG_HDR_MAGIC) {
            continue;
        }
        if (hdr->kind != HZ3_SEG_KIND_SMALL) {
            continue;
        }
        if (hdr->owner != my_shard) {
            continue;
        }

        candidate_segs++;

        for (int page = 1; page < HZ3_PAGES_PER_SEG; page++) {
            total_pages++;
            int word = page / 64;
            int bit = page % 64;
            if (hdr->free_bits[word] & (1ULL << bit)) {
                free_pages++;
            } else {
                used_pages++;
            }

#if HZ3_S135_PARTIAL_SC_OBS
            // S135-1B: Classify page as empty/partial/full
            void* page_base = (char*)slot_base + ((size_t)page << HZ3_PAGE_SHIFT);

            // Get size class via safe boundary helper
            uint8_t sc = 0;
            if (!hz3_small_v2_page_try_get_sc(page_base, &sc)) {
                continue;  // Skip uninitialized/corrupted pages
            }

            // Get live_count
            uint16_t live = hz3_s69_live_count_read(page_base);

            // Get capacity
            size_t capacity = hz3_small_v2_page_capacity((int)sc);
            if (capacity == 0) {
                continue;  // Skip if capacity is 0 (avoid division by zero)
            }

            // Track total pages for this SC (atomic add to global stats)
            atomic_fetch_add_explicit(&g_s135_total_count[sc], 1, memory_order_relaxed);

            // Classify page state
            if (live == 0) {
                atomic_fetch_add_explicit(&g_s135_empty_count[sc], 1, memory_order_relaxed);
            } else if (live >= capacity) {  // Use >= for safety (live should never exceed capacity)
                atomic_fetch_add_explicit(&g_s135_full_count[sc], 1, memory_order_relaxed);

#if HZ3_S135_FULL_SC_OBS
                // S135-1C: Track tail waste on full pages
                size_t tail_waste = hz3_small_v2_page_tail_waste((int)sc);
                atomic_fetch_add_explicit(&g_s135c_full_waste_sum[sc], tail_waste, memory_order_relaxed);
                atomic_fetch_add_explicit(&g_s135c_full_count[sc], 1, memory_order_relaxed);
#endif
            } else {
                // Partial page: 0 < live < capacity
                atomic_fetch_add_explicit(&g_s135_partial_count[sc], 1, memory_order_relaxed);

                // Occupancy histogram (5 buckets)
                uint32_t percent = (live * 100) / (uint32_t)capacity;
                int bucket = (percent < 100) ? (percent / 20) : 4;  // 0-4
                atomic_fetch_add_explicit(&g_s135_occ_buckets[sc][bucket], 1, memory_order_relaxed);
            }

#if HZ3_S135_FULL_SC_OBS
            // S135-1C: Track tail waste across all pages (for average calculation)
            // Note: tail_waste is constant per SC, but we accumulate per-page for verification
            size_t tail_waste = hz3_small_v2_page_tail_waste((int)sc);
            atomic_fetch_add_explicit(&g_s135c_tail_waste_sum[sc], tail_waste, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s135c_tail_waste_count[sc], 1, memory_order_relaxed);
#endif

#endif  // HZ3_S135_PARTIAL_SC_OBS
        }
    }

    if (scanned_segs > 0) {
        HZ3_DTOR_STAT_ADD(g_s62_scanned_segs, scanned_segs);
    }
    if (candidate_segs > 0) {
        HZ3_DTOR_STAT_ADD(g_s62_candidate_segs, candidate_segs);
        HZ3_DTOR_STAT_ADD(g_s62_total_pages, total_pages);
        HZ3_DTOR_STAT_ADD(g_s62_free_pages, free_pages);
        HZ3_DTOR_STAT_ADD(g_s62_used_pages, used_pages);
    }
}

static void hz3_s62_atexit_dump(void) {
    uint32_t dtor_calls = HZ3_DTOR_STAT_LOAD(g_s62_dtor_calls);
    uint32_t scanned_segs = HZ3_DTOR_STAT_LOAD(g_s62_scanned_segs);
    uint32_t candidate_segs = HZ3_DTOR_STAT_LOAD(g_s62_candidate_segs);
    uint32_t total_pages = HZ3_DTOR_STAT_LOAD(g_s62_total_pages);
    uint32_t free_pages = HZ3_DTOR_STAT_LOAD(g_s62_free_pages);
    uint32_t used_pages = HZ3_DTOR_STAT_LOAD(g_s62_used_pages);

    // Best-effort snapshot: short, single-thread benchmarks may exit without running
    // thread destructors, which means hz3_s62_observe() never ran.
    if (dtor_calls == 0 && candidate_segs == 0) {
        hz3_s62_snapshot_best_effort();
        dtor_calls = HZ3_DTOR_STAT_LOAD(g_s62_dtor_calls);
        scanned_segs = HZ3_DTOR_STAT_LOAD(g_s62_scanned_segs);
        candidate_segs = HZ3_DTOR_STAT_LOAD(g_s62_candidate_segs);
        total_pages = HZ3_DTOR_STAT_LOAD(g_s62_total_pages);
        free_pages = HZ3_DTOR_STAT_LOAD(g_s62_free_pages);
        used_pages = HZ3_DTOR_STAT_LOAD(g_s62_used_pages);
    }

    if (dtor_calls > 0 || candidate_segs > 0) {
        uint64_t purge_potential_bytes = (uint64_t)free_pages * HZ3_PAGE_SIZE;
        fprintf(stderr, "[HZ3_S62_OBSERVE] dtor_calls=%u scanned_segs=%u candidate_segs=%u "
                        "total_pages=%u free_pages=%u used_pages=%u purge_potential_bytes=%lu\n",
                dtor_calls, scanned_segs, candidate_segs,
                total_pages, free_pages, used_pages, (unsigned long)purge_potential_bytes);

#if HZ3_S135_PARTIAL_SC_OBS
        // S135-1B: Find top 5 size classes by partial page count
        typedef struct {
            uint8_t sc;
            uint32_t partial;
            uint32_t total;
            uint32_t empty;
            uint32_t full;
            uint32_t occ[5];  // Occupancy histogram
        } ScPartialStats;

        ScPartialStats top5[5] = {{0}};  // Top 5 by partial count
        int top5_count = 0;

        // O(HZ3_SMALL_NUM_SC × 5) selection: iterate all SCs, maintain top 5
        for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
            uint32_t partial = atomic_load_explicit(&g_s135_partial_count[sc], memory_order_relaxed);
            if (partial == 0) {
                continue;  // Skip SCs with no partial pages
            }

            ScPartialStats current = {
                .sc = (uint8_t)sc,
                .partial = partial,
                .total = atomic_load_explicit(&g_s135_total_count[sc], memory_order_relaxed),
                .empty = atomic_load_explicit(&g_s135_empty_count[sc], memory_order_relaxed),
                .full = atomic_load_explicit(&g_s135_full_count[sc], memory_order_relaxed),
                .occ = {
                    atomic_load_explicit(&g_s135_occ_buckets[sc][0], memory_order_relaxed),
                    atomic_load_explicit(&g_s135_occ_buckets[sc][1], memory_order_relaxed),
                    atomic_load_explicit(&g_s135_occ_buckets[sc][2], memory_order_relaxed),
                    atomic_load_explicit(&g_s135_occ_buckets[sc][3], memory_order_relaxed),
                    atomic_load_explicit(&g_s135_occ_buckets[sc][4], memory_order_relaxed)
                }
            };

            // Insert into top5 (sorted by partial count descending)
            int insert_pos = -1;
            for (int i = 0; i < 5; i++) {
                if (i >= top5_count || current.partial > top5[i].partial) {
                    insert_pos = i;
                    break;
                }
            }

            if (insert_pos >= 0) {
                // Shift elements down
                for (int i = 4; i > insert_pos; i--) {
                    if (i - 1 < top5_count) {
                        top5[i] = top5[i - 1];
                    }
                }
                top5[insert_pos] = current;
                if (top5_count < 5) {
                    top5_count++;
                }
            }
        }

        // S135-1B: Report top 5 size classes by partial page count
        if (top5_count > 0) {
            fprintf(stderr, "[HZ3_S135_PARTIAL_SC] top_sc=");
            for (int i = 0; i < top5_count; i++) {
                fprintf(stderr, "%s%u", (i > 0 ? "," : ""), top5[i].sc);
            }
            fprintf(stderr, " pages=");
            for (int i = 0; i < top5_count; i++) {
                fprintf(stderr, "%s%u/%u/%u/%u", (i > 0 ? "," : ""),
                        top5[i].partial, top5[i].total, top5[i].empty, top5[i].full);
            }
            fprintf(stderr, " occ=");
            for (int i = 0; i < top5_count; i++) {
                fprintf(stderr, "%s[%u;%u;%u;%u;%u]", (i > 0 ? "," : ""),
                        top5[i].occ[0], top5[i].occ[1], top5[i].occ[2],
                        top5[i].occ[3], top5[i].occ[4]);
            }
            fprintf(stderr, "\n");
        } else {
            fprintf(stderr, "[HZ3_S135_PARTIAL_SC] top_sc= pages= occ= (no partial pages)\n");
        }
#endif  // HZ3_S135_PARTIAL_SC_OBS

#if HZ3_S135_FULL_SC_OBS
    // S135-1C: Report top 5 size classes by total tail waste
    // Sorting strategy: total_waste (tail_waste × total_pages) measures "total impact"
    //   - High total_waste = many pages × high waste/page = big optimization opportunity
    //   - avg_tail / avg_full = "design efficiency" (waste per page)
    //   - Use total_waste for ranking, avg_* for analysis
    typedef struct {
        uint8_t sc;
        uint64_t total_waste;      // tail_waste_sum
        uint32_t total_pages;      // tail_waste_count
        uint64_t full_waste;       // full_waste_sum
        uint32_t full_pages;       // full_count
        uint32_t avg_tail_waste;   // total_waste / total_pages
        uint32_t avg_full_waste;   // full_waste / full_pages (0 if full_pages==0)
    } ScWasteStats;

    ScWasteStats top5_waste[5] = {{0}};  // Top 5 by total_waste
    int top5_waste_count = 0;

    // O(HZ3_SMALL_NUM_SC × 5) selection: iterate all SCs, maintain top 5 by total_waste
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        uint64_t total_waste = atomic_load_explicit(&g_s135c_tail_waste_sum[sc], memory_order_relaxed);
        if (total_waste == 0) {
            continue;  // Skip SCs with no tail waste
        }

        uint32_t total_pages = atomic_load_explicit(&g_s135c_tail_waste_count[sc], memory_order_relaxed);
        uint64_t full_waste = atomic_load_explicit(&g_s135c_full_waste_sum[sc], memory_order_relaxed);
        uint32_t full_pages = atomic_load_explicit(&g_s135c_full_count[sc], memory_order_relaxed);

        ScWasteStats current = {
            .sc = (uint8_t)sc,
            .total_waste = total_waste,
            .total_pages = total_pages,
            .full_waste = full_waste,
            .full_pages = full_pages,
            .avg_tail_waste = (uint32_t)(total_pages > 0 ? total_waste / total_pages : 0),
            .avg_full_waste = (uint32_t)(full_pages > 0 ? full_waste / full_pages : 0),
        };

        // Insert into top5 (sorted by total_waste descending)
        int insert_pos = -1;
        for (int i = 0; i < 5; i++) {
            if (i >= top5_waste_count || current.total_waste > top5_waste[i].total_waste) {
                insert_pos = i;
                break;
            }
        }

        if (insert_pos >= 0) {
            // Shift elements down
            for (int i = 4; i > insert_pos; i--) {
                if (i - 1 < top5_waste_count) {
                    top5_waste[i] = top5_waste[i - 1];
                }
            }
            top5_waste[insert_pos] = current;
            if (top5_waste_count < 5) {
                top5_waste_count++;
            }
        }
    }

    // S135-1C: Output top 5 size classes by tail waste
    if (top5_waste_count > 0) {
        fprintf(stderr, "[HZ3_S135_FULL_SC] top_sc=");
        for (int i = 0; i < top5_waste_count; i++) {
            fprintf(stderr, "%s%u", (i > 0 ? "," : ""), top5_waste[i].sc);
        }
        fprintf(stderr, " total_waste=");
        for (int i = 0; i < top5_waste_count; i++) {
            fprintf(stderr, "%s%lu", (i > 0 ? "," : ""), (unsigned long)top5_waste[i].total_waste);
        }
        fprintf(stderr, " total_pages=");
        for (int i = 0; i < top5_waste_count; i++) {
            fprintf(stderr, "%s%u", (i > 0 ? "," : ""), top5_waste[i].total_pages);
        }
        fprintf(stderr, " full_waste=");
        for (int i = 0; i < top5_waste_count; i++) {
            fprintf(stderr, "%s%lu", (i > 0 ? "," : ""), (unsigned long)top5_waste[i].full_waste);
        }
        fprintf(stderr, " full_pages=");
        for (int i = 0; i < top5_waste_count; i++) {
            fprintf(stderr, "%s%u", (i > 0 ? "," : ""), top5_waste[i].full_pages);
        }
        fprintf(stderr, " avg_tail=");
        for (int i = 0; i < top5_waste_count; i++) {
            fprintf(stderr, "%s%u", (i > 0 ? "," : ""), top5_waste[i].avg_tail_waste);
        }
        fprintf(stderr, " avg_full=");
        for (int i = 0; i < top5_waste_count; i++) {
            fprintf(stderr, "%s%u", (i > 0 ? "," : ""), top5_waste[i].avg_full_waste);
        }
        fprintf(stderr, "\n");
    } else {
        fprintf(stderr, "[HZ3_S135_FULL_SC] top_sc= total_waste= (no tail waste)\n");
    }
#endif  // HZ3_S135_FULL_SC_OBS
    }
}

void hz3_s62_observe_register_atexit(void) {
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s62, hz3_s62_atexit_dump);
}

// ============================================================================
// Main implementation
// ============================================================================

void hz3_s62_observe(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

    HZ3_DTOR_STAT_INC(g_s62_dtor_calls);
    hz3_s62_observe_register_atexit();

    uint8_t my_shard = t_hz3_cache.my_shard;

    (void)my_shard;
    hz3_s62_snapshot_best_effort();
}

#endif  // HZ3_S62_OBSERVE
