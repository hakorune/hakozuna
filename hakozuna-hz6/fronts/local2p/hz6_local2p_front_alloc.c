#include "hz6_local2p_front.h"

#include "../hz6_front_source.h"
#include "../hz6_front_source_prefill.h"
#include "../hz6_front_util.h"

int hz6_local2p_can_allocate(size_t size,
                             size_t align,
                             uint16_t* class_id) {
  if (!class_id || align > 16 || size != HZ6_LOCAL2P_BYTES) {
    return 0;
  }
  *class_id = HZ6_LOCAL2P_CLASS_ID;
  return 1;
}

void* hz6_local2p_alloc(Hz6Allocator* allocator,
                        uint16_t class_id,
                        size_t size) {
  (void)size;
  if (!allocator || class_id != HZ6_LOCAL2P_CLASS_ID) {
    return NULL;
  }

  size_t refill_batch = hz6_allocator_control_source_refill_batch(
      allocator, HZ6_FRONT_LOCAL2P, class_id);
  Hz6SourceKind source_kind =
      hz6_allocator_profile_source_kind(allocator);
  return hz6_front_reuse_or_prefill_source_kind(
      allocator, HZ6_FRONT_LOCAL2P, class_id, HZ6_LOCAL2P_BYTES,
      source_kind, refill_batch);
}
