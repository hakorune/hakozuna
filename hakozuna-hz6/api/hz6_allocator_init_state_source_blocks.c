#include "hz6_allocator_init_internal.h"

void hz6_allocator_init_state_source_blocks(Hz6Allocator* allocator) {
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    allocator->source_blocks[i].ptr = NULL;
    allocator->source_blocks[i].bytes = 0;
#if HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1
    allocator->source_blocks[i].source_state_flags = 0;
#endif
    hz6_source_block_set_source_kind(&allocator->source_blocks[i],
                                     HZ6_SOURCE_NONE);
    allocator->source_blocks[i].source_release = NULL;
#if !HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1
    allocator->source_blocks[i].route_backend = NULL;
#endif
    atomic_store_explicit(&allocator->source_blocks[i].ref_count,
                          0u,
                          memory_order_release);
    allocator->source_blocks[i].run_slot_bytes = 0;
    allocator->source_blocks[i].run_front_id = HZ6_FRONT_NONE;
    allocator->source_blocks[i].run_class_id = 0;
    allocator->source_blocks[i].run_slot_count = 0;
    allocator->source_blocks[i].run_used_count = 0;
    allocator->source_blocks[i].run_next_hint = 0;
    for (size_t word = 0; word < HZ6_SOURCE_RUN_BITMAP_WORDS; ++word) {
      allocator->source_blocks[i].run_used_bits[word] = 0;
    }
#if HZ6_OWNER_SOURCE_SIDE_META_L2
    allocator->source_blocks[i].owner_source_storage_allocator = NULL;
#endif
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1 && \
    HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1
    allocator->source_blocks[i].run_descriptor_indices = NULL;
#endif
    hz6_source_block_set_active(&allocator->source_blocks[i], 0);
    hz6_source_block_set_route_registered(&allocator->source_blocks[i], 0);
    hz6_source_block_set_route_shared(&allocator->source_blocks[i], 0);
    hz6_source_block_set_run_active(&allocator->source_blocks[i], 0);
  }
#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY; ++i) {
    allocator->source_block_range_index[i].page = 0;
    allocator->source_block_range_index[i].block_index = 0;
    allocator->source_block_range_index[i].active = 0;
  }
#endif
#if HZ6_SMALL_RUN_ROUTE_ARMED_L1
  allocator->small_run_route_range_registered = 0;
#endif
}
