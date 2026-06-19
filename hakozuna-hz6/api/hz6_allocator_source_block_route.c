#include "hz6_allocator.h"
#include "hz6_allocator_route_domain.h"

int hz6_allocator_source_block_register_invalid_range(
    Hz6Allocator* allocator,
    Hz6SourceBlock* block,
    uint16_t front_id,
    uint16_t class_id) {
  if (!allocator || !block || !hz6_source_block_active(block) || !block->ptr ||
      block->bytes == 0) {
    return 0;
  }
  if (hz6_source_block_route_registered(block)) {
    return 1;
  }
  size_t* probe_ptr = NULL;
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  probe_ptr = &probes;
#endif
  int route_shared = 0;
  int route_domain_locked = 0;
  hz6_allocator_route_domain_lock(allocator);
  route_domain_locked = 1;
  if (!hz6_route_backend_register_invalid_range(&allocator->route_backend,
                                                block->ptr,
                                                block->bytes,
                                                front_id,
                                                class_id,
                                                probe_ptr)) {
    hz6_allocator_route_domain_unlock(allocator);
    route_domain_locked = 0;
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_ELASTIC_ROUTE_OVERFLOW_L1
    if (!hz6_allocator_route_register_shared_invalid_range(allocator,
                                                           block->ptr,
                                                           block->bytes,
                                                           front_id,
                                                           class_id)) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.elastic_route_overflow_register_fail;
#endif
      return 0;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.elastic_route_overflow_register;
#endif
    route_shared = 1;
#else
    return 0;
#endif
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
  if (route_domain_locked) {
    hz6_allocator_route_domain_unlock(allocator);
  }
#if !HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1
  block->route_backend = route_shared ? NULL : &allocator->route_backend;
#endif
  hz6_source_block_set_route_registered(block, 1);
  hz6_source_block_set_route_shared(block, route_shared);
  return 1;
}
