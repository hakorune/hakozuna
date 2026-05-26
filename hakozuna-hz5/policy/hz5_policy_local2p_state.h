#ifndef HZ5_POLICY_LOCAL2P_STATE_H
#define HZ5_POLICY_LOCAL2P_STATE_H

#include <stdint.h>
#include <stdatomic.h>

#include "hz5_wrapper.h"

static inline uint32_t hz5_policy_local2p_common_next_generation(
    uint32_t* generation) {
  uint32_t next = ++(*generation);
  if (next == 0u) {
    next = ++(*generation);
  }
  return next;
}

static inline uintptr_t hz5_policy_local2p_common_owner_token(
    uintptr_t* token,
    uintptr_t seed) {
  if (!*token) {
    *token = seed;
  }
  return *token;
}

#if HZ5_WRAPPER_LOCAL2P_ENABLED
static inline void hz5_policy_local2p_common_activate(Hz5WrapperHdr* header,
                                                      uintptr_t owner,
                                                      uint32_t generation,
                                                      uint64_t cookie) {
  header->local2p_owner = owner;
  header->local2p_generation = generation;
  header->local2p_cookie = cookie;
  atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_ACTIVE,
                        memory_order_release);
}

static inline int hz5_policy_local2p_common_mark_freed(Hz5WrapperHdr* header) {
  uint32_t old_state = atomic_exchange_explicit(
      &header->local2p_state, HZ5_LOCAL2P_STATE_FREED,
      memory_order_acq_rel);
  return old_state == HZ5_LOCAL2P_STATE_ACTIVE;
}

static inline int hz5_policy_local2p_common_metadata_present(
    const Hz5WrapperHdr* header) {
  if (!header) {
    return 0;
  }
  return header->local2p_cookie != 0 ||
         header->local2p_generation != 0 ||
         header->local2p_owner != 0 ||
         atomic_load_explicit(&header->local2p_state,
                              memory_order_acquire) !=
             HZ5_LOCAL2P_STATE_INVALID;
}
#endif

#endif
