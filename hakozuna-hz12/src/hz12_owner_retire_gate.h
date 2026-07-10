#ifndef HZ12_OWNER_RETIRE_GATE_H
#define HZ12_OWNER_RETIRE_GATE_H

#include <stdint.h>
#include <stdio.h>

#include "hz12_owner_registry.h"

typedef struct H12OwnerRetireGateStats {
  uint64_t query;
  uint64_t open;
  uint64_t blocked_epoch;
  uint64_t blocked_inbox;
  uint32_t last_pending;
} H12OwnerRetireGateStats;

int h12_owner_retire_gate_ready(H12OwnerToken owner);
void h12_owner_retire_gate_reset(void);
void h12_owner_retire_gate_stats(H12OwnerRetireGateStats* out);
void h12_owner_retire_gate_dump(FILE* out);

#endif /* HZ12_OWNER_RETIRE_GATE_H */
