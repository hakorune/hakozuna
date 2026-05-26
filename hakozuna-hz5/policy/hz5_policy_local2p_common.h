#ifndef HZ5_POLICY_LOCAL2P_COMMON_H
#define HZ5_POLICY_LOCAL2P_COMMON_H

#include <stddef.h>

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

#endif
