#include "hz6_allocator_route_resolve_free.h"

#include "hz6_allocator.h"
#include "hz6_allocator_route_shared_directory.h"

static Hz6FreeRouteResolveResult hz6_free_route_resolved(
    Hz6FreeRouteResolveKind kind,
    Hz6RouteResult route,
    int visible_hit) {
  Hz6FreeRouteResolveResult result;
  result.kind = kind;
  result.route = route;
  result.visible_hit = visible_hit;
  return result;
}

static Hz6FreeRouteResolveKind hz6_free_route_kind_for_route(
    const Hz6Allocator* allocator,
    Hz6RouteResult route,
    int visible_hit) {
  if (route.kind == HZ6_ROUTE_VALID) {
    if (visible_hit ||
        (route.route_allocator && route.route_allocator != allocator)) {
      return HZ6_FREE_ROUTE_FOREIGN_VALID;
    }
    return HZ6_FREE_ROUTE_LOCAL_VALID;
  }
  if (route.kind == HZ6_ROUTE_INVALID) {
    return HZ6_FREE_ROUTE_OWNED_INVALID;
  }
  return HZ6_FREE_ROUTE_PROVEN_EXTERNAL;
}

static Hz6RouteResult hz6_free_route_local_lookup(Hz6Allocator* allocator,
                                                  const void* ptr) {
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator) {
    ++allocator->stats.free_resolve_local_lookup;
  }
#endif
#if HZ6_REMOTE_FREE_RESOLVE_LOCAL_EXACT_ONLY_L1
  return hz6_allocator_route_lookup_exact(allocator, ptr);
#else
  return hz6_allocator_route_lookup(allocator, ptr);
#endif
}

#if HZ6_SHARED_ROUTE_DIRECTORY_L1
static Hz6SharedRouteLookup hz6_free_route_shared_lookup(Hz6Allocator* allocator,
                                                         const void* ptr) {
#if !HZ6_DIAGNOSTIC_PROBES
  (void)allocator;
#endif
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator) {
    ++allocator->stats.free_resolve_shared_probe;
  }
#endif
  Hz6SharedRouteLookup lookup =
      hz6_shared_route_directory_lookup_snapshot(ptr);
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator) {
    if (lookup.status == HZ6_SHARED_ROUTE_LOOKUP_VALID) {
      ++allocator->stats.free_resolve_shared_valid;
    } else if (lookup.status == HZ6_SHARED_ROUTE_LOOKUP_RETRY) {
      ++allocator->stats.free_resolve_shared_retry;
    } else if (lookup.status == HZ6_SHARED_ROUTE_LOOKUP_STALE) {
      ++allocator->stats.free_resolve_shared_stale;
    } else {
      ++allocator->stats.free_resolve_shared_miss;
    }
  }
#endif
  return lookup;
}

static int hz6_free_route_apply_shared_lookup(
    const Hz6Allocator* allocator,
    Hz6SharedRouteLookup lookup,
    Hz6RouteResult* route,
    int* visible_lookup_done,
    int* visible_hit,
    Hz6FreeRouteResolveResult* out) {
  if (!route || !visible_lookup_done || !visible_hit || !out) {
    return 0;
  }
  if (lookup.status == HZ6_SHARED_ROUTE_LOOKUP_VALID) {
    *route = lookup.route;
    *visible_lookup_done = 1;
    *visible_hit = lookup.route.route_allocator != allocator;
    return 0;
  }
  if (lookup.status == HZ6_SHARED_ROUTE_LOOKUP_RETRY) {
    *out = hz6_free_route_resolved(HZ6_FREE_ROUTE_RETRY, *route, 0);
    return 1;
  }
  if (lookup.status == HZ6_SHARED_ROUTE_LOOKUP_STALE) {
    *out = hz6_free_route_resolved(
        HZ6_FREE_ROUTE_UNRESOLVED_INTEGRITY, *route, 0);
    return 1;
  }
  return 0;
}
#endif

static Hz6FreeRouteResolveResult hz6_free_route_finish(
    Hz6Allocator* allocator,
    Hz6FreeRouteResolveResult result) {
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator) {
    if (result.kind == HZ6_FREE_ROUTE_LOCAL_VALID) {
      ++allocator->stats.free_resolve_result_local_valid;
    } else if (result.kind == HZ6_FREE_ROUTE_FOREIGN_VALID) {
      ++allocator->stats.free_resolve_result_foreign_valid;
    } else if (result.kind == HZ6_FREE_ROUTE_OWNED_INVALID) {
      ++allocator->stats.free_resolve_result_owned_invalid;
    } else if (result.kind == HZ6_FREE_ROUTE_RETRY) {
      ++allocator->stats.free_resolve_result_retry;
    } else if (result.kind == HZ6_FREE_ROUTE_UNRESOLVED_INTEGRITY) {
      ++allocator->stats.free_resolve_result_unresolved_integrity;
    } else {
      ++allocator->stats.free_resolve_result_proven_external;
    }
  }
#else
  (void)allocator;
#endif
  return result;
}

