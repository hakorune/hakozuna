#include "hz6_front_source_block.h"

void* hz6_front_source_block_slot(Hz6Allocator* allocator,
                                  uint16_t front_id,
                                  uint16_t class_id,
                                  size_t user_bytes,
                                  size_t source_offset,
                                  Hz6SourceBlock* source_block) {
  if (!allocator || !source_block || !source_block->active ||
      !source_block->ptr || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      user_bytes == 0 || source_offset > source_block->bytes ||
      user_bytes > source_block->bytes - source_offset) {
    return NULL;
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    return NULL;
  }
  if (!source_block->route_registered) {
    if (!hz6_allocator_source_block_register_invalid_range(
            allocator, source_block, front_id, class_id)) {
      return NULL;
    }
  }
  if (!hz6_allocator_retain_source_block(source_block)) {
    return NULL;
  }

  void* user_ptr = (void*)((unsigned char*)source_block->ptr + source_offset);
  if (!hz6_allocator_prepare_descriptor(
          allocator, descriptor, user_ptr, user_bytes, source_block->ptr,
          source_block->bytes, source_block, class_id,
          source_block->source_kind, source_block->source_release,
          HZ6_STATE_ACTIVE)) {
    hz6_allocator_release_descriptor_source(descriptor);
    return NULL;
  }

  if (!hz6_allocator_route_register_exact(
          allocator, user_ptr, user_bytes, front_id, class_id,
          descriptor->generation, descriptor)) {
    hz6_allocator_release_descriptor_source(descriptor);
    return NULL;
  }

  return user_ptr;
}
