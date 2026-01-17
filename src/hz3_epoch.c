#define _GNU_SOURCE

#include "hz3_epoch.h"
#include "hz3_tcache.h"
#include "hz3_knobs.h"
#include "hz3_central.h"
#include "hz3_learn.h"
#include "hz3_arena.h"

#if HZ3_S55_RETENTION_FROZEN
#include "hz3_retention_policy.h"
#endif
#if HZ3_S58_TCACHE_DECAY
#include "hz3_tcache_decay.h"
#endif
#if HZ3_S60_PURGE_RANGE_QUEUE
#include "hz3_purge_range_queue.h"
#endif
#if HZ3_S64_TCACHE_DUMP
#include "hz3_s64_tcache_dump.h"
#endif
#if HZ3_S64_RETIRE_SCAN
#include "hz3_s64_retire_scan.h"
#endif
#if HZ3_S64_PURGE_DELAY
#include "hz3_s64_purge_delay.h"
#endif
#if HZ3_S65_RELEASE_LEDGER
#include "hz3_release_ledger.h"
#endif
#if HZ3_S65_MEDIUM_RECLAIM
#include "hz3_s65_medium_reclaim.h"
#endif

#include <string.h>

// Remote path detection for S65/S62 guard
#ifndef HZ3_S42_SMALL_XFER_DISABLE
#define HZ3_S42_SMALL_XFER_DISABLE 0
#endif
#ifndef HZ3_S44_OWNER_STASH_DISABLE
#define HZ3_S44_OWNER_STASH_DISABLE 0
#endif
#define HZ3_REMOTE_ENABLED \
    ((HZ3_S42_SMALL_XFER && !HZ3_S42_SMALL_XFER_DISABLE) || \
     (HZ3_S44_OWNER_STASH && !HZ3_S44_OWNER_STASH_DISABLE))

// ============================================================================
// Day 6: Epoch - periodic maintenance
// Actions:
//   1. Flush all outboxes
//   2. Trim bins (excess -> central)
//   3. Update knobs snapshot if version changed
// ============================================================================

// TLS epoch state
static __thread int t_in_epoch = 0;  // re-entry guard

// Forward declarations
extern __thread Hz3TCache t_hz3_cache;
void hz3_outbox_flush(uint8_t owner, int sc);
#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
void hz3_retention_observe_event(void);
#endif

#if HZ3_BIN_LAZY_COUNT && HZ3_S123_TRIM_BOUNDED
static void* hz3_bin_detach_after_n_bounded(
    void** head_ptr,
    uint16_t keep,
    uint16_t batch,
    uint32_t* out_n,
    void** out_tail) {
    if (!head_ptr || !*head_ptr || batch == 0) {
        if (out_n) *out_n = 0;
        if (out_tail) *out_tail = NULL;
        return NULL;
    }

    void* head = *head_ptr;
    void* prev = NULL;
    if (keep > 0) {
        prev = head;
        for (uint16_t i = 1; i < keep && prev; i++) {
            prev = hz3_obj_get_next(prev);
        }
        if (!prev) {
            if (out_n) *out_n = 0;
            if (out_tail) *out_tail = NULL;
            return NULL;
        }
    }

    void* batch_head = (keep == 0) ? head : hz3_obj_get_next(prev);
    if (!batch_head) {
        if (out_n) *out_n = 0;
        if (out_tail) *out_tail = NULL;
        return NULL;
    }

    void* tail = batch_head;
    uint32_t n = 1;
    while (n < batch) {
        void* next = hz3_obj_get_next(tail);
        if (!next) {
            break;
        }
        tail = next;
        n++;
    }

    void* remainder = hz3_obj_get_next(tail);
    hz3_obj_set_next(tail, NULL);
    if (keep == 0) {
        *head_ptr = remainder;
    } else {
        hz3_obj_set_next(prev, remainder);
    }

    if (out_n) *out_n = n;
    if (out_tail) *out_tail = tail;
    return batch_head;
}
#endif

