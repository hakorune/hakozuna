#include "hz6_allocator.h"

int hz6_allocator_cache_active_descriptor(Hz6Allocator* allocator,
                                          Hz6ObjectDescriptor* descriptor,
                                          void* ptr) {
  if (!allocator || !descriptor || !ptr ||
      descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr ||
      descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !hz6_owner_equal(descriptor->owner, allocator->owner.token)) {
    return 0;
  }

  descriptor->state = HZ6_STATE_LOCAL_FREE;
  Hz6FrontCacheEntry entry;
  entry.ptr = ptr;
  entry.descriptor = descriptor;
  entry.class_id = descriptor->class_id;
  entry.generation = descriptor->generation;
  if (hz6_allocator_frontcache_push(allocator, entry.class_id, entry)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_DEAD;
  hz6_allocator_route_unregister_exact(allocator, ptr);
  hz6_allocator_release_descriptor_source(descriptor);
  return 1;
}

int hz6_allocator_remote_free_active_descriptor(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr) {
  if (!allocator || !descriptor || !ptr ||
      descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr) {
    return 0;
  }

  if (hz6_allocator_profile_strict_owner_remote(allocator)) {
    if (!hz6_owner_equal(descriptor->owner, allocator->owner.token)) {
      return 0;
    }
    descriptor->state = HZ6_STATE_REMOTE_PENDING;
    return 1;
  }

  Hz6TransferObject object;
  object.ptr = ptr;
  object.descriptor = descriptor;
  object.class_id = descriptor->class_id;
  object.generation = descriptor->generation;

  descriptor->state = HZ6_STATE_TRANSFER_FREE;
  descriptor->owner = (Hz6OwnerToken){0};
  if (!hz6_allocator_transfer_push(allocator, object)) {
    descriptor->state = HZ6_STATE_ACTIVE;
    descriptor->owner = allocator->owner.token;
    return 0;
  }

  hz6_allocator_note_transfer_push(allocator);
  return 1;
}
