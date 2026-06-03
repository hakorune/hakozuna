#include "hz6_allocator.h"

static size_t hz6_allocator_scavenge_cost(
    const Hz6ObjectDescriptor* descriptor) {
  if (!descriptor) {
    return 0;
  }
  if (descriptor->source_block) {
    return descriptor->bytes;
  }
  size_t source_bytes = 0;
  if (hz6_allocator_descriptor_source_meta(descriptor, NULL, &source_bytes,
                                           NULL) &&
      source_bytes != 0) {
    return source_bytes;
  }
  return descriptor->bytes;
}

size_t hz6_allocator_scavenge_orphans(Hz6Allocator* allocator,
                                      size_t max_bytes) {
  if (!allocator || max_bytes == 0) {
    return 0;
  }

  Hz6ScavengeBudget budget;
  hz6_scavenge_budget_init(&budget, max_bytes);

  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr || descriptor->state != HZ6_STATE_ORPHAN) {
      continue;
    }

    size_t bytes = hz6_allocator_scavenge_cost(descriptor);
    if (!hz6_scavenge_can_release(&budget, bytes)) {
      continue;
    }

    hz6_allocator_route_unregister_exact(allocator, descriptor->ptr);
#if HZ6_DIAGNOSTIC_PROBES
    if (hz6_allocator_descriptor_has_source_release(descriptor)) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(descriptor);
    hz6_scavenge_account_release(&budget, bytes);
  }

  return budget.objects_released;
}
