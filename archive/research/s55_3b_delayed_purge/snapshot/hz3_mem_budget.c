// ============================================================================
// S45: Memory Budget Box - Segment-level reclaim on arena exhaustion
// ============================================================================

#define _DEFAULT_SOURCE
#include "hz3_mem_budget.h"

#if HZ3_MEM_BUDGET_ENABLE

#include "hz3_arena.h"
#include "hz3_central.h"
#include "hz3_inbox.h"
#include "hz3_segmap.h"
#include "hz3_segment.h"
#include "hz3_tcache.h"
#include "hz3_types.h"
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif
#if HZ3_S55_3B_DELAYED_PURGE
#include "hz3_retention_policy.h"  // for t_repay_epoch_counter
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

// ============================================================================
// One-shot logging (event-only, not hot path)
// ============================================================================
#if HZ3_OOM_SHOT
#define HZ3_MEM_BUDGET_LOG(fmt, ...) fprintf(stderr, "[hz3_mem_budget] " fmt "\n", ##__VA_ARGS__)
#else
#define HZ3_MEM_BUDGET_LOG(fmt, ...) ((void)0)
#endif

// ----------------------------------------------------------------------------
// Emergency flush: move medium bins/inbox to central so reclaim can work.
// Event-only: called on arena exhaustion boundary.
// ----------------------------------------------------------------------------
void hz3_mem_budget_emergency_flush(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

    uint8_t shard = (uint8_t)t_hz3_cache.my_shard;

    // 1) Flush local medium bins to central (current thread only).
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        uint32_t bin_idx = hz3_bin_index_medium(sc);
        void* head = t_hz3_cache.local_head[bin_idx];
        if (!head) {
            continue;
        }
        void* tail = head;
        uint32_t n = 1;
        void* obj = hz3_obj_get_next(head);
        while (obj) {
            tail = obj;
            n++;
            obj = hz3_obj_get_next(obj);
        }
        hz3_central_push_list(shard, sc, head, tail, n);
        t_hz3_cache.local_head[bin_idx] = NULL;
#if !HZ3_BIN_LAZY_COUNT
        t_hz3_cache.local_count[bin_idx] = 0;
#endif
#else
        Hz3Bin* bin = hz3_tcache_get_bin(sc);
        if (hz3_bin_is_empty(bin)) {
            continue;
        }
        void* head = hz3_bin_take_all(bin);
        void* tail = head;
        uint32_t n = 1;
        void* obj = hz3_obj_get_next(head);
        while (obj) {
            tail = obj;
            n++;
            obj = hz3_obj_get_next(obj);
        }
        hz3_central_push_list(shard, sc, head, tail, n);
#endif
    }

    // 2) Drain inbox (medium) to central for this shard.
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        (void)hz3_inbox_drain_to_central(shard, sc);
    }
}

