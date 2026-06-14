#include "hz6_allocator.h"

#if HZ6_DIAGNOSTIC_PROBES
static void hz6_allocator_note_frontcache_push_highwater(
    Hz6Allocator* allocator) {
  ++allocator->diagnostic_frontcache_total_current;
  if (allocator->diagnostic_frontcache_total_current >
      allocator->stats.frontcache_total_max) {
    allocator->stats.frontcache_total_max =
        allocator->diagnostic_frontcache_total_current;
  }
}

static void hz6_allocator_note_frontcache_pop_highwater(
    Hz6Allocator* allocator) {
  if (allocator && allocator->diagnostic_frontcache_total_current != 0) {
    --allocator->diagnostic_frontcache_total_current;
  }
}
#endif

int hz6_allocator_frontcache_push(Hz6Allocator* allocator,
                                  uint16_t class_id,
                                  Hz6FrontCacheEntry entry) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  {
    int ok = hz6_frontcache_push(&allocator->frontcache_bins[class_id], entry);
#if HZ6_DIAGNOSTIC_PROBES
    if (ok && class_id < HZ6_STATS_CLASS_COUNT) {
      ++allocator->stats.frontcache_push_by_class[class_id];
      size_t count = allocator->frontcache_bins[class_id].count;
      if (count > allocator->stats.frontcache_bin_max_by_class[class_id]) {
        allocator->stats.frontcache_bin_max_by_class[class_id] = count;
      }
    }
    if (ok) {
      hz6_allocator_note_frontcache_push_highwater(allocator);
    }
#endif
    return ok;
  }
}

int hz6_allocator_frontcache_pop(Hz6Allocator* allocator,
                                 uint16_t class_id,
                                 Hz6FrontCacheEntry* out) {
  if (!allocator || !out || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  {
    int ok = hz6_frontcache_pop(&allocator->frontcache_bins[class_id], out);
#if HZ6_DIAGNOSTIC_PROBES
    if (!ok && class_id < HZ6_STATS_CLASS_COUNT) {
      ++allocator->stats.frontcache_pop_empty_by_class[class_id];
    }
    if (ok) {
      hz6_allocator_note_frontcache_pop_highwater(allocator);
    }
#endif
    return ok;
  }
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
  {
    int ok = hz6_frontcache_remove(&allocator->frontcache_bins[class_id],
                                   ptr,
                                   descriptor,
                                   generation,
                                   removed);
#if HZ6_DIAGNOSTIC_PROBES
    if (ok) {
      hz6_allocator_note_frontcache_pop_highwater(allocator);
    }
#endif
    return ok;
  }
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

  hz6_allocator_route_unregister_exact_reason(
      allocator, entry.ptr, HZ6_ROUTE_UNREGISTER_REASON_FRONTCACHE_OVERFLOW);
#if HZ6_DIAGNOSTIC_PROBES
  if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
    ++allocator->stats.source_owned_release;
  }
#endif
  hz6_allocator_release_descriptor_source(allocator, descriptor);
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.frontcache_spill_success;
#endif
  return 1;
#endif
}

int hz6_allocator_reclaim_frontcache_descriptor_for_source_run(
    Hz6Allocator* allocator,
    uint16_t requested_class_id) {
  if (!allocator || requested_class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_run_reuse_descriptor_reclaim_attempt;
#endif

#if !HZ6_SOURCE_RUN_RECLAIM_DESCRIPTOR_L1
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_run_reuse_descriptor_reclaim_no_candidate;
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
    ++allocator->stats.source_run_reuse_descriptor_reclaim_no_candidate;
#endif
    return 0;
  }

  Hz6FrontCacheEntry entry;
  if (!hz6_allocator_frontcache_pop(allocator, (uint16_t)donor_class,
                                    &entry)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_descriptor_reclaim_no_candidate;
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

  hz6_allocator_route_unregister_exact_reason(
      allocator, entry.ptr, HZ6_ROUTE_UNREGISTER_REASON_SOURCE_SLOT_RELEASE);
#if HZ6_DIAGNOSTIC_PROBES
  if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
    ++allocator->stats.source_owned_release;
  }
#endif
  hz6_allocator_release_descriptor_source(allocator, descriptor);
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_run_reuse_descriptor_reclaim_success;
#endif
  return 1;
#endif
}

