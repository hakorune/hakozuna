#include "hz6_allocator.h"

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
                                               block->ptr,
                                               NULL);
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
