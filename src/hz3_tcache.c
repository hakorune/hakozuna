// hz3_tcache.c - Thread-local cache core (init, destructor, slow path)
// Split into modules: hz3_tcache_stats.c, hz3_tcache_remote.c,
//                     hz3_tcache_pressure.c, hz3_tcache_alloc.c
#define _GNU_SOURCE

#include "hz3_tcache_internal.h"
#include "hz3_eco_mode.h"
#include "hz3_segment.h"
#include "hz3_segmap.h"
#include "hz3_inbox.h"
#include "hz3_knobs.h"
#include "hz3_epoch.h"
#include "hz3_small.h"
#include "hz3_small_v2.h"
#include "hz3_seg_hdr.h"
#include "hz3_tag.h"
#include "hz3_medium_debug.h"
#include "hz3_watch_ptr.h"
#include "hz3_owner_lease.h"
#include "hz3_owner_stash.h"
#if HZ3_LANE_SPLIT
#include "hz3_lane.h"
#endif

#include <sys/mman.h>
#if HZ3_S47_SEGMENT_QUARANTINE
#include "hz3_segment_quarantine.h"
#endif
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif
#if HZ3_S58_TCACHE_DECAY
#include "hz3_tcache_decay.h"
#endif
#if HZ3_S61_DTOR_HARD_PURGE
#include "hz3_dtor_hard_purge.h"
#endif
#if HZ3_S62_RETIRE
#include "hz3_s62_retire.h"
#endif
#if HZ3_S62_SUB4K_RETIRE
#include "hz3_s62_sub4k.h"
#endif
#if HZ3_S62_PURGE
#include "hz3_s62_purge.h"
#endif
#if HZ3_S62_OBSERVE
#include "hz3_s62_observe.h"
#endif
#if HZ3_S65_RELEASE_LEDGER
#include "hz3_release_ledger.h"
#endif

#include <string.h>

// ============================================================================
// S40: Static assertions for SoA + pow2 padding
// ============================================================================
#if HZ3_BIN_PAD_LOG2
_Static_assert((HZ3_BIN_PAD & (HZ3_BIN_PAD - 1)) == 0, "HZ3_BIN_PAD must be power of 2");
_Static_assert(HZ3_BIN_TOTAL <= HZ3_BIN_PAD, "HZ3_BIN_TOTAL must fit in HZ3_BIN_PAD");
#endif

// ============================================================================
// TLS and Global Variables (shared via hz3_tcache_internal.h)
// ============================================================================

// Thread-local cache (zero-initialized by TLS semantics)
__thread Hz3TCache t_hz3_cache;

// Day 4: Shard assignment
_Atomic uint32_t g_shard_counter = 0;

// Detect shard collisions (multiple threads concurrently assigned same shard id).
static _Atomic int g_shard_collision_detected = 0;
static _Atomic uint32_t g_shard_live_count[HZ3_NUM_SHARDS] = {0};
#if HZ3_SHARD_COLLISION_SHOT
_Atomic int g_shard_collision_shot_fired = 0;
#endif

// Day 5: Thread exit cleanup via pthread_key
pthread_key_t g_hz3_tcache_key;
pthread_once_t g_hz3_tcache_key_once = PTHREAD_ONCE_INIT;

#if HZ3_OOM_SHOT
// OOM triage: defined in hz3_tcache_stats.c, declared extern here
extern _Atomic uint64_t g_medium_page_alloc_sc[HZ3_NUM_SC];
#endif

#if HZ3_SEG_SCAVENGE_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
pthread_once_t g_scavenge_obs_atexit_once = PTHREAD_ONCE_INIT;
#endif

#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
pthread_once_t g_retention_obs_atexit_once = PTHREAD_ONCE_INIT;
#endif

#if HZ3_S62_ATEXIT_GATE
// S62-1A: AtExitGate - File-scope guards (follow existing atexit pattern)
pthread_once_t g_s62_atexit_once = PTHREAD_ONCE_INIT;
static _Atomic int g_s62_atexit_executed = 0;
#endif

#if HZ3_S56_ACTIVE_SET && HZ3_S56_ACTIVE_SET_STATS && !HZ3_SHIM_FORWARD_ONLY
_Atomic uint64_t g_s56_active_choose_alt = 0;
#endif

#if HZ3_LANE_SPLIT
static inline Hz3Lane* hz3_lane_alloc(void) {
    void* mem = mmap(NULL, HZ3_PAGE_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return NULL;
    }
    return (Hz3Lane*)mem;
}

