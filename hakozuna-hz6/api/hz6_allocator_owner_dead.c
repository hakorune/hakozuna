#include "hz6_allocator.h"

void hz6_allocator_mark_owner_dead(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  Hz6OwnerToken old_owner = allocator->owner.token;
  allocator->owner.state = HZ6_OWNER_DEAD;

  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr ||
        !hz6_allocator_descriptor_owner_equal_at(
            allocator, descriptor, old_owner,
            HZ6_OWNER_EQUAL_SITE_OWNER_DEAD)) {
      continue;
    }
    if (descriptor->state == HZ6_STATE_ACTIVE ||
        descriptor->state == HZ6_STATE_LOCAL_FREE ||
        descriptor->state == HZ6_STATE_REMOTE_PENDING) {
      descriptor->state = HZ6_STATE_ORPHAN;
    }
  }
}
