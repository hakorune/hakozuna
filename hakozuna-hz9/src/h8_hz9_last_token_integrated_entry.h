#ifndef H8_HZ9_LAST_TOKEN_INTEGRATED_ENTRY_H
#define H8_HZ9_LAST_TOKEN_INTEGRATED_ENTRY_H

#include "h8_hz9_local_slab_inline_body.h"

#include <stdbool.h>
#include <stdint.h>

#if defined(H9_LOCAL_SLAB_PAGE_ROUTE_BOUNDARY_L0)

#if defined(__GNUC__) || defined(__clang__)
#define H9_LSP_COLD_NOINLINE __attribute__((noinline, cold))
#else
#define H9_LSP_COLD_NOINLINE
#endif

typedef struct H9LspIntegratedEntry {
  H9LspInlinePage page;
  void* last_ptr;
  uint32_t last_slot;
  uint32_t last_generation;
  uint32_t class_id;
  uint8_t last_live;
  uint64_t fast_hits;
  uint64_t fallback_hits;
  uint64_t state_mismatch;
} H9LspIntegratedEntry;

static inline void h9_lsp_integrated_init(H9LspIntegratedEntry* entry,
                                          uintptr_t payload_base,
                                          uint32_t slot_size,
                                          uint32_t slot_count,
                                          uint32_t class_id) {
  h9_lsp_inline_page_init(&entry->page, payload_base, slot_size, slot_count);
  entry->page.class_id = class_id;
  entry->last_ptr = 0;
  entry->last_slot = 0u;
  entry->last_generation = 0u;
  entry->class_id = class_id;
  entry->last_live = 0u;
  entry->fast_hits = 0u;
  entry->fallback_hits = 0u;
  entry->state_mismatch = 0u;
}

static inline bool h9_lsp_integrated_alloc(H9LspIntegratedEntry* entry,
                                           void** ptr_out) {
  uint32_t slot = UINT32_MAX;
  void* ptr = 0;
  if (!h9_lsp_inline_alloc_slot(&entry->page, &slot, &ptr)) {
    return false;
  }
  entry->last_ptr = ptr;
  entry->last_slot = slot;
  entry->last_generation = entry->page.generation;
  entry->last_live = 1u;
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return true;
}

static H9_LSP_COLD_NOINLINE bool h9_lsp_integrated_free_fallback(
    H9LspIntegratedEntry* entry, void* ptr) {
  (void)ptr;
  entry->fallback_hits++;
  return false;
}

static inline bool h9_lsp_integrated_free(H9LspIntegratedEntry* entry,
                                          void* ptr) {
  if (entry->last_live && entry->last_ptr == ptr &&
      entry->last_generation == entry->page.generation &&
      entry->last_slot < entry->page.slot_count) {
    uint32_t slot = entry->last_slot;
    if (h9_lsp_inline_free_slot(&entry->page, slot)) {
      entry->last_live = 0u;
      entry->fast_hits++;
      return true;
    }
    entry->last_live = 0u;
    entry->state_mismatch++;
  }
  return h9_lsp_integrated_free_fallback(entry, ptr);
}

#undef H9_LSP_COLD_NOINLINE

#endif

#endif
