#ifndef HZ6_ALLOCATOR_API_OWNER_H
#define HZ6_ALLOCATOR_API_OWNER_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

Hz6OwnerToken hz6_allocator_owner_token(const Hz6Allocator* allocator);

int hz6_allocator_debug_set_owner_slot(Hz6Allocator* allocator,
                                       uint32_t slot);

void hz6_allocator_mark_owner_dead(Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
