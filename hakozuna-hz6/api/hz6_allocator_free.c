#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

void hz6_free(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return;
  }

  int visible_hit = 0;
  Hz6RouteResult route = hz6_allocator_route_lookup(allocator, ptr);
  if (route.kind == HZ6_ROUTE_MISS &&
      !hz6_allocator_profile_strict_owner_remote(allocator)) {
    route = hz6_allocator_route_lookup_visible(allocator, ptr);
    visible_hit = (route.kind != HZ6_ROUTE_MISS);
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
        int needs_rehome = visible_hit && !local_owner;
        int ok = 0;
#if HZ6_DIAGNOSTIC_PROBES
        if (!local_owner) {
          ++allocator->stats.lifecycle_foreign_free_attempt;
          if (visible_hit) {
            ++allocator->stats.lifecycle_owner_mismatch;
          }
        }
#endif
        if (needs_rehome && descriptor && descriptor->source_release) {
#if HZ6_DIAGNOSTIC_PROBES
          ++allocator->stats.source_owned_remote_free_attempt;
#endif
        }
        if (!front) {
          ok = 0;
        } else if (local_owner) {
          ok = front->free_tagged &&
               front->free_tagged(allocator, ptr, route);
        } else {
          if (needs_rehome) {
#if HZ6_DIAGNOSTIC_PROBES
            ++allocator->stats.route_rehome_attempt;
#endif
          }
          ok = front->remote_free_tagged &&
               front->remote_free_tagged(allocator, ptr, route);
        }
        if (ok && needs_rehome) {
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
#if HZ6_DIAGNOSTIC_PROBES
        if (!local_owner) {
          if (ok) {
            ++allocator->stats.lifecycle_foreign_free_handled;
          } else {
            ++allocator->stats.lifecycle_foreign_free_invalid;
          }
        }
#endif
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
