#define _GNU_SOURCE

#include "hz3_s64_purge_delay.h"

#if HZ3_S64_PURGE_DELAY

#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_seg_hdr.h"
#include "hz3_os_purge.h"
#include "hz3_dtor_stats.h"
#include "hz3_platform.h"
#include <time.h>

// ============================================================================
// S64-B: PurgeDelayBox - delayed madvise(DONTNEED) for retired pages
// ============================================================================
//
// Ring buffer semantics:
// - enqueue: write at tail, advance tail (mod QUEUE_SIZE)
// - dequeue: read at head, advance head (mod QUEUE_SIZE)
// - full: (tail + 1) % size == head → drop oldest (advance head first)
// - empty: head == tail
//
// Purge timing:
// - Only purge when (current_epoch - retire_epoch) >= DELAY_EPOCHS
// - Budget-limited by HZ3_S64_PURGE_BUDGET_PAGES and HZ3_S64_PURGE_MAX_CALLS

extern HZ3_TLS Hz3TCache t_hz3_cache;

// TLS purge queue (separate from Hz3TCache to avoid size increase when S64 is OFF)
static HZ3_TLS Hz3S64PurgeQueue t_s64_purge_queue;
static HZ3_TLS int t_s64_purge_queue_init = 0;

// Stats (atomic for multi-thread safety)
#if HZ3_S64_STATS
HZ3_DTOR_STAT(g_s64p_tick_calls);
HZ3_DTOR_STAT(g_s64p_entries_enqueued);
HZ3_DTOR_STAT(g_s64p_pages_enqueued);
HZ3_DTOR_STAT(g_s64p_pages_purged);
HZ3_DTOR_STAT(g_s64p_madvise_calls);
HZ3_DTOR_STAT(g_s64p_queue_dropped);
HZ3_DTOR_STAT(g_s64p_pages_dropped);
HZ3_DTOR_STAT(g_s64p_queue_max);
HZ3_DTOR_STAT(g_s64p_delay_stops);
HZ3_DTOR_STAT(g_s64p_budget_stops);
HZ3_DTOR_STAT(g_s64p_immediate_calls);
HZ3_DTOR_STAT(g_s64p_forced_purge_triggers);
HZ3_DTOR_STAT(g_s64p_forced_purge_calls);
HZ3_DTOR_STAT(g_s64p_forced_purge_pages);
HZ3_DTOR_STAT(g_s64p_pump_invokes);
HZ3_DTOR_STAT(g_s64p_pump_calls);
HZ3_DTOR_STAT(g_s64p_pump_pages);

HZ3_DTOR_ATEXIT_FLAG(g_s64p);

static void hz3_s64p_atexit_dump(void) {
    uint32_t ticks = HZ3_DTOR_STAT_LOAD(g_s64p_tick_calls);
    uint32_t enq = HZ3_DTOR_STAT_LOAD(g_s64p_entries_enqueued);
    uint32_t enq_pages = HZ3_DTOR_STAT_LOAD(g_s64p_pages_enqueued);
    uint32_t purged = HZ3_DTOR_STAT_LOAD(g_s64p_pages_purged);
    uint32_t calls = HZ3_DTOR_STAT_LOAD(g_s64p_madvise_calls);
    uint32_t dropped = HZ3_DTOR_STAT_LOAD(g_s64p_queue_dropped);
    uint32_t dropped_pages = HZ3_DTOR_STAT_LOAD(g_s64p_pages_dropped);
    uint32_t qmax = HZ3_DTOR_STAT_LOAD(g_s64p_queue_max);
    uint32_t delay_stops = HZ3_DTOR_STAT_LOAD(g_s64p_delay_stops);
    uint32_t budget_stops = HZ3_DTOR_STAT_LOAD(g_s64p_budget_stops);
    uint32_t immediate = HZ3_DTOR_STAT_LOAD(g_s64p_immediate_calls);
    uint32_t forced_triggers = HZ3_DTOR_STAT_LOAD(g_s64p_forced_purge_triggers);
    uint32_t forced_calls = HZ3_DTOR_STAT_LOAD(g_s64p_forced_purge_calls);
    uint32_t forced_pages = HZ3_DTOR_STAT_LOAD(g_s64p_forced_purge_pages);
    uint32_t pump_invokes = HZ3_DTOR_STAT_LOAD(g_s64p_pump_invokes);
    uint32_t pump_calls = HZ3_DTOR_STAT_LOAD(g_s64p_pump_calls);
    uint32_t pump_pages = HZ3_DTOR_STAT_LOAD(g_s64p_pump_pages);
    if (ticks > 0 || purged > 0) {
        fprintf(stderr,
                "[HZ3_S64_PURGE_DELAY] ticks=%u enq=%u enq_pages=%u purged_pages=%u madvise_calls=%u "
                "dropped=%u dropped_pages=%u qmax=%u delay_stops=%u budget_stops=%u immediate=%u "
                "forced_triggers=%u forced_calls=%u forced_pages=%u pump_invokes=%u pump_calls=%u pump_pages=%u\n",
                ticks, enq, enq_pages, purged, calls, dropped, dropped_pages, qmax, delay_stops,
                budget_stops, immediate, forced_triggers, forced_calls, forced_pages, pump_invokes, pump_calls, pump_pages);
    }
}
#endif

