#include "hz10_large_alloc.h"
#include "hz10_pagemap.h"
#include "hz10_platform.h"

#include <stdint.h>

#if defined(_WIN32)
/* VirtualAlloc's allocation granularity (64KiB) already matches
 * HZ10_PAGE_QUANTUM and applies regardless of the requested size, so any
 * quantum-multiple reservation comes back naturally aligned -- same
 * reasoning as hz10_freelist_page.c's Windows branch, generalized from
 * exactly one quantum to any number of them. */
static void* hz10_large_reserve_aligned(size_t rounded_size) {
  return hz10_platform_reserve_rw(rounded_size);
}
#else
/* Same over-reserve-then-trim shape as hz10_freelist_page.c's
 * hz10_freelist_reserve_aligned_quantum(), generalized to an arbitrary
 * quantum-multiple size instead of exactly one quantum: plain mmap only
 * guarantees page-size alignment, not HZ10_PAGE_QUANTUM, so this reserves
 * one extra quantum and trims whichever unaligned head/tail slice results. */
static void* hz10_large_reserve_aligned(size_t rounded_size) {
  size_t raw_bytes = rounded_size + HZ10_PAGE_QUANTUM;
  void* raw = hz10_platform_reserve_rw(raw_bytes);
  if (!raw) {
    return NULL;
  }
  uintptr_t raw_addr = (uintptr_t)raw;
  uintptr_t aligned_addr = (raw_addr + (HZ10_PAGE_QUANTUM - 1u)) &
                           ~(uintptr_t)(HZ10_PAGE_QUANTUM - 1u);
  size_t head_trim = (size_t)(aligned_addr - raw_addr);
  size_t tail_trim = raw_bytes - head_trim - rounded_size;
  if (head_trim > 0u) {
    hz10_platform_release(raw, head_trim);
  }
  if (tail_trim > 0u) {
    hz10_platform_release((void*)(aligned_addr + rounded_size), tail_trim);
  }
  return (void*)aligned_addr;
}
#endif

void* hz10_large_alloc(size_t size) {
  if (size == 0u) {
    return NULL;
  }
  if (size > SIZE_MAX - (size_t)HZ10_PAGE_QUANTUM) {
    return NULL; /* would overflow rounding up */
  }
  size_t rounded_size =
      ((size + (size_t)HZ10_PAGE_QUANTUM - 1u) / (size_t)HZ10_PAGE_QUANTUM) *
      (size_t)HZ10_PAGE_QUANTUM;
  if (rounded_size > (size_t)UINT32_MAX) {
    return NULL; /* Box 1's slot_size field is uint32_t */
  }

  void* base = hz10_large_reserve_aligned(rounded_size);
  if (!base) {
    return NULL;
  }

  uint32_t generation = hz10_pagemap_register_with_owner_and_flags(
      base, (uint32_t)rounded_size, 1u, NULL, HZ10_PAGEMAP_FLAG_LARGE);
  if (generation == 0u) {
    hz10_platform_release(base, rounded_size);
    return NULL;
  }
  return base;
}

void hz10_large_free(void* base, uint32_t rounded_size) {
  if (!base) {
    return;
  }
  hz10_pagemap_release(base);
  hz10_platform_release(base, rounded_size);
}
