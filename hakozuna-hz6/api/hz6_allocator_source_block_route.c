#include "hz6_allocator.h"

int hz6_allocator_source_block_register_invalid_range(
    Hz6Allocator* allocator,
    Hz6SourceBlock* block,
    uint16_t front_id,
    uint16_t class_id) {
  if (!allocator || !block || !block->active || !block->ptr ||
      block->bytes == 0) {
    return 0;
  }
  if (block->route_registered) {
    return 1;
  }
  if (!hz6_route_backend_register_invalid_range(&allocator->route_backend,
                                                block->ptr,
                                                block->bytes,
                                                front_id,
                                                class_id)) {
    return 0;
  }
  block->route_backend = &allocator->route_backend;
  block->route_registered = 1;
  return 1;
}
