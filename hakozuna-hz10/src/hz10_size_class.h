#ifndef HZ10_SIZE_CLASS_H
#define HZ10_SIZE_CLASS_H

#include <stddef.h>
#include <stdint.h>

#include "hz10_pagemap.h"

/*
 * Size-class table for the multi-class public entry (src/hz10_public_entry.h).
 * 24 classes, 16B..64KiB (HZ10_PAGE_QUANTUM) -- jemalloc-style spacing
 * (roughly 4 classes per doubling: size and 1.5x size) instead of the
 * original 13 exact-power-of-two classes, to cut worst-case internal
 * fragmentation from up to ~50% (a 17-byte request rounding to 32 bytes)
 * down to ~33% (rounding to the nearest of size or 1.5x size). Still
 * covers both the main_local0-style range (16..32768) and the
 * medium_local0-style range (4097..65536) this project's benches use.
 *
 * The very first doubling (16 < size <= 32) has no 1.5x class: 1.5*16=24
 * is not a multiple of HZ10_MIN_ALIGN (16), and Box 1's route requires
 * every slot offset to land on a 16-byte boundary, so slot_size itself
 * must always be a multiple of 16. Every 1.5x class from 48 upward stays
 * a clean multiple of 16 (1.5 * 2^e is a multiple of 16 once 2^e >= 32).
 *
 * Known, accepted tradeoff: 1.5x-sized classes do not evenly divide
 * HZ10_PAGE_QUANTUM (65536 is a pure power of two, 1.5x classes are not),
 * so each such class's page has a small amount of unused tail space
 * (e.g. class 48's page holds 1365 slots, wasting 16 of 65536 bytes --
 * negligible; the largest 1.5x class, 49152, holds only 1 slot per page
 * and wastes 16384 bytes of that page -- a real cost, but it is still a
 * net win for the *requesting* application, which sees far less
 * request-to-slot rounding than always rounding up to 65536 for anything
 * in (32768, 65536]. Box 1's route already rejects any pointer landing in
 * that tail slack as out-of-range, so this is a space cost, not a
 * correctness one.
 *
 * L0 scope: every class still fits exactly one Box 1 quantum (slot_size *
 * slot_count <= HZ10_PAGE_QUANTUM), matching every prior box's
 * single-quantum-per-page limit. Sizes over HZ10_PAGE_QUANTUM are not
 * supported by this box -- that would need spanning multiple quanta per
 * allocation, which is out of scope here (see current_task.md's
 * large-object-path follow-up).
 */

#define HZ10_CLASS_COUNT 24u

extern const uint32_t hz10_size_class_table[HZ10_CLASS_COUNT];

/* Returns the class index [0, HZ10_CLASS_COUNT) whose slot_size is the
 * smallest one >= size, or HZ10_CLASS_COUNT (invalid) if size is 0 or
 * larger than the biggest class (HZ10_PAGE_QUANTUM). */
static inline uint32_t hz10_size_class_for(size_t size) {
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

  uint64_t low_pow = (uint64_t)1u << e;             /* 2^e */
  uint64_t half_point = low_pow + (low_pow >> 1u);  /* 1.5 * 2^e */
  uint32_t octave_pos = e - 5u;                     /* e=5 -> 0 */
  uint32_t base_index = 2u + octave_pos * 2u;       /* 1.5x class */

  return (size <= (size_t)half_point) ? base_index : base_index + 1u;
}

static inline uint32_t hz10_size_class_slot_size(uint32_t class_id) {
  if (class_id >= HZ10_CLASS_COUNT) {
    return 0u;
  }
  return hz10_size_class_table[class_id];
}

static inline uint32_t hz10_size_class_slot_count(uint32_t class_id) {
  uint32_t slot_size = hz10_size_class_slot_size(class_id);
  if (slot_size == 0u) {
    return 0u;
  }
  return HZ10_PAGE_QUANTUM / slot_size;
}

#endif
