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
  size_t ptrtoken_free_fast;
  size_t ptrtoken_free_fallback;
  size_t ptrtoken_usable_fast;
  size_t ptrtoken_usable_fallback;
  size_t ptrtoken_realloc_fast;
  size_t ptrtoken_realloc_fallback;
  size_t ptrtoken_state_mismatch;
  size_t segment_create;
  size_t segment_release;
  size_t segment_live;
  size_t segment_committed_bytes;
  size_t segment_committed_peak_bytes;
  size_t segment_reserved_bytes;
  size_t segment_reserved_peak_bytes;
  size_t segment_cap_reject;
} H9LspStats;

typedef struct H9LspRouteLeafBenchResult {
  uint64_t ok;
  uintptr_t sink;
  uint64_t fast_hits;
  uint64_t fallback_hits;
  uint64_t state_mismatch;
} H9LspRouteLeafBenchResult;

void h9_lsp_debug_reset(void);
void* h9_lsp_debug_alloc(uint32_t class_id);
bool h9_lsp_debug_alloc_slot(uint32_t class_id, void** ptr_out,
                             uint32_t* slot_out);
bool h9_lsp_debug_free(void* ptr, bool* owned_out);
H9LspRouteResult h9_lsp_debug_route(void* ptr);
H9LspRouteResult h9_lsp_debug_route_direct_owned(void* ptr);
bool h9_lsp_debug_usable_size(void* ptr, size_t* usable_out,
                              bool* owned_out);
void* h9_lsp_debug_ptrtoken_alloc(uint32_t class_id);
bool h9_lsp_debug_ptrtoken_free(void* ptr, bool* owned_out);
void* h9_lsp_debug_lasttoken_alloc(uint32_t class_id);
bool h9_lsp_debug_lasttoken_free(void* ptr, bool* owned_out);
bool h9_lsp_debug_lasttoken_usable_size(void* ptr, size_t* usable_out,
                                        bool* owned_out);
void* h9_lsp_debug_lasttoken_realloc_in_place(void* ptr, size_t size,
                                              bool* owned_out);
bool h9_lsp_debug_ptrtoken_usable_size(void* ptr, size_t* usable_out,
                                       bool* owned_out);
void* h9_lsp_debug_ptrtoken_realloc_in_place(void* ptr, size_t size,
                                             bool* owned_out);
bool h9_lsp_debug_free_direct_owned(void* ptr);
bool h9_lsp_debug_free_known_slot(uint32_t class_id, uint32_t slot);
void* h9_lsp_debug_realloc_in_place(void* ptr, size_t size, bool* owned_out);
H9LspStats h9_lsp_debug_stats(void);
bool h9_lsp_debug_routeleaf_bench(uint32_t class_id, uint64_t iters,
                                  bool touch, bool non_lifo,
                                  H9LspRouteLeafBenchResult* result_out);
bool h9_lsp_debug_routeleaf_compact_bench(
    uint32_t class_id, uint64_t iters, bool touch, bool non_lifo,
    H9LspRouteLeafBenchResult* result_out);
bool h9_lsp_debug_routeleaf_trim_bench(
    uint32_t class_id, uint64_t iters, bool touch, bool non_lifo,
    H9LspRouteLeafBenchResult* result_out);
bool h9_lsp_debug_routeleaf_tight_bench(
    uint32_t class_id, uint64_t iters, bool touch, bool non_lifo,
    H9LspRouteLeafBenchResult* result_out);
void* h9_lsp_debug_public_malloc(size_t size);
bool h9_lsp_debug_public_free(void* ptr, bool* owned_out);
bool h9_lsp_debug_publicentry_bench(size_t size, uint64_t iters, bool touch,
                                    bool non_lifo,
                                    H9LspRouteLeafBenchResult* result_out);
void* h9_lsp_debug_public_nosync_malloc(size_t size);
bool h9_lsp_debug_public_nosync_free(void* ptr, bool* owned_out);
void h9_lsp_debug_public_entry_reset(void);
bool h9_lsp_debug_public_current_free(void* ptr);
bool h9_lsp_debug_public_product_free(void* ptr, bool* owned_out);
bool h9_lsp_debug_publicentry_nosync_bench(
    size_t size, uint64_t iters, bool touch, bool non_lifo,
    H9LspRouteLeafBenchResult* result_out);

#endif

#endif
