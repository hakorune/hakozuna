#ifndef HZ12_RETIRED_RECLAIM_SHADOW_H
#define HZ12_RETIRED_RECLAIM_SHADOW_H

#include <stdint.h>
#include <stdio.h>

#include "hz12_owner_registry.h"

typedef struct H12RetiredReclaimShadow {
  uint32_t owner_spans;
  uint32_t stale_generation_spans;
  uint32_t accounting_candidates;
  uint32_t detach_ready_spans;
  uint32_t reclaimable_spans;
  uint32_t blocked_owner_gate;
  uint32_t blocked_foreign_cache_scope;
  uint32_t blocked_accounting;
  uint32_t blocked_current;
  uint32_t blocked_front_cache;
  uint32_t blocked_returned_sink;
  uint32_t blocked_route;
  uint64_t detach_ready_bytes;
  uint64_t reclaimable_bytes;
  uint64_t accounting_candidate_bytes;
  uint8_t owner_gate_open;
  uint8_t thread_scope_complete;
} H12RetiredReclaimShadow;

H12RetiredReclaimShadow h12_retired_reclaim_shadow_scan(
    H12OwnerToken owner, int thread_scope_complete);
void h12_retired_reclaim_shadow_dump(FILE* out,
                                     const H12RetiredReclaimShadow* result);

#endif /* HZ12_RETIRED_RECLAIM_SHADOW_H */
