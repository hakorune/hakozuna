#ifndef HZ6_STATS_SNAPSHOT_H
#define HZ6_STATS_SNAPSHOT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6Allocator Hz6Allocator;

#ifndef HZ6_STATS_CLASS_COUNT
#define HZ6_STATS_CLASS_COUNT 16u
#endif

#ifndef HZ6_ROUTE_PROBE_BUCKET_COUNT
#define HZ6_ROUTE_PROBE_BUCKET_COUNT 6u
#endif

typedef struct Hz6StatsSnapshot {
#include "hz6_stats_snapshot_fields.inc"
} Hz6StatsSnapshot;

#ifdef __cplusplus
}
#endif

#endif
