// hakozuna/hz3/src/hz3_segment_decay.c
//
// S57-A: Segment Decay Trim
// Return fully-free segments to OS via munmap after grace period
//
// Design:
// - Per-shard FIFO queue of fully-free segments
// - Grace period ensures in-flight inbox/outbox operations complete
// - Budgeted processing to avoid syscall storms

#define _GNU_SOURCE

#include "hz3_config.h"

#if HZ3_S57_DECAY_TRIM

#include "hz3_segment_decay.h"
#include "hz3_segment.h"
#include "hz3_tcache.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Global State (per-shard decay queues)
// ============================================================================

// Per-shard decay queue (FIFO: head is oldest, tail is newest)
static Hz3SegMeta* g_decay_head[HZ3_NUM_SHARDS];
static Hz3SegMeta* g_decay_tail[HZ3_NUM_SHARDS];
static uint32_t    g_decay_count[HZ3_NUM_SHARDS];

// Global epoch counter (shared across all threads for consistent timing)
static _Atomic uint32_t g_decay_epoch = 0;

// ============================================================================
// Stats (observability)
// ============================================================================

#if HZ3_S57_DECAY_STATS
static _Atomic uint64_t g_s57_enqueued = 0;
static _Atomic uint64_t g_s57_trimmed = 0;
static _Atomic uint64_t g_s57_reused = 0;
static _Atomic uint64_t g_s57_skipped_young = 0;

// atexit handler for stats
static void hz3_s57_atexit(void) {
    fprintf(stderr, "[HZ3_S57_STATS] enqueued=%lu trimmed=%lu reused=%lu skipped_young=%lu\n",
            (unsigned long)atomic_load(&g_s57_enqueued),
            (unsigned long)atomic_load(&g_s57_trimmed),
            (unsigned long)atomic_load(&g_s57_reused),
            (unsigned long)atomic_load(&g_s57_skipped_young));
}

static _Atomic int g_s57_atexit_registered = 0;

static void hz3_s57_ensure_atexit(void) {
    int expected = 0;
    if (atomic_compare_exchange_strong(&g_s57_atexit_registered, &expected, 1)) {
        atexit(hz3_s57_atexit);
    }
}
#endif  // HZ3_S57_DECAY_STATS

// ============================================================================
// API Implementation
// ============================================================================

uint32_t hz3_decay_epoch_now(void) {
    return atomic_load_explicit(&g_decay_epoch, memory_order_relaxed);
}

void hz3_segment_decay_enqueue(Hz3SegMeta* meta) {
    if (!meta) return;

    uint8_t owner = meta->owner;
    if (owner >= HZ3_NUM_SHARDS) return;

    // Already in decay queue?
    if (meta->decay_state == HZ3_SEG_DECAYING) return;

    // Mark as decaying
    meta->decay_state = HZ3_SEG_DECAYING;
    meta->decay_epoch = hz3_decay_epoch_now();
    meta->decay_next = NULL;

    // Append to tail (FIFO)
    if (g_decay_tail[owner]) {
        g_decay_tail[owner]->decay_next = meta;
    } else {
        g_decay_head[owner] = meta;
    }
    g_decay_tail[owner] = meta;
    g_decay_count[owner]++;

#if HZ3_S57_DECAY_STATS
    atomic_fetch_add(&g_s57_enqueued, 1);
    hz3_s57_ensure_atexit();
#endif
}

