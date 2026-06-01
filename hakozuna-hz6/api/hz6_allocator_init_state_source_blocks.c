#include "hz6_allocator_init_internal.h"

void hz6_allocator_init_state_source_blocks(Hz6Allocator* allocator) {
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    allocator->source_blocks[i].ptr = NULL;
    allocator->source_blocks[i].bytes = 0;
    allocator->source_blocks[i].source_kind = HZ6_SOURCE_NONE;
    allocator->source_blocks[i].source_release = NULL;
    allocator->source_blocks[i].route_backend = NULL;
    allocator->source_blocks[i].ref_count = 0;
    allocator->source_blocks[i].run_slot_bytes = 0;
    allocator->source_blocks[i].run_class_id = 0;
    allocator->source_blocks[i].run_slot_count = 0;
    allocator->source_blocks[i].run_used_count = 0;
    allocator->source_blocks[i].run_next_hint = 0;
    for (size_t word = 0; word < HZ6_SOURCE_RUN_BITMAP_WORDS; ++word) {
      allocator->source_blocks[i].run_used_bits[word] = 0;
    }
    allocator->source_blocks[i].active = 0;
    allocator->source_blocks[i].route_registered = 0;
    allocator->source_blocks[i].run_active = 0;
  }
}
