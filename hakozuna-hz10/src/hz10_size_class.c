#include "hz10_size_class.h"
#include "hz10_pagemap.h"

static const uint32_t hz10_size_class_table[HZ10_CLASS_COUNT] = {
    16u,   32u,   64u,    128u,   256u,   512u,  1024u,
    2048u, 4096u, 8192u, 16384u, 32768u, 65536u};

uint32_t hz10_size_class_for(size_t size) {
  if (size == 0u || size > (size_t)HZ10_PAGE_QUANTUM) {
    return HZ10_CLASS_COUNT;
  }
  if (size <= 16u) {
    return 0u;
  }
  /* Classes are exact powers of two, so the class index is just "how many
   * bits does size-1 need", offset so 16 (2^4) lands on class 0. */
  unsigned long long rounded = (unsigned long long)size - 1ull;
  unsigned bits = 64u - (unsigned)__builtin_clzll(rounded);
  return bits - 4u;
}

uint32_t hz10_size_class_slot_size(uint32_t class_id) {
  if (class_id >= HZ10_CLASS_COUNT) {
    return 0u;
  }
  return hz10_size_class_table[class_id];
}

uint32_t hz10_size_class_slot_count(uint32_t class_id) {
  uint32_t slot_size = hz10_size_class_slot_size(class_id);
  if (slot_size == 0u) {
    return 0u;
  }
  return HZ10_PAGE_QUANTUM / slot_size;
}
