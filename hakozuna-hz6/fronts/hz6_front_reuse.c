#include "hz6_front_util.h"

static void* hz6_front_materialize_descriptorless_entry(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    Hz6FrontCacheEntry entry,
    Hz6AllocPath* path) {
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  const size_t entry_bytes = hz6_frontcache_entry_bytes(&entry);
  const uint16_t entry_class_id = hz6_frontcache_entry_class_id(&entry);
  if (!allocator || entry.descriptor || !entry.ptr || !entry.source_block ||
      entry_bytes == 0 || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return NULL;
  }

  Hz6SourceBlock* block = (Hz6SourceBlock*)entry.source_block;
  if (!block->active || !block->ptr || entry_class_id != class_id ||
      class_id != block->run_class_id) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descriptorless_frontcache_invalid;
#endif
    return NULL;
  }

  Hz6ObjectDescriptor* descriptor = NULL;
#if HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
  descriptor = (Hz6ObjectDescriptor*)entry.reserved_descriptor;
  if (!descriptor || descriptor->ptr ||
      descriptor->state != HZ6_STATE_DESCRIPTOR_RESERVED) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descriptorreserve_frontcache_missing;
#endif
    return NULL;
  }
#else
#if HZ6_DESCRIPTOR_COLD_GOV_L1
  if (hz6_frontcache_entry_descgov_detached(&entry) &&
      !hz6_allocator_descgov_descriptor_available(allocator)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descgov_materialize_block_no_descriptor;
    if (class_id < HZ6_STATS_CLASS_COUNT) {
      ++allocator->stats.descgov_materialize_fail_class[class_id];
    }
#endif
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  if (hz6_frontcache_entry_descgov_detached(&entry)) {
    ++allocator->stats.descgov_materialize_admit;
  }
#endif
#endif
  descriptor = hz6_allocator_find_free_descriptor(allocator);
#endif
  if (!descriptor) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descriptorless_frontcache_descriptor_fail;
#if HZ6_DESCRIPTOR_COLD_GOV_L1
    ++allocator->stats.descgov_materialize_fail;
    if (class_id < HZ6_STATS_CLASS_COUNT) {
      ++allocator->stats.descgov_materialize_fail_class[class_id];
    }
#endif
#endif
    return NULL;
  }

  if (!hz6_allocator_prepare_descriptor(
          allocator, descriptor, entry.ptr, entry_bytes, block->ptr,
          block->bytes, block, class_id, block->source_kind,
          block->source_release, HZ6_STATE_ACTIVE)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descriptorless_frontcache_invalid;
#if HZ6_DESCRIPTOR_COLD_GOV_L1
    ++allocator->stats.descgov_materialize_fail;
    if (class_id < HZ6_STATS_CLASS_COUNT) {
      ++allocator->stats.descgov_materialize_fail_class[class_id];
    }
#endif
#endif
    return NULL;
  }

  if (!hz6_allocator_route_register_exact_reason(
          allocator, entry.ptr, entry_bytes, front_id, class_id,
          descriptor->generation, descriptor,
          HZ6_ROUTE_REGISTER_REASON_MATERIALIZE)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descriptorless_frontcache_route_fail;
#if HZ6_DESCRIPTOR_COLD_GOV_L1
    ++allocator->stats.descgov_materialize_fail;
    if (class_id < HZ6_STATS_CLASS_COUNT) {
      ++allocator->stats.descgov_materialize_fail_class[class_id];
    }
#endif
#endif
#if HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
    hz6_allocator_reserve_descriptor_keep_source_slot(allocator, descriptor);
#else
    hz6_allocator_detach_descriptor_keep_source_slot(allocator, descriptor);
#endif
    return NULL;
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.descriptorless_frontcache_pop;
#if HZ6_DESCRIPTOR_COLD_GOV_L1
  if (hz6_frontcache_entry_descgov_detached(&entry) &&
      allocator->stats.descgov_detached_current > 0) {
    --allocator->stats.descgov_detached_current;
  }
#endif
#if HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
  ++allocator->stats.descriptorreserve_frontcache_pop;
#endif
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
    if (hz6_allocator_activate_descriptor(allocator, descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
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
    if (hz6_allocator_activate_descriptor(allocator, descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
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
