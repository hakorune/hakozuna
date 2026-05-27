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
    if (hz6_allocator_activate_descriptor(
            descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
            hz6_allocator_owner_token(allocator))) {
      return entry.ptr;
    }
  }

  Hz6ObjectDescriptor* descriptor = NULL;
  while (hz6_allocator_large_span_pool_pop(allocator, class_id, &descriptor)) {
    if (!descriptor) {
      continue;
    }
    if (hz6_allocator_activate_descriptor(
            descriptor, HZ6_STATE_CENTRAL_FREE, descriptor->ptr,
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

  descriptor->state = HZ6_STATE_CENTRAL_FREE;
  descriptor->owner = (Hz6OwnerToken){0};
  if (hz6_allocator_large_span_pool_push(allocator, descriptor)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_ACTIVE;
  descriptor->owner = allocator->owner.token;
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
  descriptor->owner = (Hz6OwnerToken){0};
  if (hz6_allocator_large_span_pool_push(allocator, descriptor)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_ACTIVE;
  descriptor->owner = allocator->owner.token;
  return hz6_allocator_remote_free_active_descriptor(allocator, descriptor,
                                                     ptr);
}
