#ifndef HZ12_OWNER_BATCH_LEDGER_H
#define HZ12_OWNER_BATCH_LEDGER_H

#include <stdint.h>

#include "hz12_owner_registry.h"

typedef struct H12OwnerBatchLedgerRecord {
  uint32_t span_id;
  uint32_t slot_capacity;
  uint32_t carved_slots;
  uint32_t outstanding_slots;
  uint32_t owner_drain_objects;
  uint32_t rejected_transitions;
  H12OwnerToken owner;
  uint8_t class_id;
  uint8_t full_span;
  uint8_t ledger_candidate;
} H12OwnerBatchLedgerRecord;

typedef struct H12OwnerBatchLedgerAgreement {
  uint32_t compared_spans;
  uint32_t matching_spans;
  uint32_t mismatched_spans;
  uint32_t ledger_candidates;
  uint32_t atomic_candidates;
} H12OwnerBatchLedgerAgreement;

void h12_owner_batch_ledger_reset(void);
uint32_t h12_owner_batch_ledger_acquire_range(H12OwnerToken owner,
                                              void* const* items,
                                              uint32_t count);
uint32_t h12_owner_batch_ledger_owner_drain_range(H12OwnerToken owner,
                                                  void* const* items,
                                                  uint32_t count);
uint32_t h12_owner_batch_ledger_return_range(H12OwnerToken owner,
                                             void* const* items,
                                             uint32_t count);
uint32_t h12_owner_batch_ledger_return_owned_range(H12OwnerToken owner,
                                                   void* const* items,
                                                   uint32_t count);
int h12_owner_batch_ledger_query(const void* ptr,
                                 H12OwnerBatchLedgerRecord* out);
H12OwnerBatchLedgerAgreement h12_owner_batch_ledger_compare_atomic(void);
H12OwnerBatchLedgerAgreement h12_owner_batch_ledger_compare_owner_atomic(
    H12OwnerToken owner);
int h12_owner_batch_ledger_recycle_span(const void* ptr,
                                        H12OwnerToken owner);

#endif /* HZ12_OWNER_BATCH_LEDGER_H */
