#include "hz6_allocator.h"

Hz6StatsSnapshot hz6_stats_snapshot(const Hz6Allocator* allocator) {
  Hz6StatsSnapshot empty = {0};
  return allocator ? allocator->stats : empty;
}