size_t hz3_mem_budget_reclaim_segments(size_t target_bytes) {
    // TLS initialization guard (current_seg/small_current_seg cleanup + owner safety).
    if (!t_hz3_cache.initialized) {
        return 0;
    }

    // CRITICAL: Only operate on my_shard. hz3_segment_free() tears down segmap + meta.
    // Freeing a segment belonging to another shard can race with its owner thread.
    int my_shard = t_hz3_cache.my_shard;
    size_t reclaimed = 0;

    // Get arena state via accessor API
    void* arena_base = hz3_arena_get_base();
    if (!arena_base) {
        return 0;
    }

    uint32_t slots = hz3_arena_slots();

    for (uint32_t i = 0; i < slots && reclaimed < target_bytes; i++) {
        // Skip if slot is not in use
        if (!hz3_arena_slot_used(i)) {
            continue;
        }

        // Calculate segment base address
        void* seg_base = (char*)arena_base + (size_t)i * HZ3_SEG_SIZE;

        // Get segment metadata from segmap
        Hz3SegMeta* meta = hz3_segmap_get(seg_base);
        if (!meta) {
            continue;  // Safety check: segmap entry not set
        }

        // my_shard only (safety): don't tear down other owners' segments
        if (meta->owner != my_shard) {
            continue;
        }

        // Only reclaim fully-free segments (all 512 pages free)
        if (meta->free_pages != HZ3_PAGES_PER_SEG) {
            continue;
        }

        // Skip if this is the current thread's active segment
        // (current_seg is Hz3SegMeta*, compare via seg_base field)
        if (t_hz3_cache.current_seg && t_hz3_cache.current_seg->seg_base == seg_base) {
            t_hz3_cache.current_seg = NULL;  // Clear stale reference
        }

        // Skip if this is the current thread's small_current_seg
        // (small_current_seg is Hz3SegHdr* at segment base)
        if (t_hz3_cache.small_current_seg && (void*)t_hz3_cache.small_current_seg == seg_base) {
            t_hz3_cache.small_current_seg = NULL;  // Clear stale reference
        }

#if HZ3_S49_SEGMENT_PACKING
        // S49: Remove from pack pool before freeing segment (meta will be unmapped).
        hz3_pack_on_drain((uint8_t)my_shard, meta);
#endif

        // Free the segment (releases arena slot, clears segmap, madvise DONTNEED)
        // Note: hz3_segment_free() acquires arena lock internally
        hz3_segment_free(seg_base);

        reclaimed += HZ3_SEG_SIZE;
    }

    if (reclaimed > 0) {
        HZ3_MEM_BUDGET_LOG("reclaim_segments: %zu bytes (%zu segments)",
                           reclaimed, reclaimed / HZ3_SEG_SIZE);
    }

    return reclaimed;
}

// ============================================================================
// Phase 2: Pop medium runs from central and return to segment
// ============================================================================
//
// Medium objects freed by user go to tcache/central, but don't call
// hz3_segment_free_run(). This means segment free_pages never increases.
// Phase 2 explicitly pops from central and returns runs to their segments.
//
// CRITICAL SAFETY CONSTRAINT: my_shard のみ操作
// ============================================================================
// hz3_segment_free_run() はロックを持たない。free_bits/free_pages を直接変更する。
// 他スレッドの shard に属する segment を触るとデータレースになる。
// したがって、my_shard に属する central pool のみを drain する。
// 他 shard の central は触らない。
//
// Safety checklist:
// 1. my_shard only - hz3_segment_free_run has no internal lock
// 2. t_hz3_cache.initialized guard - TLS must be ready
// 3. HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES - runaway prevention
// 4. page_idx + pages <= HZ3_PAGES_PER_SEG - boundary check
// 5. hz3_segmap_get() NULL check - external allocation guard

size_t hz3_mem_budget_reclaim_medium_runs(void) {
    // TLS initialization guard
    if (!t_hz3_cache.initialized) {
        return 0;
    }

    // CRITICAL: Only operate on my_shard. Other shards would cause data races.
    int my_shard = t_hz3_cache.my_shard;
    size_t reclaimed_pages = 0;
    size_t max_reclaim = HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES;

    // Iterate over medium size classes (0..7)
    for (int sc = 0; sc < HZ3_NUM_SC && reclaimed_pages < max_reclaim; sc++) {
        void* obj;
        while ((obj = hz3_central_pop(my_shard, sc)) != NULL) {
            // Object -> Segment/Page reversal (O(1))
            uintptr_t ptr = (uintptr_t)obj;
            uintptr_t seg_base = ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1);

            // Get segment metadata
            Hz3SegMeta* meta = hz3_segmap_get((void*)seg_base);
            if (!meta) {
                continue;  // Safety: external allocation or unmapped
            }

            // Calculate page index and pages for this size class
            uint32_t page_idx = (uint32_t)((ptr - seg_base) >> HZ3_PAGE_SHIFT);
            uint32_t pages = (uint32_t)(sc + 1);  // sc=0 -> 1 page (4KB), sc=7 -> 8 pages (32KB)

            // Boundary check
            if (page_idx + pages > HZ3_PAGES_PER_SEG) {
                continue;  // Safety: invalid run bounds
            }

            // Return run to segment (marks pages as free, increments free_pages)
            hz3_segment_free_run(meta, page_idx, pages);
#if HZ3_S49_SEGMENT_PACKING
            // S49: Update pack pool after free_run (may increase pack_max_fit)
            hz3_pack_on_free((uint8_t)my_shard, meta);
#endif
            reclaimed_pages += pages;

            if (reclaimed_pages >= max_reclaim) {
                break;  // Runaway prevention
            }
        }
    }

    if (reclaimed_pages > 0) {
        HZ3_MEM_BUDGET_LOG("reclaim_medium_runs: %zu pages (%zu KB) from shard %d",
                           reclaimed_pages, reclaimed_pages * 4, my_shard);
    }

    return reclaimed_pages;
}

