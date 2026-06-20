#ifndef HZ6_ALLOCATOR_ROUTE_DOMAIN_H
#define HZ6_ALLOCATOR_ROUTE_DOMAIN_H

#include "hz6_allocator_types.h"

void hz6_allocator_route_domain_init(Hz6Allocator* allocator);

void hz6_allocator_route_domain_lock(const Hz6Allocator* allocator);

void hz6_allocator_route_domain_unlock(const Hz6Allocator* allocator);

void hz6_allocator_route_domain_read_lock(const Hz6Allocator* allocator);

void hz6_allocator_route_domain_read_unlock(const Hz6Allocator* allocator);

void hz6_allocator_route_domain_note_compact_debt(Hz6Allocator* allocator);

int hz6_allocator_route_domain_consume_compact_debt(
    Hz6Allocator* allocator);

#endif
