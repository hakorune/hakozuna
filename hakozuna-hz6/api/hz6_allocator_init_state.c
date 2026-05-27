#include "hz6_allocator_init_internal.h"

static uint32_t g_hz6_allocator_owner_slot_seed = 1;

void hz6_allocator_init_state(Hz6Allocator* allocator,
                              Hz6ProfileId profile_id) {
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
}
