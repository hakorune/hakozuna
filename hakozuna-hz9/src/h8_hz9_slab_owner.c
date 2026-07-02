#include "h8_internal.h"

#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)

H9SlabOwnerState* h9_slab_owner_state(H8OwnerRecord* owner, bool create) {
  if (!owner) {
    return NULL;
  }
#if defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
  (void)create;
  return NULL;
#elif defined(H9_SLAB_OWNER_SIDECAR_L1)
  if (!owner->h9_slab_owner_state && create) {
    owner->h9_slab_owner_state =
        h8_sys_calloc(1, sizeof(*owner->h9_slab_owner_state));
  }
  return owner->h9_slab_owner_state;
#else
  (void)create;
  return &owner->h9_slab_state;
#endif
}

bool h9_slab_owner_prepare(H8OwnerRecord* owner) {
  return h9_slab_owner_state(owner, true) != NULL;
}

void h9_slab_owner_reset(H8OwnerRecord* owner) {
  H9SlabOwnerState* state = h9_slab_owner_state(owner, false);
#if defined(H9_SLAB_OWNER_SIDECAR_L1) && \
    !defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
  owner->h9_slab_owner_state = state;
#endif
  if (!state) {
    return;
  }
  atomic_store_explicit(&state->pending_head, NULL, memory_order_relaxed);
  state->pending_carry = NULL;
  atomic_store_explicit(&state->pending_count, 0, memory_order_relaxed);
#if defined(H9_SLAB_REMOTE_ADAPTIVE_L1)
  atomic_store_explicit(&state->remote_hot_mask, 0, memory_order_relaxed);
  for (uint32_t i = 0; i < H8_MEDIUM_CLASS_COUNT; ++i) {
    atomic_store_explicit(&state->remote_hot[i], 0, memory_order_relaxed);
  }
#endif
}

void h9_slab_owner_destroy(H8OwnerRecord* owner) {
#if defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
  (void)owner;
#elif defined(H9_SLAB_OWNER_SIDECAR_L1)
  if (owner) {
    h8_sys_free(owner->h9_slab_owner_state);
    owner->h9_slab_owner_state = NULL;
  }
#else
  h9_slab_owner_reset(owner);
#endif
}

size_t h9_slab_owner_pending_count(H8OwnerRecord* owner) {
  H9SlabOwnerState* state = h9_slab_owner_state(owner, false);
  return state ? atomic_load_explicit(&state->pending_count,
                                      memory_order_acquire)
               : 0;
}

#if defined(H9_SLAB_REMOTE_ADAPTIVE_L1)
void h9_slab_owner_mark_remote_hot(H8OwnerRecord* owner, uint16_t class_id) {
  if (!owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H9SlabOwnerState* state = h9_slab_owner_state(owner, true);
  if (!state) {
    return;
  }
  atomic_fetch_or_explicit(&state->remote_hot_mask, (uint8_t)(1u << class_id),
                           memory_order_release);
  uint8_t old = atomic_exchange_explicit(&state->remote_hot[class_id], 1,
                                         memory_order_release);
  H8_DEBUG_INC(h9_slab_adaptive_remote_mark);
  if (old == 0) {
    H8_DEBUG_INC(h9_slab_adaptive_remote_mark_first);
  }
}

bool h9_slab_owner_remote_hot(H8OwnerRecord* owner, uint16_t class_id,
                              bool use_mask) {
  if (!owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SlabOwnerState* state = h9_slab_owner_state(owner, false);
  if (!state) {
    return false;
  }
  if (use_mask) {
    uint8_t mask =
        atomic_load_explicit(&state->remote_hot_mask, memory_order_acquire);
    return (mask & (uint8_t)(1u << class_id)) != 0;
  }
  return atomic_load_explicit(&state->remote_hot[class_id],
                              memory_order_acquire) != 0;
}

uint8_t h9_slab_owner_remote_hot_mask(H8OwnerRecord* owner) {
  H9SlabOwnerState* state = h9_slab_owner_state(owner, false);
  if (!state) {
    return 0;
  }
  return atomic_load_explicit(&state->remote_hot_mask, memory_order_acquire);
}
#endif

#else
typedef int h9_slab_owner_translation_unit_silence;
#endif
