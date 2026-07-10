#ifndef HZ12_RETIRED_RECLAIM_DECOMMIT_H
#define HZ12_RETIRED_RECLAIM_DECOMMIT_H

#include <stdint.h>
#include <stdio.h>

#include "hz12_owner_registry.h"

typedef struct H12RetiredReclaimDecommitResult {
  uint32_t budget;
  uint32_t candidates;
  uint32_t attempts;
  uint32_t succeeded;
  uint32_t failed;
  uint64_t decommitted_bytes;
} H12RetiredReclaimDecommitResult;

int h12_retired_reclaim_decommit_bounded(
    H12OwnerToken owner, uint32_t budget,
    H12RetiredReclaimDecommitResult* out);
void h12_retired_reclaim_decommit_dump(
    FILE* out, const H12RetiredReclaimDecommitResult* result);

#endif /* HZ12_RETIRED_RECLAIM_DECOMMIT_H */
