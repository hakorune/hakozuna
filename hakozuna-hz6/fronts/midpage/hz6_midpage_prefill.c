#include "hz6_midpage_front.h"

#include "../hz6_front_source.h"
#include "../hz6_front_source_block.h"

size_t hz6_midpage_prefill_run(Hz6Allocator* allocator, uint16_t class_id) {
  Hz6MidPageRunPolicy policy;
  if (!allocator || !hz6_midpage_prefill_policy_for_class(class_id, &policy) ||
      class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }

  if (hz6_allocator_frontcache_count(allocator, class_id) >=
      hz6_allocator_frontcache_capacity(allocator, class_id)) {
    return 0;
  }

  size_t filled = hz6_front_prefill_source_block_kind(
      allocator, HZ6_FRONT_MIDPAGE, class_id, policy.slot_bytes,
      policy.run_bytes, hz6_allocator_profile_source_kind(allocator),
      policy.slots_per_run);
#if HZ6_DIAGNOSTIC_PROBES
  if (class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
    ++allocator->stats.midpage_8k_prefill_run_call;
    allocator->stats.midpage_8k_prefill_run_filled += filled;
  } else if (class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
    ++allocator->stats.midpage_32k_prefill_run_call;
    allocator->stats.midpage_32k_prefill_run_filled += filled;
  }
#endif
  return filled;
}
