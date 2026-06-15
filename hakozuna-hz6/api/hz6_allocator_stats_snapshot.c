#include "hz6_allocator.h"
#include "../fronts/midpage/hz6_midpage_front.h"

#if HZ6_DIAGNOSTIC_PROBES
typedef struct Hz6ThinDescriptorHotCandidate {
  void* ptr;
  Hz6SourceBlock* source_block;
  Hz6OwnerToken owner;
  uint32_t bytes;
  uint32_t generation;
  uint16_t class_id;
  uint8_t state;
  uint8_t flags;
} Hz6ThinDescriptorHotCandidate;

typedef struct Hz6OwnerlessDescriptorHotCandidate {
  void* ptr;
  Hz6SourceBlock* source_block;
  uint32_t bytes;
  uint32_t generation;
  uint32_t cold_index;
  uint16_t class_id;
  uint8_t source_kind;
  uint8_t state;
} Hz6OwnerlessDescriptorHotCandidate;

typedef struct Hz6Owner16DescriptorHotCandidate {
  void* ptr;
  Hz6SourceBlock* source_block;
  uint32_t bytes;
  uint32_t generation;
  uint32_t cold_index;
  uint16_t owner_slot16;
  uint16_t owner_generation16;
  uint16_t class_id;
  uint8_t source_kind;
  uint8_t state;
} Hz6Owner16DescriptorHotCandidate;

typedef struct Hz6ThinDescriptorColdCandidate {
  void* source_ptr;
  Hz6SourceReleaseFn source_release;
  uint32_t source_bytes;
  uint32_t generation;
  uint8_t source_kind;
  uint8_t active;
  uint16_t flags;
} Hz6ThinDescriptorColdCandidate;

typedef struct Hz6SlimRouteEntryCandidate {
  uintptr_t base;
  void* descriptor;
  uint32_t generation;
  uint32_t bytes32;
  uint16_t front_id;
  uint16_t class_id;
  uint16_t flags;
} Hz6SlimRouteEntryCandidate;

typedef struct Hz6SlimSourceBlockCandidate {
  void* ptr;
  size_t bytes;
  size_t ref_count;
  size_t run_slot_bytes;
  uint64_t run_used_bits[HZ6_SOURCE_RUN_BITMAP_WORDS];
  uint16_t run_class_id;
  uint16_t run_slot_count;
  uint16_t run_used_count;
  uint16_t run_next_hint;
  uint16_t flags;
} Hz6SlimSourceBlockCandidate;

typedef struct Hz6SlimFrontCacheEntryCandidate {
  void* ptr;
  void* descriptor;
  size_t bytes;
  uint32_t generation;
  uint16_t class_id;
  uint16_t flags;
} Hz6SlimFrontCacheEntryCandidate;

static size_t hz6_sub_saturating_size(size_t a, size_t b) {
  return a > b ? a - b : 0;
}

typedef struct Hz6MidPageResidencyCounts {
  size_t active;
  size_t local_free;
  size_t transfer_free;
  size_t remote_pending;
  size_t descriptor_refs;
} Hz6MidPageResidencyCounts;