static inline void hz3_lane_free(Hz3Lane* lane) {
    if (!lane) {
        return;
    }
    munmap(lane, HZ3_PAGE_SIZE);
}
#endif

// ============================================================================
// S62-1G: SingleThreadRetireGate - Check if total live threads == 1
// ============================================================================

#if HZ3_S62_SINGLE_THREAD_GATE
static inline int hz3_s62_single_thread_ok(void) {
    uint32_t total = 0;
    for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
        total += hz3_shard_live_count((uint8_t)i);
    }

    // Defense: total==0 should never happen (executing thread hasn't decremented yet).
    // Treat as unsafe (assume multi-thread) to avoid unexpected S62 execution.
    if (total == 0) {
#if HZ3_S62_SINGLE_THREAD_FAILFAST
        fprintf(stderr, "[HZ3_S62_SINGLE_THREAD_FAILFAST] total_live==0 during destructor (impossible)\n");
        abort();
#endif
        return 0;
    }

    return (total <= 1);
}
#endif  // HZ3_S62_SINGLE_THREAD_GATE

#if HZ3_S62_ATEXIT_GATE
// ============================================================================
// S62-1A: AtExitGate - One-shot atexit handler
// ============================================================================
static void hz3_s62_atexit_handler(void) {
    // One-shot guard: prevent double execution
    int expected = 0;
    if (!atomic_compare_exchange_strong(&g_s62_atexit_executed, &expected, 1)) {
        return;
    }

    // Uninitialized guard: skip if tcache never initialized
    if (!t_hz3_cache.initialized) {
        return;
    }

#if HZ3_S62_ATEXIT_LOG
    fprintf(stderr, "[HZ3_S62_ATEXIT] S62 cleanup at process exit\n");
#endif

    // Safety check: verify single-thread context
#if HZ3_S62_SINGLE_THREAD_GATE
    int single = hz3_s62_single_thread_ok();
#if HZ3_S62_SINGLE_THREAD_FAILFAST
    if (!single) {
        abort();  // Multi-thread detected: failfast
    }
#endif
    if (!single) {
        return;  // Multi-thread detected: skip S62
    }
#endif

    // Execute S62 sequence: retire → purge
#if HZ3_S62_RETIRE && !HZ3_S62_RETIRE_DISABLE
    hz3_s62_retire();
#endif
#if HZ3_S62_SUB4K_RETIRE && !HZ3_S62_SUB4K_RETIRE_DISABLE
    hz3_s62_sub4k_retire();
#endif
#if HZ3_S62_PURGE && !HZ3_S62_PURGE_DISABLE
    hz3_s62_purge();
#endif
}

static void hz3_s62_atexit_register(void) {
    atexit(hz3_s62_atexit_handler);
}
#endif  // HZ3_S62_ATEXIT_GATE

// ============================================================================
// TLS Destructor
// ============================================================================

