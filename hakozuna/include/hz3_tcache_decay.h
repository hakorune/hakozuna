#pragma once

#include "hz3_config.h"

#if HZ3_S58_TCACHE_DECAY

// ============================================================================
// S58: TCache Decay Box API
// ============================================================================
//
// S58-1: Adjust bin_target based on overage + lowwater (epoch boundary)
//        Reuses existing hz3_bin_trim() for actual dump to central.
//
// S58-2: Reclaim objects from central to segment (epoch boundary, budgeted)
//        Calls hz3_segment_free_run() to increase segment free_pages.

// S58-1: Adjust bin targets (called at epoch before trim)
// - Observes current len vs max_len (overage detection)
// - Shrinks max_len after HZ3_S58_MAX_OVERAGES consecutive overages
// - Sets bin_target = lowwater / 2 (floor = refill_batch)
void hz3_s58_adjust_targets(void);

// S58-2: Reclaim from central to segment (called at epoch after trim)
// - Pops objects from my_shard's central bins
// - Returns them to segment via hz3_segment_free_run()
// - Budgeted: stops after HZ3_S58_RECLAIM_BUDGET objects
void hz3_s58_central_reclaim(void);

// S58: Initialize decay fields in TLS (called from hz3_tcache_ensure_init_slow)
void hz3_s58_init(void);

#if HZ3_S58_STATS
// S58: Register atexit handler for stats dump
void hz3_s58_stats_register_atexit(void);
#endif

#endif  // HZ3_S58_TCACHE_DECAY
