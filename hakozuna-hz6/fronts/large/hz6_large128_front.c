#include "hz6_large128_front.h"

#include "../hz6_front_util.h"
#include "../../source/linux_source_mmap.h"

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

  Hz6OsMemoryOps source_ops = hz6_linux_mmap_source_ops();
  return hz6_front_reuse_or_source_ops(
      allocator, HZ6_FRONT_LARGE, class_id, HZ6_LARGE128_BYTES, &source_ops,
      HZ6_SOURCE_LINUX_MMAP);
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

const Hz6FrontOps* hz6_large128_front_ops(void) {
  static const Hz6FrontOps ops = {
      HZ6_FRONT_LARGE,
      "large128",
      hz6_large128_can_allocate,
      hz6_large128_alloc,
      hz6_large128_free_local,
      hz6_large128_free_remote,
  };
  return &ops;
}
