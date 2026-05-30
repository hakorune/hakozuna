#include "hz6_large128_front.h"

#include "../hz6_front_source.h"
#include "../hz6_front_source_prefill.h"
#include "../hz6_front_util.h"

int hz6_large128_can_allocate(size_t size,
                              size_t align,
                              uint16_t* class_id) {
  if (!class_id || align > 16 || size <= 4096 || size > HZ6_LARGE128_BYTES) {
    return 0;
  }
  *class_id = HZ6_LARGE128_CLASS_ID;
  return 1;
}

void* hz6_large128_alloc(Hz6Allocator* allocator,
                         uint16_t class_id,
                         size_t size) {
  (void)size;
  if (!allocator || class_id != HZ6_LARGE128_CLASS_ID) {
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
                                        HZ6_LARGE128_BYTES, source_kind);
}

size_t hz6_large128_prefill(Hz6Allocator* allocator,
                            uint16_t class_id,
                            size_t count) {
  if (class_id != 0 && class_id != HZ6_LARGE128_CLASS_ID) {
    return 0;
  }
  count = hz6_allocator_control_source_prefill_count(
      allocator, HZ6_FRONT_LARGE, HZ6_LARGE128_CLASS_ID, count);
  return hz6_front_prefill_source_kind(
      allocator, HZ6_FRONT_LARGE, HZ6_LARGE128_CLASS_ID, HZ6_LARGE128_BYTES,
      hz6_allocator_profile_source_kind(allocator), count);
}
