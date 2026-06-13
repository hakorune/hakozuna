#ifndef HZ6_ALLOCATOR_ROUTE_LAST_HIT_H
#define HZ6_ALLOCATOR_ROUTE_LAST_HIT_H

#include "hz6_allocator.h"

void hz6_allocator_route_last_hit_clear(Hz6Allocator* allocator);

Hz6RouteResult hz6_allocator_route_last_hit_lookup(
    const Hz6Allocator* allocator,
    const void* ptr);

void hz6_allocator_route_last_hit_fill(Hz6Allocator* allocator,
                                       void* base,
                                       uint16_t front_id,
                                       uint16_t class_id,
                                       uint32_t generation,
                                       void* descriptor);

void hz6_allocator_route_last_hit_fill_route(Hz6Allocator* allocator,
                                             const void* ptr,
                                             Hz6RouteResult route);

#endif
