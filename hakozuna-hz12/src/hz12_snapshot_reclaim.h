#ifndef HZ12_SNAPSHOT_RECLAIM_H
#define HZ12_SNAPSHOT_RECLAIM_H

#include <stdint.h>

#include "hz12_owner_registry.h"

typedef struct H12SnapshotReclaimResult {
  uint32_t budget;
  uint32_t candidates;
  uint32_t detached;
  uint32_t decommitted;
  uint32_t failed;
  uint64_t decommitted_bytes;
} H12SnapshotReclaimResult;

int h12_snapshot_reclaim_retired_bounded(H12OwnerToken owner,
                                         uint32_t budget,
                                         H12SnapshotReclaimResult* out);

#endif /* HZ12_SNAPSHOT_RECLAIM_H */
