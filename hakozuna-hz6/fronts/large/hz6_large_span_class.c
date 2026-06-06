#include "hz6_large_span_class.h"

static const Hz6LargeSpanClass kHz6LargeSpanClasses[] = {
    {HZ6_LARGE128_CLASS_ID, HZ6_LARGE128_BYTES, HZ6_LARGE128_BYTES,
     "large128"},
    {HZ6_LARGE256_CLASS_ID, HZ6_LARGE256_BYTES, HZ6_LARGE256_BYTES,
     "large256"},
    {HZ6_LARGE512_CLASS_ID, HZ6_LARGE512_BYTES, HZ6_LARGE512_BYTES,
     "large512"},
    {HZ6_LARGE1M_CLASS_ID, HZ6_LARGE1M_BYTES, HZ6_LARGE1M_BYTES,
     "large1m"},
};

size_t hz6_large_span_class_count(void) {
  return sizeof(kHz6LargeSpanClasses) / sizeof(kHz6LargeSpanClasses[0]);
}

const Hz6LargeSpanClass* hz6_large_span_class_for_class_id(
    uint16_t class_id) {
  for (size_t i = 0; i < hz6_large_span_class_count(); ++i) {
    if (kHz6LargeSpanClasses[i].class_id == class_id) {
      return &kHz6LargeSpanClasses[i];
    }
  }
  return NULL;
}

const Hz6LargeSpanClass* hz6_large_span_class_for_request(size_t size,
                                                          size_t align) {
  if (align > 16 || size <= HZ6_LARGE_SPAN_MIN_REQUEST_BYTES) {
    return NULL;
  }
  for (size_t i = 0; i < hz6_large_span_class_count(); ++i) {
    if (size <= kHz6LargeSpanClasses[i].max_request_bytes) {
      return &kHz6LargeSpanClasses[i];
    }
  }
  return NULL;
}
