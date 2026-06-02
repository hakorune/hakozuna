#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"
#include "../fronts/hz6_front_util.h"

#if HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1 || HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
static int hz6_allocator_direct_local_alloc_front_eligible(
    uint16_t front_id) {
  return front_id == HZ6_FRONT_TOY || front_id == HZ6_FRONT_MIDPAGE ||
         front_id == HZ6_FRONT_LOCAL2P;
}
#endif

#if HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
static void* hz6_allocator_direct_local_reuse(Hz6Allocator* allocator,
                                              uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
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
            descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
            hz6_allocator_owner_token(allocator))) {
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
#endif

#if HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
static void* hz6_allocator_direct_local_alloc(Hz6Allocator* allocator,
                                              uint16_t front_id,
                                              uint16_t class_id) {
  if (!hz6_allocator_direct_local_alloc_front_eligible(front_id)) {
    return NULL;
  }
#if HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
  return hz6_allocator_direct_local_reuse(allocator, class_id);
#else
  return hz6_front_reuse_cached_or_transfer(allocator, front_id, class_id,
                                            NULL);
#endif
}
#endif

void* hz6_malloc(Hz6Allocator* allocator, size_t size) {
  if (!allocator) {
    return NULL;
  }
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    return NULL;
  }

  uint16_t class_id = 0;
  const Hz6FrontOps* front = hz6_front_for_allocation(size, 16, &class_id);
  if (!front || !front->alloc) {
    ++allocator->stats.alloc_fail;
    return NULL;
  }

#if HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
  void* direct_ptr = hz6_allocator_direct_local_alloc(
      allocator, front->front_id, class_id);
  if (direct_ptr) {
    return direct_ptr;
  }
#endif

  void* ptr = front->alloc(allocator, class_id, size);
  if (!ptr) {
    ++allocator->stats.alloc_fail;
  }
  return ptr;
}