int hz6_allocator_reclaim_frontcache_descriptor_for_source_run_same_class(
    Hz6Allocator* allocator,
    uint16_t requested_class_id) {
  if (!allocator || requested_class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_run_reuse_same_class_reclaim_attempt;
#endif

#if !HZ6_SOURCE_RUN_RECLAIM_SAME_CLASS_L1
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_run_reuse_same_class_reclaim_no_candidate;
#endif
  return 0;
#else
  Hz6FrontCacheBin* bin = &allocator->frontcache_bins[requested_class_id];
  if (!bin->entries || bin->count <= 1) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_same_class_reclaim_no_candidate;
#endif
    return 0;
  }

  Hz6FrontCacheEntry entry = bin->entries[bin->count - 1];
  Hz6ObjectDescriptor* descriptor =
      (Hz6ObjectDescriptor*)entry.descriptor;
  if (!descriptor || descriptor->ptr != entry.ptr ||
      descriptor->generation != entry.generation ||
      descriptor->state != HZ6_STATE_LOCAL_FREE ||
      descriptor->class_id != requested_class_id) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_same_class_reclaim_no_candidate;
#endif
    return 0;
  }

  if (!hz6_allocator_frontcache_pop(allocator, requested_class_id, &entry)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_same_class_reclaim_no_candidate;
#endif
    return 0;
  }

  hz6_allocator_route_unregister_exact_reason(
      allocator, entry.ptr, HZ6_ROUTE_UNREGISTER_REASON_SOURCE_SLOT_RELEASE);
#if HZ6_DIAGNOSTIC_PROBES
  if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
    ++allocator->stats.source_owned_release;
  }
#endif
  hz6_allocator_release_descriptor_source(allocator, descriptor);
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_run_reuse_same_class_reclaim_success;
#endif
  return 1;
#endif
}

void* hz6_allocator_borrow_larger_frontcache(Hz6Allocator* allocator,
                                             uint16_t front_id,
                                             uint16_t requested_class_id,
                                             size_t requested_bytes) {
  if (!allocator || requested_class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      requested_bytes == 0) {
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.frontcache_borrow_attempt;
#endif

#if !HZ6_FRONTCACHE_BORROW_LARGER_ON_CLASS_MISS
  (void)front_id;
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.frontcache_borrow_no_candidate;
#endif
  return NULL;
#else
  for (size_t i = requested_class_id + 1; i < HZ6_FRONT_CACHE_CLASS_COUNT;
       ++i) {
    Hz6FrontCacheBin* bin = &allocator->frontcache_bins[i];
    for (size_t j = bin->count; j > 0; --j) {
      size_t index = j - 1;
      Hz6FrontCacheEntry entry = bin->entries[index];
      Hz6ObjectDescriptor* descriptor =
          (Hz6ObjectDescriptor*)entry.descriptor;
      if (!descriptor || descriptor->state != HZ6_STATE_LOCAL_FREE ||
          descriptor->ptr != entry.ptr || descriptor->bytes < requested_bytes ||
          descriptor->generation != entry.generation) {
        continue;
      }
      Hz6RouteResult route =
          hz6_allocator_route_lookup_exact(allocator, entry.ptr);
      if (route.kind != HZ6_ROUTE_VALID || route.front_id != front_id) {
        continue;
      }

      bin->entries[index] = bin->entries[--bin->count];
#if HZ6_DIAGNOSTIC_PROBES
      hz6_allocator_note_frontcache_pop_highwater(allocator);
#endif
      if (!hz6_allocator_activate_descriptor(allocator, descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
              hz6_allocator_owner_token(allocator))) {
#if HZ6_DIAGNOSTIC_PROBES
        ++allocator->stats.frontcache_borrow_invalid;
#endif
        return NULL;
      }
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_borrow_success;
#endif
      return entry.ptr;
    }
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.frontcache_borrow_no_candidate;
#endif
  return NULL;
#endif
}
