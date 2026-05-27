#include "hz6_midpage_front.h"

#include "../hz6_front_util.h"

int hz6_midpage_class_bytes(uint16_t class_id, size_t* bytes) {
  if (!bytes) {
    return 0;
  }
  switch (class_id) {
    case HZ6_MIDPAGE_8K_CLASS_ID:
      *bytes = HZ6_MIDPAGE_8K_BYTES;
      return 1;
    case HZ6_MIDPAGE_32K_CLASS_ID:
      *bytes = HZ6_MIDPAGE_32K_BYTES;
      return 1;
    default:
      return 0;
  }
}

int hz6_midpage_policy_for_size(size_t size,
                                size_t align,
                                Hz6MidPageRunPolicy* policy) {
  if (!policy || align > 16 || size <= 4096 || size > HZ6_MIDPAGE_BYTES) {
    return 0;
  }

  policy->run_bytes = HZ6_MIDPAGE_RUN_BYTES;
  if (size <= HZ6_MIDPAGE_8K_BYTES) {
    policy->class_id = HZ6_MIDPAGE_8K_CLASS_ID;
    policy->slot_bytes = HZ6_MIDPAGE_8K_BYTES;
  } else {
    policy->class_id = HZ6_MIDPAGE_32K_CLASS_ID;
    policy->slot_bytes = HZ6_MIDPAGE_32K_BYTES;
  }
  policy->slots_per_run = policy->run_bytes / policy->slot_bytes;
  return policy->slots_per_run != 0;
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

  return hz6_front_reuse_or_source_kind(
      allocator, HZ6_FRONT_MIDPAGE, class_id, bytes, HZ6_SOURCE_OS_PAGED);
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
      hz6_midpage_free_local,
      hz6_midpage_free_remote,
  };
  return &ops;
}
