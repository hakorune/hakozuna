#ifndef HZ12_OWNER_LEDGER_RETIRE_GATE_H
#define HZ12_OWNER_LEDGER_RETIRE_GATE_H

#include <stdint.h>

#include "hz12_owner_registry.h"

typedef struct H12OwnerLedgerRetireGate {
  uint32_t flush_owner_pending;
  uint32_t compared_spans;
  uint32_t mismatched_spans;
  uint8_t owner_gate_open;
  uint8_t flush_owner_found;
  uint8_t ledger_agrees;
  uint8_t gate_open;
} H12OwnerLedgerRetireGate;

int h12_owner_ledger_retire_gate_query(H12OwnerToken owner,
                                       H12OwnerLedgerRetireGate* out);

#endif /* HZ12_OWNER_LEDGER_RETIRE_GATE_H */
