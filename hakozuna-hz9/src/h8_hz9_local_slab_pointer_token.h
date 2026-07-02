#ifndef H8_HZ9_LOCAL_SLAB_POINTER_TOKEN_H
#define H8_HZ9_LOCAL_SLAB_POINTER_TOKEN_H

#include "h8_hz9_local_slab_inline_body.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(H9_LOCAL_SLAB_PAGE_ROUTE_BOUNDARY_L0)

#define H9_LSP_PTR_LEDGER_SIZE 64u

typedef struct H9LspPtrToken {
  void* ptr;
  H9LspInlinePage* page;
  uint32_t slot;
  uint32_t generation;
  uint32_t class_id;
  uint32_t slot_size;
  uint8_t live;
} H9LspPtrToken;

typedef struct H9LspPtrLedger {
  H9LspPtrToken entries[H9_LSP_PTR_LEDGER_SIZE];
} H9LspPtrLedger;

typedef struct H9LspPtrTokenCache {
  H9LspPtrToken last;
  H9LspPtrLedger ledger;
} H9LspPtrTokenCache;

static inline uint32_t h9_lsp_ptr_ledger_index(const void* ptr) {
  return (uint32_t)(((uintptr_t)ptr >> 6u) &
                    (uintptr_t)(H9_LSP_PTR_LEDGER_SIZE - 1u));
}

static inline void h9_lsp_ptr_ledger_clear_entry(H9LspPtrToken* token) {
  if (!token) {
    return;
  }
  token->ptr = NULL;
  token->page = NULL;
  token->slot = 0u;
  token->generation = 0u;
  token->class_id = 0u;
  token->slot_size = 0u;
  token->live = 0u;
}

static inline void h9_lsp_ptr_ledger_insert(H9LspPtrLedger* ledger, void* ptr,
                                            H9LspInlinePage* page,
                                            uint32_t slot,
                                            uint32_t class_id) {
  if (!ledger || !ptr || !page) {
    return;
  }
  H9LspPtrToken* token = &ledger->entries[h9_lsp_ptr_ledger_index(ptr)];
  token->ptr = ptr;
  token->page = page;
  token->slot = slot;
  token->generation = page->generation;
  token->class_id = class_id;
  token->slot_size = page->slot_size;
  token->live = 1u;
}

static inline void h9_lsp_ptr_token_fill(H9LspPtrToken* token, void* ptr,
                                         H9LspInlinePage* page, uint32_t slot,
                                         uint32_t class_id) {
  if (!token || !ptr || !page) {
    return;
  }
  *token = (H9LspPtrToken){
      .ptr = ptr,
      .page = page,
      .slot = slot,
      .generation = page->generation,
      .class_id = class_id,
      .slot_size = page->slot_size,
      .live = 1u,
  };
}

static inline H9LspPtrToken* h9_lsp_ptr_ledger_lookup(
    H9LspPtrLedger* ledger, const void* ptr) {
  if (!ledger || !ptr) {
    return NULL;
  }
  H9LspPtrToken* token = &ledger->entries[h9_lsp_ptr_ledger_index(ptr)];
  return token->live && token->ptr == ptr ? token : NULL;
}

static inline bool h9_lsp_ptr_ledger_free_hit(H9LspPtrLedger* ledger,
                                              void* ptr) {
  H9LspPtrToken* token = h9_lsp_ptr_ledger_lookup(ledger, ptr);
  if (!token || !token->page || token->page->generation != token->generation ||
      token->slot >= token->page->slot_count) {
    return false;
  }
  H9LspInlinePage* page = token->page;
  uint32_t slot = token->slot;
  bool ok = h9_lsp_inline_free_slot(page, slot);
  h9_lsp_ptr_ledger_clear_entry(token);
  return ok;
}

static inline void h9_lsp_ptr_cache_insert(H9LspPtrTokenCache* cache,
                                           void* ptr, H9LspInlinePage* page,
                                           uint32_t slot, uint32_t class_id) {
  if (!cache) {
    return;
  }
  if (cache->last.live) {
    h9_lsp_ptr_ledger_insert(&cache->ledger, cache->last.ptr,
                             cache->last.page, cache->last.slot,
                             cache->last.class_id);
  }
  h9_lsp_ptr_token_fill(&cache->last, ptr, page, slot, class_id);
}

static inline bool h9_lsp_ptr_token_free_checked(H9LspPtrToken* token,
                                                 void* ptr) {
  if (!token || !token->live || token->ptr != ptr || !token->page ||
      token->page->generation != token->generation ||
      token->slot >= token->page->slot_count) {
    return false;
  }
  bool ok = h9_lsp_inline_free_slot(token->page, token->slot);
  h9_lsp_ptr_ledger_clear_entry(token);
  return ok;
}

static inline bool h9_lsp_ptr_cache_free_hit(H9LspPtrTokenCache* cache,
                                             void* ptr) {
  if (!cache) {
    return false;
  }
  if (h9_lsp_ptr_token_free_checked(&cache->last, ptr)) {
    return true;
  }
  return h9_lsp_ptr_ledger_free_hit(&cache->ledger, ptr);
}

#endif

#endif
