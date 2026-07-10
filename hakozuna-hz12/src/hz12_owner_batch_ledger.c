#include "hz12_owner_batch_ledger.h"

#include "hz12_span.h"
#include "hz12_span_owner_shadow.h"

#include <string.h>

#define H12_LEDGER_WORD_BITS 64u
#define H12_LEDGER_MAX_SLOTS (HZ12_SPAN_BYTES / HZ12_MIN_SIZE)
#define H12_LEDGER_WORDS_PER_SPAN \
  ((H12_LEDGER_MAX_SLOTS + H12_LEDGER_WORD_BITS - 1u) / H12_LEDGER_WORD_BITS)

typedef struct H12OwnerBatchLedgerSpan {
  H12OwnerToken owner;
  uint32_t carved_slots;
  uint32_t outstanding_slots;
  uint32_t owner_drain_objects;
  uint32_t rejected_transitions;
  uint8_t class_plus_one;
} H12OwnerBatchLedgerSpan;

static H12OwnerBatchLedgerSpan h12_ledger_spans[HZ12_SPAN_COUNT];
static uint64_t
    h12_ledger_outstanding[HZ12_SPAN_COUNT][H12_LEDGER_WORDS_PER_SPAN];

static int h12_ledger_span_id(const void* ptr, uint32_t* span_id) {
  uintptr_t value;
  uintptr_t base;
  if (!ptr || !span_id || !hz12_arena_base) return 0;
  value = (uintptr_t)ptr;
  base = (uintptr_t)hz12_arena_base;
  if (value < base || value - base >= (uintptr_t)HZ12_ARENA_BYTES) return 0;
  *span_id = (uint32_t)((value - base) >> HZ12_SPAN_SHIFT);
  return 1;
}

static int h12_ledger_location(const void* ptr, uint32_t* span_id,
                               uint32_t* slot_index, uint8_t* class_id) {
  uintptr_t value;
  uintptr_t base;
  uintptr_t offset;
  size_t slot_bytes;
  if (!ptr || !span_id || !slot_index || !class_id || !hz12_arena_base) {
    return 0;
  }
  value = (uintptr_t)ptr;
  base = (uintptr_t)hz12_arena_base;
  if (value < base || value - base >= (uintptr_t)HZ12_ARENA_BYTES) return 0;
  offset = value - base;
  *span_id = (uint32_t)(offset >> HZ12_SPAN_SHIFT);
  if (!hz12_span_classify(ptr, class_id)) return 0;
  slot_bytes = hz12_class_slot_size(*class_id);
  if (slot_bytes == 0u) return 0;
  offset &= (uintptr_t)(HZ12_SPAN_BYTES - 1u);
  if ((offset % slot_bytes) != 0u) return 0;
  *slot_index = (uint32_t)(offset / slot_bytes);
  return *slot_index < H12_LEDGER_MAX_SLOTS;
}

static int h12_ledger_owner_matches(uint32_t span_id, H12OwnerToken owner) {
  H12OwnerToken observed;
  return h12_span_owner_shadow_query(span_id, &observed) &&
         observed.slot == owner.slot &&
         observed.generation == owner.generation;
}

static int h12_ledger_prepare(H12OwnerBatchLedgerSpan* span,
                              uint32_t span_id, uint8_t class_id,
                              H12OwnerToken owner) {
  if (!h12_ledger_owner_matches(span_id, owner)) return 0;
  if (span->class_plus_one == 0u) {
    span->owner = owner;
    span->class_plus_one = (uint8_t)(class_id + 1u);
    return 1;
  }
  return span->owner.slot == owner.slot &&
         span->owner.generation == owner.generation &&
         span->class_plus_one == (uint8_t)(class_id + 1u);
}

void h12_owner_batch_ledger_reset(void) {
  memset(h12_ledger_spans, 0, sizeof(h12_ledger_spans));
  memset(h12_ledger_outstanding, 0, sizeof(h12_ledger_outstanding));
}

uint32_t h12_owner_batch_ledger_acquire_range(H12OwnerToken owner,
                                              void* const* items,
                                              uint32_t count) {
  uint32_t accepted = 0u;
  for (uint32_t i = 0u; items && i < count; ++i) {
    H12OwnerBatchLedgerSpan* span;
    uint32_t span_id, slot_index, word;
    uint64_t mask;
    uint8_t class_id;
    if (!h12_ledger_location(items[i], &span_id, &slot_index, &class_id)) {
      continue;
    }
    span = &h12_ledger_spans[span_id];
    if (!h12_ledger_prepare(span, span_id, class_id, owner)) {
      span->rejected_transitions += 1u;
      continue;
    }
    word = slot_index / H12_LEDGER_WORD_BITS;
    mask = UINT64_C(1) << (slot_index % H12_LEDGER_WORD_BITS);
    if ((h12_ledger_outstanding[span_id][word] & mask) != 0u) {
      span->rejected_transitions += 1u;
      continue;
    }
    h12_ledger_outstanding[span_id][word] |= mask;
    if (span->carved_slots < slot_index + 1u) {
      span->carved_slots = slot_index + 1u;
    }
    span->outstanding_slots += 1u;
    accepted += 1u;
  }
  return accepted;
}