// Initialize queue if needed
static inline void s64_purge_queue_ensure_init(void) {
    if (__builtin_expect(!t_s64_purge_queue_init, 0)) {
        t_s64_purge_queue.head = 0;
        t_s64_purge_queue.tail = 0;
        t_s64_purge_queue.dropped = 0;
        t_s64_purge_queue.enqueued = 0;
        t_s64_purge_queue.purged = 0;
        t_s64_purge_queue_init = 1;
    }
}

// Ring buffer helpers (power of 2 size assumed)
#define QUEUE_MASK (HZ3_S64_PURGE_QUEUE_SIZE - 1)

static inline int s64_queue_is_full(Hz3S64PurgeQueue* q) {
    return ((q->tail + 1) & QUEUE_MASK) == q->head;
}

static inline int s64_queue_is_empty(Hz3S64PurgeQueue* q) {
    return q->head == q->tail;
}

static inline uint16_t s64_queue_len(Hz3S64PurgeQueue* q) {
    return (uint16_t)((q->tail - q->head) & QUEUE_MASK);
}

#if HZ3_S64_PURGE_DELAY_MS > 0
static inline uint64_t hz3_time_mono_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}
#endif

#if HZ3_S64_PURGE_CHECK_FREE
static inline int s64_run_is_free(const uint64_t* bits, size_t page_idx, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        size_t idx = page_idx + i;
        size_t word = idx / 64;
        size_t bit = idx % 64;
        if ((bits[word] & (1ULL << bit)) == 0) {
            return 0;
        }
    }
    return 1;
}
#endif

#if HZ3_S64_STATS
static inline void s64_stat_update_max(_Atomic(uint32_t)* stat, uint32_t val) {
    uint32_t cur = atomic_load_explicit(stat, memory_order_relaxed);
    while (val > cur &&
           !atomic_compare_exchange_weak_explicit(stat, &cur, val,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
    }
}
#endif

#if HZ3_S64_PURGE_FORCE_ON_FULL
// Helper: force purge entries from queue (bypass delay_epochs check)
static inline uint32_t s64_force_purge(Hz3S64PurgeQueue* q, uint32_t max_calls, uint32_t max_pages) {
#if HZ3_S64_STATS
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s64p, hz3_s64p_atexit_dump);
#endif
    uint32_t pages_purged = 0;
    uint32_t madvise_calls = 0;

    while (!s64_queue_is_empty(q) && madvise_calls < max_calls && pages_purged < max_pages) {
        Hz3S64PurgeEntry* ent = &q->entries[q->head];
        q->head = (q->head + 1) & QUEUE_MASK;

        void* page_addr = (void*)((uintptr_t)ent->seg_base + ((size_t)ent->page_idx << HZ3_PAGE_SHIFT));
        size_t purge_size = (size_t)ent->pages << HZ3_PAGE_SHIFT;

        int ret = hz3_os_madvise_dontneed_checked(page_addr, purge_size);
        if (ret == 0) {
            pages_purged += ent->pages;
            q->purged += ent->pages;
        }
        madvise_calls++;
    }

#if HZ3_S64_STATS
    if (pages_purged > 0) {
        HZ3_DTOR_STAT_ADD(g_s64p_pages_purged, pages_purged);
    }
    if (madvise_calls > 0) {
        HZ3_DTOR_STAT_ADD(g_s64p_madvise_calls, madvise_calls);
        HZ3_DTOR_STAT_ADD(g_s64p_forced_purge_calls, madvise_calls);
        HZ3_DTOR_STAT_ADD(g_s64p_forced_purge_pages, pages_purged);
    }
#endif

    return pages_purged;
}
#endif

