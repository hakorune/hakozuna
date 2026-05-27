#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

size_t hz6_allocator_prefill_size(Hz6Allocator* allocator,
                                  size_t size,
                                  size_t count) {
  if (!allocator || size == 0 || count == 0) {
    return 0;
  }
  return hz6_front_prefill_for_allocation(allocator, size, 16, count);
}

size_t hz6_allocator_prefill_front(Hz6Allocator* allocator,
                                   uint16_t front_id,
                                   size_t count) {
  return hz6_allocator_prefill_front_class(allocator, front_id, 0, count);
}

size_t hz6_allocator_prefill_front_class(Hz6Allocator* allocator,
                                         uint16_t front_id,
                                         uint16_t class_id,
                                         size_t count) {
  if (!allocator || count == 0) {
    return 0;
  }
  return hz6_front_prefill_by_id_class(allocator, front_id, class_id, count);
}
