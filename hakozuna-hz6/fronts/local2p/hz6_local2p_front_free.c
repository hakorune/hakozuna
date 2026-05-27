#include "hz6_local2p_front.h"

#include "../hz6_front_util.h"

int hz6_local2p_free_local(Hz6Allocator* allocator,
                           void* ptr,
                           Hz6RouteResult route) {
  return hz6_front_free_local_to_cache(allocator, ptr, route,
                                       HZ6_LOCAL2P_CLASS_ID);
}

int hz6_local2p_free_remote(Hz6Allocator* allocator,
                            void* ptr,
                            Hz6RouteResult route) {
  return hz6_front_free_remote_to_transfer(allocator, ptr, route,
                                           HZ6_LOCAL2P_CLASS_ID);
}
