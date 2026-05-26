#include "hz5_wrapper.h"

#if defined(_WIN32)
#include <windows.h>
#endif

uintptr_t hz5_wrapper_cookie(uintptr_t raw, uintptr_t aligned) {
  return raw ^ aligned ^ (uintptr_t)HZ5_WRAPPER_HDR_COOKIE;
}

void hz5_wrapper_init(Hz5WrapperHdr* header, uintptr_t raw, uintptr_t aligned,
                      size_t requested, size_t raw_bytes, uint32_t source) {
  hz5_wrapper_init_prefix(header, raw, aligned, requested, raw_bytes, source);
#if BENCHLAB_HZ5_P43_WRAPPER_TOKEN
  header->p43_segment_token = 0;
  header->p43_slot_index = 0;
  header->p43_token_cookie = 0;
#endif
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
  header->bridge_cookie = 0;
  atomic_store_explicit(&header->bridge_state,
                        HZ5_BRIDGE_ATTR_STATE_INVALID,
                        memory_order_relaxed);
  header->bridge_generation = 0;
#endif
#if HZ5_WRAPPER_LOCAL2P_ENABLED
  header->local2p_cookie = 0;
  atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_INVALID,
                        memory_order_relaxed);
  header->local2p_generation = 0;
  header->local2p_owner = 0;
#endif
}

int hz5_wrapper_decode(void* ptr, Hz5WrapperHdr** header_out) {
  /*
   * Trusted decode primitive.
   * Callers must already have narrowed ptr to an HZ5-owned candidate.
   * This validates only the inline wrapper header and cookie; it is not a
   * foreign-pointer safety boundary.
   */
  if (!ptr) {
    return 0;
  }

  uintptr_t aligned = (uintptr_t)ptr;
  if (aligned < sizeof(Hz5WrapperHdr)) {
    return 0;
  }

  Hz5WrapperHdr* header =
      (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
#if defined(_WIN32)
  __try {
    if (header->magic != HZ5_WRAPPER_HDR_MAGIC) {
      return 0;
    }
    if (header->layout_version != HZ5_WRAPPER_LAYOUT_VERSION ||
        header->layout_size != sizeof(Hz5WrapperHdr)) {
      return 0;
    }

    uintptr_t raw = header->raw;
    if (raw == 0 || header->cookie != hz5_wrapper_cookie(raw, aligned)) {
      return 0;
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
#else
  if (header->magic != HZ5_WRAPPER_HDR_MAGIC) {
    return 0;
  }
  if (header->layout_version != HZ5_WRAPPER_LAYOUT_VERSION ||
      header->layout_size != sizeof(Hz5WrapperHdr)) {
    return 0;
  }

  uintptr_t raw = header->raw;
  if (raw == 0 || header->cookie != hz5_wrapper_cookie(raw, aligned)) {
    return 0;
  }
#endif

  if (header_out) {
    *header_out = header;
  }
  return 1;
}
