#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

void hz6_free(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return;
  }

  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  switch (route.kind) {
    case HZ6_ROUTE_VALID:
      ++allocator->stats.route_valid;
      {
        const Hz6FrontOps* front = hz6_front_for_id(route.front_id);
        if (!front || !front->free_tagged ||
            !front->free_tagged(allocator, ptr, route)) {
          ++allocator->stats.route_invalid;
        }
      }
      return;
    case HZ6_ROUTE_INVALID:
      ++allocator->stats.route_invalid;
      return;
    case HZ6_ROUTE_MISS:
    default:
      ++allocator->stats.route_miss;
      return;
  }
}