static void hz3_tcache_destructor(void* arg) {
    (void)arg;
    if (!t_hz3_cache.initialized) return;

#if HZ3_S12_V2_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S12-3C triage: Aggregate thread stats before cleanup
    hz3_s12_v2_stats_aggregate_thread();
#endif
    // Day 6: Force epoch cleanup first
    hz3_epoch_force();

// Remote path detection for S65/S62 guard
#ifndef HZ3_S42_SMALL_XFER_DISABLE
#define HZ3_S42_SMALL_XFER_DISABLE 0
#endif
#ifndef HZ3_S44_OWNER_STASH_DISABLE
#define HZ3_S44_OWNER_STASH_DISABLE 0
#endif
#ifndef HZ3_REMOTE_ENABLED
#define HZ3_REMOTE_ENABLED \
    ((HZ3_S42_SMALL_XFER && !HZ3_S42_SMALL_XFER_DISABLE) || \
     (HZ3_S44_OWNER_STASH && !HZ3_S44_OWNER_STASH_DISABLE))
#endif

// S65 Remote Guard: Skip S65 ledger flush when remote paths are active
#ifndef HZ3_S65_REMOTE_GUARD
#define HZ3_S65_REMOTE_GUARD 0
#endif
#if !(HZ3_S65_REMOTE_GUARD && HZ3_REMOTE_ENABLED)
#if HZ3_S65_RELEASE_LEDGER
    // S65: Flush TLS ledger (drain all pending entries)
    hz3_s65_ledger_flush_all();
#endif
#endif  // S65_REMOTE_GUARD

#if HZ3_PTAG_DSTBIN_ENABLE
    // S24-1: Full flush on thread exit (event-only)
    hz3_dstbin_flush_remote_all();
#endif

    // S121-D: Flush page packets before flushing other remote state
    hz3_s121d_packet_flush_all_extern();

    // S121-M: Flush pending pageq batch before thread exit
    hz3_s121m_flush_pending_extern();

    // 1. Flush all outboxes (redundant after epoch, but safe)
    for (int owner = 0; owner < HZ3_NUM_SHARDS; owner++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            hz3_outbox_flush((uint8_t)owner, sc);
        }
    }

    // 2. Return bins to central (batch)
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        uint32_t bin_idx = hz3_bin_index_medium(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (head) {
            // Build list: find tail
            void* tail = head;
            uint32_t n = 1;
            void* obj = hz3_obj_get_next(head);
            while (obj) {
                tail = obj;
                n++;
                obj = hz3_obj_get_next(obj);
            }
            hz3_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
            hz3_local_head_clear(bin_idx);
        }
#else
        Hz3Bin* bin = hz3_tcache_get_bin(sc);
        if (!hz3_bin_is_empty(bin)) {
            // Build list: find tail
            void* head = hz3_bin_take_all(bin);
            void* tail = head;
            uint32_t n = 1;
            void* obj = hz3_obj_get_next(head);
            while (obj) {
                tail = obj;
                n++;
                obj = hz3_obj_get_next(obj);
            }
            hz3_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
        }
#endif
    }

    // 3. Return small bins to central
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        // S40: SoA - directly access local_head for small bins
        uint32_t bin_idx = hz3_bin_index_small(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (head) {
            void* tail = head;
            uint32_t n = 1;
            void* cur = hz3_obj_get_next(head);
            while (cur) {
                tail = cur;
                n++;
                cur = hz3_obj_get_next(cur);
            }
#if HZ3_SMALL_V2_ENABLE
            hz3_small_v2_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
#endif
            hz3_local_head_clear(bin_idx);
        }
#else  // !HZ3_TCACHE_SOA_LOCAL
#if HZ3_SMALL_V2_ENABLE
        hz3_small_v2_bin_flush(sc, hz3_tcache_get_small_bin(sc));
#elif HZ3_SMALL_V1_ENABLE
        hz3_small_bin_flush(sc, hz3_tcache_get_small_bin(sc));
#endif
#endif  // HZ3_TCACHE_SOA_LOCAL
    }

#if HZ3_S145_CENTRAL_LOCAL_CACHE
    // S145-A: Flush TLS central cache back to central pool
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        if (t_hz3_cache.s145_cache_count[sc] > 0) {
            void* head = t_hz3_cache.s145_cache_head[sc];
            void* tail = head;
            uint32_t n = 1;
            void* cur = hz3_obj_get_next(head);
            while (cur) {
                tail = cur;
                n++;
                cur = hz3_obj_get_next(cur);
            }
            hz3_small_v2_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
            t_hz3_cache.s145_cache_head[sc] = NULL;
            t_hz3_cache.s145_cache_count[sc] = 0;
        }
    }
#endif  // HZ3_S145_CENTRAL_LOCAL_CACHE

#if HZ3_SUB4K_ENABLE
    // 4. Return sub4k bins to central
    for (int sc = 0; sc < HZ3_SUB4K_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        // S40: SoA - directly access local_head for sub4k bins
        uint32_t bin_idx = hz3_bin_index_sub4k(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (head) {
            void* tail = head;
            uint32_t n = 1;
            void* cur = hz3_obj_get_next(head);
            while (cur) {
                tail = cur;
                n++;
                cur = hz3_obj_get_next(cur);
            }
            hz3_sub4k_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
            hz3_local_head_clear(bin_idx);
        }
#else
        hz3_sub4k_bin_flush(sc, hz3_tcache_get_sub4k_bin(sc));
#endif
    }
#endif

    // =========================================================================
    // S62 Remote Guard: Skip S62 when remote paths (xfer/stash) are enabled
    // =========================================================================
    // Compile-time check: if remote paths are active, S62 may race with
    // objects still in transit, causing memory corruption.
    // Note: HZ3_REMOTE_ENABLED defined earlier in this file (before S65 guard)
    //
    // S62-1G: SingleThreadRetireGate overrides REMOTE_GUARD when total_live==1.
    // This allows RSS reduction in safe single-thread scenarios while maintaining
    // safety in multi-thread environments.

    int s62_allowed = 1;

#if HZ3_S62_REMOTE_GUARD && HZ3_REMOTE_ENABLED
    s62_allowed = 0;  // Remote guard active: block S62 by default
#endif

#if HZ3_S62_SINGLE_THREAD_GATE
    int single = hz3_s62_single_thread_ok();  // total_live <= 1
    if (!s62_allowed && single) {
        s62_allowed = 1;  // single-threadなら remote guard を解除
    }
#if HZ3_S62_SINGLE_THREAD_FAILFAST
    if (!single) {
        uint32_t total = 0;
        for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
            total += hz3_shard_live_count((uint8_t)i);
        }
        fprintf(stderr, "[HZ3_S62_SINGLE_THREAD_FAILFAST] Multi-thread detected (total=%u)\n", total);
        abort();  // multi-thread検出時のみ failfast
    }