static void hz6_stats_snapshot_count_midpage_block_descriptors(
    const Hz6Allocator* allocator,
    const Hz6SourceBlock* block,
    uint16_t class_id,
    Hz6MidPageResidencyCounts* counts) {
  if (!allocator || !block || !counts) {
    return;
  }
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    const Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (descriptor->source_block != block || descriptor->class_id != class_id) {
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
}

static size_t hz6_stats_snapshot_count_frontcache_entries_for_block(
    const Hz6Allocator* allocator,
    const Hz6SourceBlock* block,
    uint16_t class_id) {
  if (!allocator || !block || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  const Hz6FrontCacheBin* bin = &allocator->frontcache_bins[class_id];
  size_t count = 0;
  for (size_t i = 0; i < bin->count; ++i) {
    const Hz6FrontCacheEntry* entry = &bin->entries[i];
    const Hz6ObjectDescriptor* descriptor =
        (const Hz6ObjectDescriptor*)entry->descriptor;
    if (descriptor && descriptor->source_block == block &&
        descriptor->class_id == class_id) {
      ++count;
    }
  }
  return count;
}

static void hz6_stats_snapshot_note_midpage_residency(
    const Hz6SourceBlock* block,
    uint16_t class_id,
    const Hz6MidPageResidencyCounts* counts,
    size_t frontcache_entries,
    Hz6StatsSnapshot* snapshot) {
  if (!block || !counts || !snapshot) {
    return;
  }
  size_t* source_blocks = class_id == HZ6_MIDPAGE_32K_CLASS_ID
                              ? &snapshot->memory_midpage_32k_source_blocks
                              : &snapshot->memory_midpage_8k_source_blocks;
  size_t* payload_bytes = class_id == HZ6_MIDPAGE_32K_CLASS_ID
                              ? &snapshot->memory_midpage_32k_payload_bytes
                              : &snapshot->memory_midpage_8k_payload_bytes;
  size_t* active = class_id == HZ6_MIDPAGE_32K_CLASS_ID
                       ? &snapshot->memory_midpage_32k_active_descriptors
                       : &snapshot->memory_midpage_8k_active_descriptors;
  size_t* local_free =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_local_free_descriptors
          : &snapshot->memory_midpage_8k_local_free_descriptors;
  size_t* transfer_free =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_transfer_free_descriptors
          : &snapshot->memory_midpage_8k_transfer_free_descriptors;
  size_t* remote_pending =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_remote_pending_descriptors
          : &snapshot->memory_midpage_8k_remote_pending_descriptors;
  size_t* all_local_free_blocks =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_all_local_free_blocks
          : &snapshot->memory_midpage_8k_all_local_free_blocks;
  size_t* all_local_free_payload =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_all_local_free_payload_bytes
          : &snapshot->memory_midpage_8k_all_local_free_payload_bytes;
  size_t* active_zero_local_free_nonzero_blocks =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_active_zero_local_free_nonzero_blocks
          : &snapshot->memory_midpage_8k_active_zero_local_free_nonzero_blocks;
  size_t* low_active_blocks =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_low_active_1_4_blocks
          : &snapshot->memory_midpage_8k_low_active_1_4_blocks;
  size_t* low_active_payload =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_low_active_1_4_payload_bytes
          : &snapshot->memory_midpage_8k_low_active_1_4_payload_bytes;
  size_t* retire_candidate_blocks =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_retire_candidate_blocks
          : &snapshot->memory_midpage_8k_retire_candidate_blocks;
  size_t* retire_candidate_payload =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_retire_candidate_payload_bytes
          : &snapshot->memory_midpage_8k_retire_candidate_payload_bytes;
  size_t* retire_candidate_descriptors =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_retire_candidate_descriptors
          : &snapshot->memory_midpage_8k_retire_candidate_descriptors;
  size_t* retire_candidate_frontcache_entries =
      class_id == HZ6_MIDPAGE_32K_CLASS_ID
          ? &snapshot->memory_midpage_32k_retire_candidate_frontcache_entries
          : &snapshot->memory_midpage_8k_retire_candidate_frontcache_entries;

  ++*source_blocks;
  *payload_bytes += block->bytes;
  *active += counts->active;
  *local_free += counts->local_free;
  *transfer_free += counts->transfer_free;
  *remote_pending += counts->remote_pending;

  if (counts->active == 0 && counts->local_free != 0 &&
      counts->transfer_free == 0 && counts->remote_pending == 0) {
    ++*all_local_free_blocks;
    *all_local_free_payload += block->bytes;
    ++*active_zero_local_free_nonzero_blocks;
    ++*retire_candidate_blocks;
    *retire_candidate_payload += block->bytes;
    *retire_candidate_descriptors += counts->local_free;
    *retire_candidate_frontcache_entries += frontcache_entries;
  }
  if (counts->active >= 1 && counts->active <= 4) {
    ++*low_active_blocks;
    *low_active_payload += block->bytes;
  }
  if (hz6_source_block_ref_count(block) != counts->descriptor_refs) {
    ++snapshot->memory_midpage_ref_mismatch_blocks;
  }
}

static void hz6_stats_snapshot_memory_attribution(
    const Hz6Allocator* allocator,
    Hz6StatsSnapshot* snapshot) {
  if (!allocator || !snapshot) {
    return;
  }

  snapshot->memory_descriptor_table_bytes =
      sizeof(allocator->descriptors);
#if HZ6_THIN_DESCRIPTOR_L1
  snapshot->memory_descriptor_table_bytes +=
      sizeof(allocator->descriptor_cold_sources);
#endif
#if HZ6_DESCRIPTOR_SIDE_OWNER16_L1 || HZ6_DESCRIPTOR_STORAGE_OWNER16_L1
  snapshot->memory_descriptor_table_bytes +=
      sizeof(allocator->descriptor_side_owner16);
#endif
  snapshot->memory_route_table_bytes =
      sizeof(allocator->route_entries);
#if HZ6_ROUTE_PACKED_META_L1
  snapshot->memory_route_table_bytes += sizeof(allocator->route_bytes);
#endif
  snapshot->memory_source_block_table_bytes =
      sizeof(allocator->source_blocks);
#if HZ6_ELASTIC_SOURCE_BLOCK_OVERFLOW_L1
  snapshot->memory_source_block_depot_bytes =
      sizeof(Hz6SourceBlock) * HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY;
#endif
  snapshot->memory_frontcache_table_bytes =
#if HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1
      sizeof(allocator->frontcache_entries_c0) +
      sizeof(allocator->frontcache_entries_c1) +
      sizeof(allocator->frontcache_entries_c2) +
      sizeof(allocator->frontcache_entries_c3) +
      sizeof(allocator->frontcache_entries_c4) +
      sizeof(allocator->frontcache_entries_c5) +
      sizeof(allocator->frontcache_entries_cold) +
#else
      sizeof(allocator->frontcache_entries) +
#endif
      sizeof(allocator->frontcache_bins);
  snapshot->memory_transfer_table_bytes =
      sizeof(allocator->transfer_objects);
  snapshot->memory_shared_route_directory_bytes =
      hz6_allocator_shared_route_directory_bytes();
  snapshot->memory_owner_locality_index_bytes =
      hz6_allocator_owner_locality_index_bytes();
  snapshot->memory_ownerlocality_index_bytes =
      snapshot->memory_shared_route_directory_bytes +
      snapshot->memory_owner_locality_index_bytes;

  size_t active_descriptors = 0;
  size_t local_free_descriptors = 0;
  size_t transfer_free_descriptors = 0;
  size_t remote_pending_descriptors = 0;
  size_t dead_with_ptr_descriptors = 0;
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    const Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    switch (descriptor->state) {
      case HZ6_STATE_ACTIVE:
        ++active_descriptors;
        break;
      case HZ6_STATE_LOCAL_FREE:
        ++local_free_descriptors;
        break;
      case HZ6_STATE_TRANSFER_FREE:
        ++transfer_free_descriptors;
        break;
      case HZ6_STATE_REMOTE_PENDING:
        ++remote_pending_descriptors;
        break;
      case HZ6_STATE_DEAD:
        if (descriptor->ptr) {
          ++dead_with_ptr_descriptors;
        }
        break;
      default:
        break;
    }
  }
  snapshot->memory_active_descriptors = active_descriptors;
  snapshot->memory_local_free_descriptors = local_free_descriptors;
  snapshot->memory_transfer_free_descriptors = transfer_free_descriptors;
  snapshot->memory_remote_pending_descriptors = remote_pending_descriptors;
  snapshot->memory_dead_with_ptr_descriptors = dead_with_ptr_descriptors;

  size_t active_source_blocks = 0;
  size_t registered_source_blocks = 0;
  size_t ref_nonzero_source_blocks = 0;
  size_t ref_zero_source_blocks = 0;
  size_t source_block_payload_bytes = 0;
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    const Hz6SourceBlock* block = &allocator->source_blocks[i];
    if (!hz6_source_block_active(block)) {
      continue;
    }
    ++active_source_blocks;
    source_block_payload_bytes += block->bytes;
    if (hz6_source_block_route_registered(block)) {
      ++registered_source_blocks;
    }
    if (hz6_source_block_ref_count(block) != 0) {
      ++ref_nonzero_source_blocks;
    } else {
      ++ref_zero_source_blocks;
    }
    const uint16_t midpage_classes[] = {
        HZ6_MIDPAGE_8K_CLASS_ID,
        HZ6_MIDPAGE_32K_CLASS_ID,
    };
    for (size_t class_index = 0; class_index < 2u; ++class_index) {
      uint16_t class_id = midpage_classes[class_index];
      Hz6MidPageResidencyCounts counts = {0};
      hz6_stats_snapshot_count_midpage_block_descriptors(
          allocator, block, class_id, &counts);
      if (counts.descriptor_refs == 0 &&
          !(block->run_front_id == HZ6_FRONT_MIDPAGE &&
            block->run_class_id == class_id)) {
        continue;
      }
      size_t frontcache_entries =
          hz6_stats_snapshot_count_frontcache_entries_for_block(
              allocator, block, class_id);
      hz6_stats_snapshot_note_midpage_residency(
          block, class_id, &counts, frontcache_entries, snapshot);
    }
  }
  snapshot->memory_active_source_blocks = active_source_blocks;
  snapshot->memory_registered_source_blocks = registered_source_blocks;
  snapshot->memory_ref_nonzero_source_blocks = ref_nonzero_source_blocks;
  snapshot->memory_ref_zero_source_blocks = ref_zero_source_blocks;
  snapshot->memory_source_block_payload_bytes = source_block_payload_bytes;
  snapshot->memory_source_block_committed_estimate = source_block_payload_bytes;
  snapshot->memory_static_table_bytes =
      snapshot->memory_descriptor_table_bytes +
      snapshot->memory_route_table_bytes +
      snapshot->memory_source_block_table_bytes +
      snapshot->memory_source_block_depot_bytes +
      snapshot->memory_frontcache_table_bytes +
      snapshot->memory_transfer_table_bytes +
      snapshot->memory_ownerlocality_index_bytes;
  snapshot->memory_static_plus_payload_bytes =
      snapshot->memory_static_table_bytes +
      snapshot->memory_source_block_committed_estimate;

  size_t frontcache_total = 0;
  size_t frontcache_largest_bin = 0;
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    size_t count = allocator->frontcache_bins[i].count;
    frontcache_total += count;
    if (count > frontcache_largest_bin) {
      frontcache_largest_bin = count;
    }
  }
  snapshot->memory_frontcache_total = frontcache_total;
  snapshot->memory_frontcache_largest_bin = frontcache_largest_bin;

  snapshot->metadata_descriptor_entry_bytes = sizeof(Hz6ObjectDescriptor);
  snapshot->metadata_descriptor_thin_hot_entry_bytes =
      sizeof(Hz6ThinDescriptorHotCandidate);
  snapshot->metadata_descriptor_thin_hot_table_bytes =
      sizeof(Hz6ThinDescriptorHotCandidate) * HZ6_OBJECT_DESCRIPTOR_CAPACITY;
