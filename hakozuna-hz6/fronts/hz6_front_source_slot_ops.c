#include "hz6_front_source.h"

#include "hz6_front_util.h"

void* hz6_front_source_slot_ops(Hz6Allocator* allocator,
                                uint16_t front_id,
                                uint16_t class_id,
                                size_t user_bytes,
                                size_t source_bytes,
                                size_t source_offset,
                                const Hz6OsMemoryOps* source_ops,
                                Hz6SourceKind source_kind) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      user_bytes == 0 || source_bytes == 0 || source_offset > source_bytes ||
      user_bytes > source_bytes - source_offset) {
    return NULL;
  }
  if (!hz6_source_ops_valid(source_ops) || source_kind == HZ6_SOURCE_NONE) {
    return NULL;
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    return NULL;
  }

  void* source_ptr =
      source_ops->reserve(source_bytes, source_ops->allocation_granularity);
  if (!source_ptr) {
    return NULL;
  }

  void* user_ptr = (void*)((unsigned char*)source_ptr + source_offset);
  if (!hz6_allocator_prepare_descriptor(
          allocator, descriptor, user_ptr, user_bytes, source_ptr,
          source_bytes, NULL, class_id, source_kind, source_ops->release,
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

  hz6_allocator_note_source_alloc(allocator);
  return user_ptr;
}
