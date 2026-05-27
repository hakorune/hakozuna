#include "hz6_allocator.h"

static size_t hz6_allocator_scavenge_cost(
    const Hz6ObjectDescriptor* descriptor) {
  if (!descriptor) {
    return 0;
  }
  return descriptor->source_block ? descriptor->bytes
                                  : (descriptor->source_bytes
                                         ? descriptor->source_bytes
                                         : descriptor->bytes);
}

size_t hz6_allocator_scavenge_local_free(Hz6Allocator* allocator,
                                         size_t max_bytes) {
  if (!allocator || max_bytes == 0) {
    return 0;
  }

  Hz6ScavengeBudget budget;
  hz6_scavenge_budget_init(&budget, max_bytes);

  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr || descriptor->state != HZ6_STATE_LOCAL_FREE ||
        descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
      continue;
    }

    size_t bytes = hz6_allocator_scavenge_cost(descriptor);
    if (!hz6_scavenge_can_release(&budget, bytes)) {
      continue;
    }

    if (!hz6_allocator_frontcache_remove(allocator,
                                         descriptor->class_id,
                                         descriptor->ptr,
                                         descriptor,
                                         descriptor->generation,
                                         NULL)) {
      continue;
    }

    hz6_allocator_route_unregister_exact(allocator, descriptor->ptr);
    hz6_allocator_release_descriptor_source(descriptor);
    hz6_scavenge_account_release(&budget, bytes);
  }

  return budget.objects_released;
}
