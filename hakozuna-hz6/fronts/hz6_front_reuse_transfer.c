#include "hz6_front_util.h"

void* hz6_front_reuse_transfer(Hz6Allocator* allocator,
                               uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !hz6_allocator_profile_transfer_first(allocator)) {
    return NULL;
  }

  Hz6TransferObject transfer;
  while (hz6_allocator_transfer_pop(allocator, class_id, &transfer)) {
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)transfer.descriptor;
    if (!hz6_allocator_activate_descriptor(
            descriptor, HZ6_STATE_TRANSFER_FREE, transfer.ptr,
            transfer.generation, hz6_allocator_owner_token(allocator))) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.transfer_reuse_invalid;
#endif
      continue;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.transfer_reuse_hit;
#endif
    hz6_allocator_note_transfer_pop(allocator);
    return transfer.ptr;
  }

  return NULL;
}