// ============================================================================
// S55-3: Budgeted medium runs reclaim with subrelease
// ============================================================================

#if HZ3_S55_3_MEDIUM_SUBRELEASE

// Statistics (global atomic, cross-thread aggregation)
// - Pattern: S53/S54/S55-1 と同じく pthread_once + global _Atomic
static _Atomic uint64_t g_s55_3_calls = 0;
static _Atomic uint64_t g_s55_3_demote_pages = 0;
static _Atomic uint64_t g_s55_3_madvise_pages = 0;
static _Atomic uint64_t g_s55_3_madvise_calls = 0;
static _Atomic uint64_t g_s55_3_madvise_bytes = 0;

// ============================================================================
// S55-3B: TLS delayed purge queue (one per thread)
// ============================================================================

#if HZ3_S55_3B_DELAYED_PURGE
// TLS queue (SPSC pattern, one per thread)
// - CRITICAL: No cross-thread access (enqueue/process from same thread)
static __thread Hz3DelayedPurgeQueue t_hz3_delayed_purge;

// Statistics (global atomic, S55-3B specific)
static _Atomic uint64_t g_s55_3b_enqueued = 0;
static _Atomic uint64_t g_s55_3b_processed = 0;
static _Atomic uint64_t g_s55_3b_reused = 0;      // runs that were reused (discarded)
static _Atomic uint64_t g_s55_3b_madvised = 0;    // runs that were madvised
static _Atomic uint64_t g_s55_3b_dropped = 0;     // queue overflow drops
#endif

// atexit handler (one-shot via pthread_once)
static void hz3_s55_3_atexit(void) {
    uint64_t calls = atomic_load(&g_s55_3_calls);
    if (calls > 0) {
        fprintf(stderr, "[HZ3_S55_3_MEDIUM_SUBRELEASE] calls=%lu demote_pages=%lu madvise_calls=%lu madvise_pages=%lu madvise_bytes=%lu\n",
                calls,
                atomic_load(&g_s55_3_demote_pages),
                atomic_load(&g_s55_3_madvise_calls),
                atomic_load(&g_s55_3_madvise_pages),
                atomic_load(&g_s55_3_madvise_bytes));
    }
}

// pthread_once registration (one-shot, thread-safe)
static pthread_once_t s55_3_atexit_once = PTHREAD_ONCE_INIT;
static void hz3_s55_3_atexit_register(void) {
    atexit(hz3_s55_3_atexit);
}

