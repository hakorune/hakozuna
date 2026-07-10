#ifndef HZ12_RETIRED_RECLAIM_DETACH_H
#define HZ12_RETIRED_RECLAIM_DETACH_H

#include <stdint.h>
#include <stdio.h>

#include "hz12_owner_registry.h"

typedef struct H12RetiredReclaimDetachResult {
  uint32_t budget;
  uint32_t candidates;
  uint32_t attempts;
  uint32_t succeeded;
  uint32_t failed;
  uint64_t detached_bytes;
} H12RetiredReclaimDetachResult;

int h12_retired_reclaim_detach_bounded(
    H12OwnerToken owner, uint32_t budget,
    H12RetiredReclaimDetachResult* out);
void h12_retired_reclaim_detach_dump(
    FILE* out, const H12RetiredReclaimDetachResult* result);

#endif /* HZ12_RETIRED_RECLAIM_DETACH_H */
