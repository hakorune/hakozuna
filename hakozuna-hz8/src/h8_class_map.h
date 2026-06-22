#ifndef H8_CLASS_MAP_H
#define H8_CLASS_MAP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(H8_CLASS_MAP_UPPER1P5)
#define H8_CLASS_MAP_ID "upper1p5-v0"
#define H8_CLASS_COUNT 11u
#else
#define H8_CLASS_MAP_ID "p2-v0"
#define H8_CLASS_COUNT 9u
#endif
#define H8_MAX_SMALL_SIZE 4096u

static inline uint32_t h8_class_size(uint32_t class_id) {
#if defined(H8_CLASS_MAP_UPPER1P5)
  static const uint32_t sizes[H8_CLASS_COUNT] = {
      16, 32, 64, 128, 256, 512, 1024, 1536, 2048, 3072, 4096};
#else
  static const uint32_t sizes[H8_CLASS_COUNT] = {
      16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
#endif
  return sizes[class_id];
}

static inline bool h8_class_is_power2(uint32_t class_id) {
  uint32_t size = h8_class_size(class_id);
  return (size & (size - 1u)) == 0;
}

static inline uint32_t h8_class_shift(uint32_t class_id) {
#if defined(H8_CLASS_MAP_UPPER1P5)
  uint32_t size = h8_class_size(class_id);
  if (class_id == 7u) return 9u;  /* 1536 = 3 * 512 */
  if (class_id == 9u) return 10u; /* 3072 = 3 * 1024 */
  uint32_t shift = 0;
  while ((UINT32_C(1) << shift) < size) {
    ++shift;
  }
  return shift;
#else
  return 4u + class_id;
#endif
}

static inline uint32_t h8_class_for_size(size_t size) {
  if (size <= 16u) return 0;
  if (size <= 32u) return 1;
  if (size <= 64u) return 2;
  if (size <= 128u) return 3;
  if (size <= 256u) return 4;
  if (size <= 512u) return 5;
  if (size <= 1024u) return 6;
#if defined(H8_CLASS_MAP_UPPER1P5)
  if (size <= 1536u) return 7;
  if (size <= 2048u) return 8;
  if (size <= 3072u) return 9;
  return 10;
#else
  if (size <= 2048u) return 7;
  return 8;
#endif
}

static inline size_t h8_class_slot_count(uint32_t class_id) {
  return 65536u / h8_class_size(class_id);
}

#endif