size_t hz3_mem_budget_reclaim_medium_runs_subrelease(size_t max_reclaim_pages) {
    // TLS initialization guard
    if (!t_hz3_cache.initialized) {
        return 0;
    }

    // Register atexit handler once (pthread_once ensures thread-safety)
    pthread_once(&s55_3_atexit_once, hz3_s55_3_atexit_register);

    // CRITICAL: Only operate on my_shard
    int my_shard = t_hz3_cache.my_shard;
    size_t reclaimed_pages = 0;
    size_t madvised_pages = 0;
    size_t madvise_call_count = 0;

    // Update call count (atomic)
    atomic_fetch_add(&g_s55_3_calls, 1);

    // CRITICAL: Iterate over medium size classes in REVERSE order (large -> small)
    // - Reason: Reduces madvise syscall count by reclaiming larger runs first
    // - Example: sc=7 -> 8 pages (32KB), sc=0 -> 1 page (4KB)
    for (int sc = HZ3_NUM_SC - 1; sc >= 0 && reclaimed_pages < max_reclaim_pages; sc--) {
        // Break if MAX_CALLS limit reached (madvise syscall throttle)
        if (madvise_call_count >= HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS) {
            break;
        }

        void* obj;
        while ((obj = hz3_central_pop(my_shard, sc)) != NULL &&
               reclaimed_pages < max_reclaim_pages &&
               madvise_call_count < HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS) {
            // Object -> Segment/Page reversal (O(1))
            uintptr_t ptr = (uintptr_t)obj;
            uintptr_t seg_base = ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1);

            // Get segment metadata
            Hz3SegMeta* meta = hz3_segmap_get((void*)seg_base);
            if (!meta) {
                continue;  // Safety: external allocation or unmapped
            }

            // Calculate page index and pages for this size class
            uint32_t page_idx = (uint32_t)((ptr - seg_base) >> HZ3_PAGE_SHIFT);
            uint32_t pages = (uint32_t)(sc + 1);  // sc=7 -> 8 pages (32KB), sc=0 -> 1 page (4KB)

            // Boundary check
            if (page_idx + pages > HZ3_PAGES_PER_SEG) {
                continue;  // Safety: invalid run bounds
            }

            // Step 1: Return run to segment (marks pages as free, increments free_pages)
            hz3_segment_free_run(meta, page_idx, pages);
#if HZ3_S49_SEGMENT_PACKING
            // S49: Update pack pool after free_run
            hz3_pack_on_free((uint8_t)my_shard, meta);
#endif

            // Step 2: Enqueue for delayed madvise (S55-3B) OR immediate madvise (S55-3)
#if HZ3_S55_3B_DELAYED_PURGE
            // S55-3B: Enqueue run for delayed purge (TLS queue)
            Hz3DelayedPurgeQueue* q = &t_hz3_delayed_purge;  // TLS (not per-shard)
            uint32_t head = q->head;  // Non-atomic (TLS SPSC)
            uint32_t tail = q->tail;
            uint32_t size = HZ3_S55_3B_QUEUE_SIZE;
            uint32_t mask = size - 1;  // power-of-2 optimization

            // Check queue full
            if ((head - tail) >= size) {
                // Overflow: drop based on policy
#if HZ3_S55_3B_DROP_NEWEST_ON_OVERFLOW
                atomic_fetch_add(&g_s55_3b_dropped, 1);
                continue;  // Skip enqueue, drop newest
#else
                // Drop oldest: advance tail (FIFO eviction)
                q->tail = (uint32_t)(tail + 1);
                atomic_fetch_add(&g_s55_3b_dropped, 1);
#endif
            }

            // Bounds check: page_idx + pages <= HZ3_PAGES_PER_SEG
            if (page_idx + pages > HZ3_PAGES_PER_SEG) {
                // Safety: invalid run bounds, skip enqueue
                continue;
            }

            // Enqueue entry (stamp with current global epoch)
            // CRITICAL: Use hz3_repay_epoch_get() (global atomic) NOT t_repay_epoch_counter (TLS)
            uint32_t idx = head & mask;
            q->ring[idx] = (Hz3DelayedPurgeEntry){
                .seg_base = seg_base,
                .page_idx = (uint16_t)page_idx,
                .pages = (uint8_t)pages,
                ._pad = 0,
                .stamp_epoch = (uint32_t)hz3_repay_epoch_get()  // Global atomic epoch
            };
            q->head = (uint32_t)(head + 1);  // Non-atomic (TLS SPSC)
            atomic_fetch_add(&g_s55_3b_enqueued, 1);
#else
            // S55-3: Immediate madvise (original behavior)
            void* run_addr = (void*)(seg_base + ((uintptr_t)page_idx << HZ3_PAGE_SHIFT));
            size_t run_bytes = (size_t)pages << HZ3_PAGE_SHIFT;

            if (madvise(run_addr, run_bytes, MADV_DONTNEED) == 0) {
                madvised_pages += pages;
                atomic_fetch_add(&g_s55_3_madvise_bytes, run_bytes);
            } else {
#if HZ3_S55_3_MEDIUM_SUBRELEASE_FAILFAST
                perror("[HZ3_S55_3_MEDIUM_SUBRELEASE] madvise(MADV_DONTNEED) failed");
                abort();
#endif
            }

            madvise_call_count++;
#endif

            reclaimed_pages += pages;
        }
    }

    // Update global statistics (atomic)
    atomic_fetch_add(&g_s55_3_demote_pages, reclaimed_pages);
    atomic_fetch_add(&g_s55_3_madvise_pages, madvised_pages);
    atomic_fetch_add(&g_s55_3_madvise_calls, madvise_call_count);

    if (reclaimed_pages > 0) {
        HZ3_MEM_BUDGET_LOG("reclaim_medium_runs_subrelease: demote=%zu madvise_calls=%zu madvise_pages=%zu",
                           reclaimed_pages, madvise_call_count, madvised_pages);
    }

    return reclaimed_pages;
}

