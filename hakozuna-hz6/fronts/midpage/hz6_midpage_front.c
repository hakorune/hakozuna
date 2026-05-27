#include "hz6_midpage_front.h"

#include "../hz6_front_source.h"
#include "../hz6_front_util.h"

static size_t hz6_midpage_prefill(Hz6Allocator* allocator,
                                  uint16_t class_id,
                                  size_t count) {
  size_t filled = 0;
  while (filled < count) {
    size_t run_filled = hz6_midpage_prefill_run(allocator, class_id);
    if (run_filled == 0) {
      break;
    }
    filled += run_filled;
  }
  return filled;
}

static int hz6_midpage_can_allocate(size_t size,
                                    size_t align,
                                    uint16_t* class_id) {
  Hz6MidPageRunPolicy policy;
  if (!class_id || !hz6_midpage_policy_for_size(size, align, &policy)) {
    return 0;
  }
  *class_id = policy.class_id;
  return 1;
}

static void* hz6_midpage_alloc(Hz6Allocator* allocator,
                               uint16_t class_id,
                               size_t size) {
  (void)size;
  size_t bytes = 0;
  if (!allocator || !hz6_midpage_class_bytes(class_id, &bytes)) {
    return NULL;
  }

  void* reused = hz6_front_reuse_transfer_or_cached(allocator, class_id);
  if (reused) {
    return reused;
  }

  if (hz6_midpage_prefill_run(allocator, class_id) != 0) {
    reused = hz6_front_reuse_transfer_or_cached(allocator, class_id);
    if (reused) {
      return reused;
    }
  }

  return hz6_front_reuse_or_source_kind(
      allocator, HZ6_FRONT_MIDPAGE, class_id, bytes,
      hz6_allocator_profile_source_kind(allocator));
}

static int hz6_midpage_free_local(Hz6Allocator* allocator,
                                  void* ptr,
                                  Hz6RouteResult route) {
  size_t bytes = 0;
  if (!hz6_midpage_class_bytes(route.class_id, &bytes)) {
    return 0;
  }
  (void)bytes;
  return hz6_front_free_local_to_cache(allocator, ptr, route, route.class_id);
}

static int hz6_midpage_free_remote(Hz6Allocator* allocator,
                                   void* ptr,
                                   Hz6RouteResult route) {
  size_t bytes = 0;
  if (!hz6_midpage_class_bytes(route.class_id, &bytes)) {
    return 0;
  }
  (void)bytes;
  return hz6_front_free_remote_to_transfer(allocator, ptr, route,
                                           route.class_id);
}

const Hz6FrontOps* hz6_midpage_front_ops(void) {
  static const Hz6FrontOps ops = {
      HZ6_FRONT_MIDPAGE,
      "midpage",
      hz6_midpage_can_allocate,
      hz6_midpage_alloc,
      hz6_midpage_prefill,
      hz6_midpage_free_local,
      hz6_midpage_free_remote,
  };
  return &ops;
}
