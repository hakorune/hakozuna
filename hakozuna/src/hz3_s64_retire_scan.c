#define _GNU_SOURCE

#include "hz3_s64_retire_scan.h"

#if HZ3_S64_RETIRE_SCAN

#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_seg_hdr.h"
#include "hz3_segment.h"
#include "hz3_small_v2.h"
#include "hz3_small.h"
#include "hz3_s64_purge_delay.h"
#include "hz3_dtor_stats.h"
#include "hz3_release_boundary.h"
#include "hz3_small_xfer.h"      // S71: for hz3_small_xfer_has_pending
#include "hz3_owner_stash.h"     // S71: for hz3_owner_stash_has_pending
#include <string.h>

// S64-D: DuplicateGuardBox (debug-only)
// Detect duplicate object pointers while retire-scan is rewriting next pointers.
// Duplicates imply double-free / double-push / cross-structure contamination and
// can turn into non-deterministic crashes (delayed corruption).
#ifndef HZ3_S64_DUP_GUARD
#define HZ3_S64_DUP_GUARD 0
#endif

#ifndef HZ3_S64_DUP_GUARD_FAILFAST
#define HZ3_S64_DUP_GUARD_FAILFAST HZ3_S64_DUP_GUARD
#endif

#ifndef HZ3_S64_DUP_GUARD_SHOT
#define HZ3_S64_DUP_GUARD_SHOT 1
#endif

#ifndef HZ3_S64_DUP_GUARD_TABLE_SIZE
// Must be power of 2. 2048 keys ~= 16KB stack (uintptr_t).
#define HZ3_S64_DUP_GUARD_TABLE_SIZE 2048
#endif

#if HZ3_S64_DUP_GUARD
#if HZ3_S64_DUP_GUARD_SHOT
static _Atomic int g_s64_dup_guard_shot = 0;
#else
static _Atomic int g_s64_dup_guard_shot __attribute__((unused)) = 0;
#endif

static inline int hz3_s64_dup_guard_should_log(void) {
#if HZ3_S64_DUP_GUARD_SHOT
    return (atomic_exchange_explicit(&g_s64_dup_guard_shot, 1, memory_order_relaxed) == 0);
#else
    return 1;
#endif
}

static inline int hz3_s64_dup_guard_insert(uintptr_t* table, uintptr_t key) {
    // NOTE: key is a pointer; 0 is reserved for empty.
    size_t mask = (HZ3_S64_DUP_GUARD_TABLE_SIZE - 1u);
    size_t idx = (key >> 4) & mask;
    for (;;) {
        uintptr_t cur = table[idx];
        if (cur == 0) {
            table[idx] = key;
            return 0;  // inserted
        }
        if (cur == key) {
            return 1;  // duplicate
        }
        idx = (idx + 1) & mask;
    }
}
#endif  // HZ3_S64_DUP_GUARD

// S64-E: CentralPopGuardBox (debug-only)
// Validate that objects popped from central belong to (owner, sc) and have a non-zero tag.
// This aims to fail-fast before delayed crashes when S64 rewrites next pointers.
#ifndef HZ3_S64_POP_GUARD
#define HZ3_S64_POP_GUARD 0
#endif

#ifndef HZ3_S64_POP_GUARD_FAILFAST
#define HZ3_S64_POP_GUARD_FAILFAST HZ3_S64_POP_GUARD
#endif

#ifndef HZ3_S64_POP_GUARD_SHOT
#define HZ3_S64_POP_GUARD_SHOT 1
#endif

#if HZ3_S64_POP_GUARD
#if HZ3_S64_POP_GUARD_SHOT
static _Atomic int g_s64_pop_guard_shot = 0;
#else
static _Atomic int g_s64_pop_guard_shot __attribute__((unused)) = 0;
#endif

static inline int hz3_s64_pop_guard_should_log(void) {
#if HZ3_S64_POP_GUARD_SHOT
    return (atomic_exchange_explicit(&g_s64_pop_guard_shot, 1, memory_order_relaxed) == 0);
#else
    return 1;
#endif
}
#endif  // HZ3_S64_POP_GUARD

// S64-F: TickTraceBox (debug-only, one-shot)
#ifndef HZ3_S64_TICK_TRACE
#define HZ3_S64_TICK_TRACE 0
#endif
#ifndef HZ3_S64_TICK_TRACE_SHOT
#define HZ3_S64_TICK_TRACE_SHOT 1
#endif
#if HZ3_S64_TICK_TRACE
#if HZ3_S64_TICK_TRACE_SHOT
static _Atomic int g_s64_tick_trace_shot = 0;
static _Atomic int g_s64_tick_trace_done_shot = 0;
#else
static _Atomic int g_s64_tick_trace_shot __attribute__((unused)) = 0;
static _Atomic int g_s64_tick_trace_done_shot __attribute__((unused)) = 0;
#endif
static inline int hz3_s64_tick_trace_should_log(void) {
#if HZ3_S64_TICK_TRACE_SHOT
    return (atomic_exchange_explicit(&g_s64_tick_trace_shot, 1, memory_order_relaxed) == 0);
#else
    return 1;
#endif
}
static inline int hz3_s64_tick_trace_done_should_log(void) {
#if HZ3_S64_TICK_TRACE_SHOT
    return (atomic_exchange_explicit(&g_s64_tick_trace_done_shot, 1, memory_order_relaxed) == 0);
#else
    return 1;
#endif
}
#endif

