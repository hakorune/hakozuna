#include "hz6_allocator.h"

void hz6_allocator_init(Hz6Allocator* allocator) {
  hz6_allocator_init_with_profile(allocator, HZ6_PROFILE_STRICT);
}

void hz6_allocator_init_with_profile(Hz6Allocator* allocator,
                                     Hz6ProfileId profile_id) {
  if (!allocator) {
    return;
  }
  allocator->profile = hz6_profile_config(profile_id);
  allocator->stats.route_valid = 0;
  allocator->stats.route_invalid = 0;
  allocator->stats.route_miss = 0;
  allocator->stats.transfer_push = 0;
  allocator->stats.transfer_pop = 0;
  allocator->stats.source_alloc = 0;
  hz6_route_table_init(&allocator->route_table, allocator->route_entries,
                       HZ6_ROUTE_TABLE_CAPACITY);
  hz6_transfer_init(&allocator->transfer_cache, allocator->transfer_objects,
                    HZ6_TRANSFER_CACHE_CAPACITY);
}

void* hz6_malloc(Hz6Allocator* allocator, size_t size) {
  (void)allocator;
  (void)size;
  return NULL;
}

void hz6_free(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return;
  }

  Hz6RouteResult route = hz6_route_lookup(&allocator->route_table, ptr);
  switch (route.kind) {
    case HZ6_ROUTE_VALID:
      ++allocator->stats.route_valid;
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

int hz6_owns(Hz6Allocator* allocator, const void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }
  Hz6RouteResult route = hz6_route_lookup(&allocator->route_table, ptr);
  return route.kind == HZ6_ROUTE_VALID || route.kind == HZ6_ROUTE_INVALID;
}

Hz6StatsSnapshot hz6_stats_snapshot(const Hz6Allocator* allocator) {
  Hz6StatsSnapshot empty;
  empty.route_valid = 0;
  empty.route_invalid = 0;
  empty.route_miss = 0;
  empty.transfer_push = 0;
  empty.transfer_pop = 0;
  empty.source_alloc = 0;
  return allocator ? allocator->stats : empty;
}

