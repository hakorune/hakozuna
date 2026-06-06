#include "hz6_large128_front.h"

#include "../hz6_front_source.h"
#include "../hz6_front_source_prefill.h"
#include "../hz6_front_util.h"

static size_t hz6_large_direct_round_up(size_t value, size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  size_t remainder = value % alignment;
  if (remainder == 0) {
    return value;
  }
  size_t add = alignment - remainder;
  if (value > (size_t)-1 - add) {
    return 0;
  }
  return value + add;
}

static void* hz6_large_direct_alloc(Hz6Allocator* allocator, size_t size) {
  Hz6SourceKind source_kind = hz6_allocator_profile_source_kind(allocator);
  const Hz6OsMemoryOps* source_ops =
      hz6_allocator_source_ops(allocator, source_kind);
  if (!hz6_source_ops_valid(source_ops) || source_kind == HZ6_SOURCE_NONE) {
    return NULL;
  }

#if HZ6_LARGE_DIRECT_RETAIN_L1
  Hz6ObjectDescriptor* retained = NULL;
  while (hz6_allocator_large_span_pool_pop_exact_bytes(
      allocator, HZ6_LARGE_DIRECT_CLASS_ID, size, &retained)) {
    if (retained &&
        hz6_allocator_activate_descriptor(allocator, retained,
                                          HZ6_STATE_CENTRAL_FREE,
                                          retained->ptr,
                                          retained->generation,
                                          hz6_allocator_owner_token(
                                              allocator))) {
      hz6_allocator_note_front_alloc_path(allocator, HZ6_FRONT_LARGE,
                                          HZ6_ALLOC_PATH_RELEASED_REUSE);
      return retained->ptr;
    }
    if (retained) {
      hz6_allocator_release_descriptor_source(allocator, retained);
    }
  }
#endif

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    hz6_allocator_note_descgov_descriptor_fail(
        allocator, HZ6_LARGE_DIRECT_CLASS_ID);
    return NULL;
  }

  size_t source_bytes =
      hz6_large_direct_round_up(size, source_ops->allocation_granularity);
  if (source_bytes == 0) {
    return NULL;
  }
  void* ptr = source_ops->reserve(source_bytes,
                                  source_ops->allocation_granularity);
  if (!ptr) {
    return NULL;
  }

  if (!hz6_allocator_prepare_descriptor(
          allocator, descriptor, ptr, size, ptr, source_bytes, NULL,
          HZ6_LARGE_DIRECT_CLASS_ID, source_kind, source_ops->release,
          HZ6_STATE_ACTIVE)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (source_ops->release) {
      ++allocator->stats.source_owned_release;
    }
#endif
    if (source_ops->release) {
      source_ops->release(ptr, source_bytes);
    }
    return NULL;
  }
  if (!hz6_allocator_route_register_exact_reason(
          allocator, ptr, size, HZ6_FRONT_LARGE, HZ6_LARGE_DIRECT_CLASS_ID,
          descriptor->generation, descriptor,
          HZ6_ROUTE_REGISTER_REASON_DIRECT_SOURCE)) {
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
  hz6_allocator_note_source_alloc_for_front(allocator, HZ6_FRONT_LARGE);
  hz6_allocator_note_front_alloc_path(allocator, HZ6_FRONT_LARGE,
                                      HZ6_ALLOC_PATH_DIRECT_SOURCE);
  return ptr;
}

int hz6_large128_can_allocate(size_t size,
                              size_t align,
                              uint16_t* class_id) {
  const Hz6LargeSpanClass* span_class =
      hz6_large_span_class_for_request(size, align);
  if (!class_id) {
    return 0;
  }
  if (span_class) {
    *class_id = span_class->class_id;
    return 1;
  }
  if (hz6_large_direct_can_allocate(size, align)) {
    *class_id = HZ6_LARGE_DIRECT_CLASS_ID;
    return 1;
  }
  return 0;
}

void* hz6_large128_alloc(Hz6Allocator* allocator,
                         uint16_t class_id,
                         size_t size) {
  if (!allocator) {
    return NULL;
  }
  if (hz6_large_direct_class_id(class_id)) {
    if (!hz6_large_direct_can_allocate(size, 16)) {
      return NULL;
    }
    return hz6_large_direct_alloc(allocator, size);
  }
  const Hz6LargeSpanClass* span_class =
      hz6_large_span_class_for_class_id(class_id);
  if (!span_class || size > span_class->max_request_bytes) {
    return NULL;
  }

  Hz6SourceKind source_kind =
      hz6_allocator_profile_source_kind(allocator);

  void* reused = hz6_large128_reuse_cached_or_central(allocator, class_id);
  if (reused) {
    return reused;
  }

  if (source_kind == HZ6_SOURCE_NONE) {
    return NULL;
  }

  return hz6_front_reuse_or_source_kind(allocator, HZ6_FRONT_LARGE, class_id,
                                        span_class->span_bytes, source_kind);
}

size_t hz6_large128_prefill(Hz6Allocator* allocator,
                            uint16_t class_id,
                            size_t count) {
  const uint16_t prefill_class_id =
      class_id == 0 ? HZ6_LARGE128_CLASS_ID : class_id;
  const Hz6LargeSpanClass* span_class =
      hz6_large_span_class_for_class_id(prefill_class_id);
  if (!span_class) {
    return 0;
  }
  count = hz6_allocator_control_source_prefill_count(
      allocator, HZ6_FRONT_LARGE, prefill_class_id, count);
  return hz6_front_prefill_source_kind(
      allocator, HZ6_FRONT_LARGE, prefill_class_id, span_class->span_bytes,
      hz6_allocator_profile_source_kind(allocator), count);
}
