#pragma once

#include "hz3_types.h"

// ============================================================================
// LaneSplit: lane-local state (future-facing, lane0 embedded in shard core)
// ============================================================================

struct Hz3ShardCore;

typedef struct Hz3Lane {
    struct Hz3ShardCore* core;
    uint8_t placeholder;
} Hz3Lane;

typedef struct Hz3ShardCore {
    Hz3Lane lane0;
} Hz3ShardCore;

#if HZ3_LANE_SPLIT
extern Hz3ShardCore g_hz3_shards[HZ3_NUM_SHARDS];
#endif
