#include "hz6_local2p_front.h"

#include "../hz6_front_source.h"
#include "../hz6_front_source_prefill.h"

size_t hz6_local2p_prefill(Hz6Allocator* allocator,
                           uint16_t class_id,
                           size_t count) {
  if (class_id != 0 && class_id != HZ6_LOCAL2P_CLASS_ID) {
    return 0;
  }
  return hz6_front_prefill_source_kind(
      allocator, HZ6_FRONT_LOCAL2P, HZ6_LOCAL2P_CLASS_ID, HZ6_LOCAL2P_BYTES,
      hz6_allocator_profile_source_kind(allocator), count);
}