// ============================================================================
// S55-3B: Delayed Purge Processing (epoch boundary)
// ============================================================================

#if HZ3_S55_3B_DELAYED_PURGE

// atexit handler (S55-3B statistics)
static void hz3_s55_3b_atexit(void) {
    uint64_t enqueued = atomic_load(&g_s55_3b_enqueued);
    if (enqueued > 0) {
        fprintf(stderr, "[HZ3_S55_3B_DELAYED_PURGE] enqueued=%lu processed=%lu reused=%lu madvised=%lu dropped=%lu\n",
                enqueued,
                atomic_load(&g_s55_3b_processed),
                atomic_load(&g_s55_3b_reused),
                atomic_load(&g_s55_3b_madvised),
                atomic_load(&g_s55_3b_dropped));
    }
}

// pthread_once registration
static pthread_once_t s55_3b_atexit_once = PTHREAD_ONCE_INIT;
static void hz3_s55_3b_atexit_register(void) {
    atexit(hz3_s55_3b_atexit);
}

void hz3_delayed_purge_process(uint32_t current_epoch) {
    // TLS initialization guard
    if (!t_hz3_cache.initialized) {
        return;
    }

    // Register atexit handler once (pthread_once ensures thread-safety)
    pthread_once(&s55_3b_atexit_once, hz3_s55_3b_atexit_register);

    // CRITICAL: Use passed-in current_epoch (global atomic from caller)
    // - No global atomic++ here (avoids "time distortion")
    // - Caller (hz3_retention_repay_epoch) provides the authoritative epoch

    Hz3DelayedPurgeQueue* q = &t_hz3_delayed_purge;  // TLS queue

    uint32_t head = q->head;  // Non-atomic (TLS SPSC)
    uint32_t tail = q->tail;
    uint32_t mask = HZ3_S55_3B_QUEUE_SIZE - 1;

    size_t processed = 0;
    size_t reused = 0;
    size_t madvised_pages = 0;
    size_t madvise_call_count = 0;

    // Process queue entries (FIFO order, up to MAX_CALLS syscall limit)
    while (tail != head && madvise_call_count < HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS) {
        uint32_t idx = tail & mask;
        Hz3DelayedPurgeEntry* entry = &q->ring[idx];

        // Step 1: Age check (has delay_epochs passed?)
        uint32_t age = current_epoch - entry->stamp_epoch;
        if (age < HZ3_S55_3B_DELAY_EPOCHS) {
            break;  // Not old enough yet, stop processing (FIFO order)
        }

        // Step 2: Bounds check (paranoid safety)
        if (entry->page_idx + entry->pages > HZ3_PAGES_PER_SEG) {
            // Safety: invalid run bounds, discard entry
            tail = (uint32_t)(tail + 1);
            processed++;
            reused++;  // Count as reused (can't verify)
            continue;
        }

        // Step 3: Get segment metadata
        Hz3SegMeta* meta = hz3_segmap_get((void*)entry->seg_base);
        if (!meta) {
            // Segment was released (external or unmapped), discard entry
            tail = (uint32_t)(tail + 1);
            processed++;
            reused++;  // Count as reused (can't verify)
            continue;
        }

        // Step 4: Check if run is still free (bitmap check)
        // CRITICAL: This is the core of delayed purge - verify run wasn't reused
        // ASSUMPTION: free_bits is updated by my_shard only (scale lane: shards==threads)
        // - If shard collision possible, this is a data race
        // - Mitigation: scale lane enforces shards==threads, or add lock around this check
        int still_free = 1;
        uint32_t page_idx = entry->page_idx;
        uint32_t pages = entry->pages;

        for (uint32_t p = 0; p < pages; p++) {
            uint32_t pidx = page_idx + p;
            size_t word = pidx / 64;
            size_t bit = pidx % 64;
            if (!(meta->free_bits[word] & (1ULL << bit))) {
                // Page was reused (bit=0 means used)
                still_free = 0;
                break;
            }
        }

        // Step 5: Decision based on still_free check
        if (still_free) {
            // Run is still free → madvise to OS
            void* run_addr = (void*)(entry->seg_base + ((uintptr_t)page_idx << HZ3_PAGE_SHIFT));
            size_t run_bytes = (size_t)pages << HZ3_PAGE_SHIFT;

            if (madvise(run_addr, run_bytes, MADV_DONTNEED) == 0) {
                madvised_pages += pages;
                atomic_fetch_add(&g_s55_3_madvise_bytes, run_bytes);
                atomic_fetch_add(&g_s55_3b_madvised, 1);
            } else {
#if HZ3_S55_3_MEDIUM_SUBRELEASE_FAILFAST
                perror("[HZ3_S55_3B_DELAYED_PURGE] madvise(MADV_DONTNEED) failed");
                abort();
#endif
            }
            madvise_call_count++;
        } else {
            // Run was reused → discard entry (don't madvise)
            reused++;
            atomic_fetch_add(&g_s55_3b_reused, 1);
        }

        // Step 6: Dequeue entry (advance tail)
        tail = (uint32_t)(tail + 1);
        processed++;
    }

    // Update queue tail
    q->tail = tail;

    // Update global statistics
    atomic_fetch_add(&g_s55_3b_processed, processed);
    atomic_fetch_add(&g_s55_3_madvise_pages, madvised_pages);
    atomic_fetch_add(&g_s55_3_madvise_calls, madvise_call_count);

    // Log only if progress made (throttled: max 1 per second, same pattern as S55-3)
    if (processed > 0) {
        static _Atomic uint64_t g_last_purge_log_ns = 0;
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
        uint64_t last_ns = atomic_load_explicit(&g_last_purge_log_ns, memory_order_relaxed);

        if (now_ns - last_ns > 1000000000ULL) {  // 1 second
            if (atomic_compare_exchange_strong_explicit(
                    &g_last_purge_log_ns, &last_ns, now_ns,
                    memory_order_relaxed, memory_order_relaxed)) {
                fprintf(stderr, "[HZ3_S55_3B_DELAYED_PURGE] processed=%zu reused=%zu madvised_pages=%zu madvise_calls=%zu\n",
                        processed, reused, madvised_pages, madvise_call_count);
            }
        }
    }
}