// ============================================================================
// S64-A: RetireScanBox - epoch-based scan for empty pages
// ============================================================================
//
// Algorithm (similar to S62):
// 1. For each size class, pop objects from central bin in budgeted batches
// 2. Group objects per-page in a hash table (count + list head/tail)
// 3. When a page reaches full capacity, retire it immediately:
//    - mark free_bits/free_pages
//    - enqueue to PurgeDelayBox
//    - drop the page's object list (do not push back to central)
// 4. Push remaining (non-retired) page lists back to central
//
// Key difference from S62:
// - Called every epoch tick (not just thread-exit)
// - Budget-limited (HZ3_S64_RETIRE_BUDGET_OBJS per tick)
// - Enqueues to PurgeDelayBox instead of immediate purge

extern HZ3_TLS Hz3TCache t_hz3_cache;

// Stats (atomic for multi-thread safety)
#if HZ3_S64_STATS
HZ3_DTOR_STAT(g_s64r_tick_calls);
HZ3_DTOR_STAT(g_s64r_pages_retired);
HZ3_DTOR_STAT(g_s64r_objs_processed);

HZ3_DTOR_ATEXIT_FLAG(g_s64r);

static void hz3_s64r_atexit_dump(void) {
    uint32_t calls = HZ3_DTOR_STAT_LOAD(g_s64r_tick_calls);
    uint32_t retired = HZ3_DTOR_STAT_LOAD(g_s64r_pages_retired);
    uint32_t processed = HZ3_DTOR_STAT_LOAD(g_s64r_objs_processed);
    if (calls > 0 || retired > 0) {
        fprintf(stderr, "[HZ3_S64_RETIRE_SCAN] ticks=%u pages_retired=%u objs_processed=%u\n",
                calls, retired, processed);
    }
}
#endif

// Page tracking: stack-local for concurrent tick safety
#define S64R_PAGE_TABLE_SIZE 256
#define S64R_BATCH_SIZE      64

typedef struct {
    uintptr_t page_base;
    uint16_t  count;
    uint8_t   retired;  // 1 = retired, 0 = not retired
    void*     head;     // per-page object list head
    void*     tail;     // per-page object list tail
} S64RPageEntry;

// Find page_base in page_table using linear probe
static inline size_t s64r_page_table_find(S64RPageEntry* table, uintptr_t page_base) {
    size_t idx = (page_base >> HZ3_PAGE_SHIFT) % S64R_PAGE_TABLE_SIZE;
    while (table[idx].page_base != 0 && table[idx].page_base != page_base) {
        idx = (idx + 1) % S64R_PAGE_TABLE_SIZE;
    }
    return idx;
}

// S70: Helper to check retire condition
// Requires BOTH:
//   (a) count==capacity: all objects from this page are in central (not in tcache/xfer)
//   (b) live_count==0 (S69): no objects held by user (prevents race with user operations)
// Removing (a) caused corruption - objects in tcache would be orphaned when page retired.
static inline int s64r_should_retire(S64RPageEntry* ent, size_t capacity) {
    // (a) All objects from page must be collected in central
    if (ent->count != capacity) {
        return 0;
    }
#if HZ3_S69_LIVECOUNT
    // (b) S69: additionally require live_count==0 (user holds no references)
    uint16_t live = hz3_s69_live_count_read((void*)ent->page_base);
    if (live != 0) {
        hz3_s69_retire_blocked_inc();  // Stats: track blocked retires
        return 0;
    }
#endif
    return 1;
}

void hz3_s64_retire_scan_tick(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

#if HZ3_S64_TICK_TRACE
    if (hz3_s64_tick_trace_should_log()) {
        fprintf(stderr, "[HZ3_S64_TICK] owner=%u epoch=%u budget=%u\n",
                (unsigned)t_hz3_cache.my_shard,
                (unsigned)t_hz3_cache.epoch_counter,
                (unsigned)HZ3_S64_RETIRE_BUDGET_OBJS);
    }
#endif

#if HZ3_S64_STATS
    HZ3_DTOR_STAT_INC(g_s64r_tick_calls);
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s64r, hz3_s64r_atexit_dump);
#endif

    uint8_t my_shard = t_hz3_cache.my_shard;
    // CollisionGuardBox: RetireScanBox assumes "single owner thread per shard".
    // Under shard collision, retirement can orphan free objects still held in other
    // colliding threads' TLS bins (not centralized yet). Correctness > RSS: skip.
    if (hz3_shard_live_count(my_shard) > 1) {
        return;
    }
    uint32_t total_processed = 0;
    uint32_t pages_retired = 0;
    uint32_t budget_remaining = HZ3_S64_RETIRE_BUDGET_OBJS;

    // Stack-local buffers (no static/global to avoid race with concurrent ticks)
    void* batch_buf[S64R_BATCH_SIZE];
    S64RPageEntry page_table[S64R_PAGE_TABLE_SIZE];
