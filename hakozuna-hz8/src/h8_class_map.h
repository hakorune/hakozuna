#ifndef H8_CLASS_MAP_H
#define H8_CLASS_MAP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(H8_CLASS_MAP_UPPER1P5)
#define H8_CLASS_MAP_ID "upper1p5-v0"
#define H8_CLASS_COUNT 11u
#elif defined(H8_CLASS_MAP_UPPER3072)
#define H8_CLASS_MAP_ID "upper3072-v0"
#define H8_CLASS_COUNT 10u
#else
#define H8_CLASS_MAP_ID "p2-v0"
#define H8_CLASS_COUNT 9u
#endif
#define H8_MAX_SMALL_SIZE 4096u

static inline uint32_t h8_class_size(uint32_t class_id) {
#if defined(H8_CLASS_MAP_UPPER1P5)
  static const uint32_t sizes[H8_CLASS_COUNT] = {
      16, 32, 64, 128, 256, 512, 1024, 1536, 2048, 3072, 4096};
  return sizes[class_id];
#elif defined(H8_CLASS_MAP_UPPER3072)
  static const uint32_t sizes[H8_CLASS_COUNT] = {
      16, 32, 64, 128, 256, 512, 1024, 2048, 3072, 4096};
  return sizes[class_id];
#else
  return 16u << class_id;
#endif
}

static inline uint32_t h8_class_shift(uint32_t class_id) {
#if defined(H8_CLASS_MAP_UPPER1P5)
  static const uint8_t shifts[H8_CLASS_COUNT] = {
      4, 5, 6, 7, 8, 9, 10, 9, 11, 10, 12};
  return shifts[class_id];
#elif defined(H8_CLASS_MAP_UPPER3072)
  static const uint8_t shifts[H8_CLASS_COUNT] = {
      4, 5, 6, 7, 8, 9, 10, 11, 10, 12};
  return shifts[class_id];
#else
  return 4u + class_id;
#endif
}

static inline uint32_t h8_class_factor(uint32_t class_id) {
#if defined(H8_CLASS_MAP_UPPER1P5)
  return (class_id == 7u || class_id == 9u) ? 3u : 1u;
#elif defined(H8_CLASS_MAP_UPPER3072)
  return class_id == 8u ? 3u : 1u;
#else
  (void)class_id;
  return 1u;
#endif
}

static inline uint32_t h8_class_for_size(size_t size) {
#if !defined(H8_CLASS_MAP_UPPER1P5)
  if (size <= 16u) {
    return 0;
  }
#if defined(H8_CLASS_MAP_UPPER3072)
  if (size > 2048u) {
    return size <= 3072u ? 8u : 9u;
  }
#endif
  size_t rounded = size - 1u;
#if defined(__GNUC__) || defined(__clang__)
  uint32_t bits = (uint32_t)(8u * sizeof(unsigned long long) -
                             (uint32_t)__builtin_clzll((unsigned long long)rounded));
#else
  uint32_t bits = 0;
  while (rounded != 0) {
    ++bits;
    rounded >>= 1u;
  }
#endif
  uint32_t class_id = bits > 4u ? bits - 4u : 0u;
  return class_id < H8_CLASS_COUNT ? class_id : H8_CLASS_COUNT - 1u;
#else
  if (size <= 16u) return 0;
  if (size <= 32u) return 1;
  if (size <= 64u) return 2;
  if (size <= 128u) return 3;
  if (size <= 256u) return 4;
  if (size <= 512u) return 5;
  if (size <= 1024u) return 6;
  if (size <= 1536u) return 7;
  if (size <= 2048u) return 8;
  if (size <= 3072u) return 9;
  return 10;
#endif
}

static inline size_t h8_class_slot_count(uint32_t class_id) {
#if defined(H8_CLASS_MAP_UPPER1P5) || defined(H8_CLASS_MAP_UPPER3072)
  return 65536u / h8_class_size(class_id);
#else
  return (size_t)4096u >> class_id;
#endif
}

#endif
