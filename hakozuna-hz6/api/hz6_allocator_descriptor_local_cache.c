#include "hz6_allocator.h"

#if HZ6_DESCRIPTORLESS_OVER_CAP_ONLY_L1
static int hz6_allocator_frontcache_class_over_soft_cap(
    Hz6Allocator* allocator,
    uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  size_t capacity = hz6_allocator_frontcache_capacity(allocator, class_id);
  if (capacity == 0) {
    return 0;
  }
  size_t soft_cap = capacity / 8;
  if (soft_cap < 4) {
    soft_cap = capacity < 4 ? capacity : 4;
  }
  if (soft_cap == 0) {
    soft_cap = 1;
  }
  return hz6_allocator_frontcache_count(allocator, class_id) >= soft_cap;
}
#endif

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
  Hz6FrontCacheEntry entry = {0};
  entry.ptr = ptr;
  entry.descriptor = descriptor;
  entry.source_block = descriptor->source_block;
  entry.bytes = descriptor->bytes;
  entry.class_id = descriptor->class_id;
  entry.generation = descriptor->generation;
  hz6_allocator_note_frontcache_cap_dryrun(allocator, entry.class_id);
  if (hz6_allocator_frontcache_should_cap_release(allocator, entry.class_id)) {
    descriptor->state = HZ6_STATE_DEAD;
    hz6_allocator_route_unregister_exact(allocator, ptr);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_cap_release;
    if (descriptor->source_release) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(descriptor);
    return 1;
  }
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  if (descriptor->source_block && descriptor->source_block->run_active &&
      descriptor->source_block->run_class_id == entry.class_id &&
      descriptor->source_block->run_slot_bytes == entry.bytes &&
#if HZ6_DESCRIPTORLESS_OVER_CAP_ONLY_L1
      hz6_allocator_frontcache_class_over_soft_cap(allocator, entry.class_id) &&
#endif
      hz6_allocator_frontcache_count(allocator, entry.class_id) <
          hz6_allocator_frontcache_capacity(allocator, entry.class_id)) {
    Hz6FrontCacheEntry descriptorless_entry = entry;
    descriptorless_entry.descriptor = NULL;
    descriptorless_entry.generation = 0;
#if HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
    descriptorless_entry.reserved_descriptor = descriptor;
#endif
    if (hz6_allocator_frontcache_push(allocator, entry.class_id,
                                      descriptorless_entry)) {
      hz6_allocator_route_unregister_exact(allocator, ptr);
#if HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
      hz6_allocator_reserve_descriptor_keep_source_slot(descriptor);
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.descriptorreserve_frontcache_push;
#endif
#else
      hz6_allocator_detach_descriptor_keep_source_slot(descriptor);
#endif
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.descriptorless_frontcache_push;
#endif
      return 1;
    }
  }
#endif

  if (hz6_allocator_frontcache_push(allocator, entry.class_id, entry)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_DEAD;
  hz6_allocator_route_unregister_exact(allocator, ptr);
#if HZ6_DIAGNOSTIC_PROBES
  if (descriptor->source_release) {
    ++allocator->stats.source_owned_release;
  }
#endif
  hz6_allocator_release_descriptor_source(descriptor);
  return 1;
}
