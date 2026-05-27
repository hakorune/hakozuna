#include "hz6_allocator.h"

Hz6SourceBlock* hz6_allocator_create_source_block(
    Hz6Allocator* allocator,
    size_t bytes,
    const Hz6OsMemoryOps* source_ops,
    Hz6SourceKind source_kind) {
  if (!allocator || bytes == 0 || !hz6_source_ops_valid(source_ops) ||
      source_kind == HZ6_SOURCE_NONE) {
    return NULL;
  }

  Hz6SourceBlock* block = NULL;
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    if (!allocator->source_blocks[i].active) {
      block = &allocator->source_blocks[i];
      break;
    }
  }
  if (!block) {
    return NULL;
  }

  void* ptr = source_ops->reserve(bytes, source_ops->allocation_granularity);
  if (!ptr) {
    return NULL;
  }

  block->ptr = ptr;
  block->bytes = bytes;
  block->source_kind = source_kind;
  block->source_release = source_ops->release;
  block->route_backend = NULL;
  block->ref_count = 0;
  block->active = 1;
  block->route_registered = 0;
  return block;
}
