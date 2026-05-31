#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

int hz6_free_remote(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }

  int visible_hit = 0;
  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  if (route.kind == HZ6_ROUTE_MISS &&
      !hz6_allocator_profile_strict_owner_remote(allocator)) {
    route = hz6_allocator_route_lookup_visible(allocator, ptr);
    visible_hit = (route.kind != HZ6_ROUTE_MISS);
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
  const Hz6ObjectDescriptor* descriptor =
      (const Hz6ObjectDescriptor*)route.descriptor;
  int needs_rehome = visible_hit && descriptor &&
                     !hz6_owner_equal(descriptor->owner, allocator->owner.token);
  if (needs_rehome) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.route_rehome_attempt;
#endif
  }
  if (!front || !front->remote_free_tagged ||
      !front->remote_free_tagged(allocator, ptr, route)) {
    ++allocator->stats.route_invalid;
    return 0;
  }
  if (needs_rehome) {
#if HZ6_DIAGNOSTIC_PROBES
    int rehome_ok = hz6_allocator_route_rehome_exact(allocator, &route);
    if (rehome_ok) {
      ++allocator->stats.route_rehome_success;
    } else {
      ++allocator->stats.route_rehome_fail;
    }
#else
    (void)hz6_allocator_route_rehome_exact(allocator, &route);
#endif
  }

  return 1;
}
