#include "hz6_toy_front.h"

#include "../../frontcache/hz6_size_class.h"
#include "../hz6_front_util.h"

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
  Hz6SizeClass size_class = hz6_size_class_for_request(size);
  if (size_class.id != class_id) {
    return NULL;
  }
  if (!allocator || !hz6_size_class_valid(size_class)) {
    return NULL;
  }

  return hz6_front_reuse_or_source(allocator, HZ6_FRONT_TOY, size_class.id,
                                   size_class.bytes);
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
      hz6_toy_front_free_local,
      hz6_toy_front_free_remote,
  };
  return &ops;
}
