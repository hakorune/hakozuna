#ifndef HZ6_ALLOCATOR_ROUTE_OWNER_LOCALITY_H
#define HZ6_ALLOCATOR_ROUTE_OWNER_LOCALITY_H

#include "hz6_allocator.h"

void hz6_owner_locality_index_register(Hz6Allocator* allocator,
                                       void* base,
                                       uint32_t generation);

void hz6_owner_locality_index_unregister(Hz6Allocator* allocator,
                                         void* base);

#endif
