#include "h8_medium_page_backend.h"
#if defined(H8_MEDIUM_PAGE_SUBSTRATE_FIXED8K_L1)
#include "hz10_freelist_page.h"
#include "hz10_pagemap.h"
#include "hz10_public_entry.h"
void* h8_medium_page_backend_malloc(size_t size) {
  if (size != 8192u) return NULL;
  void* ptr = hz10_malloc(size);
  return ptr;
}
bool h8_medium_page_backend_free(void* ptr, bool* owned_out) {
  if (owned_out) *owned_out = false;
  H10RouteResult route = hz10_pagemap_route(ptr, HZ10_GENERATION_ANY);
  if (route.kind == H10_ROUTE_MISS) return false;
  if (owned_out) *owned_out = true;
  if (route.kind != H10_ROUTE_VALID || route.flags != 0u || !route.owner)
    return false;
  Hz10FreelistPage* page = (Hz10FreelistPage*)route.owner;
  if (hz10_freelist_page_is_marked_local_free(page, ptr)) return false;
  return hz10_free(ptr) != 0;
}
#else
void* h8_medium_page_backend_malloc(size_t size) { (void)size; return NULL; }
bool h8_medium_page_backend_free(void* ptr, bool* owned_out) {
  (void)ptr;
  if (owned_out) *owned_out = false;
  return false;
}
#endif
