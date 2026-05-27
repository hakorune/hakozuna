#include "hz6_allocator.h"

void hz6_allocator_destroy(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  allocator->owner.state = HZ6_OWNER_DYING;
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr) {
      continue;
    }
    hz6_allocator_route_unregister_exact(allocator, descriptor->ptr);
    hz6_allocator_release_descriptor_source(descriptor);
  }
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    Hz6SourceBlock* block = &allocator->source_blocks[i];
    if (!block->active || !block->ptr) {
      continue;
    }
    if (block->route_registered && block->route_backend) {
      hz6_route_backend_unregister_invalid_range(block->route_backend,
                                                 block->ptr);
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
  allocator->owner.state = HZ6_OWNER_DEAD;
}