uint32_t h12_owner_batch_ledger_owner_drain_range(H12OwnerToken owner,
                                                  void* const* items,
                                                  uint32_t count) {
  uint32_t accepted = 0u;
  for (uint32_t i = 0u; items && i < count; ++i) {
    H12OwnerBatchLedgerSpan* span;
    uint32_t span_id, slot_index, word;
    uint64_t mask;
    uint8_t class_id;
    if (!h12_ledger_location(items[i], &span_id, &slot_index, &class_id)) {
      continue;
    }
    span = &h12_ledger_spans[span_id];
    word = slot_index / H12_LEDGER_WORD_BITS;
    mask = UINT64_C(1) << (slot_index % H12_LEDGER_WORD_BITS);
    if (!h12_ledger_prepare(span, span_id, class_id, owner) ||
        (h12_ledger_outstanding[span_id][word] & mask) == 0u) {
      span->rejected_transitions += 1u;
      continue;
    }
    span->owner_drain_objects += 1u;
    accepted += 1u;
  }
  return accepted;
}

uint32_t h12_owner_batch_ledger_return_range(H12OwnerToken owner,
                                             void* const* items,
                                             uint32_t count) {
  uint32_t accepted = 0u;
  for (uint32_t i = 0u; items && i < count; ++i) {
    H12OwnerBatchLedgerSpan* span;
    uint32_t span_id, slot_index, word;
    uint64_t mask;
    uint8_t class_id;
    if (!h12_ledger_location(items[i], &span_id, &slot_index, &class_id)) {
      continue;
    }
    span = &h12_ledger_spans[span_id];
    word = slot_index / H12_LEDGER_WORD_BITS;
    mask = UINT64_C(1) << (slot_index % H12_LEDGER_WORD_BITS);
    if (!h12_ledger_prepare(span, span_id, class_id, owner) ||
        (h12_ledger_outstanding[span_id][word] & mask) == 0u ||
        span->outstanding_slots == 0u) {
      span->rejected_transitions += 1u;
      continue;
    }
    h12_ledger_outstanding[span_id][word] &= ~mask;
    span->outstanding_slots -= 1u;
    accepted += 1u;
  }
  return accepted;
}

int h12_owner_batch_ledger_query(const void* ptr,
                                 H12OwnerBatchLedgerRecord* out) {
  H12OwnerBatchLedgerSpan* span;
  uint32_t span_id, capacity;
  uint8_t class_id;
  size_t slot_bytes;
  if (!out || !h12_ledger_span_id(ptr, &span_id)) return 0;
  span = &h12_ledger_spans[span_id];
  if (span->class_plus_one == 0u) return 0;
  class_id = (uint8_t)(span->class_plus_one - 1u);
  slot_bytes = hz12_class_slot_size(class_id);
  capacity = slot_bytes != 0u ? (uint32_t)(HZ12_SPAN_BYTES / slot_bytes) : 0u;
  memset(out, 0, sizeof(*out));
  out->span_id = span_id;
  out->slot_capacity = capacity;
  out->carved_slots = span->carved_slots;
  out->outstanding_slots = span->outstanding_slots;
  out->owner_drain_objects = span->owner_drain_objects;
  out->rejected_transitions = span->rejected_transitions;
  out->owner = span->owner;
  out->class_id = class_id;
  out->full_span = (uint8_t)(capacity != 0u && span->carved_slots == capacity);
  out->ledger_candidate = (uint8_t)(out->full_span &&
      span->outstanding_slots == 0u && span->rejected_transitions == 0u);
  return 1;
}

int h12_owner_batch_ledger_recycle_span(const void* ptr,
                                        H12OwnerToken owner) {
  H12OwnerBatchLedgerSpan* span;
  uint32_t span_id, slot_index;
  uint8_t class_id;
  if (!h12_ledger_location(ptr, &span_id, &slot_index, &class_id) ||
      !h12_ledger_owner_matches(span_id, owner)) {
    return 0;
  }
  (void)slot_index;
  span = &h12_ledger_spans[span_id];
  memset(span, 0, sizeof(*span));
  memset(h12_ledger_outstanding[span_id], 0,
         sizeof(h12_ledger_outstanding[span_id]));
  span->owner = owner;
  span->class_plus_one = (uint8_t)(class_id + 1u);
  return 1;
}