#endif  // HZ3_S55_3B_DELAYED_PURGE

#endif  // HZ3_S55_3_MEDIUM_SUBRELEASE

// ============================================================================
// S45-FOCUS: Focused reclaim - concentrate on ONE segment
// ============================================================================
//
// Problem: reclaim_medium_runs scatters reclaimed pages across many segments.
// No single segment reaches free_pages == 512 (fully free).
//
// Solution: Focus reclaim on ONE segment:
// 1. Sample objects from central (multiple size classes)
// 2. Group by seg_base, pick best (most pages_sum + owner match)
// 3. Return only best seg's runs to segment, push others back to central
// 4. If free_pages == 512, call hz3_segment_free() to release arena slot
//
// Safety:
// - my_shard only: hz3_segment_free_run() has no internal lock
// - owner check: skip segments owned by other shards
// - ptr page boundary assertion: runs must be page-aligned
// ============================================================================

#if HZ3_S45_FOCUS_RECLAIM

// Data structure for sampled objects (stack-allocated)
typedef struct {
    void*     ptr;
    int       sc;
    uint16_t  pages;
    uintptr_t seg_base;
} FocusSample;

// Data structure for segment grouping (stack-allocated)
typedef struct {
    uintptr_t seg_base;
    uint16_t  pages_sum;
} FocusSegInfo;

