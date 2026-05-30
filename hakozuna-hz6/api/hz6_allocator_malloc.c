#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

void* hz6_malloc(Hz6Allocator* allocator, size_t size) {
  if (!allocator) {
    return NULL;
  }
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    return NULL;
  }

  uint16_t class_id = 0;
  const Hz6FrontOps* front = hz6_front_for_allocation(size, 16, &class_id);
  if (!front || !front->alloc) {
    ++allocator->stats.alloc_fail;
    return NULL;
  }

  void* ptr = front->alloc(allocator, class_id, size);
  if (!ptr) {
    ++allocator->stats.alloc_fail;
  }
  return ptr;
}