#endif
#endif

    if (s62_allowed) {
#if HZ3_S62_RETIRE && !HZ3_S62_RETIRE_DISABLE
        // S62-1: PageRetireBox - retire fully-empty small pages to free_bits
        hz3_s62_retire();
#endif

#if HZ3_S62_SUB4K_RETIRE && !HZ3_S62_SUB4K_RETIRE_DISABLE
        // S62-1b: Sub4kRunRetireBox - retire fully-empty sub4k 2-page runs to free_bits
        hz3_s62_sub4k_retire();
#endif

#if HZ3_S62_PURGE && !HZ3_S62_PURGE_DISABLE
        // S62-2: DtorSmallPagePurgeBox - purge retired small pages
        hz3_s62_purge();
#endif
    }

#if HZ3_S62_OBSERVE && !HZ3_S62_OBSERVE_DISABLE
    // S62-0: OBSERVE - count free_bits purge potential (stats only, no madvise)
    hz3_s62_observe();
#endif

#if HZ3_S61_DTOR_HARD_PURGE
    // S61: Hard purge of central pool (thread-exit, cold path only)
    hz3_s61_dtor_hard_purge();
#endif

    // Note: segment cleanup is deferred to process exit

#if HZ3_LANE_SPLIT
    if (t_hz3_cache.lane) {
        Hz3ShardCore* core = &g_hz3_shards[t_hz3_cache.my_shard];
        if (t_hz3_cache.lane != &core->lane0) {
            hz3_lane_free(t_hz3_cache.lane);
        }
        t_hz3_cache.lane = NULL;
    }
#endif

    // Shard collision tracking: decrement only after flushing all TLS state.
    // If we decrement earlier, S65 reclaim in another thread could release a run
    // while this thread still holds stale pointers in bins.
    {
        uint8_t shard = t_hz3_cache.my_shard;
        if (shard < HZ3_NUM_SHARDS) {
            (void)atomic_fetch_sub_explicit(&g_shard_live_count[shard], 1, memory_order_acq_rel);
        }
    }
}

static void hz3_tcache_create_key(void) {
    pthread_key_create(&g_hz3_tcache_key, hz3_tcache_destructor);
}

// ============================================================================
// TLS Initialization (slow path)
// ============================================================================

