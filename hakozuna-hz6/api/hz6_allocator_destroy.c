#include "hz6_allocator.h"

#include "hz6_allocator_destroy_internal.h"
#include "hz6_allocator_midpage_active_map.h"
#include "hz6_allocator_toy_small_diag.h"

void hz6_allocator_destroy(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  allocator->owner.state = HZ6_OWNER_DYING;
  hz6_allocator_remote_pending_closeout_for_destroy(allocator);
  hz6_allocator_route_visibility_unregister(allocator);
  hz6_allocator_destroy_descriptors(allocator);
  hz6_allocator_destroy_source_blocks(allocator);
#if HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1
  hz6_toy_small_active_map_destroy(allocator);
#endif
  hz6_midpage_active_map_destroy(allocator);
  hz6_allocator_remote_pending_note_after_destroy(allocator);
  hz6_allocator_remote_pending_storage_release(allocator);
  allocator->owner.state = HZ6_OWNER_DEAD;
}
