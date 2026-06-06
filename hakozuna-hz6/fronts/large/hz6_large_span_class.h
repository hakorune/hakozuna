#ifndef HZ6_LARGE_SPAN_CLASS_H
#define HZ6_LARGE_SPAN_CLASS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HZ6_LARGE_SPAN_MIN_REQUEST_BYTES ((size_t)4096)
#define HZ6_LARGE128_CLASS_ID ((uint16_t)8)
#define HZ6_LARGE128_BYTES ((size_t)131072)
#define HZ6_LARGE256_CLASS_ID ((uint16_t)9)
#define HZ6_LARGE256_BYTES ((size_t)262144)
#define HZ6_LARGE512_CLASS_ID ((uint16_t)10)
#define HZ6_LARGE512_BYTES ((size_t)524288)
#define HZ6_LARGE1M_CLASS_ID ((uint16_t)11)
#define HZ6_LARGE1M_BYTES ((size_t)1048576)
#define HZ6_LARGE_DIRECT_CLASS_ID ((uint16_t)12)
#define HZ6_LARGE_DIRECT_MIN_BYTES HZ6_LARGE1M_BYTES
#define HZ6_LARGE_DIRECT_MAX_BYTES ((size_t)8388608)

typedef struct Hz6LargeSpanClass {
  uint16_t class_id;
  size_t max_request_bytes;
  size_t span_bytes;
  const char* name;
} Hz6LargeSpanClass;

const Hz6LargeSpanClass* hz6_large_span_class_for_request(size_t size,
                                                          size_t align);

const Hz6LargeSpanClass* hz6_large_span_class_for_class_id(
    uint16_t class_id);

size_t hz6_large_span_class_count(void);

int hz6_large_direct_can_allocate(size_t size, size_t align);

int hz6_large_direct_class_id(uint16_t class_id);

#ifdef __cplusplus
}
#endif

#endif
