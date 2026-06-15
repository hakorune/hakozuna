#include "hz6_allocator.h"

#include "../fronts/midpage/hz6_midpage_front.h"

#if HZ6_MIDPAGE_32K_COLD_RETIRE_L1
#if defined(__GNUC__) || defined(__clang__)
#define HZ6_MIDPAGE_32K_COLD_RETIRE_NOINLINE __attribute__((noinline))
#else
#define HZ6_MIDPAGE_32K_COLD_RETIRE_NOINLINE
#endif

typedef struct Hz6MidPage32KColdRetireCounts {
  size_t active;
  size_t local_free;
  size_t transfer_free;
  size_t remote_pending;
  size_t descriptor_refs;
  size_t frontcache_entries;
} Hz6MidPage32KColdRetireCounts;

static void hz6_allocator_midpage_32k_cold_retire_count_block(
    Hz6Allocator* allocator,
    Hz6SourceBlock* block,
    Hz6MidPage32KColdRetireCounts* counts) {
  if (!allocator || !block || !counts) {
    return;
  }

  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (descriptor->source_block != block ||
        descriptor->class_id != HZ6_MIDPAGE_32K_CLASS_ID) {
      continue;
    }
    switch (descriptor->state) {
      case HZ6_STATE_ACTIVE:
        ++counts->active;
        ++counts->descriptor_refs;
        break;
      case HZ6_STATE_LOCAL_FREE:
        ++counts->local_free;
        ++counts->descriptor_refs;
        break;
      case HZ6_STATE_TRANSFER_FREE:
        ++counts->transfer_free;
        ++counts->descriptor_refs;
        break;
      case HZ6_STATE_REMOTE_PENDING:
        ++counts->remote_pending;
        ++counts->descriptor_refs;
        break;
      default:
        break;
    }
  }

  Hz6FrontCacheBin* bin =
      &allocator->frontcache_bins[HZ6_MIDPAGE_32K_CLASS_ID];
  for (size_t i = 0; bin->entries && i < bin->count; ++i) {
    Hz6FrontCacheEntry* entry = &bin->entries[i];
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry->descriptor;
    if (descriptor && descriptor->source_block == block &&
        descriptor->class_id == HZ6_MIDPAGE_32K_CLASS_ID &&
        descriptor->state == HZ6_STATE_LOCAL_FREE &&
        descriptor->ptr == entry->ptr &&
        descriptor->generation == entry->generation) {
      ++counts->frontcache_entries;
    }
  }
}

static int hz6_allocator_midpage_32k_cold_retire_candidate(
    Hz6SourceBlock* block,
    const Hz6MidPage32KColdRetireCounts* counts) {
  if (!block || !counts || !hz6_source_block_active(block) || !block->ptr ||
      counts->active != 0 || counts->transfer_free != 0 ||
      counts->remote_pending != 0 || counts->local_free == 0 ||
      counts->descriptor_refs != counts->local_free ||
      counts->frontcache_entries != counts->local_free ||
      hz6_source_block_ref_count(block) != counts->local_free) {
    return 0;
  }
  return 1;
}