Hz6FreeRouteResolveResult hz6_allocator_route_resolve_free(
    Hz6Allocator* allocator,
    const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_free_route_resolved(HZ6_FREE_ROUTE_PROVEN_EXTERNAL,
                                   hz6_route_miss(), 0);
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.free_resolve_attempt;
#endif

  int visible_hit = 0;
  int visible_lookup_done = 0;
  Hz6RouteResult route = hz6_route_miss();
  Hz6FreeRouteResolveResult early_result =
      hz6_free_route_resolved(HZ6_FREE_ROUTE_PROVEN_EXTERNAL, route, 0);
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
  Hz6SharedRouteLookup shared_lookup;
  shared_lookup.status = HZ6_SHARED_ROUTE_LOOKUP_MISS;
  shared_lookup.route = hz6_route_miss();
  int shared_lookup_done = 0;
#endif

#if HZ6_REMOTE_FREE_RESOLVE_SHARED_FIRST_L1 && HZ6_SHARED_ROUTE_DIRECTORY_L1
  if (!hz6_allocator_profile_strict_owner_remote(allocator)) {
    shared_lookup = hz6_free_route_shared_lookup(allocator, ptr);
    shared_lookup_done = 1;
    if (hz6_free_route_apply_shared_lookup(allocator,
                                           shared_lookup,
                                           &route,
                                           &visible_lookup_done,
                                           &visible_hit,
                                           &early_result)) {
      return hz6_free_route_finish(allocator, early_result);
    }
  }
#endif

#if HZ6_OWNER_LOCALITY_INDEX_L1
  if (route.kind == HZ6_ROUTE_MISS) {
    route = hz6_allocator_route_lookup_exact(allocator, ptr);
  }
  if (route.kind == HZ6_ROUTE_MISS) {
    Hz6OwnerLocalityKind locality =
        hz6_allocator_route_owner_locality_hint(allocator, ptr);
    if (locality == HZ6_OWNER_LOCALITY_DEFINITELY_FOREIGN) {
      shared_lookup = hz6_free_route_shared_lookup(allocator, ptr);
      shared_lookup_done = 1;
      if (hz6_free_route_apply_shared_lookup(allocator,
                                             shared_lookup,
                                             &route,
                                             &visible_lookup_done,
                                             &visible_hit,
                                             &early_result)) {
        return hz6_free_route_finish(allocator, early_result);
      }
      if (shared_lookup.status == HZ6_SHARED_ROUTE_LOOKUP_MISS &&
          HZ6_SHARED_ROUTE_DIRECTORY_MANDATORY_L1) {
        return hz6_free_route_finish(
            allocator,
            hz6_free_route_resolved(
                HZ6_FREE_ROUTE_UNRESOLVED_INTEGRITY, route, 0));
      }
      if (route.kind != HZ6_ROUTE_MISS) {
        visible_lookup_done = 1;
        visible_hit = (route.route_allocator != allocator);
      }
    }
    if (route.kind == HZ6_ROUTE_MISS) {
      route = hz6_free_route_local_lookup(allocator, ptr);
    }
  }
#else
#if HZ6_LOCAL_EXACT_FIRST_FREE_L1 || \
    HZ6_REMOTE_FREE_RESOLVE_LOCAL_EXACT_ONLY_L1
  if (route.kind == HZ6_ROUTE_MISS) {
    route = hz6_allocator_route_lookup_exact(allocator, ptr);
  }
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
  if (route.kind == HZ6_ROUTE_MISS &&
      !HZ6_REMOTE_FREE_RESOLVE_LOCAL_EXACT_ONLY_L1) {
    route = hz6_free_route_local_lookup(allocator, ptr);
  }
#endif

  if (!visible_lookup_done && route.kind == HZ6_ROUTE_MISS &&
      !hz6_allocator_profile_strict_owner_remote(allocator)) {
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
    if (!shared_lookup_done) {
      shared_lookup = hz6_free_route_shared_lookup(allocator, ptr);
      shared_lookup_done = 1;
    }
    if (hz6_free_route_apply_shared_lookup(allocator,
                                           shared_lookup,
                                           &route,
                                           &visible_lookup_done,
                                           &visible_hit,
                                           &early_result)) {
      return hz6_free_route_finish(allocator, early_result);
    }
#endif
  }

  if (!visible_lookup_done && route.kind == HZ6_ROUTE_MISS &&
      !hz6_allocator_profile_strict_owner_remote(allocator)) {
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_DIAGNOSTIC_PROBES
    hz6_allocator_route_shared_directory_dryrun(allocator, ptr);
#endif
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.free_resolve_visible_shadow;
#endif
    route = hz6_allocator_route_lookup_visible_after_local_miss(allocator, ptr);
    visible_hit = (route.kind != HZ6_ROUTE_MISS);
#if HZ6_DIAGNOSTIC_PROBES
    if (visible_hit) {
      ++allocator->stats.free_resolve_visible_hit;
    }
#endif
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
    if (visible_hit && HZ6_SHARED_ROUTE_DIRECTORY_MANDATORY_L1 &&
        shared_lookup_done &&
        shared_lookup.status == HZ6_SHARED_ROUTE_LOOKUP_MISS) {
      return hz6_free_route_finish(
          allocator,
          hz6_free_route_resolved(
              HZ6_FREE_ROUTE_UNRESOLVED_INTEGRITY, route, visible_hit));
    }
#endif
  }

  return hz6_free_route_finish(
      allocator,
      hz6_free_route_resolved(
          hz6_free_route_kind_for_route(allocator, route, visible_hit), route,
          visible_hit));
}
