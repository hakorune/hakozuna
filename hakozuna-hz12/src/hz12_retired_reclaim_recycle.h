#ifndef HZ12_RETIRED_RECLAIM_RECYCLE_H
#define HZ12_RETIRED_RECLAIM_RECYCLE_H

#include <stdint.h>
#include <stdio.h>

#include "hz12_owner_registry.h"

typedef struct H12RetiredReclaimRecycleResult {
  uint32_t budget;
  uint32_t initial_put;
  uint32_t take_succeeded;
  uint32_t spans_exercised;
  uint32_t second_detach;
  uint32_t second_decommit;
  uint32_t final_put;
  uint32_t depot_final;
  uint32_t address_mismatch;
  uint32_t owner_mismatch;
  uint32_t failures;
  uint64_t slots_touched;
  uint64_t bytes_redecommitted;
} H12RetiredReclaimRecycleResult;

int h12_retired_reclaim_recycle_bounded(
    H12OwnerToken owner, uint32_t budget,
    H12RetiredReclaimRecycleResult* out);
void h12_retired_reclaim_recycle_dump(
    FILE* out, const H12RetiredReclaimRecycleResult* result);

#endif /* HZ12_RETIRED_RECLAIM_RECYCLE_H */