static size_t hz6_allocator_midpage_32k_cold_retire_block(
    Hz6Allocator* allocator,
    Hz6SourceBlock* block,
    size_t expected_entries,
    size_t* retired_bytes) {
  if (retired_bytes) {
    *retired_bytes = 0;
  }
  if (!allocator || !block || expected_entries == 0) {
    return 0;
  }

  size_t payload_bytes = block->bytes;
  size_t retired = 0;
  while (retired < expected_entries) {
    Hz6FrontCacheBin* bin =
        &allocator->frontcache_bins[HZ6_MIDPAGE_32K_CLASS_ID];
    size_t found = bin->count;
    for (size_t i = 0; bin->entries && i < bin->count; ++i) {
      Hz6FrontCacheEntry* entry = &bin->entries[i];
      Hz6ObjectDescriptor* descriptor =
          (Hz6ObjectDescriptor*)entry->descriptor;
      if (descriptor && descriptor->source_block == block &&
          descriptor->class_id == HZ6_MIDPAGE_32K_CLASS_ID &&
          descriptor->state == HZ6_STATE_LOCAL_FREE &&
          descriptor->ptr == entry->ptr &&
          descriptor->generation == entry->generation) {
        found = i;
        break;
      }
    }

    if (found >= bin->count) {
      ++allocator->stats.midpage_32k_cold_retire_frontcache_remove_fail;
      return retired;
    }

    Hz6FrontCacheEntry entry = bin->entries[found];
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    Hz6FrontCacheEntry removed = {0};
    if (!hz6_allocator_frontcache_remove(
            allocator, HZ6_MIDPAGE_32K_CLASS_ID, entry.ptr, descriptor,
            entry.generation, &removed)) {
      ++allocator->stats.midpage_32k_cold_retire_frontcache_remove_fail;
      return retired;
    }

    descriptor->state = HZ6_STATE_DEAD;
    hz6_allocator_route_unregister_exact_reason(
        allocator, entry.ptr, HZ6_ROUTE_UNREGISTER_REASON_SOURCE_SLOT_RELEASE);
#if HZ6_DIAGNOSTIC_PROBES
    if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(allocator, descriptor);
    ++retired;
  }

  if (retired_bytes) {
    *retired_bytes = payload_bytes;
  }
  return retired;
}

static HZ6_MIDPAGE_32K_COLD_RETIRE_NOINLINE void
hz6_allocator_midpage_32k_cold_retire_maybe(Hz6Allocator* allocator) {
  if (!allocator ||
      hz6_allocator_frontcache_count(allocator, HZ6_MIDPAGE_32K_CLASS_ID) <
          HZ6_MIDPAGE_32K_COLD_RETIRE_HIGH_WATER) {
    return;
  }
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
  if (allocator->midpage_active_map_current >
      HZ6_MIDPAGE_32K_COLD_RETIRE_ACTIVE_LOW_WATER) {
    return;
  }
#endif

  ++allocator->stats.midpage_32k_cold_retire_attempt;
  size_t retired_blocks = 0;
  size_t scanned = 0;
  size_t cursor = allocator->midpage_32k_cold_retire_cursor;
  while (scanned < HZ6_MIDPAGE_32K_COLD_RETIRE_SCAN_BLOCKS_PER_CALL &&
         retired_blocks < HZ6_MIDPAGE_32K_COLD_RETIRE_MAX_BLOCKS_PER_CALL) {
    size_t index = cursor % HZ6_SOURCE_BLOCK_CAPACITY;
    Hz6SourceBlock* block = &allocator->source_blocks[index];
    ++scanned;
    ++cursor;
    ++allocator->stats.midpage_32k_cold_retire_scan_blocks;

    if (!hz6_source_block_active(block) || !block->ptr) {
      continue;
    }

    Hz6MidPage32KColdRetireCounts counts = {0};
    hz6_allocator_midpage_32k_cold_retire_count_block(allocator, block,
                                                      &counts);
    if (!hz6_allocator_midpage_32k_cold_retire_candidate(block, &counts)) {
      ++allocator->stats.midpage_32k_cold_retire_blocked;
      continue;
    }

    ++allocator->stats.midpage_32k_cold_retire_candidate_blocks;
    size_t retired_bytes = 0;
    size_t retired_descriptors =
        hz6_allocator_midpage_32k_cold_retire_block(
            allocator, block, counts.frontcache_entries, &retired_bytes);
    if (retired_descriptors != counts.frontcache_entries) {
      ++allocator->stats.midpage_32k_cold_retire_blocked;
      break;
    }

    ++retired_blocks;
    ++allocator->stats.midpage_32k_cold_retire_retired_blocks;
    allocator->stats.midpage_32k_cold_retire_retired_descriptors +=
        retired_descriptors;
    allocator->stats.midpage_32k_cold_retire_retired_bytes += retired_bytes;
  }

  allocator->midpage_32k_cold_retire_cursor =
      cursor % HZ6_SOURCE_BLOCK_CAPACITY;
}
#endif

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
#if HZ6_MIDPAGE_32K_COLD_RETIRE_L1
    if (entry_class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
      hz6_allocator_midpage_32k_cold_retire_maybe(allocator);
    }
