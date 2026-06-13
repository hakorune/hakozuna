#include "hz6_allocator_route_last_hit.h"

#if HZ6_ROUTE_LAST_HIT_CACHE_L1
void hz6_allocator_route_last_hit_clear(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }
  allocator->route_last_hit = (Hz6RouteLastHitCache){0};
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.route_lookup_last_hit_clear;
#endif
}

static int hz6_allocator_route_last_hit_descriptor_alive(
    const Hz6ObjectDescriptor* descriptor,
    const void* ptr,
    uint32_t generation) {
  if (!descriptor || descriptor->ptr != ptr ||
      descriptor->generation != generation ||
      descriptor->state == HZ6_STATE_DEAD ||
      descriptor->state == HZ6_STATE_DESCRIPTOR_RESERVED) {
    return 0;
  }
  if (descriptor->source_block &&
      (!hz6_source_block_active(descriptor->source_block) ||
       !descriptor->source_block->ptr)) {
    return 0;
  }
  return 1;
}

Hz6RouteResult hz6_allocator_route_last_hit_lookup(
    const Hz6Allocator* allocator,
    const void* ptr) {
  if (!allocator || !ptr || allocator->route_last_hit.base != ptr) {
    return hz6_route_miss();
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++((Hz6Allocator*)allocator)->stats.route_lookup_last_hit_attempt;
#endif
  const Hz6ObjectDescriptor* descriptor =
      (const Hz6ObjectDescriptor*)allocator->route_last_hit.descriptor;
  if (!hz6_allocator_route_last_hit_descriptor_alive(
          descriptor, ptr, allocator->route_last_hit.generation)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++((Hz6Allocator*)allocator)->stats.route_lookup_last_hit_stale;
#endif
    return hz6_route_miss();
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++((Hz6Allocator*)allocator)->stats.route_lookup_last_hit_hit;
#endif
  Hz6RouteResult route =
      hz6_route_valid(allocator->route_last_hit.front_id,
                      allocator->route_last_hit.class_id,
                      allocator->route_last_hit.generation,
                      allocator->route_last_hit.descriptor);
  route.route_allocator = (Hz6Allocator*)allocator;
  return route;
}

void hz6_allocator_route_last_hit_fill(Hz6Allocator* allocator,
                                       void* base,
                                       uint16_t front_id,
                                       uint16_t class_id,
                                       uint32_t generation,
                                       void* descriptor) {
  if (!allocator || !base || !descriptor) {
    return;
  }
  allocator->route_last_hit.base = base;
  allocator->route_last_hit.descriptor = descriptor;
  allocator->route_last_hit.generation = generation;
  allocator->route_last_hit.front_id = front_id;
  allocator->route_last_hit.class_id = class_id;
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.route_lookup_last_hit_fill;
#endif
}

void hz6_allocator_route_last_hit_fill_route(Hz6Allocator* allocator,
                                             const void* ptr,
                                             Hz6RouteResult route) {
  if (!allocator || !ptr || route.kind != HZ6_ROUTE_VALID ||
      !route.descriptor) {
    return;
  }
  hz6_allocator_route_last_hit_fill(allocator,
                                    (void*)ptr,
                                    route.front_id,
                                    route.class_id,
                                    route.generation,
                                    route.descriptor);
}
#endif
