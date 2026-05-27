#include "hz6_allocator.h"

Hz6TransferBackendKind hz6_allocator_transfer_backend_kind(
    const Hz6Allocator* allocator) {
  if (!allocator) {
    return HZ6_TRANSFER_BACKEND_SINGLE_CACHE;
  }
  return allocator->transfer_backend.kind;
}

size_t hz6_allocator_transfer_capacity(const Hz6Allocator* allocator) {
  return allocator
             ? hz6_transfer_backend_capacity(&allocator->transfer_backend)
             : 0;
}

size_t hz6_allocator_transfer_count(const Hz6Allocator* allocator) {
  return allocator ? hz6_transfer_backend_count(&allocator->transfer_backend)
                   : 0;
}

size_t hz6_allocator_transfer_count_class(const Hz6Allocator* allocator,
                                          uint16_t class_id) {
  return allocator
             ? hz6_transfer_backend_count_class(&allocator->transfer_backend,
                                                class_id)
             : 0;
}

size_t hz6_allocator_transfer_shard_count_at(const Hz6Allocator* allocator,
                                             size_t shard_index) {
  return allocator ? hz6_transfer_backend_shard_count_at(
                         &allocator->transfer_backend, shard_index)
                   : 0;
}

size_t hz6_allocator_transfer_shard_capacity_at(
    const Hz6Allocator* allocator,
    size_t shard_index) {
  return allocator ? hz6_transfer_backend_shard_capacity_at(
                         &allocator->transfer_backend, shard_index)
                   : 0;
}

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