#if HZ6_THIN_DESCRIPTOR_L1
  snapshot->metadata_descriptor_thin_hot_table_bytes +=
      sizeof(Hz6ThinDescriptorColdCandidate) *
      HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY;
#endif
  snapshot->metadata_descriptor_thin_hot_savings_bytes =
      hz6_sub_saturating_size(snapshot->memory_descriptor_table_bytes,
                              snapshot->metadata_descriptor_thin_hot_table_bytes);
  snapshot->metadata_descriptor_ownerless_hot_entry_bytes =
      sizeof(Hz6OwnerlessDescriptorHotCandidate);
  snapshot->metadata_descriptor_ownerless_hot_table_bytes =
      sizeof(Hz6OwnerlessDescriptorHotCandidate) *
      HZ6_OBJECT_DESCRIPTOR_CAPACITY;
#if HZ6_THIN_DESCRIPTOR_L1
  snapshot->metadata_descriptor_ownerless_hot_table_bytes +=
      sizeof(Hz6ThinDescriptorColdCandidate) *
      HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY;
#endif
  snapshot->metadata_descriptor_ownerless_hot_savings_bytes =
      hz6_sub_saturating_size(
          snapshot->memory_descriptor_table_bytes,
          snapshot->metadata_descriptor_ownerless_hot_table_bytes);
  snapshot->metadata_descriptor_owner16_hot_entry_bytes =
      sizeof(Hz6Owner16DescriptorHotCandidate);
  snapshot->metadata_descriptor_owner16_hot_table_bytes =
      sizeof(Hz6Owner16DescriptorHotCandidate) *
      HZ6_OBJECT_DESCRIPTOR_CAPACITY;
