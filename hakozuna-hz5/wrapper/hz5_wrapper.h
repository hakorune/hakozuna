#ifndef HZ5_WRAPPER_H
#define HZ5_WRAPPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  HZ5_WRAPPER_SOURCE_HZ3 = 1,
  HZ5_WRAPPER_SOURCE_P17_PAGEBIN = 2,
  HZ5_WRAPPER_SOURCE_P20_CRT_BYPASS = 3,
  HZ5_WRAPPER_SOURCE_P24_RAW64K = 4,
  HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE = 5
};

typedef struct Hz5WrapperHdr {
  uint64_t magic;
  uintptr_t raw;
  uintptr_t cookie;
  size_t requested;
  size_t raw_bytes;
  uint32_t source;
  uint32_t reserved;
} Hz5WrapperHdr;

uintptr_t hz5_wrapper_cookie(uintptr_t raw, uintptr_t aligned);
void hz5_wrapper_init(Hz5WrapperHdr* header, uintptr_t raw, uintptr_t aligned,
                      size_t requested, size_t raw_bytes, uint32_t source);
int hz5_wrapper_decode(void* ptr, Hz5WrapperHdr** header_out);

#ifdef __cplusplus
}
#endif

#endif
