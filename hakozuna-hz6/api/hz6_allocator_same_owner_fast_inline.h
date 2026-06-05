#ifndef HZ6_ALLOCATOR_SAME_OWNER_FAST_INLINE_H
#define HZ6_ALLOCATOR_SAME_OWNER_FAST_INLINE_H

static inline int hz6_allocator_same_owner_fast_front_eligible_inline(
    uint16_t front_id) {
  return front_id == HZ6_FRONT_TOY || front_id == HZ6_FRONT_MIDPAGE ||
         front_id == HZ6_FRONT_LOCAL2P;
}

static inline void* hz6_allocator_same_owner_fast_alloc_inline(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !hz6_allocator_same_owner_fast_front_eligible_inline(front_id)) {
    return NULL;
  }

  Hz6FrontCacheEntry entry;
  while (hz6_allocator_frontcache_pop(allocator, class_id, &entry)) {
    if (!entry.descriptor) {
      (void)hz6_allocator_frontcache_push(allocator, class_id, entry);
      return NULL;
    }

    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    if (descriptor->class_id == class_id &&
        hz6_allocator_activate_descriptor(
            allocator, descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr,
            entry.generation, hz6_allocator_owner_token(allocator))) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_reuse_hit;
#endif
      return entry.ptr;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_reuse_invalid;
#endif
  }

  return NULL;
}

static inline int hz6_allocator_same_owner_fast_free_inline(
    Hz6Allocator* allocator,
    void* ptr,
    Hz6RouteResult route) {
  if (!allocator || !ptr ||
      !hz6_allocator_same_owner_fast_front_eligible_inline(route.front_id)) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (!descriptor ||
      !hz6_allocator_descriptor_owner_equal_at(
          allocator, descriptor, allocator->owner.token,
          HZ6_OWNER_EQUAL_SITE_SAME_OWNER_FAST)) {
    return 0;
  }

  return hz6_allocator_cache_active_descriptor(allocator, descriptor, ptr);
}

#endif
