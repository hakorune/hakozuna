#include "hz6_allocator.h"

int hz6_allocator_orphan_adopt_commit(
    Hz6Allocator* adopter,
    Hz6Allocator* source,
    void* ptr,
    const Hz6RouteResult* source_route,
    Hz6ObjectDescriptor* source_descriptor,
    Hz6ObjectDescriptor* adopted_descriptor) {
  if (!adopter || !source || !ptr || !source_route || !source_descriptor ||
      !adopted_descriptor) {
    return 0;
  }

#if HZ6_DIAGNOSTIC_PROBES
  size_t adopt_register_probes = 0;
  if (!hz6_route_backend_register_exact(&adopter->route_backend,
                                        ptr,
                                        adopted_descriptor->bytes,
                                        source_route->front_id,
                                        source_route->class_id,
                                        adopted_descriptor->generation,
                                        adopted_descriptor,
                                        &adopt_register_probes)) {
    *adopted_descriptor = (Hz6ObjectDescriptor){0};
    return 0;
  }
  adopter->stats.route_register_probe_total += adopt_register_probes;
  if (adopt_register_probes > adopter->stats.route_register_probe_max) {
    adopter->stats.route_register_probe_max = adopt_register_probes;
  }
  adopter->stats.route_active_current =
      adopter->route_backend.exact_table.active_count;
  if (adopter->stats.route_active_current > adopter->stats.route_active_max) {
    adopter->stats.route_active_max = adopter->stats.route_active_current;
  }
#else
  if (!hz6_route_backend_register_exact(&adopter->route_backend,
                                        ptr,
                                        adopted_descriptor->bytes,
                                        source_route->front_id,
                                        source_route->class_id,
                                        adopted_descriptor->generation,
                                        adopted_descriptor,
                                        NULL)) {
    *adopted_descriptor = (Hz6ObjectDescriptor){0};
    return 0;
  }
#endif

  Hz6FrontCacheEntry entry;
  entry.ptr = ptr;
  entry.descriptor = adopted_descriptor;
  entry.class_id = adopted_descriptor->class_id;
  entry.generation = adopted_descriptor->generation;
  if (!hz6_allocator_frontcache_push(adopter, entry.class_id, entry)) {
    hz6_allocator_route_unregister_exact(adopter, ptr);
    *adopted_descriptor = (Hz6ObjectDescriptor){0};
    return 0;
  }

  hz6_allocator_route_unregister_exact(source, ptr);
  source_descriptor->ptr = NULL;
  source_descriptor->bytes = 0;
  source_descriptor->source_ptr = NULL;
  source_descriptor->source_bytes = 0;
  source_descriptor->source_block = NULL;
  source_descriptor->class_id = 0;
  source_descriptor->source_kind = HZ6_SOURCE_NONE;
  source_descriptor->source_release = NULL;
  source_descriptor->owner = (Hz6OwnerToken){0};
  source_descriptor->generation = 0;
  source_descriptor->state = HZ6_STATE_DEAD;
  return 1;
}
