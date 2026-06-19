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

int hz6_allocator_transfer_reserve(Hz6Allocator* allocator,
                                   uint16_t class_id,
                                   Hz6TransferReservation* out) {
  if (!allocator || !out) {
    return 0;
  }
  size_t producer_shard = hz6_profile_transfer_producer_shard(
      &allocator->profile, allocator->owner.token.slot, class_id);
  return hz6_transfer_backend_reserve_to_shard(&allocator->transfer_backend,
                                               producer_shard,
                                               out);
}

void hz6_allocator_transfer_cancel(Hz6TransferReservation* reservation) {
  hz6_transfer_backend_cancel(reservation);
}

void hz6_allocator_transfer_commit(Hz6TransferReservation* reservation,
                                   Hz6TransferObject object) {
  hz6_transfer_backend_commit(reservation, object);
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

int hz6_allocator_remote_free_overflow_reserve(
    Hz6Allocator* allocator,
    Hz6TransferReservation* out) {
#if HZ6_REMOTE_FREE_OVERFLOW_L1
  if (!allocator || !out) {
    return 0;
  }
  return hz6_transfer_reserve(&allocator->remote_free_overflow_cache, out);
#else
  (void)allocator;
  (void)out;
  return 0;
#endif
}

void hz6_allocator_remote_free_overflow_commit(
    Hz6TransferReservation* reservation,
    Hz6TransferObject object) {
#if HZ6_REMOTE_FREE_OVERFLOW_L1
  hz6_transfer_commit(reservation, object);
#else
  (void)reservation;
  (void)object;
#endif
}

int hz6_allocator_remote_free_overflow_pop(Hz6Allocator* allocator,
                                           uint16_t class_id,
                                           Hz6TransferObject* out) {
#if HZ6_REMOTE_FREE_OVERFLOW_L1
  if (!allocator || !out) {
    return 0;
  }
  return hz6_transfer_pop(&allocator->remote_free_overflow_cache,
                          class_id, out);
#else
  (void)allocator;
  (void)class_id;
  (void)out;
  return 0;
#endif
}

void hz6_allocator_note_transfer_push(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.transfer_push;
    allocator->stats.transfer_current =
        hz6_allocator_transfer_count(allocator);
    if (allocator->stats.transfer_current >
        allocator->stats.transfer_current_max) {
      allocator->stats.transfer_current_max =
          allocator->stats.transfer_current;
    }
  }
}

void hz6_allocator_note_transfer_pop(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.transfer_pop;
    allocator->stats.transfer_current =
        hz6_allocator_transfer_count(allocator);
    if (allocator->stats.transfer_current >
        allocator->stats.transfer_current_max) {
      allocator->stats.transfer_current_max =
          allocator->stats.transfer_current;
    }
  }
}
