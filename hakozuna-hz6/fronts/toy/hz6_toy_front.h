#ifndef HZ6_TOY_FRONT_H
#define HZ6_TOY_FRONT_H

#include "../../api/hz6_allocator.h"
#include "../hz6_front.h"

#ifdef __cplusplus
extern "C" {
#endif

const Hz6FrontOps* hz6_toy_front_ops(void);

int hz6_toy_front_free_remote(Hz6Allocator* allocator,
                              void* ptr,
                              Hz6RouteResult route);

#ifdef __cplusplus
}
#endif

#endif
