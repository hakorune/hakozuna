#ifndef HZ12_OWNER_BATCH_COUNT_LEDGER_H
#define HZ12_OWNER_BATCH_COUNT_LEDGER_H

#include <stddef.h>
#include <stdint.h>

#include "hz12_owner_registry.h"

typedef struct H12OwnerBatchCountRecord {
  H12OwnerToken owner;
  uint32_t carved_slots;
  uint32_t outstanding_slots;
  uint32_t rejected_transitions;
  uint8_t class_id;
  uint8_t candidate;
} H12OwnerBatchCountRecord;

void h12_owner_batch_count_reset(void);
int h12_owner_batch_count_assign_span(const void* span_base,
                                      H12OwnerToken owner,
                                      uint8_t class_id);
uint32_t h12_owner_batch_count_acquire_contiguous(
    H12OwnerToken owner, void* first, size_t slot_bytes, uint32_t count);
uint32_t h12_owner_batch_count_acquire_range(H12OwnerToken owner,
                                             void* const* items,
                                             uint32_t count);
uint32_t h12_owner_batch_count_return_owned_range(H12OwnerToken owner,
                                                  void* const* items,
                                                  uint32_t count);
int h12_owner_batch_count_query(const void* ptr,
                                H12OwnerBatchCountRecord* out);

#endif /* HZ12_OWNER_BATCH_COUNT_LEDGER_H */