bool hz3_segment_decay_dequeue(Hz3SegMeta* meta) {
    if (!meta) return false;

    uint8_t owner = meta->owner;
    if (owner >= HZ3_NUM_SHARDS) return false;

    // Not in decay queue?
    if (meta->decay_state != HZ3_SEG_DECAYING) return false;

    // Search and remove from queue
    Hz3SegMeta* prev = NULL;
    Hz3SegMeta* curr = g_decay_head[owner];

    while (curr) {
        if (curr == meta) {
            // Found - remove from list
            if (prev) {
                prev->decay_next = curr->decay_next;
            } else {
                g_decay_head[owner] = curr->decay_next;
            }

            if (g_decay_tail[owner] == curr) {
                g_decay_tail[owner] = prev;
            }

            g_decay_count[owner]--;

            // Reset state
            meta->decay_state = HZ3_SEG_ACTIVE;
            meta->decay_next = NULL;

            return true;
        }
        prev = curr;
        curr = curr->decay_next;
    }

    // Not found (inconsistent state, but be defensive)
    meta->decay_state = HZ3_SEG_ACTIVE;
    return false;
}

Hz3SegMeta* hz3_segment_decay_try_reuse(uint8_t owner, uint16_t pages_needed) {
    if (owner >= HZ3_NUM_SHARDS) return NULL;
    if (g_decay_head[owner] == NULL) return NULL;

    // Pop from head (oldest first for FIFO fairness)
    Hz3SegMeta* meta = g_decay_head[owner];

    // Verify segment can satisfy the request
    if (meta->free_pages < pages_needed) {
        // Segment was modified while in queue (shouldn't happen, but defensive)
        return NULL;
    }

    // Dequeue
    g_decay_head[owner] = meta->decay_next;
    if (g_decay_tail[owner] == meta) {
        g_decay_tail[owner] = NULL;
    }
    g_decay_count[owner]--;

    // Reset state
    meta->decay_state = HZ3_SEG_ACTIVE;
    meta->decay_next = NULL;

#if HZ3_S57_DECAY_STATS
    atomic_fetch_add(&g_s57_reused, 1);
#endif

    return meta;
}

// Check if segment can be safely munmapped
static bool hz3_segment_can_munmap(Hz3SegMeta* meta, uint32_t now_epoch) {
    // 1. Must be in decaying state
    if (meta->decay_state != HZ3_SEG_DECAYING) return false;

    // 2. Must still be fully free
    if (meta->free_pages != HZ3_PAGES_PER_SEG) return false;

    // 3. Grace period must have passed
    uint32_t age = now_epoch - meta->decay_epoch;
    if (age < HZ3_S57_DECAY_GRACE_EPOCHS) {
#if HZ3_S57_DECAY_STATS
        atomic_fetch_add(&g_s57_skipped_young, 1);
#endif
        return false;
    }

    return true;
}

void hz3_segment_decay_tick(void) {
    extern __thread Hz3TCache t_hz3_cache;
    uint8_t my_shard = t_hz3_cache.my_shard;

    if (my_shard >= HZ3_NUM_SHARDS) return;

    // Increment global epoch
    atomic_fetch_add_explicit(&g_decay_epoch, 1, memory_order_relaxed);
    uint32_t now_epoch = hz3_decay_epoch_now();

    // Process this shard's decay queue (budgeted)
    uint32_t budget = HZ3_S57_DECAY_BUDGET_SEGS;
    uint32_t trimmed = 0;

    while (g_decay_head[my_shard] && trimmed < budget) {
        Hz3SegMeta* meta = g_decay_head[my_shard];

        if (!hz3_segment_can_munmap(meta, now_epoch)) {
            // Oldest segment not ready yet - stop processing
            // (younger segments won't be ready either since FIFO)
            break;
        }

        // Dequeue from head
        g_decay_head[my_shard] = meta->decay_next;
        if (g_decay_tail[my_shard] == meta) {
            g_decay_tail[my_shard] = NULL;
        }
        g_decay_count[my_shard]--;

        // Reset state before free (defensive)
        meta->decay_state = HZ3_SEG_ACTIVE;
        meta->decay_next = NULL;

        // Free the segment (munmap)
        hz3_segment_free(meta->seg_base);

        trimmed++;
#if HZ3_S57_DECAY_STATS
        atomic_fetch_add(&g_s57_trimmed, 1);
#endif
    }
}

#endif  // HZ3_S57_DECAY_TRIM
