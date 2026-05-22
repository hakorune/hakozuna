#ifndef HZ5_WRAPPER_H
#define HZ5_WRAPPER_H

#include <stddef.h>
#include <stdint.h>
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR || BENCHLAB_HZ5_LINUX_LOCAL2P
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define HZ5_WRAPPER_HDR_MAGIC UINT64_C(0x485a355752415036)
#define HZ5_WRAPPER_HDR_COOKIE UINT64_C(0xd1b54a32d192ed03)

enum {
  HZ5_WRAPPER_SOURCE_HZ3 = 1,
  HZ5_WRAPPER_SOURCE_P17_PAGEBIN = 2,
  HZ5_WRAPPER_SOURCE_P20_CRT_BYPASS = 3,
  HZ5_WRAPPER_SOURCE_P24_RAW64K = 4,
  /*
   * Historical name: this tag means the HZ5 P25 lowpage64 route owns the raw
   * block. It is HZ4-inspired, but it is not an HZ4 or HZ3 fallback path.
   */
  HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE = 5,
  HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P = 6
};

#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
enum {
  HZ5_BRIDGE_ATTR_STATE_INVALID = 0,
  HZ5_BRIDGE_ATTR_STATE_ACTIVE = 1,
  HZ5_BRIDGE_ATTR_STATE_FREED = 3
};
#endif

#if BENCHLAB_HZ5_LINUX_LOCAL2P
enum {
  HZ5_LOCAL2P_STATE_INVALID = 0,
  HZ5_LOCAL2P_STATE_ACTIVE = 1,
  HZ5_LOCAL2P_STATE_FREED = 3
};
#endif

/*
 * Wrapper headers are flag-tail-extended.
 * The prefix records the layout version and total size so decode can fail
 * closed when feature flags change the tail.
 */
typedef struct Hz5WrapperHdr {
  uint64_t magic;
  uint32_t layout_version;
  uint32_t layout_size;
  uintptr_t raw;
  uintptr_t cookie;
  size_t requested;
  size_t raw_bytes;
  uint32_t source;
  uint32_t reserved;
#if BENCHLAB_HZ5_P43_WRAPPER_TOKEN
  uintptr_t p43_segment_token;
  uint32_t p43_slot_index;
  uint32_t p43_token_cookie;
#endif
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
  uint64_t bridge_cookie;
  _Atomic uint32_t bridge_state;
  uint32_t bridge_generation;
#endif
#if BENCHLAB_HZ5_LINUX_LOCAL2P
  uint64_t local2p_cookie;
  _Atomic uint32_t local2p_state;
  uint32_t local2p_generation;
  uintptr_t local2p_owner;
#endif
} Hz5WrapperHdr;

enum {
  HZ5_WRAPPER_LAYOUT_VERSION = 1
};

uintptr_t hz5_wrapper_cookie(uintptr_t raw, uintptr_t aligned);
static inline void hz5_wrapper_init_prefix(Hz5WrapperHdr* header,
                                           uintptr_t raw,
                                           uintptr_t aligned,
                                           size_t requested,
                                           size_t raw_bytes,
                                           uint32_t source) {
  header->magic = HZ5_WRAPPER_HDR_MAGIC;
  header->layout_version = HZ5_WRAPPER_LAYOUT_VERSION;
  header->layout_size = (uint32_t)sizeof(Hz5WrapperHdr);
  header->raw = raw;
  header->cookie = hz5_wrapper_cookie(raw, aligned);
  header->requested = requested;
  header->raw_bytes = raw_bytes;
  header->source = source;
  header->reserved = 0;
}
void hz5_wrapper_init(Hz5WrapperHdr* header, uintptr_t raw, uintptr_t aligned,
                      size_t requested, size_t raw_bytes, uint32_t source);
int hz5_wrapper_decode(void* ptr, Hz5WrapperHdr** header_out);

#ifdef __cplusplus
}
#endif

#endif
