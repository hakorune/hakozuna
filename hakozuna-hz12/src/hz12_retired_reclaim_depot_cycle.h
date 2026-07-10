#ifndef HZ12_RETIRED_RECLAIM_DEPOT_CYCLE_H
#define HZ12_RETIRED_RECLAIM_DEPOT_CYCLE_H

#include <stdint.h>
#include <stdio.h>

#include "hz12_owner_registry.h"

typedef struct H12RetiredReclaimDepotCycleResult {
  uint32_t budget;
  uint32_t candidates;
  uint32_t put_attempts;
  uint32_t put_succeeded;
  uint32_t take_attempts;
  uint32_t take_succeeded;
  uint32_t address_mismatch;
  uint32_t owner_mismatch;
  uint32_t checkpoint_count;
  uint32_t depot_remaining;
  uint64_t recommitted_bytes;
} H12RetiredReclaimDepotCycleResult;

int h12_retired_reclaim_depot_cycle_bounded(
    H12OwnerToken owner, uint32_t budget,
    H12RetiredReclaimDepotCycleResult* out);
void h12_retired_reclaim_depot_cycle_dump(
    FILE* out, const H12RetiredReclaimDepotCycleResult* result);

#endif /* HZ12_RETIRED_RECLAIM_DEPOT_CYCLE_H */
