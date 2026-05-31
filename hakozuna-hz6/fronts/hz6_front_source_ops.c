#include "hz6_front_source.h"

#include "hz6_front_util.h"

void* hz6_front_reuse_or_source_ops(Hz6Allocator* allocator,
                                    uint16_t front_id,
                                    uint16_t class_id,
                                    size_t bytes,
                                    const Hz6OsMemoryOps* source_ops,
                                    Hz6SourceKind source_kind) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT || bytes == 0) {
    return NULL;
  }
  if (!hz6_source_ops_valid(source_ops) || source_kind == HZ6_SOURCE_NONE) {
    return NULL;
  }

  void* reused = hz6_front_reuse_cached_or_transfer(allocator, front_id,
                                                    class_id, NULL);
  if (reused) {
    return reused;
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    return NULL;
  }

  void* ptr = source_ops->reserve(bytes, source_ops->allocation_granularity);
  if (!ptr) {
    return NULL;
  }

  if (!hz6_allocator_prepare_descriptor(
          allocator, descriptor, ptr, bytes, ptr, bytes, NULL, class_id,
          source_kind, source_ops->release, HZ6_STATE_ACTIVE)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (source_ops->release) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(descriptor);
    return NULL;
  }
  if (!hz6_allocator_route_register_exact(allocator, ptr, bytes,
                                          front_id, class_id,
                                          descriptor->generation, descriptor)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (descriptor->source_release) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(descriptor);
    return NULL;
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.front_source_ops_alloc;
#endif
  hz6_allocator_note_source_alloc_for_front(allocator, front_id);
  hz6_allocator_note_front_alloc_path(allocator, front_id,
                                      HZ6_ALLOC_PATH_DIRECT_SOURCE);
  return ptr;
}
