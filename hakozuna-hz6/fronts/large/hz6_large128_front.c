#include "hz6_large128_front.h"

#include "../hz6_front_util.h"

static int hz6_large128_can_allocate(size_t size,
                                     size_t align,
                                     uint16_t* class_id) {
  if (!class_id || align > 16 || size <= 4096 || size > HZ6_LARGE128_BYTES) {
    return 0;
  }
  *class_id = HZ6_LARGE128_CLASS_ID;
  return 1;
}

static void* hz6_large128_alloc(Hz6Allocator* allocator,
                                uint16_t class_id,
                                size_t size) {
  (void)size;
  if (!allocator || class_id != HZ6_LARGE128_CLASS_ID) {
    return NULL;
  }

  return hz6_front_reuse_or_prefill_source_kind(
      allocator, HZ6_FRONT_LARGE, class_id, HZ6_LARGE128_BYTES,
      allocator->profile.source_kind, allocator->profile.source_batch);
}

static int hz6_large128_free_local(Hz6Allocator* allocator,
                                   void* ptr,
                                   Hz6RouteResult route) {
  return hz6_front_free_local_to_cache(allocator, ptr, route,
                                       HZ6_LARGE128_CLASS_ID);
}

static int hz6_large128_free_remote(Hz6Allocator* allocator,
                                    void* ptr,
                                    Hz6RouteResult route) {
  return hz6_front_free_remote_to_transfer(allocator, ptr, route,
                                           HZ6_LARGE128_CLASS_ID);
}

size_t hz6_large128_prefill(Hz6Allocator* allocator,
                            uint16_t class_id,
                            size_t count) {
  if (class_id != 0 && class_id != HZ6_LARGE128_CLASS_ID) {
    return 0;
  }
  return hz6_front_prefill_source_kind(
      allocator, HZ6_FRONT_LARGE, HZ6_LARGE128_CLASS_ID, HZ6_LARGE128_BYTES,
      allocator->profile.source_kind, count);
}

const Hz6FrontOps* hz6_large128_front_ops(void) {
  static const Hz6FrontOps ops = {
      HZ6_FRONT_LARGE,
      "large128",
      hz6_large128_can_allocate,
      hz6_large128_alloc,
      hz6_large128_prefill,
      hz6_large128_free_local,
      hz6_large128_free_remote,
  };
  return &ops;
}
