#include "hz5_wrapper.h"

#define HZ5_WRAPPER_HDR_MAGIC UINT64_C(0x485a355752415036)
#define HZ5_WRAPPER_HDR_COOKIE UINT64_C(0xd1b54a32d192ed03)

#if defined(_WIN32)
#include <windows.h>
#endif

uintptr_t hz5_wrapper_cookie(uintptr_t raw, uintptr_t aligned) {
  return raw ^ aligned ^ (uintptr_t)HZ5_WRAPPER_HDR_COOKIE;
}

void hz5_wrapper_init(Hz5WrapperHdr* header, uintptr_t raw, uintptr_t aligned,
                      size_t requested, size_t raw_bytes, uint32_t source) {
  header->magic = HZ5_WRAPPER_HDR_MAGIC;
  header->raw = raw;
  header->cookie = hz5_wrapper_cookie(raw, aligned);
  header->requested = requested;
  header->raw_bytes = raw_bytes;
  header->source = source;
  header->reserved = 0;
}

int hz5_wrapper_decode(void* ptr, Hz5WrapperHdr** header_out) {
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
