#include "hz6_front_source_prefill.h"

static int hz6_front_prefill_one(Hz6Allocator* allocator,
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

  hz6_allocator_note_source_alloc(allocator);
  return 1;
}

size_t hz6_front_prefill_source_kind(Hz6Allocator* allocator,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t bytes,
                                     Hz6SourceKind source_kind,
                                     size_t count) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT || bytes == 0 ||
      count == 0) {
    return 0;
  }

  const Hz6OsMemoryOps* source_ops =
      hz6_allocator_source_ops(allocator, source_kind);
  if (!hz6_source_ops_valid(source_ops) || source_kind == HZ6_SOURCE_NONE) {
    return 0;
  }

  size_t filled = 0;
  while (filled < count &&
         hz6_allocator_frontcache_count(allocator, class_id) <
             hz6_allocator_frontcache_capacity(allocator, class_id)) {
    if (!hz6_front_prefill_one(allocator, front_id, class_id, bytes,
                               source_ops, source_kind)) {
      break;
    }
    ++filled;
  }
  return filled;
}
