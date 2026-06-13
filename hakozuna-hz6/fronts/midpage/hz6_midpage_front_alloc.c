#include "hz6_midpage_front.h"

#include "../hz6_front_source.h"
#include "../hz6_front_util.h"

size_t hz6_midpage_prefill(Hz6Allocator* allocator,
                           uint16_t class_id,
                           size_t count) {
  count = hz6_allocator_control_source_prefill_count(
      allocator, HZ6_FRONT_MIDPAGE, class_id, count);
  size_t filled = 0;
  while (filled < count) {
    size_t run_filled = hz6_midpage_prefill_run(allocator, class_id);
    if (run_filled == 0) {
      break;
    }
    filled += run_filled;
  }
  return filled;
}

int hz6_midpage_can_allocate(size_t size,
                             size_t align,
                             uint16_t* class_id) {
  Hz6MidPageRunPolicy policy;
  if (!class_id || !hz6_midpage_policy_for_size(size, align, &policy)) {
    return 0;
  }
  *class_id = policy.class_id;
  return 1;
}

void* hz6_midpage_alloc(Hz6Allocator* allocator,
                        uint16_t class_id,
                        size_t size) {
  (void)size;
  size_t bytes = 0;
  if (!allocator || !hz6_midpage_class_bytes(class_id, &bytes)) {
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  if (class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
    ++allocator->stats.midpage_8k_alloc_call;
  } else if (class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
    ++allocator->stats.midpage_32k_alloc_call;
  }
#endif

  void* reused = hz6_front_reuse_transfer_or_cached(allocator,
                                                    HZ6_FRONT_MIDPAGE,
                                                    class_id,
                                                    NULL);
  if (reused) {
    return reused;
  }

  if (hz6_midpage_prefill_run(allocator, class_id) != 0) {
    reused = hz6_front_reuse_transfer_or_cached(allocator,
                                                HZ6_FRONT_MIDPAGE,
                                                class_id,
                                                NULL);
    if (reused) {
      return reused;
    }
  }

  return hz6_front_reuse_or_source_kind(
      allocator, HZ6_FRONT_MIDPAGE, class_id, bytes,
      hz6_allocator_profile_source_kind(allocator));
}
