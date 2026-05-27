#include "hz6_front_util.h"

void* hz6_front_reuse_cached_or_transfer(Hz6Allocator* allocator,
                                         uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
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

  if (!hz6_allocator_profile_transfer_first(allocator)) {
    return NULL;
  }

  return hz6_front_reuse_transfer(allocator, class_id);
}

void* hz6_front_reuse_transfer_or_cached(Hz6Allocator* allocator,
                                         uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return NULL;
  }

  if (hz6_allocator_profile_transfer_first(allocator)) {
    void* reused = hz6_front_reuse_transfer(allocator, class_id);
    if (reused) {
      return reused;
    }
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

  return NULL;
}
