#include "hz6_large128_front.h"

#include "../hz6_front_util.h"

int hz6_large128_free_local(Hz6Allocator* allocator,
                            void* ptr,
                            Hz6RouteResult route) {
  return hz6_front_free_local_to_cache(allocator, ptr, route,
                                       HZ6_LARGE128_CLASS_ID);
}

int hz6_large128_free_remote(Hz6Allocator* allocator,
                             void* ptr,
                             Hz6RouteResult route) {
  return hz6_front_free_remote_to_transfer(allocator, ptr, route,
                                           HZ6_LARGE128_CLASS_ID);
}
