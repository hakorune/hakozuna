#include "hz6_large128_front.h"

#include "../hz6_front_source.h"
#include "../hz6_front_source_prefill.h"
#include "../hz6_front_util.h"

int hz6_large128_can_allocate(size_t size,
                              size_t align,
                              uint16_t* class_id) {
  const Hz6LargeSpanClass* span_class =
      hz6_large_span_class_for_request(size, align);
  if (!class_id || !span_class) {
    return 0;
  }
  *class_id = span_class->class_id;
  return 1;
}

void* hz6_large128_alloc(Hz6Allocator* allocator,
                         uint16_t class_id,
                         size_t size) {
  if (!allocator) {
    return NULL;
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