// Trim bin to target, return excess to central
static void hz3_bin_trim(int sc, uint8_t target) {
#if HZ3_TCACHE_SOA_LOCAL
    uint32_t bin_idx = hz3_bin_index_medium(sc);

#if HZ3_BIN_SPLIT_COUNT
    // S122: Split Count version - use Hz3BinRef abstraction
    Hz3BinRef ref = hz3_tcache_get_local_binref(bin_idx);
    uint32_t cur = hz3_binref_count_get(ref);
    if (cur <= target) return;

    uint32_t excess = cur - target;
    uint32_t batch = excess;

    // Extract head pointer from tagged value
    void* head = hz3_split_ptr(*ref.head);
    void* tail = head;
    uint32_t n = 1;

    // Cut off 'batch' objects from bin
    while (n < batch && hz3_obj_get_next(tail)) {
        tail = hz3_obj_get_next(tail);
        n++;
    }

    // Update bin: new_head = next(tail), subtract n from count
    void* new_head = hz3_obj_get_next(tail);
    hz3_obj_set_next(tail, NULL);

    // Recalculate split count after removing n items
    uint32_t new_count = cur - n;
    uint32_t new_low = new_count & (uint32_t)HZ3_SPLIT_CNT_MASK;
    *ref.count = (Hz3BinCount)(new_count >> HZ3_SPLIT_CNT_SHIFT);
    *ref.head = hz3_split_pack(new_head, new_low);
#else
    void** bin_head = &t_hz3_cache.local_head[bin_idx];
#if !HZ3_BIN_LAZY_COUNT
    Hz3BinCount* bin_count = &t_hz3_cache.local_count[bin_idx];
#endif

#if HZ3_BIN_LAZY_COUNT
#if HZ3_S123_TRIM_BOUNDED
    uint32_t n = 0;
    void* tail = NULL;
    void* head = hz3_bin_detach_after_n_bounded(
        (void**)bin_head, target, HZ3_S123_TRIM_BATCH, &n, &tail);
    if (!head) return;
#else
    // S38-1: Walk-based trim - create temp Hz3Bin for detach helper
    Hz3Bin tmp_bin = { .head = *bin_head, .count = 0 };
    void* detached = hz3_bin_detach_after_n(&tmp_bin, target);
    *bin_head = tmp_bin.head;
    if (!detached) return;

    // Count and find tail of detached list
    void* head = detached;
    void* tail = head;
    uint32_t n = 1;
    while (hz3_obj_get_next(tail)) {
        tail = hz3_obj_get_next(tail);
        n++;
    }
#endif
#else
    if (*bin_count <= target) return;

    uint32_t excess = *bin_count - target;
    uint32_t batch = excess;
    void* head = *bin_head;
    void* tail = head;
    uint32_t n = 1;

    // Cut off 'batch' objects from bin
    while (n < batch && hz3_obj_get_next(tail)) {
        tail = hz3_obj_get_next(tail);
        n++;
    }

    // Update bin
    *bin_head = hz3_obj_get_next(tail);
    *bin_count -= n;
    hz3_obj_set_next(tail, NULL);
#endif
#endif  // HZ3_BIN_SPLIT_COUNT

#else  // !HZ3_TCACHE_SOA_LOCAL
    Hz3Bin* bin = hz3_tcache_get_bin(sc);

#if HZ3_BIN_LAZY_COUNT
#if HZ3_S123_TRIM_BOUNDED
    uint32_t n = 0;
    void* tail = NULL;
    void* head = hz3_bin_detach_after_n_bounded(
        &bin->head, target, HZ3_S123_TRIM_BATCH, &n, &tail);
    if (!head) return;
#else
    // S38-1: Walk-based trim (no bin->count dependency)
    void* detached = hz3_bin_detach_after_n(bin, target);
    if (!detached) return;  // bin had <= target elements

    // Count and find tail of detached list
    void* head = detached;
    void* tail = head;
    uint32_t n = 1;
    while (hz3_obj_get_next(tail)) {
        tail = hz3_obj_get_next(tail);
        n++;
    }
#endif
#else
    Hz3BinCount cur = hz3_bin_count_get(bin);
    if (cur <= target) return;

    uint32_t excess = (uint32_t)(cur - target);
    uint32_t batch = excess;
    void* head = hz3_bin_head(bin);
    void* tail = head;
    uint32_t n = 1;

    // Cut off 'batch' objects from bin
    while (n < batch && hz3_obj_get_next(tail)) {
        tail = hz3_obj_get_next(tail);
        n++;
    }

    // Update bin
    hz3_bin_set_head(bin, hz3_obj_get_next(tail));
    hz3_bin_count_sub(bin, (Hz3BinCount)n);
    hz3_obj_set_next(tail, NULL);
#endif
#endif  // HZ3_TCACHE_SOA_LOCAL

    // Update stats
    t_hz3_cache.stats.bin_trim_excess[sc] += n;

    // Return to central
    hz3_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
}

