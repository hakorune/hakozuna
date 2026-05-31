#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

void hz6_free(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return;
  }

  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  if (route.kind == HZ6_ROUTE_MISS &&
      !hz6_allocator_profile_strict_owner_remote(allocator)) {
    route = hz6_allocator_route_lookup_visible(allocator, ptr);
  }
  switch (route.kind) {
    case HZ6_ROUTE_VALID:
      ++allocator->stats.route_valid;
      {
        const Hz6FrontOps* front = hz6_front_for_id(route.front_id);
        Hz6ObjectDescriptor* descriptor =
            (Hz6ObjectDescriptor*)route.descriptor;
        int local_owner = descriptor &&
                          hz6_owner_equal(descriptor->owner,
                                          allocator->owner.token);
        int ok = 0;
        if (!front) {
          ok = 0;
        } else if (local_owner) {
          ok = front->free_tagged &&
               front->free_tagged(allocator, ptr, route);
        } else {
          ok = front->remote_free_tagged &&
               front->remote_free_tagged(allocator, ptr, route);
        }
        if (!ok) {
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