Hz3FocusProgress hz3_mem_budget_reclaim_focus_one_segment(void) {
    Hz3FocusProgress progress = {0, 0, 0};

    // TLS initialization guard
    if (!t_hz3_cache.initialized) {
        return progress;
    }

    int my_shard = t_hz3_cache.my_shard;

    // Step 1: Sample objects from central pool
    FocusSample samples[HZ3_FOCUS_MAX_OBJS];
    int n_samples = 0;

    for (int sc = 0; sc < HZ3_NUM_SC && n_samples < HZ3_FOCUS_MAX_OBJS; sc++) {
        while (n_samples < HZ3_FOCUS_MAX_OBJS) {
            void* ptr = hz3_central_pop(my_shard, sc);
            if (!ptr) break;

            uintptr_t seg_base = (uintptr_t)ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1);

            // Assert: ptr must be page-aligned (run head)
            assert(((uintptr_t)ptr & (HZ3_PAGE_SIZE - 1)) == 0);

            samples[n_samples++] = (FocusSample){
                .ptr = ptr,
                .sc = sc,
                .pages = (uint16_t)(sc + 1),  // sc=0 -> 1 page, sc=7 -> 8 pages
                .seg_base = seg_base
            };
        }
    }

    progress.sampled = (uint16_t)n_samples;

    if (n_samples == 0) {
        return progress;
    }

    // Step 2: Group by seg_base and compute pages_sum
    FocusSegInfo segs[HZ3_FOCUS_MAX_SEGS];
    int n_segs = 0;

    for (int i = 0; i < n_samples; i++) {
        uintptr_t sb = samples[i].seg_base;
        int found = -1;
        for (int j = 0; j < n_segs; j++) {
            if (segs[j].seg_base == sb) {
                found = j;
                break;
            }
        }
        if (found >= 0) {
            segs[found].pages_sum += samples[i].pages;
        } else if (n_segs < HZ3_FOCUS_MAX_SEGS) {
            segs[n_segs++] = (FocusSegInfo){
                .seg_base = sb,
                .pages_sum = samples[i].pages
            };
        }
    }

    // Step 3: Select best segment (most pages_sum + owner == my_shard)
    int best_idx = -1;
    for (int i = 0; i < n_segs; i++) {
        Hz3SegMeta* meta = hz3_segmap_get((void*)segs[i].seg_base);
        if (!meta) continue;
        if (meta->owner != my_shard) continue;  // Owner check: skip other shards
        if (best_idx < 0 || segs[i].pages_sum > segs[best_idx].pages_sum) {
            best_idx = i;
        }
    }

    if (best_idx < 0) {
        // No valid best segment found -> push all samples back to central
        for (int i = 0; i < n_samples; i++) {
            hz3_central_push(my_shard, samples[i].sc, samples[i].ptr);
        }
        HZ3_MEM_BUDGET_LOG("reclaim_focus: no valid segment (owner mismatch or unmapped)");
        return progress;
    }

    uintptr_t best_seg_base = segs[best_idx].seg_base;
    Hz3SegMeta* best_meta = hz3_segmap_get((void*)best_seg_base);

    // Step 4: Return best seg's runs to segment, push others back to central
    uint16_t focused_pages = 0;
    for (int i = 0; i < n_samples; i++) {
        if (samples[i].seg_base == best_seg_base) {
            uintptr_t ptr_addr = (uintptr_t)samples[i].ptr;
            uint32_t page_idx = (uint32_t)((ptr_addr - best_seg_base) >> HZ3_PAGE_SHIFT);
            hz3_segment_free_run(best_meta, page_idx, samples[i].pages);
            focused_pages += samples[i].pages;
        } else {
            hz3_central_push(my_shard, samples[i].sc, samples[i].ptr);
        }
    }

