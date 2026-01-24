// hz3_release_ledger.c - S65 ReleaseLedgerBox implementation
//
// TLS ring buffer with range coalescing for deferred madvise(DONTNEED).

#define _GNU_SOURCE

#include "hz3_release_ledger.h"

#if HZ3_S65_RELEASE_LEDGER

#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_os_purge.h"
#include "hz3_dtor_stats.h"
#include "hz3_seg_hdr.h"
#include "hz3_segmap.h"

#include <stdio.h>
#include <stdatomic.h>

// ============================================================================
// TLS Data
// ============================================================================

extern HZ3_TLS Hz3TCache t_hz3_cache;

// TLS ledger (separate from Hz3TCache)
static HZ3_TLS Hz3S65ReleaseLedger t_s65_ledger;
static HZ3_TLS int t_s65_ledger_init = 0;

// ============================================================================
// Stats (global atomics)
// ============================================================================

#if HZ3_S65_STATS
static _Atomic(uint64_t) g_s65_ledger_enqueued = 0;
static _Atomic(uint64_t) g_s65_ledger_coalesced = 0;
static _Atomic(uint64_t) g_s65_ledger_purged_pages = 0;
static _Atomic(uint64_t) g_s65_ledger_madvise_calls = 0;
static _Atomic(uint64_t) g_s65_ledger_dropped = 0;
static _Atomic(uint64_t) g_s65_ledger_skip_used = 0;
static _Atomic(uint64_t) g_s65_ledger_idle_ticks = 0;
static _Atomic(uint64_t) g_s65_ledger_busy_ticks = 0;

static _Atomic(int) g_s65_ledger_atexit_registered = 0;

static void s65_ledger_atexit_dump(void) {
    uint64_t enq = atomic_load_explicit(&g_s65_ledger_enqueued, memory_order_relaxed);
    uint64_t coal = atomic_load_explicit(&g_s65_ledger_coalesced, memory_order_relaxed);
    uint64_t purged = atomic_load_explicit(&g_s65_ledger_purged_pages, memory_order_relaxed);
    uint64_t calls = atomic_load_explicit(&g_s65_ledger_madvise_calls, memory_order_relaxed);
    uint64_t dropped = atomic_load_explicit(&g_s65_ledger_dropped, memory_order_relaxed);
    uint64_t skip_used = atomic_load_explicit(&g_s65_ledger_skip_used, memory_order_relaxed);
    uint64_t idle = atomic_load_explicit(&g_s65_ledger_idle_ticks, memory_order_relaxed);
    uint64_t busy = atomic_load_explicit(&g_s65_ledger_busy_ticks, memory_order_relaxed);

    if (enq > 0 || purged > 0) {
        fprintf(stderr,
                "[HZ3_S65] enq=%lu coalesce=%lu purged_pages=%lu madvise_calls=%lu\n"
                "  idle_ticks=%lu busy_ticks=%lu drop=%lu skip_used=%lu\n",
                enq, coal, purged, calls, idle, busy, dropped, skip_used);
    }
}

static void s65_stats_register_atexit(void) {
    int expected = 0;
    if (atomic_compare_exchange_strong_explicit(&g_s65_ledger_atexit_registered,
                                                 &expected, 1,
                                                 memory_order_relaxed,
                                                 memory_order_relaxed)) {
        atexit(s65_ledger_atexit_dump);
    }
}
#endif

// ============================================================================
// Ledger Initialization
// ============================================================================

static inline void s65_ledger_ensure_init(void) {
    if (__builtin_expect(!t_s65_ledger_init, 0)) {
        t_s65_ledger.head = 0;
        t_s65_ledger.tail = 0;
        t_s65_ledger.enqueued = 0;
        t_s65_ledger.coalesced = 0;
        t_s65_ledger.purged_pages = 0;
        t_s65_ledger.madvise_calls = 0;
        t_s65_ledger.dropped = 0;
        t_s65_ledger.idle_ticks = 0;
        t_s65_ledger.busy_ticks = 0;
        t_s65_ledger_init = 1;
    }
}

// ============================================================================
// Ring Buffer Helpers
// ============================================================================

