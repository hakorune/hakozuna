#include "hz6_front.h"

size_t hz6_front_prefill_by_id(Hz6Allocator* allocator,
                               uint16_t front_id,
                               size_t count) {
  return hz6_front_prefill_by_id_class(allocator, front_id, 0, count);
}

size_t hz6_front_prefill_by_id_class(Hz6Allocator* allocator,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t count) {
  const Hz6FrontOps* front = hz6_front_for_id(front_id);
  if (!front || !front->prefill) {
    return 0;
  }
  return front->prefill(allocator, class_id, count);
}

size_t hz6_front_prefill_for_allocation(Hz6Allocator* allocator,
                                        size_t size,
                                        size_t align,
                                        size_t count) {
  uint16_t class_id = 0;
  const Hz6FrontOps* front =
      hz6_front_for_allocation(size, align, &class_id);
  if (!front || !front->prefill) {
    return 0;
  }
  return front->prefill(allocator, class_id, count);
}
