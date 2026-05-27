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

int hz6_allocator_retain_source_block(Hz6SourceBlock* block) {
  if (!block || !block->active || !block->ptr) {
    return 0;
  }
  ++block->ref_count;
  return 1;
}

int hz6_allocator_release_source_block(Hz6SourceBlock* block) {
  if (!block || !block->active || !block->ptr) {
    return 0;
  }

  if (block->ref_count != 0) {
    --block->ref_count;
  }
  if (block->ref_count != 0) {
    return 1;
  }

  if (block->route_registered && block->route_backend) {
    hz6_route_backend_unregister_invalid_range(block->route_backend,
                                               block->ptr);
  }
  int released = block->source_release
                     ? block->source_release(block->ptr, block->bytes)
                     : hz6_source_system_release(block->ptr, block->bytes);
  block->ptr = NULL;
  block->bytes = 0;
  block->source_kind = HZ6_SOURCE_NONE;
  block->source_release = NULL;
  block->route_backend = NULL;
  block->ref_count = 0;
  block->active = 0;
  block->route_registered = 0;
  return released;
}