#if HZ3_S64_DUP_GUARD
    uintptr_t dup_table[HZ3_S64_DUP_GUARD_TABLE_SIZE];
    memset(dup_table, 0, sizeof(dup_table));
#endif

    // Process each size class (stop if budget exhausted)
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC && budget_remaining > 0; sc++) {
        // S71: Skip if xfer/stash has pending objects for this SC
        // Objects in transit haven't reached central yet, so count==capacity is unreliable
#if HZ3_S42_SMALL_XFER
        if (hz3_small_xfer_has_pending(my_shard, sc)) {
            continue;
        }
#endif
#if HZ3_S44_OWNER_STASH
        if (hz3_owner_stash_has_pending(my_shard, sc)) {
            continue;
        }
#endif

        // Initialize page table for this size class
        memset(page_table, 0, sizeof(page_table));
        size_t capacity = hz3_small_v2_page_capacity(sc);
        if (capacity == 0) {
            continue;
        }

        // Pop from central in batches, build per-page lists
        uint32_t want = (budget_remaining < S64R_BATCH_SIZE) ? budget_remaining : S64R_BATCH_SIZE;
        uint32_t got;
        while (budget_remaining > 0 &&
               (got = hz3_small_v2_central_pop_batch_ext(my_shard, sc, batch_buf, want)) > 0) {
            total_processed += got;
            budget_remaining = (budget_remaining > got) ? (budget_remaining - got) : 0;

            for (uint32_t i = 0; i < got; i++) {
                void* obj = batch_buf[i];
#if HZ3_S64_POP_GUARD
                if (obj) {
                    uint32_t tag32 = 0;
                    int in_range = 0;
                    (void)hz3_pagetag32_lookup_fast(obj, &tag32, &in_range);

                    uint32_t page_idx = 0;
                    int have_page_idx = hz3_arena_page_index_fast(obj, &page_idx);

                    if (!in_range) {
                        if (hz3_s64_pop_guard_should_log()) {
                            fprintf(stderr,
                                    "[HZ3_S64_POP_GUARD] where=central_pop_out_of_arena owner=%u sc=%d obj=%p\n",
                                    (unsigned)my_shard, sc, obj);
                        }
#if HZ3_S64_POP_GUARD_FAILFAST
                        abort();
#endif
                    } else if (tag32 == 0) {
                        if (hz3_s64_pop_guard_should_log()) {
                            fprintf(stderr,
                                    "[HZ3_S64_POP_GUARD] where=central_pop_ptag32_zero owner=%u sc=%d obj=%p page_idx=%u have_page_idx=%d\n",
                                    (unsigned)my_shard, sc, obj, (unsigned)page_idx, have_page_idx);
                        }
#if HZ3_S64_POP_GUARD_FAILFAST
                        abort();
#endif
                    } else {
                        uint32_t bin = hz3_pagetag32_bin(tag32);
                        uint8_t dst = hz3_pagetag32_dst(tag32);
                        if (bin != (uint32_t)hz3_bin_index_small(sc) || dst != my_shard) {
                            if (hz3_s64_pop_guard_should_log()) {
                                fprintf(stderr,
                                        "[HZ3_S64_POP_GUARD] where=central_pop_tag_mismatch owner=%u sc=%d obj=%p page_idx=%u have_page_idx=%d want_bin=%u tag32=0x%x bin=%u dst=%u\n",
                                        (unsigned)my_shard, sc, obj, (unsigned)page_idx, have_page_idx,
                                        (unsigned)hz3_bin_index_small(sc), tag32, (unsigned)bin, (unsigned)dst);
                            }
#if HZ3_S64_POP_GUARD_FAILFAST
                            abort();
#endif
                        }
                    }
                }
#endif  // HZ3_S64_POP_GUARD
#if HZ3_S64_DUP_GUARD
                // Detect duplicate pointers across all objects processed in this tick.
                // This catches the earliest concrete corruption signal before delayed crashes.
                if (obj) {
                    uintptr_t key = (uintptr_t)obj;
                    if (hz3_s64_dup_guard_insert(dup_table, key)) {
                        if (hz3_s64_dup_guard_should_log()) {
                            fprintf(stderr,
                                    "[HZ3_S64_DUP_GUARD] where=central_pop_dup owner=%u sc=%d obj=%p\n",
                                    (unsigned)my_shard, sc, obj);
                        }
#if HZ3_S64_DUP_GUARD_FAILFAST
                        abort();
#endif
                    }
                }
#endif
                uintptr_t page_base = (uintptr_t)obj & ~(HZ3_PAGE_SIZE - 1);

                size_t idx = s64r_page_table_find(page_table, page_base);
                S64RPageEntry* ent = &page_table[idx];

                if (ent->page_base == 0) {
                    ent->page_base = page_base;
                    ent->count = 0;
                    ent->retired = 0;
                    ent->head = NULL;
                    ent->tail = NULL;
                }

                if (ent->retired) {
                    // Page already retired in this pass: drop the object.
                    continue;
                }

                // Append to per-page list
                hz3_obj_set_next(obj, NULL);
                if (!ent->head) {
                    ent->head = obj;
                    ent->tail = obj;
                } else {
                    hz3_obj_set_next(ent->tail, obj);
                    ent->tail = obj;
                }

                ent->count++;

#if HZ3_S64_DUP_GUARD
                // Per-page sanity: count must never exceed capacity (duplicates from same page).
                if (ent->count > capacity) {
                    if (hz3_s64_dup_guard_should_log()) {
                        fprintf(stderr,
                                "[HZ3_S64_DUP_GUARD] where=page_overflow owner=%u sc=%d page=%p count=%u cap=%u\n",
                                (unsigned)my_shard, sc, (void*)ent->page_base,
                                (unsigned)ent->count, (unsigned)capacity);
                    }
#if HZ3_S64_DUP_GUARD_FAILFAST
                    abort();
#endif
                }
#endif
                if (s64r_should_retire(ent, capacity)) {
                    // Retire: all objects for this page are collected (+ live_count==0 if S69)
                    uintptr_t seg_base = page_base & ~(HZ3_SEG_SIZE - 1);
                    Hz3SegHdr* hdr = (Hz3SegHdr*)seg_base;
                    size_t page_idx = (page_base - seg_base) >> HZ3_PAGE_SHIFT;

                    // Safety: skip header page and verify segment header/owner
                    if (page_idx != 0 &&
                        hdr->magic == HZ3_SEG_HDR_MAGIC &&
                        hdr->kind == HZ3_SEG_KIND_SMALL &&
                        hdr->owner == my_shard) {
#if HZ3_S65_RELEASE_BOUNDARY
                        // S65: Use unified boundary API for free_bits updates.
                        // Ledger handles purge if HZ3_S65_RELEASE_LEDGER=1.
                        int ret = hz3_release_range_definitely_free(hdr, (uint32_t)page_idx, 1,
                                                                     HZ3_RELEASE_SMALL_PAGE_RETIRE);
                        if (ret == 0) {
                            pages_retired++;
                            ent->retired = 1;
                        }
#else
                        hz3_bitmap_mark_free(hdr->free_bits, page_idx, 1);
                        hdr->free_pages++;
                        pages_retired++;
                        ent->retired = 1;

                        // Enqueue to PurgeDelayBox for delayed madvise
                        hz3_s64_purge_delay_enqueue((void*)seg_base, (uint16_t)page_idx, 1);
#endif
                    }

                    // Drop the page list either way
                    if (ent->retired) {
                        ent->head = NULL;
                        ent->tail = NULL;
                    }
                }
            }

            // Update want for next iteration
            want = (budget_remaining < S64R_BATCH_SIZE) ? budget_remaining : S64R_BATCH_SIZE;
        }

        // Push back all non-retired page lists
        for (size_t i = 0; i < S64R_PAGE_TABLE_SIZE; i++) {
            S64RPageEntry* ent = &page_table[i];
            if (!ent->page_base || ent->retired || !ent->head) {
                continue;
            }
            hz3_small_v2_central_push_list(my_shard, sc, ent->head, ent->tail, ent->count);
        }
    }

#if HZ3_S64_TICK_TRACE
    if (hz3_s64_tick_trace_done_should_log()) {
        fprintf(stderr, "[HZ3_S64_TICK_DONE] owner=%u processed=%u pages_retired=%u budget_left=%u\n",
                (unsigned)my_shard, (unsigned)total_processed, (unsigned)pages_retired,
                (unsigned)budget_remaining);
    }
#endif

#if HZ3_S64_STATS
    if (total_processed > 0) {
        HZ3_DTOR_STAT_ADD(g_s64r_objs_processed, total_processed);
    }
    if (pages_retired > 0) {
        HZ3_DTOR_STAT_ADD(g_s64r_pages_retired, pages_retired);
    }
#endif
}

#endif  // HZ3_S64_RETIRE_SCAN
