#include "hz6_allocator_destroy_internal.h"

void hz6_allocator_destroy_source_blocks(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    Hz6SourceBlock* block = &allocator->source_blocks[i];
    if (!block->active || !block->ptr) {
      continue;
    }
    if (block->route_registered && block->route_backend) {
      hz6_route_backend_unregister_invalid_range(block->route_backend,
                                                 block->ptr,
                                                 NULL);
    }
    if (block->source_release) {
      block->source_release(block->ptr, block->bytes);
    } else {
      hz6_source_system_release(block->ptr, block->bytes);
    }
    block->ptr = NULL;
    block->bytes = 0;
    block->source_kind = HZ6_SOURCE_NONE;
    block->source_release = NULL;
    block->route_backend = NULL;
    block->ref_count = 0;
    block->active = 0;
    block->route_registered = 0;
  }
}