static inline int s65_ledger_is_full(Hz3S65ReleaseLedger* l) {
    return ((l->tail + 1) & HZ3_S65_LEDGER_MASK) == l->head;
}

static inline int s65_ledger_is_empty(Hz3S65ReleaseLedger* l) {
    return l->head == l->tail;
}

static inline uint16_t s65_ledger_len(Hz3S65ReleaseLedger* l) {
    return (uint16_t)((l->tail - l->head) & HZ3_S65_LEDGER_MASK);
}

#if HZ3_S65_LEDGER_VALIDATE_FREE
static inline int s65_ledger_bits_are_free(const uint64_t* bits, uint32_t page_idx, uint32_t pages) {
    for (uint32_t i = 0; i < pages; i++) {
        uint32_t idx = page_idx + i;
        uint32_t word = idx / 64;
        uint32_t bit = idx % 64;
        if ((bits[word] & (1ULL << bit)) == 0) {
            return 0;
        }
    }
    return 1;
}

static inline int s65_ledger_entry_is_free(const Hz3S65ReleaseEntry* ent) {
    if (ent->page_idx + ent->pages > HZ3_PAGES_PER_SEG) {
        return 0;
    }

    Hz3SegHdr* hdr = (Hz3SegHdr*)ent->seg_base;
    if (hdr && hdr->magic == HZ3_SEG_HDR_MAGIC && hdr->kind == HZ3_SEG_KIND_SMALL) {
        return s65_ledger_bits_are_free(hdr->free_bits, ent->page_idx, ent->pages);
    }

    Hz3SegMeta* meta = hz3_segmap_get(ent->seg_base);
    if (!meta || meta->seg_base != ent->seg_base) {
        return 0;
    }
    return s65_ledger_bits_are_free(meta->free_bits, ent->page_idx, ent->pages);
}
#endif

// ============================================================================
// Enqueue with Range Coalescing
// ============================================================================

void hz3_s65_ledger_enqueue(void* seg_base, uint32_t page_idx, uint32_t pages, Hz3ReleaseReason reason) {
    if (!t_hz3_cache.initialized) {
        return;
    }

    s65_ledger_ensure_init();
    Hz3S65ReleaseLedger* l = &t_s65_ledger;

#if HZ3_S65_STATS
    s65_stats_register_atexit();
#endif

    // Try to coalesce with existing entries (scan from tail backwards)
    // Only coalesce if same seg_base and pages are adjacent
    uint16_t scan_idx = l->tail;
    int coalesced = 0;

    // Scan up to HZ3_S65_COALESCE_SCAN_DEPTH recent entries for coalesce opportunity
    for (int i = 0; i < HZ3_S65_COALESCE_SCAN_DEPTH && scan_idx != l->head; i++) {
        scan_idx = (scan_idx - 1) & HZ3_S65_LEDGER_MASK;
        Hz3S65ReleaseEntry* ent = &l->entries[scan_idx];

        if (ent->seg_base != seg_base) {
            continue;
        }

        // Check if new range is immediately after existing
        if (ent->page_idx + ent->pages == page_idx) {
            ent->pages += (uint16_t)pages;
            coalesced = 1;
            l->coalesced++;
#if HZ3_S65_STATS
            atomic_fetch_add_explicit(&g_s65_ledger_coalesced, 1, memory_order_relaxed);
#endif
            break;
        }

        // Check if new range is immediately before existing
        if (page_idx + pages == ent->page_idx) {
            ent->page_idx = page_idx;
            ent->pages += (uint16_t)pages;
            coalesced = 1;
            l->coalesced++;
#if HZ3_S65_STATS
            atomic_fetch_add_explicit(&g_s65_ledger_coalesced, 1, memory_order_relaxed);
#endif
            break;
        }
    }

    if (coalesced) {
        l->enqueued++;
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_ledger_enqueued, 1, memory_order_relaxed);
#endif
        return;
    }

    // No coalesce possible, insert new entry
    // If queue is full, do IMMEDIATE madvise on oldest entry (no drop)
    if (s65_ledger_is_full(l)) {
        Hz3S65ReleaseEntry* oldest = &l->entries[l->head];

#if HZ3_S65_LEDGER_VALIDATE_FREE
        if (!s65_ledger_entry_is_free(oldest)) {
            l->head = (l->head + 1) & HZ3_S65_LEDGER_MASK;
            l->dropped++;
#if HZ3_S65_STATS
            atomic_fetch_add_explicit(&g_s65_ledger_dropped, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s65_ledger_skip_used, 1, memory_order_relaxed);
#endif
            goto s65_enqueue_continue;
        }
#endif

        // Immediate madvise on the oldest entry
        void* page_addr = (void*)((uintptr_t)oldest->seg_base + ((size_t)oldest->page_idx << HZ3_PAGE_SHIFT));
        size_t purge_size = (size_t)oldest->pages << HZ3_PAGE_SHIFT;
        int ret = hz3_os_madvise_dontneed_checked(page_addr, purge_size);
        if (ret == 0) {
            l->purged_pages += oldest->pages;
#if HZ3_S65_STATS
            atomic_fetch_add_explicit(&g_s65_ledger_purged_pages, oldest->pages, memory_order_relaxed);
#endif
        }
        l->madvise_calls++;
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_ledger_madvise_calls, 1, memory_order_relaxed);
#endif

        l->head = (l->head + 1) & HZ3_S65_LEDGER_MASK;
        l->dropped++;  // Still count as "overflow" for stats
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_ledger_dropped, 1, memory_order_relaxed);
#endif
    }

 #if HZ3_S65_LEDGER_VALIDATE_FREE
