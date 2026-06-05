#include "hz6_allocator.h"

#if HZ6_DESCRIPTORLESS_OVER_CAP_ONLY_L1 || HZ6_DESCRIPTOR_COLD_GOV_L1
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

#if HZ6_DESCRIPTOR_COLD_GOV_L1
static int hz6_allocator_descgov_class_allowed(size_t bytes) {
  return bytes != 0 && bytes <= 2048;
}

static int hz6_allocator_descgov_should_detach(Hz6Allocator* allocator,
                                               uint16_t class_id,
                                               size_t bytes) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !hz6_allocator_descgov_class_allowed(bytes)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (allocator) {
      ++allocator->stats.descgov_detach_class_denied;
    }
#endif
    return 0;
  }
  if (allocator->descgov_detached_budget_used >=
      HZ6_DESCRIPTOR_COLD_GOV_DETACH_BUDGET) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descgov_detach_budget_denied;
#endif
    return 0;
  }
  if (!hz6_allocator_frontcache_class_over_soft_cap(allocator, class_id)) {
    return 0;
  }
  return 1;
}
#endif

int hz6_allocator_cache_active_descriptor(Hz6Allocator* allocator,
                                          Hz6ObjectDescriptor* descriptor,
                                          void* ptr) {
  if (!allocator || !descriptor || !ptr ||
      descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr ||
      descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !hz6_allocator_descriptor_owner_equal_at(
          allocator, descriptor, allocator->owner.token,
          HZ6_OWNER_EQUAL_SITE_LOCAL_CACHE)) {
    return 0;
  }

  descriptor->state = HZ6_STATE_LOCAL_FREE;
  Hz6FrontCacheEntry entry = {0};
  entry.ptr = ptr;
  entry.descriptor = descriptor;
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  entry.source_block = descriptor->source_block;
#endif
  entry.generation = descriptor->generation;
  hz6_frontcache_entry_set_bytes(&entry, descriptor->bytes);
  hz6_frontcache_entry_set_class_id(&entry, descriptor->class_id);
  const uint16_t entry_class_id = hz6_frontcache_entry_class_id(&entry);
#if HZ6_DIAGNOSTIC_PROBES || HZ6_FRONTCACHE_CAP_ON_FREE
  hz6_allocator_note_frontcache_cap_dryrun(allocator, entry_class_id);
  if (hz6_allocator_frontcache_should_cap_release(allocator, entry_class_id)) {
    descriptor->state = HZ6_STATE_DEAD;
    hz6_allocator_route_unregister_exact_reason(
        allocator, ptr, HZ6_ROUTE_UNREGISTER_REASON_CAP_RELEASE);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_cap_release;
    if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(allocator, descriptor);
    return 1;
  }
#endif
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  const size_t entry_bytes = hz6_frontcache_entry_bytes(&entry);
  if (descriptor->source_block &&
      hz6_source_block_run_active(descriptor->source_block) &&
      descriptor->source_block->run_class_id == entry_class_id &&
      descriptor->source_block->run_slot_bytes == entry_bytes &&
#if HZ6_DESCRIPTOR_COLD_GOV_L1
      hz6_allocator_descgov_should_detach(allocator, entry_class_id,
                                          entry_bytes) &&
#endif
#if HZ6_DESCRIPTORLESS_OVER_CAP_ONLY_L1
      hz6_allocator_frontcache_class_over_soft_cap(allocator, entry_class_id) &&
#endif
      hz6_allocator_frontcache_count(allocator, entry_class_id) <
          hz6_allocator_frontcache_capacity(allocator, entry_class_id)) {
#if HZ6_DESCRIPTOR_COLD_GOV_L1 && HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.descgov_detach_attempt;
#endif
    Hz6FrontCacheEntry descriptorless_entry = entry;
    descriptorless_entry.descriptor = NULL;
    descriptorless_entry.generation = 0;
#if HZ6_DESCRIPTOR_COLD_GOV_L1
    hz6_frontcache_entry_set_descgov_detached(&descriptorless_entry, 1);
#endif
#if HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
    descriptorless_entry.reserved_descriptor = descriptor;
#endif
    if (hz6_allocator_frontcache_push(allocator, entry_class_id,
                                      descriptorless_entry)) {
      hz6_allocator_route_unregister_exact_reason(
          allocator, ptr, HZ6_ROUTE_UNREGISTER_REASON_DESCRIPTORLESS_DETACH);
#if HZ6_DESCRIPTOR_COLD_GOV_L1
      ++allocator->descgov_detached_budget_used;
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.descgov_detach_success;
      if (entry_class_id < HZ6_STATS_CLASS_COUNT) {
        ++allocator->stats.descgov_donor_class[entry_class_id];
      }
      ++allocator->stats.descgov_detached_current;
      if (allocator->stats.descgov_detached_current >
          allocator->stats.descgov_detached_max) {
        allocator->stats.descgov_detached_max =
            allocator->stats.descgov_detached_current;
      }
#endif
#endif
#if HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
      hz6_allocator_reserve_descriptor_keep_source_slot(allocator, descriptor);
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.descriptorreserve_frontcache_push;
#endif
#else
      hz6_allocator_detach_descriptor_keep_source_slot(allocator, descriptor);
#endif
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.descriptorless_frontcache_push;
#endif
      return 1;
    }
  }
#endif

  if (hz6_allocator_frontcache_push(allocator, entry_class_id, entry)) {
    return 1;
  }

#if HZ6_ROUTE_RETAINED_OVERFLOW_L1
  ++allocator->stats.route_retained_overflow_attempt;
  descriptor->state = HZ6_STATE_TRANSFER_FREE;
  {
    Hz6TransferObject object = {0};
    object.ptr = ptr;
    object.descriptor = descriptor;
    object.class_id = entry_class_id;
    object.generation = descriptor->generation;
    if (hz6_allocator_transfer_push(allocator, object)) {
      ++allocator->stats.route_retained_overflow_success;
      hz6_allocator_note_transfer_push(allocator);
      return 1;
    }
  }
  ++allocator->stats.route_retained_overflow_fail;
#endif

  descriptor->state = HZ6_STATE_DEAD;
  hz6_allocator_route_unregister_exact_reason(
      allocator, ptr, HZ6_ROUTE_UNREGISTER_REASON_FRONTCACHE_OVERFLOW);
#if HZ6_DIAGNOSTIC_PROBES
  if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
    ++allocator->stats.source_owned_release;
  }
#endif
  hz6_allocator_release_descriptor_source(allocator, descriptor);
  return 1;
}
