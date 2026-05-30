#include "hz6_allocator.h"

int hz6_allocator_source_block_register_invalid_range(
    Hz6Allocator* allocator,
    Hz6SourceBlock* block,
    uint16_t front_id,
    uint16_t class_id) {
  if (!allocator || !block || !block->active || !block->ptr ||
      block->bytes == 0) {
    return 0;
  }
  if (block->route_registered) {
    return 1;
  }
  size_t* probe_ptr = NULL;
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  probe_ptr = &probes;
#endif
  if (!hz6_route_backend_register_invalid_range(&allocator->route_backend,
                                                block->ptr,
                                                block->bytes,
                                                front_id,
                                                class_id,
                                                probe_ptr)) {
    return 0;
  }
#if HZ6_DIAGNOSTIC_PROBES
  allocator->stats.route_register_probe_total += probes;
  if (probes > allocator->stats.route_register_probe_max) {
    allocator->stats.route_register_probe_max = probes;
  }
  allocator->stats.route_active_current =
      allocator->route_backend.exact_table.active_count;
  if (allocator->stats.route_active_current >
      allocator->stats.route_active_max) {
    allocator->stats.route_active_max =
        allocator->stats.route_active_current;
  }
#endif
  block->route_backend = &allocator->route_backend;
  block->route_registered = 1;
  return 1;
}
