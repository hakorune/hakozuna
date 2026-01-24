#define _GNU_SOURCE

#include "hz3_purge_range_queue.h"

#if HZ3_S60_PURGE_RANGE_QUEUE

#include "hz3_types.h"
#include "hz3_os_purge.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// S60: PurgeRangeQueueBox Implementation
// ============================================================================

// TLS access
extern HZ3_TLS Hz3TCache t_hz3_cache;

// ============================================================================
// Stats (global, atomic for multi-thread safety)
// ============================================================================
#if HZ3_S60_STATS
static _Atomic(uint32_t) g_s60_queued_total;
static _Atomic(uint32_t) g_s60_dropped_total;
static _Atomic(uint32_t) g_s60_madvise_calls;
static _Atomic(uint32_t) g_s60_madvised_pages;
static _Atomic(uint32_t) g_s60_skipped_arena;
static _Atomic(uint32_t) g_s60_budget_dropped;

static int g_s60_atexit_registered = 0;

static void hz3_s60_atexit_dump(void) {
    uint32_t queued = atomic_load_explicit(&g_s60_queued_total, memory_order_relaxed);
    uint32_t dropped = atomic_load_explicit(&g_s60_dropped_total, memory_order_relaxed);
    uint32_t calls = atomic_load_explicit(&g_s60_madvise_calls, memory_order_relaxed);
    uint32_t pages = atomic_load_explicit(&g_s60_madvised_pages, memory_order_relaxed);
    uint32_t skipped = atomic_load_explicit(&g_s60_skipped_arena, memory_order_relaxed);
    uint32_t budget_drop = atomic_load_explicit(&g_s60_budget_dropped, memory_order_relaxed);

    if (queued > 0 || calls > 0) {
        fprintf(stderr, "[HZ3_S60_PURGE] queued=%u dropped=%u madvise_calls=%u "
                        "madvised_pages=%u skipped_arena=%u budget_dropped=%u\n",
                queued, dropped, calls, pages, skipped, budget_drop);
    }
}
#endif

// ============================================================================
// Queue operations
// ============================================================================

void hz3_s60_queue_range(void* seg_base, uint16_t page_idx, uint16_t pages) {
    Hz3PurgeRangeQueue* q = &t_hz3_cache.purge_queue;

    uint16_t h = q->head;
    uint16_t next_h = (h + 1) & (HZ3_S60_QUEUE_SIZE - 1);

    // Overflow: drop (no carryover, will be drained this epoch anyway)
    if (__builtin_expect(next_h == q->tail, 0)) {
        q->dropped++;
#if HZ3_S60_STATS
        atomic_fetch_add_explicit(&g_s60_dropped_total, 1, memory_order_relaxed);
#endif
        return;
    }

    // Push entry
    q->entries[h].seg_base = seg_base;
    q->entries[h].page_idx = page_idx;
    q->entries[h].pages = pages;
    q->head = next_h;
    q->queued++;

#if HZ3_S60_STATS
    atomic_fetch_add_explicit(&g_s60_queued_total, 1, memory_order_relaxed);

    // Register atexit once
    if (__builtin_expect(!g_s60_atexit_registered, 0)) {
        g_s60_atexit_registered = 1;
        atexit(hz3_s60_atexit_dump);
    }
#endif
}

void hz3_s60_purge_tick(void) {
    Hz3PurgeRangeQueue* q = &t_hz3_cache.purge_queue;

    uint16_t t = q->tail;
    uint16_t h = q->head;

    // Empty queue
    if (t == h) return;

    uint32_t calls = 0;
    uint32_t pages_total = 0;

    // Drain with budget limits
    while (t != h) {
        Hz3PurgeRangeEntry* entry = &q->entries[t];

        // Calculate address and length
        void* addr = (char*)entry->seg_base + ((size_t)entry->page_idx << HZ3_PAGE_SHIFT);
        size_t len = (size_t)entry->pages << HZ3_PAGE_SHIFT;

        // Check if within budget
        if (calls >= HZ3_S60_BUDGET_CALLS || pages_total >= HZ3_S60_BUDGET_PAGES) {
            // Budget exceeded: drop remaining entries (stats only)
#if HZ3_S60_STATS
            uint32_t remaining = 0;
            uint16_t tmp = t;
            while (tmp != h) {
                remaining++;
                tmp = (tmp + 1) & (HZ3_S60_QUEUE_SIZE - 1);
            }
            atomic_fetch_add_explicit(&g_s60_budget_dropped, remaining, memory_order_relaxed);
#endif
            break;  // Drop rest, no carryover
        }

        // Arena bounds check
        if (!hz3_os_in_arena_range(addr, len)) {
#if HZ3_S60_STATS
            atomic_fetch_add_explicit(&g_s60_skipped_arena, 1, memory_order_relaxed);
#endif
            t = (t + 1) & (HZ3_S60_QUEUE_SIZE - 1);
            continue;
        }

        // Apply madvise
        madvise(addr, len, MADV_DONTNEED);

        calls++;
        pages_total += entry->pages;

#if HZ3_S60_STATS
        atomic_fetch_add_explicit(&g_s60_madvise_calls, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s60_madvised_pages, entry->pages, memory_order_relaxed);
#endif

        t = (t + 1) & (HZ3_S60_QUEUE_SIZE - 1);
    }

    // Queue is now empty (no carryover - remaining entries dropped)
    q->tail = q->head;
}

#endif  // HZ3_S60_PURGE_RANGE_QUEUE
