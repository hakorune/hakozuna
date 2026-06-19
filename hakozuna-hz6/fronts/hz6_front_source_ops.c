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
  reused = hz6_allocator_borrow_larger_frontcache(allocator, front_id,
                                                  class_id, bytes);
  if (reused) {
    hz6_allocator_note_front_alloc_path(allocator, front_id,
                                        HZ6_ALLOC_PATH_LOCAL_REUSE);
    return reused;
  }
  hz6_allocator_remote_pending_note_source_alloc(allocator, front_id,
                                                 class_id);
  hz6_allocator_remote_pending_note_direct_source_attempt(allocator, front_id,
                                                          class_id);

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    hz6_allocator_note_frontcache_spill_dryrun(allocator, class_id);
    hz6_allocator_note_frontcache_borrow_dryrun(allocator, front_id,
                                                class_id, bytes);
    if (hz6_allocator_spill_frontcache_for_descriptor(allocator, class_id)) {
      descriptor = hz6_allocator_find_free_descriptor(allocator);
#if HZ6_DIAGNOSTIC_PROBES
      if (descriptor) {
        ++allocator->stats.frontcache_spill_retry_success;
      }
#endif
    }
  }
  if (!descriptor) {
    hz6_allocator_note_descriptor_frontcache_reuse_dryrun(allocator,
                                                          class_id);
    hz6_allocator_note_descgov_descriptor_fail(allocator, class_id);
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
    hz6_allocator_release_descriptor_source(allocator, descriptor);
    return NULL;
  }
  if (!hz6_allocator_route_register_exact_reason(
          allocator, ptr, bytes, front_id, class_id, descriptor->generation,
          descriptor, HZ6_ROUTE_REGISTER_REASON_DIRECT_SOURCE)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(allocator, descriptor);
    return NULL;
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.front_source_ops_alloc;
#endif
  hz6_allocator_note_source_alloc_for_front(allocator, front_id);
  hz6_allocator_remote_pending_note_direct_source_commit(allocator, front_id,
                                                         class_id);
  hz6_allocator_note_front_alloc_path(allocator, front_id,
                                      HZ6_ALLOC_PATH_DIRECT_SOURCE);
  return ptr;
}
