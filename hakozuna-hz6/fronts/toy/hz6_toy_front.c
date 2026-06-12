#include "hz6_toy_front.h"

#include "../../frontcache/hz6_size_class.h"
#include "../hz6_front_source.h"
#include "../hz6_front_source_prefill.h"
#include "../hz6_front_util.h"

#define HZ6_TOY_SOURCE_BLOCK_BYTES ((size_t)65536)

static int hz6_toy_front_can_allocate(size_t size,
                                      size_t align,
                                      uint16_t* class_id) {
  if (align > 16 || !class_id || size > 4096) {
    return 0;
  }

  Hz6SizeClass size_class = hz6_size_class_for_request(size);
  if (!hz6_size_class_valid(size_class)) {
    return 0;
  }

  *class_id = size_class.id;
  return 1;
}

static void* hz6_toy_front_alloc_with_class(Hz6Allocator* allocator,
                                            uint16_t class_id,
                                            size_t size) {
#if HZ6_TOY_CLASS_ID_FAST_ALLOC_L1
  Hz6SizeClass size_class = hz6_size_class_for_id(class_id);
  if (!allocator || !hz6_size_class_valid(size_class) ||
      size == 0 || size > size_class.bytes || size > 4096) {
    return NULL;
  }
#else
  Hz6SizeClass size_class = hz6_size_class_for_request(size);
  if (size_class.id != class_id) {
    return NULL;
  }
  if (!allocator || !hz6_size_class_valid(size_class)) {
    return NULL;
  }
#endif

  size_t refill_batch = hz6_allocator_control_source_refill_batch(
      allocator, HZ6_FRONT_TOY, size_class.id);
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.toy_source_prefill_call;
#endif
  if (refill_batch > 1) {
    return hz6_front_reuse_or_block_prefill_source_kind(
        allocator, HZ6_FRONT_TOY, size_class.id, size_class.bytes,
        HZ6_TOY_SOURCE_BLOCK_BYTES, hz6_allocator_profile_source_kind(allocator),
        refill_batch);
  }

  return hz6_front_reuse_or_prefill_source_kind(
      allocator, HZ6_FRONT_TOY, size_class.id, size_class.bytes,
      hz6_allocator_profile_source_kind(allocator), refill_batch);
}

static int hz6_toy_front_free_local(Hz6Allocator* allocator,
                                    void* ptr,
                                    Hz6RouteResult route) {
  return hz6_front_free_local_to_cache(allocator, ptr, route, route.class_id);
}

int hz6_toy_front_free_remote(Hz6Allocator* allocator,
                              void* ptr,
                              Hz6RouteResult route) {
  return hz6_front_free_remote_to_transfer(allocator, ptr, route,
                                           route.class_id);
}

const Hz6FrontOps* hz6_toy_front_ops(void) {
  static const Hz6FrontOps ops = {
      HZ6_FRONT_TOY,
      "toy",
      hz6_toy_front_can_allocate,
      hz6_toy_front_alloc_with_class,
      NULL,
      hz6_toy_front_free_local,
      hz6_toy_front_free_remote,
  };
  return &ops;
}
