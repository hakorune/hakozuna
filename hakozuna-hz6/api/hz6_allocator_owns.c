#include "hz6_allocator.h"

int hz6_owns(Hz6Allocator* allocator, const void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }
  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  return route.kind == HZ6_ROUTE_VALID || route.kind == HZ6_ROUTE_INVALID;
}