void hz3_tcache_ensure_init_slow(void) {
    if (t_hz3_cache.initialized) {
        return;
    }

    // Day 5: Register pthread_key destructor (once per process)
    pthread_once(&g_hz3_tcache_key_once, hz3_tcache_create_key);
    pthread_setspecific(g_hz3_tcache_key, &t_hz3_cache);

#if HZ3_STATS_DUMP && !HZ3_SHIM_FORWARD_ONLY
    // S15: Register atexit stats dump (once per process)
    pthread_once(&g_stats_dump_atexit_once, hz3_stats_register_atexit);
#endif
#if HZ3_S12_V2_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S12-3C triage: Register atexit stats dump (once per process)
    pthread_once(&g_s12_v2_stats_atexit_once, hz3_s12_v2_stats_register_atexit);
#endif
#if HZ3_SEG_SCAVENGE_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // S54: Register atexit scavenge observation dump (once per process)
    pthread_once(&g_scavenge_obs_atexit_once, hz3_seg_scavenge_obs_register_atexit);
#endif
#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // S55: Register atexit retention observation dump (once per process)
    pthread_once(&g_retention_obs_atexit_once, hz3_retention_obs_register_atexit);
#endif
#if HZ3_S62_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // S62-0: Register atexit observation dump (once per process).
    // Best-effort snapshot will run at exit even if thread destructors don't.
    hz3_s62_observe_register_atexit();
#endif
#if HZ3_S62_ATEXIT_GATE && !HZ3_SHIM_FORWARD_ONLY
    // S62-1A: Register atexit S62 cleanup (once per process).
    // Ensures S62 runs at process exit even if thread destructors don't fire.
    pthread_once(&g_s62_atexit_once, hz3_s62_atexit_register);
#endif
#if HZ3_S56_ACTIVE_SET && HZ3_S56_ACTIVE_SET_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S56-2: Register atexit active-set stats dump (once per process)
    hz3_s56_active_set_register_atexit();
#endif
    // 1. Shard assignment FIRST (before any segment creation)
    //
    // Prefer an exclusive shard (live_count==0) to avoid collisions when threads<=shards.
    // If no free shard exists, we must collide (threads > shards).
    uint32_t start = atomic_fetch_add_explicit(&g_shard_counter, 1, memory_order_relaxed);
    uint8_t shard = 0;
    uint32_t prev_live = 0;
    int claimed_exclusive = 0;

    for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
        shard = (uint8_t)((start + i) % HZ3_NUM_SHARDS);
        uint32_t expected = 0;
        if (atomic_compare_exchange_strong_explicit(&g_shard_live_count[shard], &expected, 1,
                memory_order_acq_rel, memory_order_acquire)) {
            claimed_exclusive = 1;
            prev_live = 0;
            break;
        }
    }

    if (!claimed_exclusive) {
        shard = (uint8_t)(start % HZ3_NUM_SHARDS);
        prev_live = atomic_fetch_add_explicit(&g_shard_live_count[shard], 1, memory_order_acq_rel);
    }

    t_hz3_cache.my_shard = shard;
#if HZ3_OWNER_STASH_INSTANCES > 1
    // S144: Initialize instance selection fields (use shard_counter as unique seed)
    t_hz3_cache.owner_stash_seed = start;  // unique per thread
    t_hz3_cache.owner_stash_rr = 0;        // round-robin cursor starts at 0
#endif
#if HZ3_S121_F_PAGEQ_SHARD
    // S121-F: Hash thread ID for sub-shard selection (use shard_counter as unique seed)
    t_hz3_cache.my_tid_hash = start;
#endif
#if HZ3_LANE_SPLIT
    t_hz3_cache.current_seg_base = NULL;
    t_hz3_cache.small_current_seg_base = NULL;

    Hz3ShardCore* core = &g_hz3_shards[shard];
    core->lane0.core = core;
    if (prev_live == 0) {
        t_hz3_cache.lane = &core->lane0;
    } else {
        Hz3Lane* lane = hz3_lane_alloc();
        if (lane) {
            lane->core = core;
            lane->placeholder = 0;
            t_hz3_cache.lane = lane;
        } else {
            t_hz3_cache.lane = &core->lane0;
        }
    }
#endif

    // Precise (concurrent) collision detection:
    // - collision means >1 thread concurrently shares the same shard.
    if (prev_live != 0) {
        atomic_store_explicit(&g_shard_collision_detected, 1, memory_order_release);
#if HZ3_SHARD_COLLISION_FAILFAST
        fprintf(stderr, "[HZ3_SHARD_COLLISION_FAILFAST] shards=%u hit_shard=%u counter=%u\n",
            (unsigned)HZ3_NUM_SHARDS, (unsigned)t_hz3_cache.my_shard,
            (unsigned)atomic_load_explicit(&g_shard_counter, memory_order_relaxed));
        abort();
#elif HZ3_SHARD_COLLISION_SHOT
        if (atomic_exchange_explicit(&g_shard_collision_shot_fired, 1, memory_order_relaxed) == 0) {
            fprintf(stderr, "[HZ3_SHARD_COLLISION] shards=%u (threads>shards) example_shard=%u counter=%u\n",
                (unsigned)HZ3_NUM_SHARDS, (unsigned)t_hz3_cache.my_shard,
                (unsigned)atomic_load_explicit(&g_shard_counter, memory_order_relaxed));
        }
#endif
    }

#if HZ3_OWNER_LEASE_STATS
    hz3_owner_lease_stats_register_atexit();
#endif

    // 2. Initialize central pool (thread-safe via pthread_once)
    hz3_central_init();

    // 3. Initialize inbox pool (thread-safe via pthread_once)
    hz3_inbox_init();

    // 4. Initialize bins and outbox
    // S40-2: LOCAL and BANK can be independently configured
