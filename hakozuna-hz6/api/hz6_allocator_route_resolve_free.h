#ifndef HZ6_ALLOCATOR_ROUTE_RESOLVE_FREE_H
#define HZ6_ALLOCATOR_ROUTE_RESOLVE_FREE_H

#include "hz6_allocator_types.h"

typedef enum Hz6FreeRouteResolveKind {
  HZ6_FREE_ROUTE_LOCAL_VALID = 0,
  HZ6_FREE_ROUTE_FOREIGN_VALID = 1,
  HZ6_FREE_ROUTE_OWNED_INVALID = 2,
  HZ6_FREE_ROUTE_PROVEN_EXTERNAL = 3,
  HZ6_FREE_ROUTE_RETRY = 4,
  HZ6_FREE_ROUTE_UNRESOLVED_INTEGRITY = 5
} Hz6FreeRouteResolveKind;

typedef struct Hz6FreeRouteResolveResult {
  Hz6FreeRouteResolveKind kind;
  Hz6RouteResult route;
  int visible_hit;
} Hz6FreeRouteResolveResult;

Hz6FreeRouteResolveResult hz6_allocator_route_resolve_free(
    Hz6Allocator* allocator,
    const void* ptr);

#endif