#endif
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

int hz6_allocator_cache_active_descriptor_trusted_owner(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr) {
#if HZ6_LOCAL_CACHE_TRUSTED_OWNER_L1
  if (!allocator || !descriptor || !ptr ||
      descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr ||
      descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
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

#if HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_L1 && !HZ6_DIAGNOSTIC_PROBES && \
    !HZ6_MIDPAGE_32K_COLD_RETIRE_L1
  {
    Hz6FrontCacheBin* bin = &allocator->frontcache_bins[entry_class_id];
    int raw_push_class_allowed =
        entry_class_id <= HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MAX_CLASS;
#if HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MIN_CLASS > 0
    raw_push_class_allowed =
        raw_push_class_allowed &&
        entry_class_id >= HZ6_DIRECT_LOCAL_FREE_RAW_PUSH_MIN_CLASS;
#endif
    if (raw_push_class_allowed && bin->entries && entry.ptr &&
        !hz6_frontcache_entry_bytes_overflow(&entry) &&
        bin->count < bin->capacity) {
      bin->entries[bin->count++] = entry;
      return 1;
    }
    if (!raw_push_class_allowed &&
        hz6_allocator_frontcache_push(allocator, entry_class_id, entry)) {
      return 1;
    }
  }
#else
  if (hz6_allocator_frontcache_push(allocator, entry_class_id, entry)) {
#if HZ6_MIDPAGE_32K_COLD_RETIRE_L1
    if (entry_class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
      hz6_allocator_midpage_32k_cold_retire_maybe(allocator);
    }
#endif
    return 1;
  }
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
#else
  return hz6_allocator_cache_active_descriptor(allocator, descriptor, ptr);
#endif
}

int hz6_allocator_activate_local_descriptor_trusted_owner(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr,
    uint32_t generation) {
#if HZ6_LOCAL_CACHE_TRUSTED_OWNER_L1
  (void)allocator;
  if (!descriptor || descriptor->state != HZ6_STATE_LOCAL_FREE ||
      descriptor->ptr != ptr || descriptor->generation != generation) {
    return 0;
  }
#if HZ6_TOY_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1
  if (descriptor->class_id <= 4u && descriptor->bytes <= 4096u) {
    descriptor->state = HZ6_STATE_ACTIVE;
    return 1;
  }
#endif
#if HZ6_MIDPAGE_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1
  if ((descriptor->class_id == HZ6_MIDPAGE_8K_CLASS_ID &&
       descriptor->bytes == HZ6_MIDPAGE_8K_BYTES) ||
      (descriptor->class_id == HZ6_MIDPAGE_32K_CLASS_ID &&
       descriptor->bytes == HZ6_MIDPAGE_32K_BYTES)) {
    descriptor->state = HZ6_STATE_ACTIVE;
    return 1;
  }
#endif
  if (descriptor->source_block) {
    const Hz6SourceBlock* block = descriptor->source_block;
    uintptr_t base = (uintptr_t)block->ptr;
    uintptr_t addr = (uintptr_t)ptr;
    if (!hz6_source_block_active(block) || !block->ptr ||
        descriptor->bytes == 0 || addr < base ||
        descriptor->bytes > block->bytes ||
        addr - base > block->bytes - descriptor->bytes) {
      return 0;
    }
  }
  descriptor->state = HZ6_STATE_ACTIVE;
  return 1;
#else
  return hz6_allocator_activate_descriptor(
      allocator, descriptor, HZ6_STATE_LOCAL_FREE, ptr, generation,
      hz6_allocator_owner_token(allocator));
#endif
}
