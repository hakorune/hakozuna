#include "hz6_allocator.h"
#include "hz6_allocator_same_owner_fast_inline.h"
#include "hz6_allocator_toy_small_diag.h"

#include "../fronts/hz6_front.h"

void hz6_free(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return;
  }

  if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
    return;
  }

#if HZ6_SOURCE_BLOCK_ROUTE_DRYRUN_L1 && HZ6_DIAGNOSTIC_PROBES
  hz6_allocator_source_block_route_dryrun(allocator, ptr);
#endif

  int visible_hit = 0;
  int visible_lookup_done = 0;
  Hz6RouteResult route = hz6_route_miss();
#if HZ6_SOURCE_BLOCK_ROUTE_BEHAVIOR_L1
  route = hz6_allocator_source_block_route_lookup(allocator, ptr);
#endif
#if HZ6_OWNER_LOCALITY_INDEX_L1
  if (route.kind == HZ6_ROUTE_MISS) {
    route = hz6_allocator_route_lookup_exact(allocator, ptr);
  }
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
        hz6_toy_small_hotpath_diag_free_route_lookup(
            allocator, route.front_id, route.class_id);
        const Hz6FrontOps* front = hz6_front_for_id(route.front_id);
        Hz6ObjectDescriptor* descriptor =
            (Hz6ObjectDescriptor*)route.descriptor;
#if HZ6_DIAGNOSTIC_PROBES
        if (descriptor) {
          Hz6Allocator* route_allocator =
              route.route_allocator ? route.route_allocator : allocator;
          if (hz6_allocator_descriptor_belongs_to(route_allocator,
                                                  descriptor)) {
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
                hz6_allocator_descriptor_storage_owner_diagnostic(
                    allocator,
                    descriptor,
                    &storage_probes);
            ++allocator->stats.descriptor_storage_lookup;
            allocator->stats.descriptor_storage_probe_total += storage_probes;
            if (storage_probes >
                allocator->stats.descriptor_storage_probe_max) {
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
        int local_owner = descriptor &&
                          hz6_allocator_descriptor_owner_equal_at(
                              allocator, descriptor, allocator->owner.token,
                              HZ6_OWNER_EQUAL_SITE_FREE);
        if (local_owner) {
          hz6_toy_small_hotpath_diag_free_owner_equal(
              allocator, route.front_id, route.class_id);
        }
        int needs_rehome = visible_hit && !local_owner &&
                            hz6_front_remote_rehome_allowed(route.front_id,
                                                            route.class_id);
        int ok = 0;
#if HZ6_DIAGNOSTIC_PROBES
        if (!local_owner) {
          ++allocator->stats.lifecycle_foreign_free_attempt;
          if (visible_hit) {
            ++allocator->stats.lifecycle_owner_mismatch;
          }
        }
#endif
        if (needs_rehome && descriptor &&
            hz6_allocator_descriptor_has_source_release(allocator,
                                                        descriptor)) {
#if HZ6_DIAGNOSTIC_PROBES
          ++allocator->stats.source_owned_remote_free_attempt;
#endif
        }
        if (!front) {
          ok = 0;
#if HZ6_DIAGNOSTIC_PROBES
          ++allocator->stats.free_invalid_no_front;
#endif
#if HZ6_SAME_OWNER_FAST_L1
        } else if (local_owner &&
                   hz6_allocator_same_owner_fast_front_eligible_inline(
                       route.front_id)) {
          ok = hz6_allocator_same_owner_fast_free_inline(allocator, ptr,
                                                         route);
          if (ok) {
            hz6_toy_small_hotpath_diag_free_fast_hit(
                allocator, route.front_id, route.class_id);
            hz6_toy_small_hotpath_diag_free_cache_push(
                allocator, route.front_id, route.class_id);
          }
#if HZ6_DIAGNOSTIC_PROBES
          if (!ok) {
            ++allocator->stats.free_invalid_same_owner_fast;
          }
#endif
#elif HZ6_LOCAL_CACHE_DIRECT_FREE_L1
        } else if (local_owner &&
                   route.class_id <= HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS &&
                   (route.front_id == HZ6_FRONT_TOY ||
                    route.front_id == HZ6_FRONT_MIDPAGE ||
                    route.front_id == HZ6_FRONT_LOCAL2P)) {
#if HZ6_LOCAL_CACHE_TRUSTED_OWNER_L1
          ok = hz6_allocator_cache_active_descriptor_trusted_owner(
              allocator, descriptor, ptr);
#else
          ok = hz6_allocator_cache_active_descriptor(allocator, descriptor,
                                                     ptr);
#endif
          if (ok) {
            hz6_toy_small_hotpath_diag_free_fast_hit(
                allocator, route.front_id, route.class_id);
            hz6_toy_small_hotpath_diag_free_cache_push(
                allocator, route.front_id, route.class_id);
          }
#if HZ6_DIAGNOSTIC_PROBES
          if (!ok) {
            ++allocator->stats.free_invalid_local_cache_direct;
          }
#endif
#endif
        } else if (local_owner) {
          ok = front->free_tagged &&
               front->free_tagged(allocator, ptr, route);
          if (ok) {
            hz6_toy_small_hotpath_diag_free_cache_push(
                allocator, route.front_id, route.class_id);
          }
#if HZ6_DIAGNOSTIC_PROBES
          if (!ok) {
            ++allocator->stats.free_invalid_front_tagged;
          }
#endif
        } else {
          if (needs_rehome) {
#if HZ6_DIAGNOSTIC_PROBES
            ++allocator->stats.route_rehome_attempt;
#endif
          }
          ok = front->remote_free_tagged &&
               front->remote_free_tagged(allocator, ptr, route);
#if HZ6_DIAGNOSTIC_PROBES
          if (!ok) {
            ++allocator->stats.free_invalid_remote_tagged;
          }
#endif
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
#if HZ6_DIAGNOSTIC_PROBES
          if (local_owner) {
            ++allocator->stats.free_invalid_local_owner;
          } else {
            ++allocator->stats.free_invalid_remote_owner;
          }
#endif
          ++allocator->stats.route_invalid;
        }
      }
      return;
    case HZ6_ROUTE_INVALID:
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.free_invalid_route_kind_invalid;
#endif
      ++allocator->stats.route_invalid;
      return;
    case HZ6_ROUTE_MISS:
    default:
      ++allocator->stats.route_miss;
      return;
  }
}
