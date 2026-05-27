#ifndef HZ6_ALLOCATOR_INIT_INTERNAL_H
#define HZ6_ALLOCATOR_INIT_INTERNAL_H

#include "hz6_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

void hz6_allocator_init_state(Hz6Allocator* allocator,
                              Hz6ProfileId profile_id);

void hz6_allocator_init_backends(Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
