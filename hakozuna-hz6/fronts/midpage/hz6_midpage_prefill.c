#include "hz6_midpage_front.h"

#include "../hz6_front_source_block.h"
#include "../hz6_front_source.h"
#include "../hz6_front_util.h"

static int hz6_midpage_policy_for_class(uint16_t class_id,
                                        Hz6MidPageRunPolicy* policy) {
  size_t bytes = 0;
  if (!policy || !hz6_midpage_class_bytes(class_id, &bytes)) {
    return 0;
  }
  policy->class_id = class_id;
  policy->slot_bytes = bytes;
  policy->run_bytes = HZ6_MIDPAGE_RUN_BYTES;
  policy->slots_per_run = policy->run_bytes / policy->slot_bytes;
  return policy->slots_per_run != 0;
}

size_t hz6_midpage_prefill_run(Hz6Allocator* allocator, uint16_t class_id) {
  Hz6MidPageRunPolicy policy;
  if (!allocator || !hz6_midpage_policy_for_class(class_id, &policy) ||
      class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }

  if (hz6_allocator_frontcache_count(allocator, class_id) >=
      hz6_allocator_frontcache_capacity(allocator, class_id)) {
    return 0;
  }

  Hz6SourceKind source_kind =
      hz6_allocator_profile_source_kind(allocator);
  const Hz6OsMemoryOps* source_ops =
      hz6_allocator_source_ops(allocator, source_kind);
  Hz6SourceBlock* block = hz6_allocator_create_source_block(
      allocator, policy.run_bytes, source_ops, source_kind);
  if (!block) {
    return 0;
  }

  size_t filled = 0;
  while (filled < policy.slots_per_run &&
         hz6_allocator_frontcache_count(allocator, class_id) <
             hz6_allocator_frontcache_capacity(allocator, class_id)) {
    size_t offset = filled * policy.slot_bytes;
    void* ptr = hz6_front_source_block_slot(
        allocator, HZ6_FRONT_MIDPAGE, class_id, policy.slot_bytes, offset,
        block);
    if (!ptr) {
      break;
    }

    Hz6RouteResult route = hz6_allocator_route_lookup(allocator, ptr);
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)route.descriptor;
    if (route.kind != HZ6_ROUTE_VALID || !descriptor) {
      break;
    }

    if (!hz6_allocator_cache_active_descriptor(allocator, descriptor, ptr)) {
      break;
    }

    ++filled;
  }

  if (filled == 0) {
    hz6_allocator_release_source_block(block);
  } else {
    hz6_allocator_note_source_alloc(allocator);
  }
  return filled;
}
