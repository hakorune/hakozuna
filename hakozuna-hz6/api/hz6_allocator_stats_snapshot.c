#include "hz6_allocator.h"

Hz6StatsSnapshot hz6_stats_snapshot(const Hz6Allocator* allocator) {
  Hz6StatsSnapshot empty;
  empty.route_valid = 0;
  empty.route_invalid = 0;
  empty.route_miss = 0;
  empty.transfer_push = 0;
  empty.transfer_pop = 0;
  empty.source_alloc = 0;
  empty.large_span_central_push = 0;
  empty.large_span_central_pop = 0;
  empty.large_span_source_alloc = 0;
  return allocator ? allocator->stats : empty;
}
