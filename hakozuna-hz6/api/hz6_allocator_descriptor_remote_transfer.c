#include "hz6_allocator.h"

int hz6_allocator_remote_free_active_descriptor(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr) {
  if (!allocator || !descriptor || !ptr ||
      descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr) {
    return 0;
  }

  ++allocator->stats.remote_free_attempt;
  if (hz6_allocator_profile_strict_owner_remote(allocator)) {
    if (!hz6_allocator_descriptor_owner_equal_at(
            allocator, descriptor, allocator->owner.token,
            HZ6_OWNER_EQUAL_SITE_REMOTE_FREE)) {
      ++allocator->stats.remote_free_strict_owner_block;
      return 0;
    }
    descriptor->state = HZ6_STATE_REMOTE_PENDING;
    return 1;
  }

  Hz6TransferObject object;
  object.ptr = ptr;
  object.descriptor = descriptor;
  object.class_id = descriptor->class_id;
  object.generation = descriptor->generation;

#if HZ6_REMOTE_FREE_COMMIT_L1
  Hz6TransferReservation reservation = {0};
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.transfer_reserve_attempt;
#endif
  if (!hz6_allocator_transfer_reserve(allocator, object.class_id,
                                      &reservation)) {
    ++allocator->stats.remote_free_transfer_fail;
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.transfer_reserve_full;
    ++allocator->stats.remote_free_backpressure;
#endif
    return 0;
  }

  descriptor->state = HZ6_STATE_TRANSFER_FREE;
  hz6_allocator_set_descriptor_owner(allocator, descriptor,
                                     (Hz6OwnerToken){0});
  hz6_allocator_transfer_commit(&reservation, object);
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.transfer_reserve_success;
#endif
  hz6_allocator_note_transfer_push(allocator);
  return 1;
#else
  descriptor->state = HZ6_STATE_TRANSFER_FREE;
  hz6_allocator_set_descriptor_owner(allocator, descriptor,
                                     (Hz6OwnerToken){0});
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.transfer_reserve_attempt;
#endif
  if (!hz6_allocator_transfer_push(allocator, object)) {
    ++allocator->stats.remote_free_transfer_fail;
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.transfer_reserve_full;
    ++allocator->stats.transfer_reserve_full_after_state_mutation;
    ++allocator->stats.remote_free_backpressure;
#endif
    descriptor->state = HZ6_STATE_ACTIVE;
    hz6_allocator_set_descriptor_owner(allocator, descriptor,
                                       allocator->owner.token);
    return 0;
  }

#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.transfer_reserve_success;
#endif
  hz6_allocator_note_transfer_push(allocator);
  return 1;
#endif
}
