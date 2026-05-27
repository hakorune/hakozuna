#include "hz6_allocator.h"

static uint32_t g_hz6_allocator_owner_slot_seed = 1;

void hz6_allocator_init(Hz6Allocator* allocator) {
  hz6_allocator_init_with_profile(allocator, HZ6_PROFILE_STRICT);
}

void hz6_allocator_init_with_profile(Hz6Allocator* allocator,
                                     Hz6ProfileId profile_id) {
  if (!allocator) {
    return;
  }
  allocator->profile = hz6_profile_config(profile_id);
  allocator->owner.token.slot = g_hz6_allocator_owner_slot_seed++;
  allocator->owner.token.generation = 1;
  allocator->owner.state = HZ6_OWNER_ALIVE;
  allocator->stats.route_valid = 0;
  allocator->stats.route_invalid = 0;
  allocator->stats.route_miss = 0;
  allocator->stats.transfer_push = 0;
  allocator->stats.transfer_pop = 0;
  allocator->stats.source_alloc = 0;
  hz6_source_registry_init(&allocator->source_registry);
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    allocator->source_blocks[i].ptr = NULL;
    allocator->source_blocks[i].bytes = 0;
    allocator->source_blocks[i].source_kind = HZ6_SOURCE_NONE;
    allocator->source_blocks[i].source_release = NULL;
    allocator->source_blocks[i].route_backend = NULL;
    allocator->source_blocks[i].ref_count = 0;
    allocator->source_blocks[i].active = 0;
    allocator->source_blocks[i].route_registered = 0;
  }
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    allocator->descriptors[i].ptr = NULL;
    allocator->descriptors[i].bytes = 0;
    allocator->descriptors[i].source_ptr = NULL;
    allocator->descriptors[i].source_bytes = 0;
    allocator->descriptors[i].source_block = NULL;
    allocator->descriptors[i].class_id = 0;
    allocator->descriptors[i].source_kind = HZ6_SOURCE_NONE;
    allocator->descriptors[i].source_release = NULL;
    allocator->descriptors[i].owner = (Hz6OwnerToken){0};
    allocator->descriptors[i].generation = 0;
    allocator->descriptors[i].state = HZ6_STATE_DEAD;
  }
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    hz6_frontcache_bin_init(&allocator->frontcache_bins[i],
                            allocator->frontcache_entries[i],
                            HZ6_FRONT_CACHE_BIN_CAPACITY);
  }
  switch (allocator->profile.route_backend_policy) {
    case HZ6_ROUTE_POLICY_PAGE_TABLE:
      hz6_route_backend_init_page_table_with_granularity(
          &allocator->route_backend, allocator->route_entries,
          HZ6_ROUTE_TABLE_CAPACITY, allocator->profile.route_page_granularity);
      break;
    case HZ6_ROUTE_POLICY_EXACT_TABLE:
    default:
      hz6_route_backend_init_exact(&allocator->route_backend,
                                   allocator->route_entries,
                                   HZ6_ROUTE_TABLE_CAPACITY);
      break;
  }
  size_t transfer_capacity = allocator->profile.transfer_capacity;
  if (transfer_capacity == 0 ||
      transfer_capacity > HZ6_TRANSFER_CACHE_CAPACITY) {
    transfer_capacity = HZ6_TRANSFER_CACHE_CAPACITY;
  }
  if (allocator->profile.transfer_shards > 1) {
    hz6_transfer_backend_init_sharded(&allocator->transfer_backend,
                                      allocator->transfer_objects,
                                      transfer_capacity,
                                      allocator->profile.transfer_shards);
  } else {
    hz6_transfer_backend_init_single(&allocator->transfer_backend,
                                     allocator->transfer_objects,
                                     transfer_capacity);
  }
}
