#ifndef HZ6_LARGE128_FRONT_H
#define HZ6_LARGE128_FRONT_H

#include "../../api/hz6_allocator.h"
#include "../hz6_front.h"
#include "hz6_large_span_class.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t hz6_large128_prefill(Hz6Allocator* allocator,
                            uint16_t class_id,
                            size_t count);

int hz6_large128_can_allocate(size_t size,
                              size_t align,
                              uint16_t* class_id);

void* hz6_large128_alloc(Hz6Allocator* allocator,
                         uint16_t class_id,
                         size_t size);

int hz6_large128_free_local(Hz6Allocator* allocator,
                            void* ptr,
                            Hz6RouteResult route);

int hz6_large128_free_remote(Hz6Allocator* allocator,
                             void* ptr,
                             Hz6RouteResult route);

void* hz6_large128_reuse_cached_or_central(Hz6Allocator* allocator,
                                           uint16_t class_id);

int hz6_large128_free_local_or_central(Hz6Allocator* allocator,
                                       void* ptr,
                                       Hz6RouteResult route);

int hz6_large128_free_remote_or_central(Hz6Allocator* allocator,
                                        void* ptr,
                                        Hz6RouteResult route);

const Hz6FrontOps* hz6_large128_front_ops(void);

#ifdef __cplusplus
}
#endif

#endif
