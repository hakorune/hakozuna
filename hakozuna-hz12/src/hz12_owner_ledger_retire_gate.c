#include "hz12_owner_ledger_retire_gate.h"

#include "hz12_flush_owner_route.h"
#include "hz12_owner_batch_ledger.h"
#include "hz12_owner_retire_gate.h"

#include <string.h>

int h12_owner_ledger_retire_gate_query(H12OwnerToken owner,
                                       H12OwnerLedgerRetireGate* out) {
  H12OwnerBatchLedgerAgreement agreement;
  if (!out || owner.generation == 0u) return 0;
  memset(out, 0, sizeof(*out));
  out->owner_gate_open = (uint8_t)h12_owner_retire_gate_ready(owner);
  out->flush_owner_found = (uint8_t)hz12_flush_owner_route_pending(
      owner.slot, owner.generation, &out->flush_owner_pending);
  agreement = h12_owner_batch_ledger_compare_owner_atomic(owner);
  out->compared_spans = agreement.compared_spans;
  out->mismatched_spans = agreement.mismatched_spans;
  out->ledger_agrees = (uint8_t)(agreement.compared_spans != 0u &&
                                 agreement.mismatched_spans == 0u);
  out->gate_open = (uint8_t)(out->owner_gate_open && out->flush_owner_found &&
                             out->flush_owner_pending == 0u &&
                             out->ledger_agrees);
  return 1;
}