#if HZ3_S49_SEGMENT_PACKING
    // S49: Update pack pool after focused reclaim
    hz3_pack_on_free((uint8_t)my_shard, best_meta);
#endif

    progress.freed_pages = focused_pages;

    HZ3_MEM_BUDGET_LOG("reclaim_focus: seg=%p focused=%u pages, free_pages=%u",
                       (void*)best_seg_base, focused_pages, best_meta->free_pages);

    // Step 5: If free_pages == 512, release the segment
    if (best_meta->free_pages == HZ3_PAGES_PER_SEG) {
        // Clear stale references (seg_base comparison)
        if (t_hz3_cache.current_seg &&
            t_hz3_cache.current_seg->seg_base == (void*)best_seg_base) {
            t_hz3_cache.current_seg = NULL;
        }
        if (t_hz3_cache.small_current_seg == (void*)best_seg_base) {
            t_hz3_cache.small_current_seg = NULL;
        }

#if HZ3_S49_SEGMENT_PACKING
        // S49: Remove from pack pool before freeing segment
        hz3_pack_on_drain((uint8_t)my_shard, best_meta);
#endif
        hz3_segment_free((void*)best_seg_base);
        progress.freed_bytes = HZ3_SEG_SIZE;
        HZ3_MEM_BUDGET_LOG("reclaim_focus: segment freed, %zu bytes reclaimed", (size_t)HZ3_SEG_SIZE);
    }

    return progress;
}

#else  // !HZ3_S45_FOCUS_RECLAIM

Hz3FocusProgress hz3_mem_budget_reclaim_focus_one_segment(void) {
    return (Hz3FocusProgress){0, 0, 0};
}

#endif  // HZ3_S45_FOCUS_RECLAIM

// ============================================================================
// S55-2B: Budgeted segment scan for epoch repay
// ============================================================================

// Cursor for round-robin scan (global, atomic, advances across all calls)
static _Atomic uint32_t g_reclaim_cursor = 0;

size_t hz3_mem_budget_reclaim_segments_budget(size_t target_bytes, uint32_t scan_slots_budget) {
    // TLS initialization guard (same as reclaim_segments)
    if (!t_hz3_cache.initialized) {
        return 0;
    }

    // CRITICAL: Only operate on my_shard (same safety as reclaim_segments)
    int my_shard = t_hz3_cache.my_shard;

    // Get arena state
    void* arena_base = hz3_arena_get_base();
    if (!arena_base) {
        return 0;
    }

    uint32_t total_slots = hz3_arena_total_slots();
    size_t reclaimed = 0;
    uint32_t scanned = 0;
    uint32_t cursor = atomic_load_explicit(&g_reclaim_cursor, memory_order_relaxed);

    while (reclaimed < target_bytes && scanned < scan_slots_budget) {
        uint32_t slot = cursor % total_slots;
        cursor++;
        scanned++;

        // Check if slot is used
        if (!hz3_arena_slot_used(slot)) {
            continue;
        }

        // Calculate segment base address
        void* seg_base = (char*)arena_base + (size_t)slot * HZ3_SEG_SIZE;

        // Get segment metadata from segmap
        Hz3SegMeta* meta = hz3_segmap_get(seg_base);
        if (!meta) {
            continue;  // Safety check: segmap entry not set
        }

        // my_shard only (safety): don't tear down other owners' segments
        if (meta->owner != my_shard) {
            continue;
        }

        // Check if fully free
        if (meta->free_pages == HZ3_PAGES_PER_SEG) {
            hz3_segment_free(seg_base);
            reclaimed += HZ3_SEG_SIZE;
        }
    }

    // Update cursor for next call
    atomic_store_explicit(&g_reclaim_cursor, cursor, memory_order_relaxed);
    return reclaimed;
}

#endif  // HZ3_MEM_BUDGET_ENABLE
