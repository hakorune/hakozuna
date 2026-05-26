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

static inline uintptr_t hz5_policy_local2p_common_align_user(uintptr_t raw,
                                                             size_t align) {
  uintptr_t header_min = raw + sizeof(Hz5WrapperHdr);
  return (header_min + (uintptr_t)(align - 1u)) &
         ~(uintptr_t)(align - 1u);
}

static inline int hz5_policy_local2p_common_layout_fits(uintptr_t raw,
                                                        uintptr_t aligned,
                                                        size_t size,
                                                        size_t raw_bytes) {
  uintptr_t header_min = raw + sizeof(Hz5WrapperHdr);
  return aligned >= header_min && aligned + size <= raw + raw_bytes;
}

static inline int hz5_policy_local2p_common_header_matches(
    const Hz5WrapperHdr* header,
    uint32_t source,
    size_t requested,
    size_t raw_bytes) {
  return header && header->source == source && header->requested == requested &&
         header->raw_bytes == raw_bytes;
}

static inline Hz5PolicyLocal2PNode* hz5_policy_local2p_common_pop_node(
    void** head,
    size_t* count) {
  Hz5PolicyLocal2PNode* node = (Hz5PolicyLocal2PNode*)*head;
  if (!node) {
    return NULL;
  }
  *head = node->next;
  if (count && *count > 0u) {
    --(*count);
  }
  return node;
}

static inline int hz5_policy_local2p_common_push_node(
    void** head,
    size_t* count,
    size_t cap,
    Hz5PolicyLocal2PNode* node) {
  if (!node || (count && *count >= cap)) {
    return 0;
  }
  node->next = (Hz5PolicyLocal2PNode*)*head;
  *head = node;
  if (count) {
    ++(*count);
  }
  return 1;
}

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

static inline void hz5_policy_local2p_common_remote_batch_push(
    uintptr_t owner,
    Hz5PolicyLocal2PNode* node,
    uintptr_t* batch_owner,
    void** batch_head,
    void** batch_tail,
    size_t* batch_count) {
  node->next = (Hz5PolicyLocal2PNode*)*batch_head;
  *batch_head = node;
  if (!*batch_tail) {
    *batch_tail = node;
  }
  *batch_owner = owner;
  ++(*batch_count);
}

static inline void hz5_policy_local2p_common_remote_batch_reset(
    uintptr_t* batch_owner,
    void** batch_head,
    void** batch_tail,
    size_t* batch_count) {
  *batch_owner = 0;
  *batch_head = NULL;
  *batch_tail = NULL;
  *batch_count = 0;
}

static inline void hz5_policy_local2p_common_inbox_push_list(
    _Atomic(void*)* inbox_head,
    Hz5PolicyLocal2PNode* head,
    Hz5PolicyLocal2PNode* tail) {
  void* old_head = atomic_load_explicit(inbox_head, memory_order_acquire);
  do {
    tail->next = (Hz5PolicyLocal2PNode*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox_head, &old_head, head, memory_order_acq_rel,
      memory_order_acquire));
}

#endif
