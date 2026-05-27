#ifndef HZ6_TOY_FRONT_H
#define HZ6_TOY_FRONT_H

#include "../../api/hz6_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

void* hz6_toy_front_alloc(Hz6Allocator* allocator, Hz6SizeClass size_class);

int hz6_toy_front_free_local(Hz6Allocator* allocator,
                             void* ptr,
                             Hz6RouteResult route);

int hz6_toy_front_free_remote(Hz6Allocator* allocator,
                              void* ptr,
                              Hz6RouteResult route);

#ifdef __cplusplus
}
#endif

#endif