s65_enqueue_continue:
    ;
#endif

    // Enqueue at tail
    Hz3S65ReleaseEntry* ent = &l->entries[l->tail];
    ent->seg_base = seg_base;
    ent->page_idx = page_idx;
    ent->pages = (uint16_t)pages;
    ent->reason = (uint16_t)reason;
    ent->retire_epoch = t_hz3_cache.epoch_counter;

    l->tail = (l->tail + 1) & HZ3_S65_LEDGER_MASK;
    l->enqueued++;

#if HZ3_S65_STATS
    atomic_fetch_add_explicit(&g_s65_ledger_enqueued, 1, memory_order_relaxed);
#endif
}

// ============================================================================
// Purge Tick
// ============================================================================

void hz3_s65_ledger_purge_tick(uint32_t delay_epochs, uint32_t budget_pages) {
    if (!t_hz3_cache.initialized) {
        return;
    }

    s65_ledger_ensure_init();
    Hz3S65ReleaseLedger* l = &t_s65_ledger;

#if HZ3_S65_STATS
    s65_stats_register_atexit();
#endif

    uint32_t current_epoch = t_hz3_cache.epoch_counter;
    uint32_t budget_calls = HZ3_S65_PURGE_MAX_CALLS;
    uint32_t pages_purged = 0;
    uint32_t madvise_calls = 0;

    // Process queue: dequeue entries that have aged past delay
    while (!s65_ledger_is_empty(l) && budget_pages > 0 && budget_calls > 0) {
        Hz3S65ReleaseEntry* ent = &l->entries[l->head];

        // Check if enough epochs have passed
        uint32_t age = current_epoch - ent->retire_epoch;
        if (age < delay_epochs) {
            // Not ready yet; FIFO ordering means we stop
            break;
        }

        // Dequeue
        l->head = (l->head + 1) & HZ3_S65_LEDGER_MASK;

#if HZ3_S65_LEDGER_VALIDATE_FREE
        if (!s65_ledger_entry_is_free(ent)) {
            l->dropped++;
#if HZ3_S65_STATS
            atomic_fetch_add_explicit(&g_s65_ledger_dropped, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s65_ledger_skip_used, 1, memory_order_relaxed);
#endif
            continue;
        }
#endif

        // Compute address and call madvise
        void* page_addr = (void*)((uintptr_t)ent->seg_base + ((size_t)ent->page_idx << HZ3_PAGE_SHIFT));
        size_t purge_size = (size_t)ent->pages << HZ3_PAGE_SHIFT;

        int ret = hz3_os_madvise_dontneed_checked(page_addr, purge_size);
        if (ret == 0) {
            pages_purged += ent->pages;
            l->purged_pages += ent->pages;
        }
        madvise_calls++;
        l->madvise_calls++;

        // Decrement page budget
        if (budget_pages > ent->pages) {
            budget_pages -= ent->pages;
        } else {
            budget_pages = 0;
        }
        budget_calls--;
    }

#if HZ3_S65_STATS
    if (pages_purged > 0) {
        atomic_fetch_add_explicit(&g_s65_ledger_purged_pages, pages_purged, memory_order_relaxed);
    }
    if (madvise_calls > 0) {
        atomic_fetch_add_explicit(&g_s65_ledger_madvise_calls, madvise_calls, memory_order_relaxed);
    }
#endif
}

