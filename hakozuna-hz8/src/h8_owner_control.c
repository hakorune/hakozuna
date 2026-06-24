#include "h8_internal.h"

#include <stdlib.h>

void h8_owner_free_stack_push(H8OwnerRecord* owner) {
  pthread_mutex_lock(&h8g.owner_lock);
  owner->free_next = h8g.owner_free;
  h8g.owner_free = owner;
  pthread_mutex_unlock(&h8g.owner_lock);
}

void h8_owner_mark_alive(H8OwnerRecord* owner, uint32_t slot, uint16_t generation,
                         bool permanent) {
  owner->slot = slot;
  owner->generation = generation;
  owner->permanent = permanent;
  owner->placement = permanent ? H8_OWNER_PLACEMENT_ORPHAN
                               : H8_OWNER_PLACEMENT_OWNED;
  for (size_t i = 0; i < H8_CLASS_COUNT; ++i) {
    owner->owned_by_class[i] = NULL;
  }
  for (size_t i = 0; i < H8_MEDIUM_CLASS_COUNT; ++i) {
    owner->medium_by_class[i] = NULL;
  }
  atomic_store_explicit(&owner->pending_head, NULL, memory_order_relaxed);
  atomic_store_explicit(&owner->pending_span_count, 0, memory_order_relaxed);
  owner->pending_carry = NULL;
  owner->medium_pending_head = NULL;
  owner->medium_pending_carry = NULL;
  atomic_store_explicit(&owner->medium_pending_count, 0, memory_order_relaxed);
  owner->owned_head = NULL;
  owner->orphan_head = NULL;
  owner->span_chunk_next = 0;
  owner->span_chunk_end = 0;
  for (size_t i = 0; i < H8_CLASS_COUNT; ++i) {
    owner->orphan_by_class[i] = NULL;
  }
  atomic_store_explicit(&owner->control,
                        h8_ctl_pack((H8CtlWord){.generation = generation,
                                                .state = H8_OWNER_ALIVE,
                                                .publish_closed = 0,
                                                .publish_refs = 0}),
                        memory_order_release);
}

void h8_owner_mark_dying(H8OwnerRecord* owner) {
  uint64_t cur = atomic_load_explicit(&owner->control, memory_order_acquire);
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(cur);
    ctl.state = H8_OWNER_DYING;
    ctl.publish_closed = 1;
    uint64_t next = h8_ctl_pack(ctl);
    if (atomic_compare_exchange_weak_explicit(&owner->control, &cur, next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      break;
    }
  }
}

void h8_owner_mark_dead(H8OwnerRecord* owner) {
  uint64_t cur = atomic_load_explicit(&owner->control, memory_order_acquire);
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(cur);
    ctl.state = H8_OWNER_DEAD;
    ctl.publish_closed = 1;
    uint64_t next = h8_ctl_pack(ctl);
    if (atomic_compare_exchange_weak_explicit(&owner->control, &cur, next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      break;
    }
  }
}

bool h8_owner_is_alive_and_open(H8OwnerRecord* owner) {
  H8CtlWord ctl = h8_ctl_unpack(atomic_load_explicit(&owner->control,
                                                    memory_order_acquire));
  return ctl.state == H8_OWNER_ALIVE && !ctl.publish_closed;
}

bool h8_owner_lifecycle_enter(H8OwnerRecord* owner, uint16_t expected_generation) {
  if (owner->permanent) {
    return true;
  }
  uint64_t cur = atomic_load_explicit(&owner->control, memory_order_acquire);
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(cur);
    if (ctl.generation != expected_generation ||
        ctl.state != H8_OWNER_ALIVE ||
        ctl.publish_closed) {
      H8_DEBUG_INC(owner_transition_count);
      return false;
    }
    if (ctl.publish_refs == UINT16_MAX) {
      abort();
    }
    H8CtlWord next = ctl;
    next.publish_refs++;
    uint64_t next_raw = h8_ctl_pack(next);
    if (atomic_compare_exchange_weak_explicit(&owner->control, &cur, next_raw,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      H8_DEBUG_INC(owner_lifecycle_enter_count);
      H8_DEBUG_INC(owner_publish_enter_count);
      return true;
    }
    H8_DEBUG_INC(owner_lifecycle_enter_cas_retry_count);
  }
}

void h8_owner_lifecycle_exit(H8OwnerRecord* owner) {
  if (owner->permanent) {
    return;
  }
  uint64_t cur = atomic_load_explicit(&owner->control, memory_order_acquire);
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(cur);
    if (ctl.publish_refs == 0) {
      abort();
    }
    H8CtlWord next = ctl;
    next.publish_refs--;
    uint64_t next_raw = h8_ctl_pack(next);
    if (atomic_compare_exchange_weak_explicit(&owner->control, &cur, next_raw,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      H8_DEBUG_INC(owner_lifecycle_exit_count);
      H8_DEBUG_INC(owner_publish_exit_count);
      return;
    }
    H8_DEBUG_INC(owner_lifecycle_exit_cas_retry_count);
  }
}

bool h8_owner_publish_enter(H8OwnerRecord* owner, uint16_t expected_generation) {
  return h8_owner_lifecycle_enter(owner, expected_generation);
}

void h8_owner_publish_exit(H8OwnerRecord* owner) {
  h8_owner_lifecycle_exit(owner);
}
