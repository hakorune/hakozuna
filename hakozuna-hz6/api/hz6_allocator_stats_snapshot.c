#include "hz6_allocator.h"

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
#if HZ6_DESCRIPTOR_SIDE_OWNER16_L1
  snapshot->memory_descriptor_table_bytes +=
      sizeof(allocator->descriptor_side_owner16);
#endif
  snapshot->memory_route_table_bytes =
      sizeof(allocator->route_entries);
  snapshot->memory_source_block_table_bytes =
      sizeof(allocator->source_blocks);
  snapshot->memory_frontcache_table_bytes =
      sizeof(allocator->frontcache_entries) +
      sizeof(allocator->frontcache_bins);
  snapshot->memory_transfer_table_bytes =
      sizeof(allocator->transfer_objects);
  snapshot->memory_ownerlocality_index_bytes =
      hz6_allocator_shared_route_directory_bytes() +
      hz6_allocator_owner_locality_index_bytes();

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
  size_t source_block_payload_bytes = 0;
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    const Hz6SourceBlock* block = &allocator->source_blocks[i];
    if (!block->active) {
      continue;
    }
    ++active_source_blocks;
    source_block_payload_bytes += block->bytes;
    if (block->route_registered) {
      ++registered_source_blocks;
    }
    if (block->ref_count != 0) {
      ++ref_nonzero_source_blocks;
    }
  }
  snapshot->memory_active_source_blocks = active_source_blocks;
  snapshot->memory_registered_source_blocks = registered_source_blocks;
  snapshot->memory_ref_nonzero_source_blocks = ref_nonzero_source_blocks;
  snapshot->memory_source_block_payload_bytes = source_block_payload_bytes;
  snapshot->memory_source_block_committed_estimate = source_block_payload_bytes;

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
      HZ6_FRONT_CACHE_CLASS_COUNT * HZ6_FRONT_CACHE_BIN_CAPACITY;
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
