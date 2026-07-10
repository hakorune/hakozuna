#include "hz12_owner_batch_count_ledger.h"

#include "hz12_span.h"

#include <string.h>

typedef struct H12OwnerBatchCountSpan {
  H12OwnerToken owner;
  uint32_t carved_slots;
  uint32_t outstanding_slots;
  uint32_t rejected_transitions;
  uint8_t class_plus_one;
} H12OwnerBatchCountSpan;

static H12OwnerBatchCountSpan h12_batch_count_spans[HZ12_SPAN_COUNT];

static int h12_batch_count_span_id(const void* ptr, uint32_t* span_id) {
  uintptr_t value;
  uintptr_t base;
  if (!ptr || !span_id || !hz12_arena_base) return 0;
  value = (uintptr_t)ptr;
  base = (uintptr_t)hz12_arena_base;
  if (value < base || value - base >= (uintptr_t)HZ12_ARENA_BYTES) return 0;
  *span_id = (uint32_t)((value - base) >> HZ12_SPAN_SHIFT);
  return 1;
}

static int h12_batch_count_owner_matches(const H12OwnerBatchCountSpan* span,
                                         H12OwnerToken owner) {
  return span->class_plus_one != 0u && span->owner.slot == owner.slot &&
         span->owner.generation == owner.generation;
}

void h12_owner_batch_count_reset(void) {
  memset(h12_batch_count_spans, 0, sizeof(h12_batch_count_spans));
}

int h12_owner_batch_count_assign_span(const void* span_base,
                                      H12OwnerToken owner,
                                      uint8_t class_id) {
  H12OwnerBatchCountSpan* span;
  uint32_t span_id;
  if (owner.generation == 0u || class_id >= HZ12_CLASS_COUNT ||
      !h12_batch_count_span_id(span_base, &span_id)) {
    return 0;
  }
  span = &h12_batch_count_spans[span_id];
  if (span->class_plus_one != 0u && span->outstanding_slots != 0u &&
      !h12_batch_count_owner_matches(span, owner)) {
    span->rejected_transitions += 1u;
    return 0;
  }
  span->owner = owner;
  span->class_plus_one = (uint8_t)(class_id + 1u);
  return 1;
}

uint32_t h12_owner_batch_count_acquire_contiguous(
    H12OwnerToken owner, void* first, size_t slot_bytes, uint32_t count) {
  H12OwnerBatchCountSpan* span;
  uintptr_t offset;
  uint32_t span_id;
  uint32_t first_slot;
  if (!first || slot_bytes == 0u || count == 0u ||
      !h12_batch_count_span_id(first, &span_id)) {
    return 0u;
  }
  span = &h12_batch_count_spans[span_id];
  if (!h12_batch_count_owner_matches(span, owner)) {
    span->rejected_transitions += 1u;
    return 0u;
  }
  offset = ((uintptr_t)first - (uintptr_t)hz12_arena_base) &
           (HZ12_SPAN_BYTES - 1u);
  if ((offset % slot_bytes) != 0u ||
      offset + (size_t)count * slot_bytes > HZ12_SPAN_BYTES) {
    span->rejected_transitions += 1u;
    return 0u;
  }
  first_slot = (uint32_t)(offset / slot_bytes);
  if (span->carved_slots < first_slot + count) {
    span->carved_slots = first_slot + count;
  }
  span->outstanding_slots += count;
  return count;
}

uint32_t h12_owner_batch_count_acquire_range(H12OwnerToken owner,
                                             void* const* items,
                                             uint32_t count) {
  uint32_t accepted = 0u;
  for (uint32_t i = 0u; items && i < count; ++i) {
    H12OwnerBatchCountSpan* span;
    uint32_t span_id;
    if (!h12_batch_count_span_id(items[i], &span_id)) continue;
    span = &h12_batch_count_spans[span_id];
    if (!h12_batch_count_owner_matches(span, owner)) {
      span->rejected_transitions += 1u;
      continue;
    }
    span->outstanding_slots += 1u;
    accepted += 1u;
  }
  return accepted;
}

uint32_t h12_owner_batch_count_return_owned_range(H12OwnerToken owner,
                                                  void* const* items,
                                                  uint32_t count) {
  uint32_t accepted = 0u;
  for (uint32_t i = 0u; items && i < count; ++i) {
    H12OwnerBatchCountSpan* span;
    uint32_t span_id;
    if (!h12_batch_count_span_id(items[i], &span_id)) continue;
    span = &h12_batch_count_spans[span_id];
    if (!h12_batch_count_owner_matches(span, owner)) continue;
    if (span->outstanding_slots == 0u) {
      span->rejected_transitions += 1u;
      continue;
    }
    span->outstanding_slots -= 1u;
    accepted += 1u;
  }
  return accepted;
}

int h12_owner_batch_count_query(const void* ptr,
                                H12OwnerBatchCountRecord* out) {
  H12OwnerBatchCountSpan* span;
  uint32_t span_id;
  uint32_t capacity;
  uint8_t class_id;
  size_t slot_bytes;
  if (!out || !h12_batch_count_span_id(ptr, &span_id)) return 0;
  span = &h12_batch_count_spans[span_id];
  if (span->class_plus_one == 0u) return 0;
  class_id = (uint8_t)(span->class_plus_one - 1u);
  slot_bytes = hz12_class_slot_size(class_id);
  capacity = slot_bytes ? (uint32_t)(HZ12_SPAN_BYTES / slot_bytes) : 0u;
  memset(out, 0, sizeof(*out));
  out->owner = span->owner;
  out->carved_slots = span->carved_slots;
  out->outstanding_slots = span->outstanding_slots;
  out->rejected_transitions = span->rejected_transitions;
  out->class_id = class_id;
  out->candidate = (uint8_t)(capacity != 0u &&
      span->carved_slots == capacity && span->outstanding_slots == 0u &&
      span->rejected_transitions == 0u);
  return 1;
}
