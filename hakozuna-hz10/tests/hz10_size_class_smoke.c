#include "../src/hz10_pagemap.h"
#include "../src/hz10_size_class.h"

#include <stdio.h>

/* Case 1: the table itself is exactly what hz10_size_class.h documents,
 * strictly increasing, and every class fits in one quantum. */
static int check_table_shape(void) {
  static const uint32_t expected[HZ10_CLASS_COUNT] = {
#if HZ10_ENABLE_FINE_SIZE_CLASSES
      16u,    32u,    48u,    64u,    80u,    96u,    112u,   128u,
      160u,   192u,   224u,   256u,   320u,   384u,   448u,   512u,
      640u,   768u,   896u,   1024u,  1280u,  1536u,  1792u,  2048u,
      2560u,  3072u,  3584u,  4096u,  5120u,  6144u,  7168u,  8192u,
      12288u, 16384u, 24576u, 32768u, 49152u, 65536u
#else
      16u,   32u,   48u,    64u,    96u,    128u,   192u,   256u,
      384u,  512u,  768u,   1024u,  1536u,  2048u,  3072u,  4096u,
      6144u, 8192u, 12288u, 16384u, 24576u, 32768u, 49152u, 65536u
#endif
  };
  uint32_t prev = 0u;
  for (uint32_t c = 0u; c < HZ10_CLASS_COUNT; ++c) {
    uint32_t slot_size = hz10_size_class_slot_size(c);
    if (slot_size != expected[c]) {
      fprintf(stderr, "table_shape: class %u slot_size=%u expected %u\n", c,
              slot_size, expected[c]);
      return 1;
    }
    if (slot_size <= prev) {
      fprintf(stderr, "table_shape: class %u not strictly increasing\n", c);
      return 1;
    }
    prev = slot_size;
    uint32_t slot_count = hz10_size_class_slot_count(c);
    if ((uint64_t)slot_size * (uint64_t)slot_count >
        (uint64_t)HZ10_PAGE_QUANTUM) {
      fprintf(stderr, "table_shape: class %u overflows one quantum\n", c);
      return 1;
    }
    if ((slot_size % 16u) != 0u) {
      fprintf(stderr,
              "table_shape: class %u slot_size=%u not a multiple of "
              "HZ10_MIN_ALIGN\n",
              c, slot_size);
      return 1;
    }
  }
  if (hz10_size_class_slot_size(HZ10_CLASS_COUNT) != 0u ||
      hz10_size_class_slot_count(HZ10_CLASS_COUNT) != 0u) {
    fprintf(stderr, "table_shape: out-of-range class_id should return 0\n");
    return 1;
  }
  return 0;
}

/* Case 2: exhaustive classification check. For every byte size from 1 to
 * HZ10_PAGE_QUANTUM, hz10_size_class_for() must return the class with the
 * SMALLEST slot_size that is still >= size -- verified against the table
 * directly (a linear scan here is fine, this is the test oracle, not the
 * O(1) implementation under test), not sampled. This is exactly the kind
 * of bit-twiddling code that hides off-by-one bugs at octave boundaries. */
static int check_exhaustive_classification(void) {
  for (size_t size = 1; size <= (size_t)HZ10_PAGE_QUANTUM; ++size) {
    uint32_t expected = HZ10_CLASS_COUNT;
    for (uint32_t c = 0u; c < HZ10_CLASS_COUNT; ++c) {
      if (hz10_size_class_slot_size(c) >= size) {
        expected = c;
        break;
      }
    }
    uint32_t got = hz10_size_class_for(size);
    if (got != expected) {
      fprintf(stderr,
              "exhaustive: size=%zu got class %u (slot_size=%u) expected "
              "%u (slot_size=%u)\n",
              size, got, hz10_size_class_slot_size(got), expected,
              hz10_size_class_slot_size(expected));
      return 1;
    }
  }
  return 0;
}

/* Case 3: rejected inputs -- 0 and anything over one quantum. */
static int check_rejected_inputs(void) {
  if (hz10_size_class_for(0) != HZ10_CLASS_COUNT) {
    fprintf(stderr, "rejected: size 0 should be invalid\n");
    return 1;
  }
  if (hz10_size_class_for((size_t)HZ10_PAGE_QUANTUM + 1u) !=
      HZ10_CLASS_COUNT) {
    fprintf(stderr, "rejected: size quantum+1 should be invalid\n");
    return 1;
  }
  if (hz10_size_class_for((size_t)HZ10_PAGE_QUANTUM) == HZ10_CLASS_COUNT) {
    fprintf(stderr, "rejected: size == quantum should be the last class\n");
    return 1;
  }
  return 0;
}

int main(void) {
  if (check_table_shape()) {
    return 2;
  }
  if (check_exhaustive_classification()) {
    return 3;
  }
  if (check_rejected_inputs()) {
    return 4;
  }
  puts("hz10_size_class_smoke ok");
  return 0;
}
