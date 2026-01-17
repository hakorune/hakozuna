#define _GNU_SOURCE

#include "hz3_s62_sub4k.h"

#if HZ3_S62_SUB4K_RETIRE

#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_seg_hdr.h"
#include "hz3_segment.h"
#include "hz3_sub4k.h"
#include "hz3_dtor_stats.h"
#include "hz3_release_boundary.h"
#include "hz3_arena.h"
#include <string.h>

// ============================================================================
// S62-1b: Sub4kRunRetireBox - retire fully-empty sub4k runs to free_bits
// ============================================================================
//
// Algorithm (O(n)):
// 1. For each size class, pop objects from central bin in batches
// 2. Group objects per-run in a hash table (using run_start_bits to find run base)
// 3. When a run reaches full capacity, retire it immediately:
//    - mark free_bits/free_pages for 2 pages
//    - clear run_start bit
//    - drop the run's object list (do not push back to central)
// 4. After draining central, push remaining (non-retired) run lists back to central
//
// Safety checks:
// - Cannot determine run_start → skip (fail-safe)
// - Verify Hz3SegHdr magic/kind/owner before touching free_bits
// - Skip page 0 (header page)
//
// Scope: sub4k only (2-page runs)

extern __thread Hz3TCache t_hz3_cache;

// Stats (atomic for multi-thread safety)
HZ3_DTOR_STAT(g_s62s_dtor_calls);
HZ3_DTOR_STAT(g_s62s_pages_retired);
HZ3_DTOR_STAT(g_s62s_objs_processed);

HZ3_DTOR_ATEXIT_FLAG(g_s62s);

// Run tracking: smaller table since sub4k has fewer runs per segment
#define S62S_RUN_TABLE_SIZE 256
#define S62S_BATCH_SIZE     256

typedef struct {
    uintptr_t run_base;     // run start address (2-page aligned)
    uint16_t  count;        // objects in this run
    uint8_t   retired;      // 1 = retired
    void*     head;         // per-run object list head
    void*     tail;         // per-run object list tail
} S62SRunEntry;

static void hz3_s62s_atexit_dump(void) {
    uint32_t calls = HZ3_DTOR_STAT_LOAD(g_s62s_dtor_calls);
    uint32_t retired = HZ3_DTOR_STAT_LOAD(g_s62s_pages_retired);
    uint32_t processed = HZ3_DTOR_STAT_LOAD(g_s62s_objs_processed);
    if (calls > 0 || retired > 0) {
        fprintf(stderr, "[HZ3_S62_SUB4K_RETIRE] dtor_calls=%u pages_retired=%u objs_processed=%u\n",
                calls, retired, processed);
    }
}

// run_start 判定: obj の page から run_start を逆算
// Returns run_base (run開始アドレス) or 0 if not determinable
static inline uintptr_t s62s_find_run_base(const void* obj, const Hz3SegHdr* hdr) {
    uintptr_t addr = (uintptr_t)obj;
    uintptr_t seg_base = (uintptr_t)hdr;
    size_t page_idx = (addr - seg_base) >> HZ3_PAGE_SHIFT;

    // Check if this page is run_start
    size_t word = page_idx / 64;
    size_t bit = page_idx % 64;
    if (hdr->sub4k_run_start_bits[word] & (1ULL << bit)) {
        // This page is run_start
        return seg_base + (page_idx << HZ3_PAGE_SHIFT);
    }

    // Check if previous page is run_start
    if (page_idx > 0) {
        size_t prev_idx = page_idx - 1;
        size_t prev_word = prev_idx / 64;
        size_t prev_bit = prev_idx % 64;
        if (hdr->sub4k_run_start_bits[prev_word] & (1ULL << prev_bit)) {
            return seg_base + (prev_idx << HZ3_PAGE_SHIFT);
        }
    }

    // Cannot determine run_start → return 0 (skip)
    return 0;
}

// run_base から run_table の index を探す（linear probe）
static inline size_t s62s_run_table_find(S62SRunEntry* table, uintptr_t run_base) {
    size_t idx = (run_base >> (HZ3_PAGE_SHIFT + 1)) % S62S_RUN_TABLE_SIZE;
    while (table[idx].run_base != 0 && table[idx].run_base != run_base) {
        idx = (idx + 1) % S62S_RUN_TABLE_SIZE;
    }
    return idx;
}