#if HZ6_THIN_DESCRIPTOR_L1
  snapshot->metadata_descriptor_owner16_hot_table_bytes +=
      sizeof(Hz6ThinDescriptorColdCandidate) *
      HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY;
#endif
  snapshot->metadata_descriptor_owner16_hot_savings_bytes =
      hz6_sub_saturating_size(snapshot->memory_descriptor_table_bytes,
                              snapshot->metadata_descriptor_owner16_hot_table_bytes);

  snapshot->metadata_route_entry_bytes = sizeof(Hz6RouteEntry);
  snapshot->metadata_route_slim_entry_bytes =
      sizeof(Hz6SlimRouteEntryCandidate);
  snapshot->metadata_route_slim_table_bytes =
      sizeof(Hz6SlimRouteEntryCandidate) * HZ6_ROUTE_TABLE_CAPACITY;
  snapshot->metadata_route_slim_savings_bytes =
      hz6_sub_saturating_size(snapshot->memory_route_table_bytes,
                              snapshot->metadata_route_slim_table_bytes);
#if HZ6_ROUTE_PACKED_META_L1
  snapshot->metadata_route_bytes16_table_bytes =
      sizeof(uint16_t) * HZ6_ROUTE_TABLE_CAPACITY;
  snapshot->metadata_route_bytes16_savings_bytes =
      hz6_sub_saturating_size(sizeof(allocator->route_bytes),
                              snapshot->metadata_route_bytes16_table_bytes);

  size_t route_bytes16_active_checked = 0;
  size_t route_bytes16_overflow = 0;
  size_t route_bytes16_max = 0;
  size_t route_bytes16_minus1_overflow = 0;
  size_t route_bytes16_minus1_zero = 0;
  size_t route_bytes16_minus1_max_stored = 0;
  for (size_t i = 0; i < HZ6_ROUTE_TABLE_CAPACITY; ++i) {
    const Hz6RouteEntry* entry = &allocator->route_entries[i];
    if (!hz6_route_entry_active(entry)) {
      continue;
    }

    const uint32_t bytes =
        hz6_route_entry_bytes(&allocator->route_backend.exact_table, i);
    ++route_bytes16_active_checked;
    if (bytes > UINT16_MAX) {
      ++route_bytes16_overflow;
    }
    if ((size_t)bytes > route_bytes16_max) {
      route_bytes16_max = (size_t)bytes;
    }
    if (bytes == 0) {
      ++route_bytes16_minus1_zero;
    } else if (bytes > ((uint32_t)UINT16_MAX + 1u)) {
      ++route_bytes16_minus1_overflow;
    } else {
      const size_t stored = (size_t)bytes - 1u;
      if (stored > route_bytes16_minus1_max_stored) {
        route_bytes16_minus1_max_stored = stored;
      }
    }
  }
  snapshot->metadata_route_bytes16_active_checked =
      route_bytes16_active_checked;
  snapshot->metadata_route_bytes16_overflow = route_bytes16_overflow;
  snapshot->metadata_route_bytes16_max = route_bytes16_max;
  snapshot->metadata_route_bytes16_minus1_overflow =
      route_bytes16_minus1_overflow;
  snapshot->metadata_route_bytes16_minus1_zero = route_bytes16_minus1_zero;
  snapshot->metadata_route_bytes16_minus1_max_stored =
      route_bytes16_minus1_max_stored;
