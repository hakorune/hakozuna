#ifndef HZ6_LOCAL2P_FRONT_H
#define HZ6_LOCAL2P_FRONT_H

#include "../../api/hz6_allocator.h"
#include "../hz6_front.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HZ6_LOCAL2P_CLASS_ID ((uint16_t)6)
#define HZ6_LOCAL2P_BYTES ((size_t)65536)

int hz6_local2p_can_allocate(size_t size,
                             size_t align,
                             uint16_t* class_id);

void* hz6_local2p_alloc(Hz6Allocator* allocator,
                        uint16_t class_id,
                        size_t size);

int hz6_local2p_free_local(Hz6Allocator* allocator,
                           void* ptr,
                           Hz6RouteResult route);

int hz6_local2p_free_remote(Hz6Allocator* allocator,
                            void* ptr,
                            Hz6RouteResult route);

size_t hz6_local2p_prefill(Hz6Allocator* allocator,
                           uint16_t class_id,
                           size_t count);

const Hz6FrontOps* hz6_local2p_front_ops(void);

#ifdef __cplusplus
}
#endif

#endif
