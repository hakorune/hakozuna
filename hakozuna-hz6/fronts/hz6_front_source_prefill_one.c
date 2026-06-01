#include "hz6_front_source_prefill.h"
#include "../include/hz6_contract_route.h"

int hz6_front_prefill_one(Hz6Allocator* allocator,
                          uint16_t front_id,
                          uint16_t class_id,
                          size_t bytes,
                          const Hz6OsMemoryOps* source_ops,
                          Hz6SourceKind source_kind) {
  size_t front_index = 0;
  int has_front_index =
      hz6_front_attr_index_from_id(front_id, &front_index) &&
      front_index < HZ6_FRONT_ATTR_COUNT;
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_prefill_attempt;
  if (has_front_index) {
    ++allocator->stats.front_source_prefill_attempt[front_index];
  }
#else
  (void)has_front_index;
#endif

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    hz6_allocator_note_frontcache_spill_dryrun(allocator, class_id);
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
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_prefill_fallback;
    if (has_front_index) {
      ++allocator->stats.front_source_prefill_fallback[front_index];
    }
#endif
    return 0;
  }

  void* ptr = source_ops->reserve(bytes, source_ops->allocation_granularity);
  if (!ptr) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_prefill_fallback;
    if (has_front_index) {
      ++allocator->stats.front_source_prefill_fallback[front_index];
    }
#endif
    return 0;
  }

  if (!hz6_allocator_prepare_descriptor(
          allocator, descriptor, ptr, bytes, ptr, bytes, NULL, class_id,
          source_kind, source_ops->release, HZ6_STATE_LOCAL_FREE)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (source_ops->release) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(descriptor);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_prefill_fallback;
    if (has_front_index) {
      ++allocator->stats.front_source_prefill_fallback[front_index];
    }
#endif
    return 0;
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
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_prefill_fallback;
    if (has_front_index) {
      ++allocator->stats.front_source_prefill_fallback[front_index];
    }
#endif
    return 0;
  }

  Hz6FrontCacheEntry entry;
  entry.ptr = ptr;
  entry.descriptor = descriptor;
  entry.class_id = class_id;
  entry.generation = descriptor->generation;
  if (!hz6_allocator_frontcache_push(allocator, class_id, entry)) {
    hz6_allocator_route_unregister_exact(allocator, ptr);
#if HZ6_DIAGNOSTIC_PROBES
    if (descriptor->source_release) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(descriptor);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_prefill_fallback;
    if (has_front_index) {
      ++allocator->stats.front_source_prefill_fallback[front_index];
    }
#endif
    return 0;
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.front_source_prefill_alloc;
  allocator->stats.source_prefill_filled += 1;
  if (has_front_index) {
    ++allocator->stats.front_source_prefill_filled[front_index];
  }
#endif
  hz6_allocator_note_source_alloc_for_front(allocator, front_id);
  hz6_allocator_note_front_alloc_path(allocator, front_id,
                                      HZ6_ALLOC_PATH_SOURCE_PREFILL);
  return 1;
}