#endif

  snapshot->metadata_source_block_entry_bytes = sizeof(Hz6SourceBlock);
  snapshot->metadata_source_block_slim_entry_bytes =
      sizeof(Hz6SlimSourceBlockCandidate);
  snapshot->metadata_source_block_slim_table_bytes =
      sizeof(Hz6SlimSourceBlockCandidate) * HZ6_SOURCE_BLOCK_CAPACITY;
  snapshot->metadata_source_block_slim_savings_bytes =
      hz6_sub_saturating_size(snapshot->memory_source_block_table_bytes,
                              snapshot->metadata_source_block_slim_table_bytes);

  snapshot->metadata_frontcache_entry_bytes = sizeof(Hz6FrontCacheEntry);
  snapshot->metadata_frontcache_slim_entry_bytes =
      sizeof(Hz6SlimFrontCacheEntryCandidate);
  snapshot->metadata_frontcache_slim_table_bytes =
      sizeof(Hz6SlimFrontCacheEntryCandidate) *
#if HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1
      (HZ6_FRONT_CACHE_CLASS0_STORAGE_CAPACITY +
       HZ6_FRONT_CACHE_CLASS1_STORAGE_CAPACITY +
       HZ6_FRONT_CACHE_CLASS2_STORAGE_CAPACITY +
       HZ6_FRONT_CACHE_CLASS3_STORAGE_CAPACITY +
       HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY +
       HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY +
       ((HZ6_FRONT_CACHE_CLASS_COUNT > 6u ? HZ6_FRONT_CACHE_CLASS_COUNT - 6u
                                          : 1u) *
        HZ6_FRONT_CACHE_COLD_CLASS_STORAGE_CAPACITY));
#else
      HZ6_FRONT_CACHE_CLASS_COUNT * HZ6_FRONT_CACHE_BIN_CAPACITY;
#endif
  snapshot->metadata_frontcache_slim_savings_bytes =
      hz6_sub_saturating_size(snapshot->memory_frontcache_table_bytes,
                              snapshot->metadata_frontcache_slim_table_bytes);
}
#endif

Hz6StatsSnapshot hz6_stats_snapshot(const Hz6Allocator* allocator) {
  Hz6StatsSnapshot empty = {0};
  if (!allocator) {
    return empty;
  }
  Hz6StatsSnapshot snapshot = allocator->stats;
#if HZ6_DIAGNOSTIC_PROBES
  hz6_stats_snapshot_memory_attribution(allocator, &snapshot);
#endif
  return snapshot;
}
