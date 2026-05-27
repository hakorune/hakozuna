#ifndef HZ6_FRONT_UTIL_H
#define HZ6_FRONT_UTIL_H

#include "../api/hz6_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

void* hz6_front_reuse_or_source(Hz6Allocator* allocator,
                                uint16_t front_id,
                                uint16_t class_id,
                                size_t bytes);

int hz6_front_free_local_to_cache(Hz6Allocator* allocator,
                                  void* ptr,
                                  Hz6RouteResult route,
                                  uint16_t expected_class_id);

int hz6_front_free_remote_to_transfer(Hz6Allocator* allocator,
                                      void* ptr,
                                      Hz6RouteResult route,
                                      uint16_t expected_class_id);

#ifdef __cplusplus
}
#endif

#endif
