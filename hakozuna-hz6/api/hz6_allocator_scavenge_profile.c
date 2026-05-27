#include "hz6_allocator.h"

size_t hz6_allocator_scavenge_profile(Hz6Allocator* allocator) {
  if (!allocator) {
    return 0;
  }

  size_t released = 0;
  size_t orphan_budget =
      hz6_profile_scavenge_orphan_budget(&allocator->profile);
  if (orphan_budget != 0) {
    released += hz6_allocator_scavenge_orphans(allocator, orphan_budget);
  }
  size_t local_free_budget =
      hz6_profile_scavenge_local_free_budget(&allocator->profile);
  if (local_free_budget != 0) {
    released += hz6_allocator_scavenge_local_free(allocator,
                                                  local_free_budget);
  }
  return released;
}
