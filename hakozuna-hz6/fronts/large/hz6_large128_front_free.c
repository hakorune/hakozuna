#include "hz6_large128_front.h"

#include "../hz6_front_util.h"

int hz6_large128_free_local(Hz6Allocator* allocator,
                            void* ptr,
                            Hz6RouteResult route) {
  return hz6_large128_free_local_or_central(allocator, ptr, route);
}

int hz6_large128_free_remote(Hz6Allocator* allocator,
                             void* ptr,
                             Hz6RouteResult route) {
  return hz6_large128_free_remote_or_central(allocator, ptr, route);
}
