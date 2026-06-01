#include "hz6_allocator.h"

int hz6_allocator_frontcache_push(Hz6Allocator* allocator,
                                  uint16_t class_id,
                                  Hz6FrontCacheEntry entry) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return hz6_frontcache_push(&allocator->frontcache_bins[class_id], entry);
}

int hz6_allocator_frontcache_pop(Hz6Allocator* allocator,
                                 uint16_t class_id,
                                 Hz6FrontCacheEntry* out) {
  if (!allocator || !out || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return hz6_frontcache_pop(&allocator->frontcache_bins[class_id], out);
}

int hz6_allocator_frontcache_remove(Hz6Allocator* allocator,
                                    uint16_t class_id,
                                    void* ptr,
                                    void* descriptor,
                                    uint32_t generation,
                                    Hz6FrontCacheEntry* removed) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return hz6_frontcache_remove(&allocator->frontcache_bins[class_id],
                               ptr,
                               descriptor,
                               generation,
                               removed);
}

int hz6_allocator_spill_frontcache_for_descriptor(
    Hz6Allocator* allocator,
    uint16_t requested_class_id) {
  if (!allocator || requested_class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.frontcache_spill_attempt;
#endif

#if !HZ6_FRONTCACHE_SPILL_ON_DESCRIPTOR_EXHAUSTION
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.frontcache_spill_no_candidate;
#endif
  return 0;
#else
  size_t donor_class = HZ6_FRONT_CACHE_CLASS_COUNT;
  size_t donor_count = 0;
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    if (i == requested_class_id) {
      continue;
    }
    size_t count = allocator->frontcache_bins[i].count;
    if (count > donor_count && count > 1) {
      donor_class = i;
      donor_count = count;
    }
  }

  if (donor_class >= HZ6_FRONT_CACHE_CLASS_COUNT) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_spill_no_candidate;
#endif
    return 0;
  }

  Hz6FrontCacheEntry entry;
  if (!hz6_allocator_frontcache_pop(allocator, (uint16_t)donor_class,
                                    &entry)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_spill_no_candidate;
#endif
    return 0;
  }

  Hz6ObjectDescriptor* descriptor =
      (Hz6ObjectDescriptor*)entry.descriptor;
  if (!descriptor || descriptor->ptr != entry.ptr ||
      descriptor->generation != entry.generation ||
      descriptor->state != HZ6_STATE_LOCAL_FREE) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_spill_invalid;
#endif
    return 0;
  }

  hz6_allocator_route_unregister_exact(allocator, entry.ptr);
#if HZ6_DIAGNOSTIC_PROBES
  if (descriptor->source_release) {
    ++allocator->stats.source_owned_release;
  }
#endif
  hz6_allocator_release_descriptor_source(descriptor);
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.frontcache_spill_success;
#endif
  return 1;
#endif
}
