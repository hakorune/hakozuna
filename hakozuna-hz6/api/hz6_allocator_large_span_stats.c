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
  switch (front_id) {
    case HZ6_FRONT_LOCAL2P:
      ++allocator->stats.local2p_source_alloc;
      break;
    case HZ6_FRONT_MIDPAGE:
      ++allocator->stats.midpage_source_alloc;
      break;
    case HZ6_FRONT_LARGE:
      ++allocator->stats.large_span_source_alloc;
      ++allocator->stats.large_source_alloc;
      break;
    case HZ6_FRONT_TOY:
      ++allocator->stats.toy_source_alloc;
      break;
    default:
      break;
  }
}

void hz6_allocator_note_front_alloc_path(Hz6Allocator* allocator,
                                         uint16_t front_id,
                                         Hz6AllocPath path) {
  size_t front_index = 0;

  if (!allocator || path >= HZ6_ALLOC_PATH_COUNT) {
    return;
  }
  if (!hz6_front_attr_index_from_id(front_id, &front_index) ||
      front_index >= HZ6_FRONT_ATTR_COUNT) {
    return;
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.front_alloc_path[front_index][path];
#else
  (void)front_index;
#endif
}
