#ifndef HZ5_POLICY_LOCAL2P_REMOTE_H
#define HZ5_POLICY_LOCAL2P_REMOTE_H

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#include "hz5_policy_local2p_cache.h"

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