void hz3_s62_sub4k_retire(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

    HZ3_DTOR_STAT_INC(g_s62s_dtor_calls);
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s62s, hz3_s62s_atexit_dump);

    uint8_t my_shard = t_hz3_cache.my_shard;
    uint32_t total_processed = 0;
    uint32_t pages_retired = 0;

    // Stack-local buffers (no static/global to avoid race with concurrent dtors)
    void* batch_buf[S62S_BATCH_SIZE];
    S62SRunEntry run_table[S62S_RUN_TABLE_SIZE];

    // Process each sub4k size class
    for (int sc = 0; sc < HZ3_SUB4K_NUM_SC; sc++) {
        // 1. Initialize run table
        memset(run_table, 0, sizeof(run_table));
        size_t capacity = hz3_sub4k_run_capacity(sc);
        if (capacity == 0) {
            continue;
        }

        // 2. Drain central in batches, build per-run lists
        //    S62-1 同様「count 到達時に即 retire」で count>capacity 異常を検出
        uint32_t got;
        while ((got = hz3_sub4k_central_pop_batch_ext(my_shard, sc, batch_buf, S62S_BATCH_SIZE)) > 0) {
            for (uint32_t i = 0; i < got; i++) {
                void* obj = batch_buf[i];

                // Find segment header
                uintptr_t seg_base = (uintptr_t)obj & ~(HZ3_SEG_SIZE - 1);
                Hz3SegHdr* hdr = (Hz3SegHdr*)seg_base;

                // Safety check
                if (hdr->magic != HZ3_SEG_HDR_MAGIC ||
                    hdr->kind != HZ3_SEG_KIND_SMALL ||
                    hdr->owner != my_shard) {
                    continue;
                }

                // Find run_base using run_start_bits
                uintptr_t run_base = s62s_find_run_base(obj, hdr);
                if (run_base == 0) {
                    continue;  // Cannot determine → skip
                }

                size_t idx = s62s_run_table_find(run_table, run_base);
                S62SRunEntry* ent = &run_table[idx];
                if (ent->run_base == 0) {
                    ent->run_base = run_base;
                }

                // Skip if already retired (count > capacity の重複検出)
                if (ent->retired) {
                    continue;
                }

                // Add to per-run list
                hz3_obj_set_next(obj, NULL);
                if (!ent->head) {
                    ent->head = obj;
                    ent->tail = obj;
                } else {
                    hz3_obj_set_next(ent->tail, obj);
                    ent->tail = obj;
                }
                ent->count++;
                total_processed++;

                // 即時 retire 判定（S62-1 と同じパターン）
                if (ent->count == capacity) {
                    size_t start_page = (run_base - seg_base) >> HZ3_PAGE_SHIFT;

                    // Safety checks: skip header page (start_page == 0)
                    if (start_page != 0 &&
                        start_page + 1 < HZ3_PAGES_PER_SEG) {
                        int retire_ok = 0;

#if HZ3_S65_RELEASE_BOUNDARY
                        // S65: Use unified boundary API for free_bits updates.
                        int ret = hz3_release_range_definitely_free(hdr, (uint32_t)start_page, 2,
                                                                     HZ3_RELEASE_SUB4K_RUN_RETIRE);
                        if (ret == 0) {
                            retire_ok = 1;
                        }
#else
                        // Retire: mark 2 pages as free
                        hz3_bitmap_mark_free(hdr->free_bits, start_page, 2);
                        hdr->free_pages += 2;
                        retire_ok = 1;
#endif

                        if (retire_ok) {
                            // Clear run_start bit
                            {
                                size_t word = start_page / 64;
                                size_t bit = start_page % 64;
                                hdr->sub4k_run_start_bits[word] &= ~(1ULL << bit);
                            }

#if !HZ3_S65_RELEASE_BOUNDARY
                            // PTAG clear at retire boundary (when boundary API is disabled).
                            // Keep PageTagMap consistent: retired run pages must be "not ours" (tag=0).
#if (HZ3_SMALL_V2_PTAG_ENABLE || HZ3_PTAG_DSTBIN_ENABLE)
                            {
                                uint32_t base_page_idx = 0;
                                if (hz3_arena_page_index_fast((void*)run_base, &base_page_idx)) {
#if HZ3_SMALL_V2_PTAG_ENABLE
                                    if (g_hz3_page_tag) {
                                        hz3_pagetag_clear(base_page_idx);
                                        hz3_pagetag_clear(base_page_idx + 1u);
                                    }
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
                                    if (g_hz3_page_tag32) {
                                        hz3_pagetag32_clear(base_page_idx);
                                        hz3_pagetag32_clear(base_page_idx + 1u);
                                    }
#endif
                                }
                            }
#endif
#if HZ3_S110_META_ENABLE
                            // S110: clear page_bin_plus1 for the 2 sub4k run pages
                            atomic_store_explicit(&hdr->page_bin_plus1[start_page], 0, memory_order_release);
                            atomic_store_explicit(&hdr->page_bin_plus1[start_page + 1], 0, memory_order_release);
#endif
#endif

                            pages_retired += 2;
                            ent->retired = 1;
                            ent->head = NULL;  // Drop list
                            ent->tail = NULL;
                        }
                    }
                }
            }
        }

        // 3. Push back non-retired runs
        for (size_t i = 0; i < S62S_RUN_TABLE_SIZE; i++) {
            S62SRunEntry* ent = &run_table[i];
            if (ent->run_base == 0 || ent->retired || !ent->head) {
                continue;
            }
            hz3_sub4k_central_push_list_ext(my_shard, sc, ent->head, ent->tail, ent->count);
        }
    }

    // Aggregate stats
    if (total_processed > 0) {
        HZ3_DTOR_STAT_ADD(g_s62s_objs_processed, total_processed);
    }
    if (pages_retired > 0) {
        HZ3_DTOR_STAT_ADD(g_s62s_pages_retired, pages_retired);
    }
}

#endif  // HZ3_S62_SUB4K_RETIRE