#if HZ3_S137_SMALL_DECAY
// S137 archived/NO-GO: stubbed out (no-op).
#endif  // HZ3_S137_SMALL_DECAY

__attribute__((noinline, cold))
void hz3_epoch_force(void) {
    // Re-entry guard (destructor / slow path / flush could overlap)
    if (t_in_epoch) return;
    t_in_epoch = 1;

#if HZ3_ARENA_PRESSURE_BOX
    hz3_pressure_check_and_flush();
#endif

    // 1. Flush all outboxes
    for (int owner = 0; owner < HZ3_NUM_SHARDS; owner++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            hz3_outbox_flush((uint8_t)owner, sc);
        }
    }

#if HZ3_PTAG_DSTBIN_ENABLE
    // S24-1: Full flush in epoch (event-only)
    hz3_dstbin_flush_remote_all();
#endif

#if HZ3_S58_TCACHE_DECAY
    // S58-1: Adjust bin_target based on overage + lowwater (before trim)
    hz3_s58_adjust_targets();
#endif

    // 2. Trim bins to target (S58-1 may have adjusted bin_target)
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        hz3_bin_trim(sc, t_hz3_cache.knobs.bin_target[sc]);
    }

#if HZ3_S137_SMALL_DECAY
    // S137 archived/NO-GO: stubbed (no-op).
#endif

#if HZ3_S58_TCACHE_DECAY
    // S58-2: Central -> Segment reclaim (after trim pushes excess to central)
    hz3_s58_central_reclaim();
#endif

#if HZ3_S60_PURGE_RANGE_QUEUE
    // S60: Purge queued ranges with madvise (after S58 reclaim queued ranges)
    // Order: S58 reclaim -> S60 purge (SSOT)
    hz3_s60_purge_tick();
#endif

#if HZ3_S64_TCACHE_DUMP
    // S64-1: Dump tcache local bins to central (supply for retire)
    hz3_s64_tcache_dump_tick();
#endif

#if HZ3_S64_RETIRE_SCAN
    // S64-A: Scan central for empty pages, retire them
    hz3_s64_retire_scan_tick();
#endif

#if HZ3_S64_PURGE_DELAY
    // S64-B: Purge retired pages after delay
    hz3_s64_purge_delay_tick();
#endif

    // S65 Remote Guard: Skip S65 reclaim/purge when remote paths are active
#ifndef HZ3_S65_REMOTE_GUARD
#define HZ3_S65_REMOTE_GUARD 0
#endif
#if !(HZ3_S65_REMOTE_GUARD && HZ3_REMOTE_ENABLED)

#if HZ3_S65_MEDIUM_RECLAIM
    // S65-2C: Medium epoch reclaim (central → segment via boundary API)
    // - Drains runs from central bins
    // - Uses hz3_release_range_definitely_free_meta() (single entry point)
    // - Enqueues to ledger for deferred purge
    hz3_s65_medium_reclaim_tick();
#endif

#if HZ3_S65_RELEASE_LEDGER
    // S65: Release ledger purge with idle/busy gate
    // - When HZ3_S65_IDLE_GATE=1, adjusts delay/budget based on pressure
    // - Coalesced ranges reduce madvise syscall count
    hz3_s65_ledger_purge_tick_gated();
#endif

#endif  // S65_REMOTE_GUARD

    // 3. Update knobs snapshot if version changed
    uint32_t ver = atomic_load_explicit(&g_hz3_knobs_ver, memory_order_acquire);
    if (t_hz3_cache.knobs_ver != ver) {
        t_hz3_cache.knobs = g_hz3_knobs;  // struct copy
        t_hz3_cache.knobs_ver = ver;
    }

    // 4. Day 6: Learning v0 (only when HZ3_LEARN_ENABLE=1)
    hz3_learn_update(&t_hz3_cache.stats);

#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // S55-1: Retention observation (epoch-only; does not require pressure).
    hz3_retention_observe_event();
#endif

#if HZ3_S55_RETENTION_FROZEN
    // S55-2: Retention level update (boundary A)
    hz3_retention_tick_epoch();
    // S55-2B: Repay epoch (boundary A, heavy, L2 only, throttled)
    hz3_retention_repay_epoch();
#endif

    t_in_epoch = 0;
}
