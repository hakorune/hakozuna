#include "hz6_large128_front.h"

#include "../hz6_front_util.h"

void* hz6_large128_reuse_cached_or_central(Hz6Allocator* allocator,
                                           uint16_t class_id) {
  if (!allocator || class_id != HZ6_LARGE128_CLASS_ID) {
    return NULL;
  }

  Hz6FrontCacheEntry entry;
  while (hz6_allocator_frontcache_pop(allocator, class_id, &entry)) {
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    if (hz6_allocator_activate_descriptor(allocator, descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
            hz6_allocator_owner_token(allocator))) {
      return entry.ptr;
    }
  }

  Hz6ObjectDescriptor* descriptor = NULL;
  while (hz6_allocator_large_span_pool_pop(allocator, class_id, &descriptor)) {
    if (!descriptor) {
      continue;
    }
    if (hz6_allocator_activate_descriptor(allocator, descriptor, HZ6_STATE_CENTRAL_FREE, descriptor->ptr,
            descriptor->generation, hz6_allocator_owner_token(allocator))) {
      return descriptor->ptr;
    }
  }

  return NULL;
}

int hz6_large128_free_local_or_central(Hz6Allocator* allocator,
                                       void* ptr,
                                       Hz6RouteResult route) {
  if (!allocator || !ptr || route.kind != HZ6_ROUTE_VALID ||
      !route.descriptor || route.class_id != HZ6_LARGE128_CLASS_ID) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr ||
      !hz6_allocator_descriptor_owner_equal(allocator, descriptor,
                                            allocator->owner.token)) {
    return 0;
  }

  descriptor->state = HZ6_STATE_LOCAL_FREE;
  Hz6FrontCacheEntry entry = {0};
  entry.ptr = ptr;
  entry.descriptor = descriptor;
  entry.bytes = descriptor->bytes;
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  entry.source_block = descriptor->source_block;
#endif
  entry.class_id = descriptor->class_id;
  entry.generation = descriptor->generation;
  if (hz6_allocator_frontcache_push(allocator, entry.class_id, entry)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_CENTRAL_FREE;
  hz6_allocator_set_descriptor_owner(allocator, descriptor,
                                     (Hz6OwnerToken){0});
  if (hz6_allocator_large_span_pool_push(allocator, descriptor)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_ACTIVE;
  hz6_allocator_set_descriptor_owner(allocator, descriptor,
                                     allocator->owner.token);
  return hz6_allocator_remote_free_active_descriptor(allocator, descriptor,
                                                     ptr);
}

int hz6_large128_free_remote_or_central(Hz6Allocator* allocator,
                                        void* ptr,
                                        Hz6RouteResult route) {
  if (!allocator || !ptr || route.kind != HZ6_ROUTE_VALID ||
      !route.descriptor || route.class_id != HZ6_LARGE128_CLASS_ID) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr) {
    return 0;
  }

  if (hz6_allocator_profile_strict_owner_remote(allocator)) {
    return hz6_allocator_remote_free_active_descriptor(allocator, descriptor,
                                                       ptr);
  }

  descriptor->state = HZ6_STATE_CENTRAL_FREE;
  hz6_allocator_set_descriptor_owner(allocator, descriptor,
                                     (Hz6OwnerToken){0});
  if (hz6_allocator_large_span_pool_push(allocator, descriptor)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_ACTIVE;
  hz6_allocator_set_descriptor_owner(allocator, descriptor,
                                     allocator->owner.token);
  return hz6_allocator_remote_free_active_descriptor(allocator, descriptor,
                                                     ptr);
}
