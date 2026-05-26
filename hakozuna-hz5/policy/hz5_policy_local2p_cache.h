#ifndef HZ5_POLICY_LOCAL2P_CACHE_H
#define HZ5_POLICY_LOCAL2P_CACHE_H

#include <stddef.h>

typedef struct Hz5PolicyLocal2PNode {
  struct Hz5PolicyLocal2PNode* next;
} Hz5PolicyLocal2PNode;

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

#endif
