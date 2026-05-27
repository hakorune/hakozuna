#include "hz6_allocator_init_internal.h"

void hz6_allocator_init_state(Hz6Allocator* allocator,
                              Hz6ProfileId profile_id) {
  if (!allocator) {
    return;
  }

  hz6_allocator_init_state_owner(allocator, profile_id);
  hz6_allocator_init_state_source_blocks(allocator);
  hz6_allocator_init_state_descriptors(allocator);
  hz6_allocator_init_state_frontcache(allocator);
}
