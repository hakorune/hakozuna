#ifndef HZ10_ORPHAN_REGISTRY_STATS_H
#define HZ10_ORPHAN_REGISTRY_STATS_H

#include <stdint.h>

typedef struct Hz10OrphanRegistryPurgeStats {
  uint64_t pages_seen;
  uint64_t pages_drained;
  uint64_t pages_reclaimed;
  uint64_t pages_busy;
  uint64_t pages_skipped_live_owner;
  uint64_t slots_merged;
} Hz10OrphanRegistryPurgeStats;

#endif