// ============================================================================
// Gated Purge Tick (with Idle/Busy adjustment)
// ============================================================================

void hz3_s65_ledger_purge_tick_gated(void) {
    s65_ledger_ensure_init();
    Hz3S65ReleaseLedger* l = &t_s65_ledger;

    bool idle = hz3_s65_is_idle();

    uint32_t delay_epochs;
    uint32_t budget_pages;

    if (idle) {
        delay_epochs = HZ3_S65_DELAY_EPOCHS_IDLE;
        budget_pages = HZ3_S65_PURGE_BUDGET_PAGES;
        l->idle_ticks++;
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_ledger_idle_ticks, 1, memory_order_relaxed);
#endif
    } else {
        delay_epochs = HZ3_S65_DELAY_EPOCHS_BUSY;
        budget_pages = HZ3_S65_PURGE_BUDGET_PAGES / 4;  // Reduced budget when busy
        l->busy_ticks++;
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_ledger_busy_ticks, 1, memory_order_relaxed);
#endif
    }

    hz3_s65_ledger_purge_tick(delay_epochs, budget_pages);
}

// ============================================================================
// Full Ledger Flush (for thread exit)
// ============================================================================

void hz3_s65_ledger_flush_all(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

    s65_ledger_ensure_init();
    Hz3S65ReleaseLedger* l = &t_s65_ledger;

#if HZ3_S65_STATS
    s65_stats_register_atexit();
#endif

    uint32_t pages_purged = 0;
    uint32_t madvise_calls = 0;

    // Drain ALL entries regardless of delay
    while (!s65_ledger_is_empty(l)) {
        Hz3S65ReleaseEntry* ent = &l->entries[l->head];

        // Dequeue
        l->head = (l->head + 1) & HZ3_S65_LEDGER_MASK;

#if HZ3_S65_LEDGER_VALIDATE_FREE
        if (!s65_ledger_entry_is_free(ent)) {
            l->dropped++;
#if HZ3_S65_STATS
            atomic_fetch_add_explicit(&g_s65_ledger_dropped, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s65_ledger_skip_used, 1, memory_order_relaxed);
#endif
            continue;
        }
#endif

        // Compute address and call madvise
        void* page_addr = (void*)((uintptr_t)ent->seg_base + ((size_t)ent->page_idx << HZ3_PAGE_SHIFT));
        size_t purge_size = (size_t)ent->pages << HZ3_PAGE_SHIFT;

        int ret = hz3_os_madvise_dontneed_checked(page_addr, purge_size);
        if (ret == 0) {
            pages_purged += ent->pages;
            l->purged_pages += ent->pages;
        }
        madvise_calls++;
        l->madvise_calls++;
    }

#if HZ3_S65_STATS
    if (pages_purged > 0) {
        atomic_fetch_add_explicit(&g_s65_ledger_purged_pages, pages_purged, memory_order_relaxed);
    }
    if (madvise_calls > 0) {
        atomic_fetch_add_explicit(&g_s65_ledger_madvise_calls, madvise_calls, memory_order_relaxed);
    }
#endif
}

// ============================================================================
// Stats Dump
// ============================================================================

void hz3_s65_ledger_dump_stats(void) {
#if HZ3_S65_STATS
    s65_ledger_atexit_dump();
#endif
}

#endif  // HZ3_S65_RELEASE_LEDGER