#if HZ3_S64_PURGE_PUMP_ON_ENQ
// Helper: purge ready entries from queue (opportunistic, no bypass)
// Supports both MS-based and epoch-based delay modes
// Called BEFORE enqueue to clean ready heads first
static inline uint32_t s64_purge_pump_ready(Hz3S64PurgeQueue* q, uint32_t max_calls, uint32_t max_pages) {
#if HZ3_S64_STATS
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s64p, hz3_s64p_atexit_dump);
    HZ3_DTOR_STAT_INC(g_s64p_pump_invokes);  // Track every pump invocation
#endif
    uint32_t pages_purged = 0;
    uint32_t madvise_calls = 0;

#if HZ3_S64_PURGE_DELAY_MS > 0
    // MS-based delay: check against deadline_ms
    uint64_t now_ms = hz3_time_mono_ms();
#else
    // Epoch-based delay: check against delay_epochs
    uint32_t current_epoch = t_hz3_cache.epoch_counter;
#endif

    while (!s64_queue_is_empty(q) && madvise_calls < max_calls && pages_purged < max_pages) {
        // Peek head: if not ready -> stop (FIFO guarantees later entries also not ready)
        Hz3S64PurgeEntry* ent = &q->entries[q->head];
        int is_ready = 0;

#if HZ3_S64_PURGE_DELAY_MS > 0
        // MS mode: ready if now >= deadline
        is_ready = (now_ms >= ent->deadline_ms);
#else
        // Epoch mode: ready if age >= delay_epochs
        uint32_t age = current_epoch - ent->retire_epoch;
        (void)age;  // suppress unused warning when EPOCHS=0
#if HZ3_S64_PURGE_DELAY_EPOCHS == 0
        // Immediate purge when delay_epochs=0
        is_ready = 1;
#else
        is_ready = (age >= HZ3_S64_PURGE_DELAY_EPOCHS);
#endif
#endif

        // IMPORTANT: Only purge if ready (respects delay semantics, no bypass)
        if (!is_ready) {
            break;  // Head not ready, stop (FIFO)
        }

        // Pop and purge
        q->head = (q->head + 1) & QUEUE_MASK;
        void* page_addr = (void*)((uintptr_t)ent->seg_base + ((size_t)ent->page_idx << HZ3_PAGE_SHIFT));
        size_t purge_size = (size_t)ent->pages << HZ3_PAGE_SHIFT;

        int ret = hz3_os_madvise_dontneed_checked(page_addr, purge_size);
        if (ret == 0) {
            pages_purged += ent->pages;
            q->purged += ent->pages;
        }
        madvise_calls++;
    }

#if HZ3_S64_STATS
    if (pages_purged > 0) {
        HZ3_DTOR_STAT_ADD(g_s64p_pages_purged, pages_purged);
    }
    if (madvise_calls > 0) {
        HZ3_DTOR_STAT_ADD(g_s64p_madvise_calls, madvise_calls);
        HZ3_DTOR_STAT_ADD(g_s64p_pump_calls, madvise_calls);
        HZ3_DTOR_STAT_ADD(g_s64p_pump_pages, pages_purged);
    }
#endif

    return pages_purged;
}
#endif

void hz3_s64_purge_delay_enqueue(void* seg_base, uint16_t page_idx, uint16_t pages) {
    if (!t_hz3_cache.initialized) {
        return;
    }

#if HZ3_S64_PURGE_IMMEDIATE_ON_ZERO && (HZ3_S64_PURGE_DELAY_EPOCHS == 0)
    s64_purge_queue_ensure_init();
#if HZ3_S64_STATS
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s64p, hz3_s64p_atexit_dump);
#endif
    void* page_addr = (void*)((uintptr_t)seg_base + ((size_t)page_idx << HZ3_PAGE_SHIFT));
    size_t purge_size = (size_t)pages << HZ3_PAGE_SHIFT;
    int ret = hz3_os_madvise_dontneed_checked(page_addr, purge_size);
