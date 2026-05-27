#include "hz6_allocator.h"

int hz6_allocator_transfer_push(Hz6Allocator* allocator,
                                Hz6TransferObject object) {
  if (!allocator) {
    return 0;
  }
  size_t producer_shard = hz6_profile_transfer_producer_shard(
      &allocator->profile, allocator->owner.token.slot, object.class_id);
  return hz6_transfer_backend_push_to_shard(&allocator->transfer_backend,
                                            object,
                                            producer_shard);
}

int hz6_allocator_transfer_pop(Hz6Allocator* allocator,
                               uint16_t class_id,
                               Hz6TransferObject* out) {
  if (!allocator || !out) {
    return 0;
  }
  size_t home_shard = hz6_profile_transfer_consumer_shard(
      &allocator->profile, allocator->owner.token.slot, class_id);
  return hz6_transfer_backend_pop_from_shard(&allocator->transfer_backend,
                                             class_id,
                                             home_shard,
                                             out);
}

void hz6_allocator_note_transfer_push(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.transfer_push;
  }
}

void hz6_allocator_note_transfer_pop(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.transfer_pop;
  }
}
