#ifndef H8_HZ9_LOCAL_SLAB_ROUTE_BOUNDARY_H
#define H8_HZ9_LOCAL_SLAB_ROUTE_BOUNDARY_H

#include "../include/h8.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(H9_LOCAL_SLAB_PAGE_ROUTE_BOUNDARY_L0)

typedef enum H9LspRouteReason {
  H9_LSP_ROUTE_REASON_NONE = 0,
  H9_LSP_ROUTE_REASON_MISS = 1,
  H9_LSP_ROUTE_REASON_INTERIOR = 2,
  H9_LSP_ROUTE_REASON_TAIL = 3,
  H9_LSP_ROUTE_REASON_DOUBLE = 4
} H9LspRouteReason;

typedef struct H9LspRouteResult {
  H8RouteKind kind;
  void* segment;
  void* payload_base;
  uint32_t slot;
  uint32_t class_id;
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t reason;
} H9LspRouteResult;

typedef struct H9LspStats {
  size_t route_attempt;
  size_t route_miss;
  size_t route_valid;
  size_t route_invalid;
  size_t malloc_call;
  size_t malloc_tls_page_hit;
  size_t malloc_page_create;
  size_t free_same_owner_local;
  size_t free_invalid_owned;
  size_t free_miss;
  size_t usable_route_valid;
  size_t realloc_route_valid;
  size_t segment_create;
  size_t segment_release;
} H9LspStats;

void h9_lsp_debug_reset(void);
void* h9_lsp_debug_alloc(uint32_t class_id);
bool h9_lsp_debug_free(void* ptr, bool* owned_out);
H9LspRouteResult h9_lsp_debug_route(void* ptr);
bool h9_lsp_debug_usable_size(void* ptr, size_t* usable_out,
                              bool* owned_out);
void* h9_lsp_debug_realloc_in_place(void* ptr, size_t size, bool* owned_out);
H9LspStats h9_lsp_debug_stats(void);

#endif

#endif
