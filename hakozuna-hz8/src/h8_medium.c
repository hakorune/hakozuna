#include "h8_medium.h"

#include <stddef.h>

static const H8MediumClassSpec k_h8_medium_classes[H8_MEDIUM_CLASS_COUNT] = {
    {8192u, H8_MEDIUM_RUN_BYTES, 8u, 1u},
    {16384u, H8_MEDIUM_RUN_BYTES, 4u, 1u},
    {32768u, H8_MEDIUM_RUN_BYTES, 2u, 1u},
    {65536u, H8_MEDIUM_RUN_BYTES, 1u, 1u},
};

_Static_assert(H8_MEDIUM_MIN_SIZE == 4097u,
               "medium range must start immediately after small");
_Static_assert(H8_MEDIUM_MAX_SIZE == 65536u,
               "medium v1 scaffold currently ends at 64KiB");
_Static_assert(H8_MEDIUM_CLASS_COUNT == 4u,
               "medium v1 scaffold expects four coarse classes");

bool h8_medium_size_supported(size_t size) {
  return size >= H8_MEDIUM_MIN_SIZE && size <= H8_MEDIUM_MAX_SIZE;
}

uint32_t h8_medium_class_for_size(size_t size) {
  if (size <= 8192u) {
    return 0u;
  }
  if (size <= 16384u) {
    return 1u;
  }
  if (size <= 32768u) {
    return 2u;
  }
  return 3u;
}

const H8MediumClassSpec* h8_medium_class_spec(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  return &k_h8_medium_classes[class_id];
}

uint32_t h8_medium_rounded_size(size_t size) {
  if (!h8_medium_size_supported(size)) {
    return 0u;
  }
  return k_h8_medium_classes[h8_medium_class_for_size(size)].slot_size;
}
