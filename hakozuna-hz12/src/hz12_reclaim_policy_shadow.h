#ifndef HZ12_RECLAIM_POLICY_SHADOW_H
#define HZ12_RECLAIM_POLICY_SHADOW_H

#include <stdint.h>

#include "hz12_owner_registry.h"

#define HZ12_RECLAIM_POLICY_MIN_BYTES (4u * 1024u * 1024u)
#define HZ12_RECLAIM_POLICY_MAX_SPANS 64u

typedef struct H12ReclaimPolicyShadow {
  uint32_t owner_spans;
  uint32_t complete_spans;
  uint32_t incomplete_spans;
  uint32_t planned_spans;
  uint32_t flush_owner_pending;
  uint64_t reclaimable_bytes;
  uint64_t planned_bytes;
  uint8_t owner_gate_open;
  uint8_t flush_owner_found;
  uint8_t threshold_met;
  uint8_t would_reclaim;
} H12ReclaimPolicyShadow;

int h12_reclaim_policy_shadow_query(H12OwnerToken owner,
                                    H12ReclaimPolicyShadow* out);

#endif /* HZ12_RECLAIM_POLICY_SHADOW_H */
