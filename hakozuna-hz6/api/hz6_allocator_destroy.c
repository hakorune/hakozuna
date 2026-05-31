#include "hz6_allocator.h"

#include "hz6_allocator_destroy_internal.h"

void hz6_allocator_destroy(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  allocator->owner.state = HZ6_OWNER_DYING;
  hz6_allocator_route_visibility_unregister(allocator);
  hz6_allocator_destroy_descriptors(allocator);
  hz6_allocator_destroy_source_blocks(allocator);
  allocator->owner.state = HZ6_OWNER_DEAD;
}
