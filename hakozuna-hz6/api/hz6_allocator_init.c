#include "hz6_allocator.h"

#include "hz6_allocator_init_internal.h"

void hz6_allocator_init(Hz6Allocator* allocator) {
  hz6_allocator_init_with_profile(allocator, HZ6_PROFILE_STRICT);
}

void hz6_allocator_init_with_profile(Hz6Allocator* allocator,
                                     Hz6ProfileId profile_id) {
  if (!allocator) {
    return;
  }

  hz6_allocator_init_state(allocator, profile_id);
  hz6_allocator_init_backends(allocator);
}