#if HZ3_TCACHE_SOA_LOCAL
    // S40: SoA layout for local bins
    memset(t_hz3_cache.local_head, 0, sizeof(t_hz3_cache.local_head));
    memset(t_hz3_cache.local_count, 0, sizeof(t_hz3_cache.local_count));
#elif HZ3_LOCAL_BINS_SPLIT
    // S33: Unified local_bins (AoS)
    memset(t_hz3_cache.local_bins, 0, sizeof(t_hz3_cache.local_bins));
#else
    // Legacy: separate bins + small_bins (AoS)
    memset(t_hz3_cache.bins, 0, sizeof(t_hz3_cache.bins));
    memset(t_hz3_cache.small_bins, 0, sizeof(t_hz3_cache.small_bins));
#endif

    // === Bank bins (remote free) ===
#if HZ3_REMOTE_STASH_SPARSE
    // S41: Sparse ring initialization
    memset(&t_hz3_cache.remote_stash, 0, sizeof(t_hz3_cache.remote_stash));
#else
#if HZ3_PTAG_DSTBIN_ENABLE
#if HZ3_TCACHE_SOA_BANK
    // S40: SoA layout for bank bins
    memset(t_hz3_cache.bank_head, 0, sizeof(t_hz3_cache.bank_head));
    memset(t_hz3_cache.bank_count, 0, sizeof(t_hz3_cache.bank_count));
#else
    // AoS layout for bank bins
    memset(t_hz3_cache.bank, 0, sizeof(t_hz3_cache.bank));
#endif
#endif
#endif

#if !HZ3_REMOTE_STASH_SPARSE
    memset(t_hz3_cache.outbox, 0, sizeof(t_hz3_cache.outbox));
#endif

#if HZ3_S67_SPILL_ARRAY2
    // S67-2: Initialize spill_count and spill_overflow
    memset(t_hz3_cache.spill_count, 0, sizeof(t_hz3_cache.spill_count));
    memset(t_hz3_cache.spill_overflow, 0, sizeof(t_hz3_cache.spill_overflow));
#elif HZ3_S67_SPILL_ARRAY
    // S67: Initialize spill_count to 0 (spill_array is lazy, count=0 means empty)
    memset(t_hz3_cache.spill_count, 0, sizeof(t_hz3_cache.spill_count));
#elif HZ3_S48_OWNER_STASH_SPILL
    // S48: Initialize stash_spill to NULL
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        t_hz3_cache.stash_spill[sc] = NULL;
    }
#endif
#if HZ3_S94_SPILL_LITE
    // S94: Initialize spill_count and spill_overflow to zero
    // s94_spill_array is lazy (count=0 means empty, no read)
    memset(t_hz3_cache.s94_spill_count, 0, sizeof(t_hz3_cache.s94_spill_count));
    memset(t_hz3_cache.s94_spill_overflow, 0, sizeof(t_hz3_cache.s94_spill_overflow));
#endif
    // S121-E: owner_pages is now GLOBAL (g_owner_pages), not TLS
    // No TLS initialization needed; g_owner_pages is static zero-initialized
#if HZ3_PTAG_DSTBIN_ENABLE && HZ3_PTAG_DSTBIN_TLS
    t_hz3_cache.arena_base =
        atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    t_hz3_cache.page_tag32 = g_hz3_page_tag32;
#endif

    // 5. No current segment yet
    t_hz3_cache.current_seg = NULL;
#if HZ3_LANE_SPLIT
    t_hz3_cache.current_seg_base = NULL;
#endif
#if HZ3_S56_ACTIVE_SET
    t_hz3_cache.active_seg1 = NULL;
#endif
    t_hz3_cache.small_current_seg = NULL;
#if HZ3_LANE_SPLIT
    t_hz3_cache.small_current_seg_base = NULL;
#endif

    // 6. Day 6: Initialize knobs (thread-safe via pthread_once)
    hz3_knobs_init();

    // 7. Day 6: Copy global knobs to TLS snapshot
    t_hz3_cache.knobs = g_hz3_knobs;
    t_hz3_cache.knobs_ver = atomic_load_explicit(&g_hz3_knobs_ver, memory_order_acquire);

    // 8. Day 6: Initialize stats to zero
    memset(&t_hz3_cache.stats, 0, sizeof(t_hz3_cache.stats));

#if HZ3_S58_TCACHE_DECAY
    // 9. S58: Initialize decay tracking fields
    hz3_s58_init();
