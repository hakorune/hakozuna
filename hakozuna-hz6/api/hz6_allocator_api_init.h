#ifndef HZ6_ALLOCATOR_API_INIT_H
#define HZ6_ALLOCATOR_API_INIT_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void hz6_allocator_init_with_profile(Hz6Allocator* allocator,
                                     Hz6ProfileId profile_id);

#ifdef __cplusplus
}
#endif

#endif
