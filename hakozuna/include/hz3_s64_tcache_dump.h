#pragma once

#include "hz3_config.h"

#if HZ3_S64_TCACHE_DUMP

// ============================================================================
// S64-1: TCacheDumpBox - supply objects to central for retire
// ============================================================================
//
// Dumps tcache local bins (small_v2 only) to central during epoch.
// This supplies objects for S64_RetireScanBox to find empty pages.
//
// Prerequisites: HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
// Hot path: 0 (epoch tick only)
// Thread boundary: my_shard fixed (TLS-based)
// Budget: from t_hz3_cache.knobs.s64_tdump_budget_objs

// Called from hz3_epoch_force() BEFORE S64_RetireScanBox.
// Order: TCacheDumpBox -> RetireScanBox -> PurgeDelayBox
void hz3_s64_tcache_dump_tick(void);

#else  // !HZ3_S64_TCACHE_DUMP

static inline void hz3_s64_tcache_dump_tick(void) {}

#endif  // HZ3_S64_TCACHE_DUMP
