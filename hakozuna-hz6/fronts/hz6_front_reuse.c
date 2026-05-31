#include "hz6_front_util.h"

void* hz6_front_reuse_cached_or_transfer(Hz6Allocator* allocator,
                                         uint16_t front_id,
                                         uint16_t class_id,
                                         Hz6AllocPath* path) {
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
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_reuse_hit;
#endif
      if (path) {
        *path = HZ6_ALLOC_PATH_LOCAL_REUSE;
      } else {
        hz6_allocator_note_front_alloc_path(allocator, front_id,
                                            HZ6_ALLOC_PATH_LOCAL_REUSE);
      }
      return entry.ptr;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_reuse_invalid;
#endif
  }

  if (!hz6_allocator_profile_transfer_first(allocator)) {
    return NULL;
  }

  return hz6_front_reuse_transfer(allocator, front_id, class_id, path);
}

void* hz6_front_reuse_transfer_or_cached(Hz6Allocator* allocator,
                                         uint16_t front_id,
                                         uint16_t class_id,
                                         Hz6AllocPath* path) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return NULL;
  }

  if (hz6_allocator_profile_transfer_first(allocator)) {
    void* reused = hz6_front_reuse_transfer(allocator, front_id, class_id,
                                            path);
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
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_reuse_hit;
#endif
      if (path) {
        *path = HZ6_ALLOC_PATH_LOCAL_REUSE;
      } else {
        hz6_allocator_note_front_alloc_path(allocator, front_id,
                                            HZ6_ALLOC_PATH_LOCAL_REUSE);
      }
      return entry.ptr;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_reuse_invalid;
#endif
  }

  return NULL;
}
