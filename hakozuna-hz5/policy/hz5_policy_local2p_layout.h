#ifndef HZ5_POLICY_LOCAL2P_LAYOUT_H
#define HZ5_POLICY_LOCAL2P_LAYOUT_H

#include <stddef.h>
#include <stdint.h>

#include "hz5_wrapper.h"

#define HZ5_POLICY_LOCAL2P_EXACT_SIZE 65536u

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

static inline uint64_t hz5_policy_local2p_common_cookie_full(
    uintptr_t raw,
    uintptr_t aligned,
    size_t raw_bytes,
    uint32_t generation,
    uintptr_t owner,
    uintptr_t anchor,
    uint64_t salt,
    uint64_t fallback) {
  uint64_t mixed = (uint64_t)(raw ^ aligned ^ owner ^ anchor) ^
                   ((uint64_t)raw_bytes << 17) ^
                   ((uint64_t)generation << 32) ^ salt;
  return mixed ? mixed : fallback;
}

static inline uint64_t hz5_policy_local2p_common_cookie_fast(
    uintptr_t raw,
    uintptr_t aligned,
    uintptr_t anchor,
    uint64_t salt,
    uint64_t fallback) {
  uint64_t mixed = (uint64_t)(raw ^ aligned ^ anchor) ^ salt;
  return mixed ? mixed : fallback;
}

#endif
