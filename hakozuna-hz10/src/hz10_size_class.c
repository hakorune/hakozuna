#include "hz10_size_class.h"
#include "hz10_pagemap.h"

/*
 * 16, 32, then (1.5x, 2x) pairs for every doubling from 32 up to 65536:
 * 48/64, 96/128, 192/256, 384/512, 768/1024, 1536/2048, 3072/4096,
 * 6144/8192, 12288/16384, 24576/32768, 49152/65536 -- see hz10_size_class.h
 * for why the very first doubling (16..32) has no 1.5x member.
 */
static const uint32_t hz10_size_class_table[HZ10_CLASS_COUNT] = {
    16u,    32u,    48u,    64u,    96u,     128u,    192u,    256u,
    384u,   512u,   768u,   1024u,  1536u,   2048u,   3072u,   4096u,
    6144u,  8192u,  12288u, 16384u, 24576u,  32768u,  49152u,  65536u};

uint32_t hz10_size_class_for(size_t size) {
  if (size == 0u || size > (size_t)HZ10_PAGE_QUANTUM) {
    return HZ10_CLASS_COUNT;
  }
  if (size <= 16u) {
    return 0u;
  }
  if (size <= 32u) {
    return 1u;
  }
  /* size is in (32, 65536]. Find e such that 2^e < size <= 2^(e+1): the
   * same "how many bits does size-1 need" trick as the old table, just
   * one octave lower since class 1 (32) already absorbed e=4. */
  unsigned long long rounded = (unsigned long long)size - 1ull;
  unsigned bits = 64u - (unsigned)__builtin_clzll(rounded);
  unsigned e = bits - 1u; /* 5..15 */

  uint64_t low_pow = (uint64_t)1u << e;         /* 2^e */
  uint64_t half_point = low_pow + (low_pow >> 1u); /* 1.5 * 2^e */
  uint32_t octave_pos = e - 5u;                  /* 0-based, e=5 -> 0 */
  uint32_t base_index = 2u + octave_pos * 2u;    /* index of the 1.5x class */

  return (size <= (size_t)half_point) ? base_index : base_index + 1u;
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
