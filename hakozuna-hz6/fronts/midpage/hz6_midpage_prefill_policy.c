#include "hz6_midpage_front.h"

int hz6_midpage_prefill_policy_for_class(uint16_t class_id,
                                         Hz6MidPageRunPolicy* policy) {
  size_t bytes = 0;
  if (!policy || !hz6_midpage_class_bytes(class_id, &bytes)) {
    return 0;
  }
  policy->class_id = class_id;
  policy->slot_bytes = bytes;
  policy->run_bytes = class_id == HZ6_MIDPAGE_32K_CLASS_ID
                          ? HZ6_MIDPAGE_32K_RUN_BYTES
                          : HZ6_MIDPAGE_RUN_BYTES;
  policy->slots_per_run = policy->run_bytes / policy->slot_bytes;
  return policy->slots_per_run != 0;
}
