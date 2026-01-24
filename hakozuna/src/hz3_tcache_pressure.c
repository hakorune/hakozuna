// hz3_tcache_pressure.c - Pressure Box flush handler
// Extracted from hz3_tcache.c for modularization
#define _GNU_SOURCE

#include "hz3_tcache_internal.h"
#include "hz3_inbox.h"
#include "hz3_owner_stash.h"
#include "hz3_arena.h"
#if HZ3_S47_SEGMENT_QUARANTINE
#include "hz3_segment_quarantine.h"
#endif

// ============================================================================
// Pressure Box: flush handler called when arena pressure is detected
// ============================================================================
#if HZ3_ARENA_PRESSURE_BOX

// Flush local medium bins to central (for pressure box)
void hz3_tcache_flush_medium_to_central(void) {
    uint8_t shard = (uint8_t)t_hz3_cache.my_shard;
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        uint32_t bin_idx = hz3_bin_index_medium(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (!head) continue;
        // Walk list to find tail and count
        void* tail = head;
        uint32_t n = 1;
        void* next = hz3_obj_get_next(head);
        while (next) {
            tail = next;
            n++;
            next = hz3_obj_get_next(next);
        }
        hz3_central_push_list(shard, sc, head, tail, n);
        hz3_local_head_clear(bin_idx);
#else
        Hz3Bin* bin = hz3_tcache_get_bin(sc);
        if (hz3_bin_is_empty(bin)) continue;
        void* head = bin->head;
        // Walk list to find tail and count
        void* tail = head;
        uint32_t n = 1;
        void* next = hz3_obj_get_next(head);
        while (next) {
            tail = next;
            n++;
            next = hz3_obj_get_next(next);
        }
        hz3_central_push_list(shard, sc, head, tail, n);
        bin->head = NULL;
        bin->count = 0;
#endif
    }
}

// Flush local small bins to central (critical for arena recovery)
void hz3_tcache_flush_small_to_central(void) {
    uint8_t shard = (uint8_t)t_hz3_cache.my_shard;
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        uint32_t bin_idx = hz3_bin_index_small(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (!head) continue;
        // Walk list to find tail and count
        void* tail = head;
        uint32_t n = 1;
        void* next = hz3_obj_get_next(head);
        while (next) {
            tail = next;
            n++;
            next = hz3_obj_get_next(next);
        }
        hz3_small_v2_central_push_list(shard, sc, head, tail, n);
        hz3_local_head_clear(bin_idx);
#else
        Hz3Bin* bin = hz3_tcache_get_small_bin(sc);
        if (hz3_bin_is_empty(bin)) continue;
        void* head = bin->head;
        // Walk list to find tail and count
        void* tail = head;
        uint32_t n = 1;
        void* next = hz3_obj_get_next(head);
        while (next) {
            tail = next;
            n++;
            next = hz3_obj_get_next(next);
        }
        hz3_small_v2_central_push_list(shard, sc, head, tail, n);
        bin->head = NULL;
        bin->count = 0;
#endif
    }
}

// Pressure check and flush handler
// Called at slow path entry points (hz3_alloc_slow, hz3_small_v2_alloc_slow)
void hz3_pressure_check_and_flush(void) {
    // 1) Epoch check (hot path: 1 acquire load only)
    uint32_t current_epoch = atomic_load_explicit(&g_hz3_arena_pressure_epoch, memory_order_acquire);
    if (current_epoch == 0) {
        return;  // No pressure (never triggered)
    }
    if (t_hz3_cache.last_pressure_epoch >= current_epoch) {
        return;  // Already flushed for this epoch
    }

    // 2) TLS initialization check
    if (!t_hz3_cache.initialized) {
        return;
    }

    // 3) Re-entrancy guard (prevent recursion during flush)
    if (t_hz3_cache.in_pressure_flush) {
        return;
    }
    t_hz3_cache.in_pressure_flush = 1;

    // 4) Update epoch first (skip if re-enter slow path during flush)
    t_hz3_cache.last_pressure_epoch = current_epoch;

    uint8_t shard = t_hz3_cache.my_shard;

    // 5) Remote stash -> central (FULL flush during pressure - critical for reclaim)
#if HZ3_PTAG_DSTBIN_ENABLE
    hz3_dstbin_flush_remote_all();
#endif

    // 6) Owner stash -> central (FULL flush during pressure)
#if HZ3_S44_OWNER_STASH
    // Use a very large budget to effectively flush all
    hz3_owner_stash_flush_to_central_budget(shard, 65536);
#endif

    // 7) Inbox -> central (medium only)
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        hz3_inbox_drain_to_central(shard, sc);
    }

    // 8) Local medium bins -> central
    hz3_tcache_flush_medium_to_central();

    // 9) Local small bins -> central
    hz3_tcache_flush_small_to_central();

#if HZ3_S47_SEGMENT_QUARANTINE
    // 10) S47: Drain central to draining segment (after flush)
    hz3_s47_compact_hard(shard);
#endif

#if HZ3_SEG_SCAVENGE_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // 11) S54: Segment page scavenge observation (OBSERVE mode, statistics only)
    hz3_seg_scavenge_observe();
#endif

#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // 12) S55: Retention observation (OBSERVE mode, statistics only)
    hz3_retention_observe_event();
#endif

    // Release re-entrancy guard
    t_hz3_cache.in_pressure_flush = 0;
}

#endif  // HZ3_ARENA_PRESSURE_BOX
