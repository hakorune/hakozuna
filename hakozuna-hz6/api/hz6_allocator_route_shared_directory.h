#ifndef HZ6_ALLOCATOR_ROUTE_SHARED_DIRECTORY_H
#define HZ6_ALLOCATOR_ROUTE_SHARED_DIRECTORY_H

#include "hz6_allocator.h"

int hz6_shared_route_directory_register(Hz6Allocator* allocator,
                                        void* base,
                                        uint16_t front_id,
                                        uint16_t class_id,
                                        uint32_t generation,
                                        void* descriptor);

void hz6_shared_route_directory_unregister(Hz6Allocator* allocator,
                                           void* base);

Hz6RouteResult hz6_shared_route_directory_lookup_raw(const void* ptr);

Hz6RouteResult hz6_shared_route_range_lookup_raw(const void* ptr);

#endif
