#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

void hz6_free(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return;
  }

  int visible_hit = 0;
  int visible_lookup_done = 0;
  Hz6RouteResult route = hz6_route_miss();
#if HZ6_OWNER_LOCALITY_INDEX_L1
  route = hz6_allocator_route_lookup_exact(allocator, ptr);
  if (route.kind == HZ6_ROUTE_MISS) {
    Hz6OwnerLocalityKind locality =
        hz6_allocator_route_owner_locality_hint(allocator, ptr);
    if (locality == HZ6_OWNER_LOCALITY_DEFINITELY_FOREIGN) {
      route = hz6_allocator_route_shared_directory_lookup_exact(allocator, ptr);
      if (route.kind != HZ6_ROUTE_MISS) {
        visible_lookup_done = 1;
        visible_hit = (route.route_allocator != allocator);
      }
    }
    if (route.kind == HZ6_ROUTE_MISS) {
      route = hz6_allocator_route_lookup(allocator, ptr);
    }
  }
#else
#if HZ6_SHARED_ROUTE_DIRECTORY_FIRST_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!hz6_allocator_profile_strict_owner_remote(allocator) &&
      allocator->stats.route_visibility_hit_foreign_owner > 0) {
    route = hz6_allocator_route_shared_directory_lookup_exact(allocator, ptr);
    if (route.kind != HZ6_ROUTE_MISS) {
      visible_lookup_done = 1;
      visible_hit = (route.route_allocator != allocator);
    }
  }
#endif
#if HZ6_NEGATIVE_FILTER_L1 && HZ6_DIAGNOSTIC_PROBES
  if (route.kind == HZ6_ROUTE_MISS &&
      !hz6_allocator_profile_strict_owner_remote(allocator)) {
    ++allocator->stats.negative_filter_attempt;
    if (allocator->stats.route_visibility_hit_foreign_owner == 0) {
      ++allocator->stats.negative_filter_not_armed;
      route = hz6_allocator_route_lookup(allocator, ptr);
    } else if (allocator->stats.route_rehome_success > 0) {
      ++allocator->stats.negative_filter_rehome_blocked;
      route = hz6_allocator_route_lookup(allocator, ptr);
    } else if (hz6_allocator_route_negative_filter_skip_local(allocator, ptr)) {
      ++allocator->stats.negative_filter_skip_local;
      route = hz6_allocator_route_lookup_visible_after_local_miss(allocator,
                                                                  ptr);
      visible_lookup_done = 1;
      if (route.kind != HZ6_ROUTE_MISS) {
        visible_hit = 1;
      }
      if (route.kind == HZ6_ROUTE_MISS) {
        Hz6RouteResult shadow = hz6_allocator_route_lookup(allocator, ptr);
        if (shadow.kind == HZ6_ROUTE_VALID) {
          ++allocator->stats.negative_filter_shadow_false_skip;
          ++allocator->stats.negative_filter_shadow_local_valid;
          route = shadow;
        } else if (shadow.kind == HZ6_ROUTE_INVALID) {
          ++allocator->stats.negative_filter_shadow_false_skip;
          ++allocator->stats.negative_filter_shadow_local_invalid;
          route = shadow;
        }
      }
    } else {
      ++allocator->stats.negative_filter_maybe_local;
      route = hz6_allocator_route_lookup(allocator, ptr);
    }
  } else
#endif
#if HZ6_VISIBLE_FIRST_FREE_L1 && HZ6_DIAGNOSTIC_PROBES
  if (route.kind == HZ6_ROUTE_MISS &&
      !hz6_allocator_profile_strict_owner_remote(allocator)) {
    Hz6RouteResult visible_route;
    ++allocator->stats.visible_first_attempt;
    visible_route = hz6_allocator_route_lookup_visible_only(allocator, ptr);
    visible_lookup_done = 1;
    if (visible_route.kind == HZ6_ROUTE_VALID) {
      route = visible_route;
      visible_hit = 1;
      ++allocator->stats.visible_first_hit;
      ++allocator->stats.visible_first_local_lookup_skipped;
    } else {
      if (visible_route.kind == HZ6_ROUTE_INVALID) {
        ++allocator->stats.visible_first_visible_invalid;
      } else {
        ++allocator->stats.visible_first_miss;
      }
      ++allocator->stats.visible_first_local_fallback;
      route = hz6_allocator_route_lookup(allocator, ptr);
      if (route.kind == HZ6_ROUTE_INVALID) {
        ++allocator->stats.visible_first_local_fallback_invalid;
      } else if (route.kind == HZ6_ROUTE_MISS) {
        route = visible_route;
      }
    }
  } else
#endif
  {
    if (route.kind == HZ6_ROUTE_MISS) {
      route = hz6_allocator_route_lookup(allocator, ptr);
    }
  }
#endif
  if (!visible_lookup_done && route.kind == HZ6_ROUTE_MISS &&
      !hz6_allocator_profile_strict_owner_remote(allocator)) {
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_DIAGNOSTIC_PROBES
    hz6_allocator_route_shared_directory_dryrun(allocator, ptr);
#endif
    route = hz6_allocator_route_lookup_visible_after_local_miss(allocator, ptr);
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
