#include "hz6_allocator.h"

int hz6_owns(Hz6Allocator* allocator, const void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }
  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  if (route.kind == HZ6_ROUTE_INVALID) {
    return 1;
  }
  if (route.kind != HZ6_ROUTE_VALID || !route.descriptor) {
    return 0;
  }

  const Hz6ObjectDescriptor* descriptor =
      (const Hz6ObjectDescriptor*)route.descriptor;
  return descriptor->state != HZ6_STATE_DEAD &&
         descriptor->state != HZ6_STATE_ORPHAN &&
         descriptor->state != HZ6_STATE_RELEASED;
}
