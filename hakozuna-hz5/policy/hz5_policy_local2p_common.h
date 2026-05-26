#ifndef HZ5_POLICY_LOCAL2P_COMMON_H
#define HZ5_POLICY_LOCAL2P_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#include "hz5_wrapper.h"

/*
 * Cross-platform Local2P algorithm vocabulary.
 *
 * Platform backends still own their raw source, lock primitive, and memory
 * return policy. This header only names the shared route/cache objects so
 * Linux and Windows lanes stay comparable without forcing one implementation.
 */
typedef struct Hz5PolicyLocal2PNode {
  struct Hz5PolicyLocal2PNode* next;
} Hz5PolicyLocal2PNode;

static inline int hz5_policy_local2p_common_exact(size_t size,
                                                  size_t align,
                                                  size_t exact_size,
                                                  size_t exact_align) {
  return size == exact_size && align == exact_align;
}

static inline uint32_t hz5_policy_local2p_common_next_generation(
    uint32_t* generation) {
  uint32_t next = ++(*generation);
  if (next == 0u) {
    next = ++(*generation);
  }
  return next;
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
#endif

#endif
