#include "hz6_allocator.h"

size_t hz6_allocator_drain_remote_pending(Hz6Allocator* allocator) {
  if (!allocator) {
    return 0;
  }
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    return 0;
  }

  size_t drained = 0;
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr || descriptor->state != HZ6_STATE_REMOTE_PENDING ||
        descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
      continue;
    }
    if (!hz6_allocator_descriptor_owner_equal(allocator, descriptor,
                                              allocator->owner.token)) {
      continue;
    }

    Hz6FrontCacheEntry entry = {0};
    entry.ptr = descriptor->ptr;
    entry.descriptor = descriptor;
    entry.bytes = descriptor->bytes;
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
    entry.source_block = descriptor->source_block;
#endif
    entry.class_id = descriptor->class_id;
    entry.generation = descriptor->generation;
    if (!hz6_allocator_frontcache_push(allocator, entry.class_id, entry)) {
      continue;
    }

    descriptor->state = HZ6_STATE_LOCAL_FREE;
    ++drained;
  }
  return drained;
}