#if HZ3_S58_STATS
    hz3_s58_stats_register_atexit();
#endif
#endif

    t_hz3_cache.initialized = 1;
}

int hz3_shard_collision_detected(void) {
    return atomic_load_explicit(&g_shard_collision_detected, memory_order_acquire) != 0;
}

uint32_t hz3_shard_live_count(uint8_t shard) {
    if (shard >= HZ3_NUM_SHARDS) {
        return 0;
    }
    return atomic_load_explicit(&g_shard_live_count[shard], memory_order_acquire);
}

// ============================================================================
// Day 5: Slow path with batch refill (inbox -> central -> segment)
// ============================================================================

void* hz3_alloc_slow(int sc) {
#if HZ3_S72_MEDIUM_DEBUG
#define HZ3_S72_MEDIUM_CHECK(where, ptr) do { \
    if ((ptr) != NULL) { \
        hz3_medium_boundary_check_ptr((where), (ptr), (sc), t_hz3_cache.my_shard); \
    } \
} while (0)
#else
#define HZ3_S72_MEDIUM_CHECK(where, ptr) ((void)0)
#endif
#if HZ3_WATCH_PTR_BOX
#define HZ3_WATCH_PTR_MEDIUM(where, ptr) do { \
    if ((ptr) != NULL) { \
        hz3_watch_ptr_on_alloc((where), (ptr), (sc), t_hz3_cache.my_shard); \
    } \
} while (0)
#else
#define HZ3_WATCH_PTR_MEDIUM(where, ptr) ((void)0)
#endif
#if HZ3_TCACHE_INIT_ON_MISS
    // S32-1: TLS init at slow path entry (hot path skips init check)
    hz3_tcache_ensure_init_slow();
#endif
#if HZ3_ARENA_PRESSURE_BOX
    hz3_pressure_check_and_flush();
#endif
#if HZ3_TCACHE_SOA_LOCAL
    uint32_t bin_idx = hz3_bin_index_medium(sc);
    Hz3BinRef binref = hz3_tcache_get_local_binref(bin_idx);
#else
    Hz3Bin* bin = hz3_tcache_get_bin(sc);
#endif
    int want = HZ3_REFILL_BATCH[sc];

    // Day 6: Update stats
    t_hz3_cache.stats.refill_calls[sc]++;

#if HZ3_PTAG_DSTBIN_ENABLE
    // S24-1: Budgeted flush (round-robin, fixed cost per refill)
#if HZ3_DSTBIN_REMOTE_HINT_ENABLE
    if (t_hz3_cache.remote_hint) {
        hz3_dstbin_flush_remote_budget(HZ3_DSTBIN_FLUSH_BUDGET_BINS);
    }
#else
    hz3_dstbin_flush_remote_budget(HZ3_DSTBIN_FLUSH_BUDGET_BINS);
#endif
#endif

    // 1. Drain inbox first (already batches to bin)
#if HZ3_TCACHE_SOA_LOCAL
    // For SoA, inbox_drain needs Hz3Bin* but we pass NULL and handle manually
#if HZ3_BIN_SPLIT_COUNT
    // S122: Extract pointer from tagged value for tmp_bin
    Hz3Bin tmp_bin = { .head = hz3_split_ptr(*binref.head), .count = *binref.count };
    void* obj = hz3_inbox_drain(t_hz3_cache.my_shard, sc, &tmp_bin);
    // Re-pack: use the new head with a recalculated count
    // Since inbox_drain modifies tmp_bin.count, we need to sync both
    uint32_t total_count = ((uint32_t)tmp_bin.count << HZ3_SPLIT_CNT_SHIFT);
    uint32_t new_low = total_count & (uint32_t)HZ3_SPLIT_CNT_MASK;
    *binref.count = (Hz3BinCount)(total_count >> HZ3_SPLIT_CNT_SHIFT);
    *binref.head = hz3_split_pack(tmp_bin.head, new_low);
#else
    Hz3Bin tmp_bin = { .head = *binref.head, .count = *binref.count };
    void* obj = hz3_inbox_drain(t_hz3_cache.my_shard, sc, &tmp_bin);
    *binref.head = tmp_bin.head;
#if !HZ3_BIN_LAZY_COUNT
    *binref.count = tmp_bin.count;
#endif
#endif  // HZ3_BIN_SPLIT_COUNT
#else
    void* obj = hz3_inbox_drain(t_hz3_cache.my_shard, sc, bin);
