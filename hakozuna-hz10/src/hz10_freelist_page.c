#include "hz10_freelist_page.h"
#include "hz10_pagemap.h"
#include "hz10_platform.h"

#include <stdlib.h>

/* Reserves an HZ10_PAGE_QUANTUM-aligned, HZ10_PAGE_QUANTUM-sized RW mapping.
 * Plain mmap only guarantees page-size alignment, not our 64KiB quantum, so
 * this over-reserves (2x) and trims the unused head/tail -- the same
 * over-allocate-then-trim shape HZ9's segment allocator uses. */
static void* hz10_freelist_reserve_aligned_quantum(void) {
  size_t bytes = HZ10_PAGE_QUANTUM;
#if defined(_WIN32)
  /* VirtualAlloc returns allocation-granularity aligned regions. With the
   * current 64KiB quantum that is already sufficient, and Windows cannot
   * MEM_RELEASE arbitrary head/tail slices from a larger reservation. */
  return hz10_platform_reserve_rw(bytes);
#else
  size_t raw_bytes = bytes * 2u;
  void* raw = hz10_platform_reserve_rw(raw_bytes);
  if (!raw) {
    return NULL;
  }
  uintptr_t raw_addr = (uintptr_t)raw;
  uintptr_t aligned_addr = (raw_addr + (bytes - 1u)) & ~(uintptr_t)(bytes - 1u);
  size_t head_trim = (size_t)(aligned_addr - raw_addr);
  size_t tail_trim = raw_bytes - head_trim - bytes;
  if (head_trim > 0u) {
    hz10_platform_release(raw, head_trim);
  }
  if (tail_trim > 0u) {
    hz10_platform_release((void*)(aligned_addr + bytes), tail_trim);
  }
  return (void*)aligned_addr;
#endif
}

/* Lays down the intrusive chain: slot i's first sizeof(void*) bytes hold a
 * pointer to slot i-1, so the head-of-chain (last one linked, slot 0) is
 * popped first. */
static void hz10_freelist_page_init_chain(Hz10FreelistPage* page) {
  void* head = NULL;
  for (uint32_t i = page->slot_count; i > 0u; --i) {
    void* slot = (char*)page->base + (size_t)(i - 1u) * (size_t)page->slot_size;
    *(void**)slot = head;
    head = slot;
  }
  page->local_free_head = head;
  page->free_count = page->slot_count;
}

Hz10FreelistPage* hz10_freelist_page_create_with_base(void* base,
                                                      uint32_t slot_size,
                                                      uint32_t slot_count) {
  if (slot_size < sizeof(void*) || slot_count == 0u) {
    return NULL;
  }
  uint64_t span = (uint64_t)slot_size * (uint64_t)slot_count;
  if (span > (uint64_t)HZ10_PAGE_QUANTUM) {
    return NULL;
  }

  int owns_base = 0;
  if (!base) {
    base = hz10_freelist_reserve_aligned_quantum();
    if (!base) {
      return NULL;
    }
    owns_base = 1;
  }

  uint32_t generation = hz10_pagemap_register(base, slot_size, slot_count);
  if (generation == 0u) {
    if (owns_base) {
      hz10_platform_release(base, HZ10_PAGE_QUANTUM);
    }
    return NULL;
  }

  Hz10FreelistPage* page = calloc(1, sizeof(*page));
  if (!page) {
    hz10_pagemap_release(base);
    if (owns_base) {
      hz10_platform_release(base, HZ10_PAGE_QUANTUM);
    }
    return NULL;
  }

  uint32_t pending_words = (slot_count + 63u) / 64u;
  _Atomic(uint64_t)* pending_bits = calloc(pending_words, sizeof(*pending_bits));
  if (!pending_bits) {
    hz10_pagemap_release(base);
    if (owns_base) {
      hz10_platform_release(base, HZ10_PAGE_QUANTUM);
    }
    free(page);
    return NULL;
  }

  page->base = base;
  page->slot_size = slot_size;
  page->slot_count = slot_count;
  page->generation = generation;
  page->pending_bits = pending_bits;
  page->pending_words = pending_words;
  hz10_freelist_page_init_chain(page);
  return page;
}

Hz10FreelistPage* hz10_freelist_page_create(uint32_t slot_size,
                                            uint32_t slot_count) {
  return hz10_freelist_page_create_with_base(NULL, slot_size, slot_count);
}

void* hz10_freelist_page_destroy_reclaim_base(Hz10FreelistPage* page) {
  if (!page) {
    return NULL;
  }
  /* Deliberately does not drain the remote stack (src/hz10_remote_stack.h)
   * itself -- Box 2 has no dependency on Box 3's module, only the reverse
   * (hz10_remote_stack.c includes this header for the struct). A caller
   * that has exposed this page to hz10_page_remote_free() must call
   * hz10_page_drain_remote(page) itself before destroy, or any
   * already-pushed-but-undrained remote free is silently lost. See
   * tests/README.md's HZ10RemoteStackDrain-L0 notes. */
  hz10_pagemap_release(page->base);
  void* base = page->base;
  free(page->pending_bits);
  free(page);
  return base;
}

void hz10_freelist_page_destroy(Hz10FreelistPage* page) {
  void* base = hz10_freelist_page_destroy_reclaim_base(page);
  if (base) {
    hz10_platform_release(base, HZ10_PAGE_QUANTUM);
  }
}
