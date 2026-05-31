#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

int hz6_free_remote(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }

  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  if (route.kind == HZ6_ROUTE_MISS &&
      !hz6_allocator_profile_strict_owner_remote(allocator)) {
    route = hz6_allocator_route_lookup_visible(allocator, ptr);
  }
  if (route.kind == HZ6_ROUTE_MISS) {
    ++allocator->stats.route_miss;
    return 0;
  }
  if (route.kind == HZ6_ROUTE_INVALID || !route.descriptor) {
    ++allocator->stats.route_invalid;
    return 0;
  }

  const Hz6FrontOps* front = hz6_front_for_id(route.front_id);
  ++allocator->stats.route_valid;
  if (!front || !front->remote_free_tagged ||
      !front->remote_free_tagged(allocator, ptr, route)) {
    ++allocator->stats.route_invalid;
    return 0;
  }

  return 1;
}