#if HZ3_S64_STATS
    HZ3_DTOR_STAT_INC(g_s64p_immediate_calls);
    HZ3_DTOR_STAT_INC(g_s64p_madvise_calls);
    if (ret == 0 && pages > 0) {
        HZ3_DTOR_STAT_ADD(g_s64p_pages_purged, pages);
    }
#endif
    (void)ret;
    return;
#endif

    s64_purge_queue_ensure_init();
    Hz3S64PurgeQueue* q = &t_s64_purge_queue;

#if HZ3_S64_PURGE_PUMP_ON_ENQ
    // S64-P4: Purge pump - purge ready entries BEFORE enqueue
    // (box boundary: clean ready heads first, then enqueue new entry)
    s64_purge_pump_ready(q, HZ3_S64_PURGE_PUMP_MAX_CALLS, HZ3_S64_PURGE_PUMP_BUDGET_PAGES);
#endif

#if HZ3_S64_PURGE_DELAY_MS > 0
    uint64_t now_ms = hz3_time_mono_ms();
#endif

#if HZ3_S64_PURGE_COALESCE
    // Tail coalesce: merge adjacent runs within the same segment
    if (!s64_queue_is_empty(q)) {
        Hz3S64PurgeEntry* last = &q->entries[(q->tail - 1) & QUEUE_MASK];
        if (last->seg_base == seg_base) {
            uint32_t last_end = (uint32_t)last->page_idx + (uint32_t)last->pages;
            if (last_end == (uint32_t)page_idx) {
                last->pages = (uint16_t)(last->pages + pages);
                last->retire_epoch = t_hz3_cache.epoch_counter;
                q->enqueued++;
#if HZ3_S64_STATS
                HZ3_DTOR_STAT_INC(g_s64p_entries_enqueued);
                if (pages > 0) {
                    HZ3_DTOR_STAT_ADD(g_s64p_pages_enqueued, pages);
                }
#endif
                return;
            }
        }
    }
#endif

    // If queue is full, drop oldest (advance head)
    if (s64_queue_is_full(q)) {
        Hz3S64PurgeEntry* drop = &q->entries[q->head];
        uint16_t drop_pages = drop->pages;
        q->head = (q->head + 1) & QUEUE_MASK;
        q->dropped++;
#if HZ3_S64_STATS
        HZ3_DTOR_STAT_INC(g_s64p_queue_dropped);
        if (drop_pages > 0) {
            HZ3_DTOR_STAT_ADD(g_s64p_pages_dropped, drop_pages);
        }
#endif
    }

    // Enqueue at tail
    Hz3S64PurgeEntry* ent = &q->entries[q->tail];
    ent->seg_base = seg_base;
    ent->page_idx = page_idx;
    ent->pages = pages;
    ent->retire_epoch = t_hz3_cache.epoch_counter;
#if HZ3_S64_PURGE_DELAY_MS > 0
    ent->deadline_ms = now_ms + HZ3_S64_PURGE_DELAY_MS;
#endif

    q->tail = (q->tail + 1) & QUEUE_MASK;
    q->enqueued++;

#if HZ3_S64_STATS
    HZ3_DTOR_STAT_INC(g_s64p_entries_enqueued);
    if (pages > 0) {
        HZ3_DTOR_STAT_ADD(g_s64p_pages_enqueued, pages);
    }
    s64_stat_update_max(&g_s64p_queue_max, s64_queue_len(q));
#endif

#if HZ3_S64_PURGE_FORCE_ON_FULL
    // Force purge when queue length exceeds watermark (bypass delay_epochs)
    uint16_t qlen = s64_queue_len(q);
    if (qlen >= HZ3_S64_PURGE_FORCE_WATERMARK) {
        HZ3_DTOR_STAT_INC(g_s64p_forced_purge_triggers);
        s64_force_purge(q, HZ3_S64_PURGE_FORCE_MAX_CALLS, HZ3_S64_PURGE_FORCE_MAX_PAGES);
    }
#endif
}

