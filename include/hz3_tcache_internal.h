// hz3_tcache_internal.h - Internal header for tcache module split
// NOT part of public API. Used only by hz3_tcache_*.c files.
#pragma once

#include "hz3_config.h"
#include "hz3_tcache.h"
#include "hz3_central.h"
#include "hz3_small_v2.h"
#include "hz3_sub4k.h"

#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// TLS and Global Variables (defined in hz3_tcache_core.c)
// ============================================================================

// Thread-local cache (already extern in hz3_tcache.h)
// extern __thread Hz3TCache t_hz3_cache;

// Day 4: Round-robin shard assignment
extern _Atomic uint32_t g_shard_counter;

#if HZ3_SHARD_COLLISION_SHOT
extern _Atomic int g_shard_collision_shot_fired;
#endif

// Day 5: Thread exit cleanup via pthread_key
extern pthread_key_t g_hz3_tcache_key;
extern pthread_once_t g_hz3_tcache_key_once;

// ============================================================================
// Stats Registration Functions (defined in hz3_tcache_stats.c)
// ============================================================================

#if HZ3_STATS_DUMP && !HZ3_SHIM_FORWARD_ONLY
extern pthread_once_t g_stats_dump_atexit_once;
void hz3_stats_register_atexit(void);
void hz3_stats_aggregate_thread(void);
#endif

#if HZ3_S12_V2_STATS && !HZ3_SHIM_FORWARD_ONLY
extern pthread_once_t g_s12_v2_stats_atexit_once;
void hz3_s12_v2_stats_register_atexit(void);
void hz3_s12_v2_stats_aggregate_thread(void);
#endif


#if HZ3_SEG_SCAVENGE_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
extern pthread_once_t g_scavenge_obs_atexit_once;
void hz3_seg_scavenge_obs_register_atexit(void);
void hz3_seg_scavenge_observe(void);
#endif

#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
extern pthread_once_t g_retention_obs_atexit_once;
void hz3_retention_obs_register_atexit(void);
// hz3_retention_observe_event is public (declared in hz3_tcache.h or elsewhere)
#endif

#if HZ3_S56_ACTIVE_SET && HZ3_S56_ACTIVE_SET_STATS && !HZ3_SHIM_FORWARD_ONLY
extern _Atomic uint64_t g_s56_active_choose_alt;
void hz3_s56_active_set_register_atexit(void);
#endif

#if HZ3_OOM_SHOT
void hz3_medium_oom_dump(int sc);
extern _Atomic uint64_t g_medium_page_alloc_sc[HZ3_NUM_SC];
#endif


// ============================================================================
// Remote Stash Functions (defined in hz3_tcache_remote.c)
// ============================================================================

#if HZ3_REMOTE_STASH_SPARSE
void hz3_remote_stash_flush_budget_impl(uint32_t budget_entries);
void hz3_remote_stash_flush_all_impl(void);
#endif

#if HZ3_PTAG_DSTBIN_ENABLE && !HZ3_REMOTE_STASH_SPARSE
void hz3_dstbin_flush_one(uint8_t dst, int bin);
#endif

// ============================================================================
// Alloc Functions (defined in hz3_tcache_alloc.c)
// ============================================================================

void* hz3_slow_alloc_from_segment(int sc);
#if HZ3_S74_LANE_BATCH
int hz3_s74_alloc_from_segment_burst(int sc, void** out, int want);
#endif

#if HZ3_SPAN_CARVE_ENABLE
void* hz3_slow_alloc_span_carve(int sc, Hz3Bin* bin);
#endif

// ============================================================================
// Pressure Functions (defined in hz3_tcache_pressure.c)
// ============================================================================

#if HZ3_ARENA_PRESSURE_BOX
void hz3_tcache_flush_medium_to_central(void);
void hz3_tcache_flush_small_to_central(void);
#endif

// ============================================================================
// Utility: atomic max for size_t (used by stats)
// ============================================================================

static inline void hz3_atomic_max_size(_Atomic size_t* dst, size_t val) {
    size_t cur = atomic_load_explicit(dst, memory_order_relaxed);
    while (val > cur) {
        if (atomic_compare_exchange_weak_explicit(
                dst, &cur, val,
                memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}
