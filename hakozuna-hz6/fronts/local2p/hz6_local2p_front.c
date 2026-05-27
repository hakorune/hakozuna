#include "hz6_local2p_front.h"

#include "../hz6_front_util.h"

static int hz6_local2p_can_allocate(size_t size,
                                    size_t align,
                                    uint16_t* class_id) {
  if (!class_id || align > 16 || size != HZ6_LOCAL2P_BYTES) {
    return 0;
  }
  *class_id = HZ6_LOCAL2P_CLASS_ID;
  return 1;
}

static void* hz6_local2p_alloc(Hz6Allocator* allocator,
                               uint16_t class_id,
                               size_t size) {
  (void)size;
  if (!allocator || class_id != HZ6_LOCAL2P_CLASS_ID) {
    return NULL;
  }

  return hz6_front_reuse_or_source_kind(
      allocator, HZ6_FRONT_LOCAL2P, class_id, HZ6_LOCAL2P_BYTES,
      HZ6_SOURCE_OS_PAGED);
}

static int hz6_local2p_free_local(Hz6Allocator* allocator,
                                  void* ptr,
                                  Hz6RouteResult route) {
  return hz6_front_free_local_to_cache(allocator, ptr, route,
                                       HZ6_LOCAL2P_CLASS_ID);
}

static int hz6_local2p_free_remote(Hz6Allocator* allocator,
                                   void* ptr,
                                   Hz6RouteResult route) {
  return hz6_front_free_remote_to_transfer(allocator, ptr, route,
                                           HZ6_LOCAL2P_CLASS_ID);
}

const Hz6FrontOps* hz6_local2p_front_ops(void) {
  static const Hz6FrontOps ops = {
      HZ6_FRONT_LOCAL2P,
      "local2p",
      hz6_local2p_can_allocate,
      hz6_local2p_alloc,
      hz6_local2p_free_local,
      hz6_local2p_free_remote,
  };
  return &ops;
}
