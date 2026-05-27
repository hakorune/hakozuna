#include "hz6_midpage_front.h"

#include "../hz6_front_util.h"

int hz6_midpage_free_local(Hz6Allocator* allocator,
                           void* ptr,
                           Hz6RouteResult route) {
  size_t bytes = 0;
  if (!hz6_midpage_class_bytes(route.class_id, &bytes)) {
    return 0;
  }
  (void)bytes;
  return hz6_front_free_local_to_cache(allocator, ptr, route, route.class_id);
}

int hz6_midpage_free_remote(Hz6Allocator* allocator,
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
