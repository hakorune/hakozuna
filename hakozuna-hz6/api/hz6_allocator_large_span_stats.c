#include "hz6_allocator.h"

#include "../include/hz6_contract_route.h"

void hz6_allocator_note_large_span_central_push(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.large_span_central_push;
  }
}

void hz6_allocator_note_large_span_central_pop(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.large_span_central_pop;
  }
}

void hz6_allocator_note_source_alloc_for_front(Hz6Allocator* allocator,
                                               uint16_t front_id) {
  if (!allocator) {
    return;
  }

  ++allocator->stats.source_alloc;
  if (front_id == HZ6_FRONT_LARGE) {
    ++allocator->stats.large_span_source_alloc;
  }
}
