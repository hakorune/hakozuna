#include "hz6_midpage_front.h"

int hz6_midpage_class_bytes(uint16_t class_id, size_t* bytes) {
  if (!bytes) {
    return 0;
  }
  switch (class_id) {
    case HZ6_MIDPAGE_8K_CLASS_ID:
      *bytes = HZ6_MIDPAGE_8K_BYTES;
      return 1;
    case HZ6_MIDPAGE_32K_CLASS_ID:
      *bytes = HZ6_MIDPAGE_32K_BYTES;
      return 1;
    default:
      return 0;
  }
}

int hz6_midpage_policy_for_size(size_t size,
                                size_t align,
                                Hz6MidPageRunPolicy* policy) {
  if (!policy || align > 16 || size <= 4096 || size > HZ6_MIDPAGE_BYTES) {
    return 0;
  }

  if (size <= HZ6_MIDPAGE_8K_BYTES) {
    policy->class_id = HZ6_MIDPAGE_8K_CLASS_ID;
    policy->slot_bytes = HZ6_MIDPAGE_8K_BYTES;
    policy->run_bytes = HZ6_MIDPAGE_RUN_BYTES;
  } else {
    policy->class_id = HZ6_MIDPAGE_32K_CLASS_ID;
    policy->slot_bytes = HZ6_MIDPAGE_32K_BYTES;
    policy->run_bytes = HZ6_MIDPAGE_32K_RUN_BYTES;
  }
  policy->slots_per_run = policy->run_bytes / policy->slot_bytes;
  return policy->slots_per_run != 0;
}