#endif
    if (obj) {
        HZ3_S72_MEDIUM_CHECK("medium_alloc_inbox", obj);
        HZ3_WATCH_PTR_MEDIUM("medium_alloc_inbox", obj);
        hz3_epoch_maybe();  // Day 6: epoch check
        return obj;
    }

    // 2. Batch pop from central pool
    void* batch[16];
    int got = hz3_central_pop_batch(t_hz3_cache.my_shard, sc, batch, want);
    if (got > 0) {
        // Push remaining to bin, return first
        for (int i = 1; i < got; i++) {
            HZ3_S72_MEDIUM_CHECK("medium_alloc_central_fill", batch[i]);
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(binref, batch[i]);
#else
            hz3_bin_push(bin, batch[i]);
#endif
        }
        // Day 6: Update stats
        t_hz3_cache.stats.central_pop_hit[sc]++;
        HZ3_S72_MEDIUM_CHECK("medium_alloc_central", batch[0]);
        HZ3_WATCH_PTR_MEDIUM("medium_alloc_central", batch[0]);
        hz3_epoch_maybe();  // Day 6: epoch check
        return batch[0];
    }
    // Day 6: Update stats
    t_hz3_cache.stats.central_pop_miss[sc]++;

    // 3. Batch allocate from segment
#if HZ3_SPAN_CARVE_ENABLE
    // S26-2: Try span carve for sc=6,7 (28KB, 32KB)
    if (sc >= 6) {
#if HZ3_TCACHE_SOA_LOCAL
        Hz3Bin tmp_bin2 = { .head = *binref.head, .count = *binref.count };
        obj = hz3_slow_alloc_span_carve(sc, &tmp_bin2);
        *binref.head = tmp_bin2.head;
#if !HZ3_BIN_LAZY_COUNT
        *binref.count = tmp_bin2.count;
#endif
#else
        obj = hz3_slow_alloc_span_carve(sc, bin);
#endif
        if (obj) {
            HZ3_S72_MEDIUM_CHECK("medium_alloc_span", obj);
            HZ3_WATCH_PTR_MEDIUM("medium_alloc_span", obj);
            hz3_epoch_maybe();
            return obj;
        }
    }
#endif
    // Fallback: normal batch allocation
    obj = NULL;
#if HZ3_S74_LANE_BATCH
    // S202: Eco Mode adaptive burst sizing
#if HZ3_ECO_MODE
    const int refill_burst = hz3_eco_refill_burst();
#define HZ3_TCACHE_BURST_ARRAY_SIZE HZ3_ECO_REFILL_BURST_ARRAY_SIZE
#else
    const int refill_burst = HZ3_S74_REFILL_BURST;
#define HZ3_TCACHE_BURST_ARRAY_SIZE HZ3_S74_REFILL_BURST
#endif
    int remaining = want;
    while (remaining > 0) {
        int burst = remaining;
        if (burst > refill_burst) {
            burst = refill_burst;
        }
        void* burst_batch[HZ3_TCACHE_BURST_ARRAY_SIZE];
        int got = hz3_s74_alloc_from_segment_burst(sc, burst_batch, burst);
        if (got <= 0) {
            break;
        }
        for (int i = 0; i < got; i++) {
            void* run = burst_batch[i];
            if (!obj) {
                obj = run;  // First one to return
                continue;
            }
            HZ3_S72_MEDIUM_CHECK("medium_alloc_segment_fill", run);
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(binref, run);
#else
            hz3_bin_push(bin, run);
#endif
        }
        remaining -= got;
        if (got < burst) {
            break;
        }
    }
#undef HZ3_TCACHE_BURST_ARRAY_SIZE
#endif
    if (!obj) {
        for (int i = 0; i < want; i++) {
            void* run = hz3_slow_alloc_from_segment(sc);
            if (!run) break;
            if (i == 0) {
                obj = run;  // First one to return
            } else {
                HZ3_S72_MEDIUM_CHECK("medium_alloc_segment_fill", run);
#if HZ3_TCACHE_SOA_LOCAL
                hz3_binref_push(binref, run);
#else
                hz3_bin_push(bin, run);
#endif
            }
        }
    }
    HZ3_S72_MEDIUM_CHECK("medium_alloc_segment", obj);
    HZ3_WATCH_PTR_MEDIUM("medium_alloc_segment", obj);
    hz3_epoch_maybe();  // Day 6: epoch check
    return obj;
}
#undef HZ3_S72_MEDIUM_CHECK
#undef HZ3_WATCH_PTR_MEDIUM
