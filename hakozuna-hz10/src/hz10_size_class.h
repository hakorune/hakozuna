#ifndef HZ10_SIZE_CLASS_H
#define HZ10_SIZE_CLASS_H

#include <stddef.h>
#include <stdint.h>

#include "hz10_pagemap.h"

/*
 * Size-class table for the multi-class public entry (src/hz10_public_entry.h).
 * Default: 24 classes, 16B..64KiB (HZ10_PAGE_QUANTUM), using the
 * established 1.5x/2x spacing.
 *
 * HZ10ClassGranularity-L1 opt-in (`HZ10_ENABLE_FINE_SIZE_CLASSES=1`):
 * 38 classes. The 64..8192 band uses quarter-step spacing
 * (2^k * {1, 1.25, 1.5, 1.75}) to reduce macro-row internal
 * fragmentation found by the python_alloc RSS attribution pass. The larger
 * band keeps the default 1.5x/2x spacing: quarter classes above 8192 would
 * burn too much single-quantum tail slack (e.g. 40960 is one slot plus
 * 37.5% unused page tail) until a multi-quantum page design exists.
 *
 * The very first doubling (16 < size <= 32) has no 1.5x class: 1.5*16=24
 * is not a multiple of HZ10_MIN_ALIGN (16), and Box 1's route requires
 * every slot offset to land on a 16-byte boundary, so slot_size itself
 * must always be a multiple of 16. Every 1.5x class from 48 upward stays
 * a clean multiple of 16 (1.5 * 2^e is a multiple of 16 once 2^e >= 32).
 *
 * Known, accepted tradeoff: non-power-of-two classes do not evenly divide
 * HZ10_PAGE_QUANTUM (65536 is a pure power of two),
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

#ifndef HZ10_ENABLE_FINE_SIZE_CLASSES
#define HZ10_ENABLE_FINE_SIZE_CLASSES 0
#endif

#if HZ10_ENABLE_FINE_SIZE_CLASSES
#define HZ10_CLASS_COUNT 38u
#else
#define HZ10_CLASS_COUNT 24u
#endif

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
#if HZ10_ENABLE_FINE_SIZE_CLASSES
  if (size <= 48u) {
    return 2u;
  }
  if (size <= 64u) {
    return 3u;
  }
  /* size is in (64, 8192]. Find e such that 2^e < size <= 2^(e+1), then
   * pick the quarter-step boundary in that octave. The exact powers of two
   * fall out as the previous octave's +4 result, e.g. size==128 maps from
   * the 64-octave to class 7. */
  if (size <= 8192u) {
    unsigned long long rounded = (unsigned long long)size - 1ull;
    unsigned bits = 64u - (unsigned)__builtin_clzll(rounded);
    unsigned e = bits - 1u; /* 6..12 */
    uint64_t low_pow = (uint64_t)1u << e;
    unsigned quarter_shift = e - 2u;
    uint64_t quarter = low_pow >> 2u;
    uint32_t base_index = 3u + (e - 6u) * 4u;
    uint64_t delta = (uint64_t)size - low_pow;
    uint32_t step =
        (uint32_t)((delta + quarter - 1u) >> quarter_shift);
    return base_index + step;
  }

  /* size is in (8192, 65536]. Keep the old 1.5x/2x spacing for the large
   * single-quantum band to avoid wasting full pages on quarter classes. */
  unsigned long long rounded = (unsigned long long)size - 1ull;
  unsigned bits = 64u - (unsigned)__builtin_clzll(rounded);
  unsigned e = bits - 1u; /* 13..15 */

  uint64_t low_pow = (uint64_t)1u << e;             /* 2^e */
  uint64_t half_point = low_pow + (low_pow >> 1u);  /* 1.5 * 2^e */
  uint32_t octave_pos = e - 13u;                    /* e=13 -> 0 */
  uint32_t base_index = 32u + octave_pos * 2u;      /* 1.5x class */

  return (size <= (size_t)half_point) ? base_index : base_index + 1u;
#else
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
#endif
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
