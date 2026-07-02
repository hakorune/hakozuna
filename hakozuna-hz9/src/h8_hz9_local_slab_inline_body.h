#ifndef H8_HZ9_LOCAL_SLAB_INLINE_BODY_H
#define H8_HZ9_LOCAL_SLAB_INLINE_BODY_H

#include <stdbool.h>
#include <stdint.h>

#if defined(H9_LOCAL_SLAB_PAGE_ROUTE_BOUNDARY_L0)

typedef struct H9LspInlinePage {
  uintptr_t base;
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t generation;
  uint32_t class_id;
  uint64_t free_bits;
  uint64_t alloc_bits;
} H9LspInlinePage;

static inline void h9_lsp_inline_page_init(H9LspInlinePage* page,
                                           uintptr_t base,
                                           uint32_t slot_size,
                                           uint32_t slot_count) {
  page->base = base;
  page->slot_size = slot_size;
  page->slot_count = slot_count;
  page->generation = 1u;
  page->class_id = UINT32_MAX;
  page->free_bits = slot_count == 64u ? UINT64_MAX
                                      : ((UINT64_C(1) << slot_count) - 1u);
  page->alloc_bits = 0u;
}

static inline bool h9_lsp_inline_alloc_slot(H9LspInlinePage* page,
                                            uint32_t* slot_out,
                                            void** ptr_out) {
  if (!page || page->free_bits == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(page->free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  page->free_bits &= ~bit;
  page->alloc_bits |= bit;
  if (slot_out) {
    *slot_out = slot;
  }
  if (ptr_out) {
    *ptr_out = (void*)(page->base +
                       (uintptr_t)slot * (uintptr_t)page->slot_size);
  }
  return true;
}

static inline bool h9_lsp_inline_free_slot(H9LspInlinePage* page,
                                           uint32_t slot) {
  if (!page || slot >= page->slot_count) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u) {
    return false;
  }
  page->alloc_bits &= ~bit;
  page->free_bits |= bit;
  return true;
}

#endif

#endif
