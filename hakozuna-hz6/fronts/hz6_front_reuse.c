#include "hz6_front_util.h"

static void* hz6_front_materialize_descriptorless_entry(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    Hz6FrontCacheEntry entry,
    Hz6AllocPath* path) {
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  if (!allocator || entry.descriptor || !entry.ptr || !entry.source_block ||
      entry.bytes == 0 || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return NULL;
  }

  Hz6SourceBlock* block = (Hz6SourceBlock*)entry.source_block;
  if (!block->active || !block->ptr || entry.class_id != class_id ||
      class_id != block->run_class_id) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descriptorless_frontcache_invalid;
#endif
    return NULL;
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descriptorless_frontcache_descriptor_fail;
#endif
    return NULL;
  }

  if (!hz6_allocator_prepare_descriptor(
          allocator, descriptor, entry.ptr, entry.bytes, block->ptr,
          block->bytes, block, class_id, block->source_kind,
          block->source_release, HZ6_STATE_ACTIVE)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descriptorless_frontcache_invalid;
#endif
    return NULL;
  }

  if (!hz6_allocator_route_register_exact(
          allocator, entry.ptr, entry.bytes, front_id, class_id,
          descriptor->generation, descriptor)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descriptorless_frontcache_route_fail;
#endif
    hz6_allocator_detach_descriptor_keep_source_slot(descriptor);
    return NULL;
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.descriptorless_frontcache_pop;
  ++allocator->stats.frontcache_reuse_hit;
#endif
  if (path) {
    *path = HZ6_ALLOC_PATH_LOCAL_REUSE;
  } else {
    hz6_allocator_note_front_alloc_path(allocator, front_id,
                                        HZ6_ALLOC_PATH_LOCAL_REUSE);
  }
  return entry.ptr;
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  (void)entry;
  (void)path;
  return NULL;
#endif
}

void* hz6_front_reuse_cached_or_transfer(Hz6Allocator* allocator,
                                         uint16_t front_id,
                                         uint16_t class_id,
                                         Hz6AllocPath* path) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return NULL;
  }

  Hz6FrontCacheEntry entry;
  while (hz6_allocator_frontcache_pop(allocator, class_id, &entry)) {
    if (!entry.descriptor) {
      void* materialized = hz6_front_materialize_descriptorless_entry(
          allocator, front_id, class_id, entry, path);
      if (materialized) {
        return materialized;
      }
      (void)hz6_allocator_frontcache_push(allocator, class_id, entry);
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_reuse_invalid;
#endif
      return NULL;
    }
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
    if (!entry.descriptor) {
      void* materialized = hz6_front_materialize_descriptorless_entry(
          allocator, front_id, class_id, entry, path);
      if (materialized) {
        return materialized;
      }
      (void)hz6_allocator_frontcache_push(allocator, class_id, entry);
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_reuse_invalid;
#endif
      return NULL;
    }
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
