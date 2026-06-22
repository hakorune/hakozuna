#ifndef H8_CLASS_MAP_H
#define H8_CLASS_MAP_H

#include <stddef.h>
#include <stdint.h>

#define H8_CLASS_MAP_ID "p2-v0"
#define H8_CLASS_COUNT 9u
#define H8_MAX_SMALL_SIZE 4096u

static inline uint32_t h8_class_size(uint32_t class_id) {
  static const uint32_t sizes[H8_CLASS_COUNT] = {
      16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
  return sizes[class_id];
}

static inline uint32_t h8_class_shift(uint32_t class_id) {
  return 4u + class_id;
}

static inline uint32_t h8_class_for_size(size_t size) {
  if (size <= 16u) return 0;
  if (size <= 32u) return 1;
  if (size <= 64u) return 2;
  if (size <= 128u) return 3;
  if (size <= 256u) return 4;
  if (size <= 512u) return 5;
  if (size <= 1024u) return 6;
  if (size <= 2048u) return 7;
  return 8;
}

static inline size_t h8_class_slot_count(uint32_t class_id) {
  return 65536u / h8_class_size(class_id);
}

#endif
