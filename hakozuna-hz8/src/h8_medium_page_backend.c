#include "h8_medium_page_backend.h"
#if defined(H8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1) && \
    defined(H8_MEDIUM_PAGE8K_HZ10_SHADOW_L1)
#error "HZ8-native page8K backend and HZ10 shadow cannot be combined"
#endif
#if defined(H8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1)
#include "h8_medium_page8k_remote.h"
void* h8_medium_page_backend_malloc(size_t size) {
  return h8_page8k_remote_malloc_current(size);
}
bool h8_medium_page_backend_accepts_size(size_t size) {
  return h8_page8k_remote_accepts_size(size);
}
bool h8_medium_page_backend_free(void* ptr, bool* owned_out) {
  return h8_page8k_remote_free_current(ptr, owned_out);
}
H8RouteKind h8_medium_page_backend_route(const void* ptr) {
  return h8_page8k_remote_route_current(ptr);
}
bool h8_medium_page_backend_usable_size(const void* ptr, size_t* usable_out,
                                        bool* owned_out) {
  return h8_page8k_remote_usable_size_current(ptr, usable_out, owned_out);
}
#elif defined(H8_MEDIUM_PAGE8K_HZ10_SHADOW_L1)
/* Evidence-only adapter; the public page backend is HZ8-native R3. */
#include "hz10_freelist_page.h"
#include "hz10_pagemap.h"
#include "hz10_public_entry.h"
void* h8_medium_page_backend_malloc(size_t size) {
  if (size != 8192u) return NULL;
  void* ptr = hz10_malloc(size);
  return ptr;
}
bool h8_medium_page_backend_accepts_size(size_t size) {
  return size == 8192u;
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
H8RouteKind h8_medium_page_backend_route(const void* ptr) {
  (void)ptr;
  return H8_ROUTE_MISS;
}
bool h8_medium_page_backend_usable_size(const void* ptr, size_t* usable_out,
                                        bool* owned_out) {
  (void)ptr;
  if (usable_out) *usable_out = 0;
  if (owned_out) *owned_out = false;
  return false;
}
#else
void* h8_medium_page_backend_malloc(size_t size) { (void)size; return NULL; }
bool h8_medium_page_backend_accepts_size(size_t size) {
  (void)size;
  return false;
}
bool h8_medium_page_backend_free(void* ptr, bool* owned_out) {
  (void)ptr;
  if (owned_out) *owned_out = false;
  return false;
}
H8RouteKind h8_medium_page_backend_route(const void* ptr) {
  (void)ptr;
  return H8_ROUTE_MISS;
}
bool h8_medium_page_backend_usable_size(const void* ptr, size_t* usable_out,
                                        bool* owned_out) {
  (void)ptr;
  if (usable_out) *usable_out = 0;
  if (owned_out) *owned_out = false;
  return false;
}
#endif