void hz3_s64_purge_delay_tick(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

    s64_purge_queue_ensure_init();

#if HZ3_S64_STATS
    HZ3_DTOR_STAT_INC(g_s64p_tick_calls);
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s64p, hz3_s64p_atexit_dump);
#endif

    Hz3S64PurgeQueue* q = &t_s64_purge_queue;
#if HZ3_S64_PURGE_DELAY_MS > 0
    uint64_t now_ms = hz3_time_mono_ms();
#else
    uint32_t current_epoch = t_hz3_cache.epoch_counter;
    uint32_t delay_epochs = HZ3_S64_PURGE_DELAY_EPOCHS;
    (void)delay_epochs;  // suppress unused warning when EPOCHS=0
#endif
    uint32_t budget_pages = HZ3_S64_PURGE_BUDGET_PAGES;
    uint32_t budget_calls = HZ3_S64_PURGE_MAX_CALLS;
    uint32_t pages_purged = 0;
    uint32_t madvise_calls = 0;

    // Process queue: dequeue entries that have aged past delay
    while (!s64_queue_is_empty(q) && budget_pages > 0 && budget_calls > 0) {
        Hz3S64PurgeEntry* ent = &q->entries[q->head];

        // Check if enough time has passed
#if HZ3_S64_PURGE_DELAY_MS > 0
        if (now_ms < ent->deadline_ms) {
            // Not ready yet; entries are FIFO, so stop here
            break;
        }
#else
        uint32_t age = current_epoch - ent->retire_epoch;
#if HZ3_S64_PURGE_DELAY_EPOCHS == 0
        // Immediate purge when delay_epochs=0 (always ready)
        (void)age;  // suppress unused warning
#else
        if (age < delay_epochs) {
            // Not ready yet; entries are FIFO, so stop here
            break;
        }
#endif
#endif

        // Dequeue
        q->head = (q->head + 1) & QUEUE_MASK;

#if HZ3_S64_PURGE_CHECK_FREE
        // Re-check free_bits before purging (skip if reused)
        Hz3SegHdr* hdr = (Hz3SegHdr*)ent->seg_base;
        if (!s64_run_is_free(hdr->free_bits, ent->page_idx, ent->pages)) {
            continue;
        }
#endif

        // Compute address and call madvise
        void* page_addr = (void*)((uintptr_t)ent->seg_base + ((size_t)ent->page_idx << HZ3_PAGE_SHIFT));
        size_t purge_size = (size_t)ent->pages << HZ3_PAGE_SHIFT;

        int ret = hz3_os_madvise_dontneed_checked(page_addr, purge_size);
        if (ret == 0) {
            pages_purged += ent->pages;
            q->purged += ent->pages;
        }
        madvise_calls++;

        // Decrement budgets
        if (budget_pages > ent->pages) {
            budget_pages -= ent->pages;
        } else {
            budget_pages = 0;
        }
        budget_calls--;
    }

#if HZ3_S64_STATS
    if (pages_purged > 0) {
        HZ3_DTOR_STAT_ADD(g_s64p_pages_purged, pages_purged);
    }
    if (madvise_calls > 0) {
        HZ3_DTOR_STAT_ADD(g_s64p_madvise_calls, madvise_calls);
    }
    if (!s64_queue_is_empty(q)) {
        Hz3S64PurgeEntry* ent = &q->entries[q->head];
#if HZ3_S64_PURGE_DELAY_MS > 0
        if (now_ms < ent->deadline_ms) {
            HZ3_DTOR_STAT_INC(g_s64p_delay_stops);
        } else if (budget_pages == 0 || budget_calls == 0) {
            HZ3_DTOR_STAT_INC(g_s64p_budget_stops);
        }
#else
        uint32_t age = current_epoch - ent->retire_epoch;
        (void)age;  // suppress unused warning when EPOCHS=0
#if HZ3_S64_PURGE_DELAY_EPOCHS == 0
        // Immediate purge: if stopped, it's due to budget, not delay
        HZ3_DTOR_STAT_INC(g_s64p_budget_stops);
#else
        if (age < delay_epochs) {
            HZ3_DTOR_STAT_INC(g_s64p_delay_stops);
        } else if (budget_pages == 0 || budget_calls == 0) {
            HZ3_DTOR_STAT_INC(g_s64p_budget_stops);
        }
#endif
#endif
    }
#endif
}

#endif  // HZ3_S64_PURGE_DELAY
