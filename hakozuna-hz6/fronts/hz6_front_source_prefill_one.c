#include "hz6_front_source_prefill.h"

int hz6_front_prefill_one(Hz6Allocator* allocator,
                          uint16_t front_id,
                          uint16_t class_id,
                          size_t bytes,
                          const Hz6OsMemoryOps* source_ops,
                          Hz6SourceKind source_kind) {
  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    return 0;
  }

  void* ptr = source_ops->reserve(bytes, source_ops->allocation_granularity);
  if (!ptr) {
    return 0;
  }

  if (!hz6_allocator_prepare_descriptor(
          allocator, descriptor, ptr, bytes, ptr, bytes, NULL, class_id,
          source_kind, source_ops->release, HZ6_STATE_LOCAL_FREE)) {
    hz6_allocator_release_descriptor_source(descriptor);
    return 0;
  }

  if (!hz6_allocator_route_register_exact(allocator, ptr, bytes,
                                          front_id, class_id,
                                          descriptor->generation, descriptor)) {
    hz6_allocator_release_descriptor_source(descriptor);
    return 0;
  }

  Hz6FrontCacheEntry entry;
  entry.ptr = ptr;
  entry.descriptor = descriptor;
  entry.class_id = class_id;
  entry.generation = descriptor->generation;
  if (!hz6_allocator_frontcache_push(allocator, class_id, entry)) {
    hz6_allocator_route_unregister_exact(allocator, ptr);
    hz6_allocator_release_descriptor_source(descriptor);
    return 0;
  }

  hz6_allocator_note_source_alloc_for_front(allocator, front_id);
  return 1;
}
