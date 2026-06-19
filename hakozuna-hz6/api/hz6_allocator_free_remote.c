#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

static int hz6_free_remote_rehome_before_transfer(
    Hz6Allocator* allocator,
    void* ptr,
    Hz6RouteResult route,
    const Hz6FrontOps* front,
    int needs_rehome) {
  if (!allocator || !ptr || !front || !front->remote_free_tagged) {
    return 0;
  }
  if (!needs_rehome) {
    return front->remote_free_tagged(allocator, ptr, route);
  }

#if HZ6_REMOTE_FREE_CONSUMER_REHOME_L1
  if (route.front_id == HZ6_FRONT_TOY ||
      route.front_id == HZ6_FRONT_MIDPAGE ||
      route.front_id == HZ6_FRONT_LOCAL2P) {
    return front->remote_free_tagged(allocator, ptr, route);
  }
#endif

  Hz6Allocator* origin = route.route_allocator;
  if (!origin || origin == allocator) {
    return front->remote_free_tagged(allocator, ptr, route);
  }

#if HZ6_DIAGNOSTIC_PROBES
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1
  ++allocator->stats.route_rehome_commit_enter;
#endif
  int rehome_ok = hz6_allocator_route_rehome_exact(allocator, &route);
  if (rehome_ok) {
    ++allocator->stats.route_rehome_success;
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1
    ++allocator->stats.route_rehome_commit_success;
#endif
  } else {
    ++allocator->stats.route_rehome_fail;
  }
#else
  int rehome_ok = hz6_allocator_route_rehome_exact(allocator, &route);
#endif
  if (!rehome_ok) {
    return 0;
  }

  if (front->remote_free_tagged(allocator, ptr, route)) {
    return 1;
  }

  Hz6RouteResult rollback_route = route;
  rollback_route.route_allocator = allocator;
#if HZ6_DIAGNOSTIC_PROBES
  if (hz6_allocator_route_rehome_exact(origin, &rollback_route)) {
    ++allocator->stats.route_rehome_rollback_success;
  } else {
    ++allocator->stats.route_rehome_rollback_fail;
  }
#else
  (void)hz6_allocator_route_rehome_exact(origin, &rollback_route);
#endif
  return 0;
}

int hz6_free_remote(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }

  Hz6RouteResult route = hz6_route_miss();
#if HZ6_REMOTE_FREE_ROUTE_RESOLVE_L1
  Hz6FreeRouteResolveResult resolved =
      hz6_allocator_route_resolve_free(allocator, ptr);
  route = resolved.route;
  int visible_hit = resolved.visible_hit;
  if (resolved.kind == HZ6_FREE_ROUTE_UNRESOLVED_INTEGRITY ||
      resolved.kind == HZ6_FREE_ROUTE_RETRY) {
    ++allocator->stats.route_invalid;
    return 0;
  }
#else
  int visible_hit = 0;
  int visible_lookup_done = 0;
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
#if HZ6_LOCAL_EXACT_FIRST_FREE_L1
  route = hz6_allocator_route_lookup_exact(allocator, ptr);
#endif
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
      visible_hit = (route.kind != HZ6_ROUTE_MISS);
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
  }
#else
  if (route.kind == HZ6_ROUTE_MISS) {
    route = hz6_allocator_route_lookup(allocator, ptr);
  }
#endif
#endif
  if (!visible_lookup_done && route.kind == HZ6_ROUTE_MISS &&
      !hz6_allocator_profile_strict_owner_remote(allocator)) {
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_DIAGNOSTIC_PROBES
    hz6_allocator_route_shared_directory_dryrun(allocator, ptr);
#endif
    route = hz6_allocator_route_lookup_visible_after_local_miss(allocator, ptr);
    visible_hit = (route.kind != HZ6_ROUTE_MISS);
  }
#endif
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
  int route_allocator_is_local = route.route_allocator &&
                                 route.route_allocator == allocator;
#if HZ6_DIAGNOSTIC_PROBES
  Hz6Allocator* route_allocator =
      route.route_allocator ? route.route_allocator : allocator;
  if (descriptor) {
    if (hz6_allocator_descriptor_belongs_to(route_allocator, descriptor)) {
      ++allocator->stats.descriptor_source_route_allocator_match;
    } else {
      ++allocator->stats.descriptor_source_route_allocator_mismatch;
    }
    if (hz6_allocator_descriptor_belongs_to(allocator, descriptor)) {
      ++allocator->stats.descriptor_source_current_allocator_match;
    } else {
      ++allocator->stats.descriptor_source_current_allocator_mismatch;
    }
    {
      size_t storage_probes = 0;
      Hz6Allocator* storage_allocator =
          hz6_allocator_descriptor_storage_owner_diagnostic(allocator,
                                                            descriptor,
                                                            &storage_probes);
      ++allocator->stats.descriptor_storage_lookup;
      allocator->stats.descriptor_storage_probe_total += storage_probes;
      if (storage_probes > allocator->stats.descriptor_storage_probe_max) {
        allocator->stats.descriptor_storage_probe_max = storage_probes;
      }
      if (storage_allocator) {
        ++allocator->stats.descriptor_storage_hit;
        if (storage_allocator == route_allocator) {
          ++allocator->stats.descriptor_storage_route_allocator_match;
        } else {
          ++allocator->stats.descriptor_storage_route_allocator_mismatch;
        }
        if (storage_allocator == allocator) {
          ++allocator->stats.descriptor_storage_current_allocator_match;
        } else {
          ++allocator->stats.descriptor_storage_current_allocator_mismatch;
        }
      } else {
        ++allocator->stats.descriptor_storage_miss;
      }
    }
  }
#endif
  int needs_rehome = visible_hit && descriptor &&
                     !route_allocator_is_local &&
                     !hz6_allocator_descriptor_owner_equal_at(
                         allocator, descriptor, allocator->owner.token,
                         HZ6_OWNER_EQUAL_SITE_REMOTE_FREE) &&
                     hz6_front_remote_rehome_allowed(route.front_id,
                                                     route.class_id);
  if (needs_rehome) {
#if HZ6_DIAGNOSTIC_PROBES
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1
    ++allocator->stats.remote_free_foreign_candidate;
#endif
    if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
      ++allocator->stats.source_owned_remote_free_attempt;
    }
    ++allocator->stats.route_rehome_attempt;
#endif
  }
#if HZ6_REMOTE_FREE_REHOME_BEFORE_TRANSFER_L1
  if (!hz6_free_remote_rehome_before_transfer(allocator, ptr, route, front,
                                              needs_rehome)) {
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    if (needs_rehome) {
      ++allocator->stats.remote_free_returned_uncommitted;
    }
#endif
    ++allocator->stats.route_invalid;
    return 0;
  }
  return 1;
#else
  if (!front || !front->remote_free_tagged ||
      !front->remote_free_tagged(allocator, ptr, route)) {
    ++allocator->stats.route_invalid;
    return 0;
  }
  if (needs_rehome) {
#if HZ6_DIAGNOSTIC_PROBES
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1
    ++allocator->stats.route_rehome_commit_enter;
#endif
    int rehome_ok = hz6_allocator_route_rehome_exact(allocator, &route);
    if (rehome_ok) {
      ++allocator->stats.route_rehome_success;
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1
      ++allocator->stats.route_rehome_commit_success;
#endif
    } else {
      ++allocator->stats.route_rehome_fail;
    }
#else
    (void)hz6_allocator_route_rehome_exact(allocator, &route);
#endif
  }

  return 1;
#endif
}
