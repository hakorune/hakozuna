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
    hz6_allocator_reset_descriptor_available(adopter, adopted_descriptor);
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
    hz6_allocator_reset_descriptor_available(adopter, adopted_descriptor);
    return 0;
  }
#endif

  Hz6FrontCacheEntry entry = {0};
  entry.ptr = ptr;
  entry.descriptor = adopted_descriptor;
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  entry.source_block = adopted_descriptor->source_block;
#endif
  entry.generation = adopted_descriptor->generation;
  hz6_frontcache_entry_set_bytes(&entry, adopted_descriptor->bytes);
  hz6_frontcache_entry_set_class_id(&entry, adopted_descriptor->class_id);
  const uint16_t entry_class_id = hz6_frontcache_entry_class_id(&entry);
  if (!hz6_allocator_frontcache_push(adopter, entry_class_id, entry)) {
    hz6_allocator_route_unregister_exact(adopter, ptr);
    hz6_allocator_reset_descriptor_available(adopter, adopted_descriptor);
    return 0;
  }

  hz6_allocator_route_unregister_exact(source, ptr);
  hz6_allocator_reset_descriptor_available(source, source_descriptor);
  return 1;
}
