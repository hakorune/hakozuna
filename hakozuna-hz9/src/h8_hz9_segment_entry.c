#include "h8_internal.h"
#include "h8_hz9_segment_entry.h"

#if defined(H9_SEGMENT_ENTRY_L1)

#include <string.h>

#ifndef H9_SEGMENT_ENTRY_PAGE_CAP
#define H9_SEGMENT_ENTRY_PAGE_CAP 64u
#endif

typedef struct H9SegmentEntryPage {
  void* base;
  uint32_t slot_size;
  uint32_t run_size;
  uint16_t slot_count;
  uint16_t class_id;
  uint64_t free_bits;
  uint64_t alloc_bits;
} H9SegmentEntryPage;

static H9SegmentEntryPage h9_segment_entry_pages[H9_SEGMENT_ENTRY_PAGE_CAP];
static uint32_t h9_segment_entry_page_count;
static _Thread_local uint32_t h9_segment_entry_active[H8_MEDIUM_CLASS_COUNT];
static _Thread_local uintptr_t h9_segment_entry_handle[H8_MEDIUM_CLASS_COUNT];

static bool h9_segment_entry_class_geometry(uint32_t class_id,
                                            uint32_t* slot_size_out,
                                            uint32_t* run_size_out,
                                            uint16_t* slot_count_out) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec || spec->slot_count == 0u || spec->slot_count > 64u) {
    return false;
  }
  if (slot_size_out) {
    *slot_size_out = spec->slot_size;
  }
  if (run_size_out) {
    *run_size_out = spec->run_size;
  }
  if (slot_count_out) {
    *slot_count_out = spec->slot_count;
  }
  return true;
}

static bool h9_segment_entry_slot_from_addr(const H9SegmentEntryPage* page,
                                            uintptr_t addr,
                                            uint32_t* slot_out) {
  uintptr_t base = (uintptr_t)page->base;
  if (addr < base) {
    return false;
  }
  uintptr_t offset = addr - base;
  size_t payload = (size_t)page->slot_size * (size_t)page->slot_count;
  if (offset >= payload || page->slot_size == 0u ||
      (offset % (uintptr_t)page->slot_size) != 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)(offset / (uintptr_t)page->slot_size);
  if (slot >= page->slot_count) {
    return false;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return true;
}

static H9SegmentEntryPage* h9_segment_entry_page_for_ptr(void* ptr,
                                                         uint32_t* page_out,
                                                         uint32_t* slot_out) {
  uintptr_t addr = (uintptr_t)ptr;
  for (uint32_t i = 0u; i < h9_segment_entry_page_count; ++i) {
    H9SegmentEntryPage* page = &h9_segment_entry_pages[i];
    uintptr_t base = (uintptr_t)page->base;
    if (!base || addr < base || addr >= base + (uintptr_t)page->run_size) {
      continue;
    }
    if (page_out) {
      *page_out = i;
    }
    if (!h9_segment_entry_slot_from_addr(page, addr, slot_out)) {
      return page;
    }
    return page;
  }
  return NULL;
}

static H9SegmentEntryPage* h9_segment_entry_create_page(uint32_t class_id) {
  if (h9_segment_entry_page_count >= H9_SEGMENT_ENTRY_PAGE_CAP) {
    return NULL;
  }
  uint32_t slot_size = 0u;
  uint32_t run_size = 0u;
  uint16_t slot_count = 0u;
  if (!h9_segment_entry_class_geometry(class_id, &slot_size, &run_size,
                                       &slot_count)) {
    return NULL;
  }
  void* base = h8_platform_reserve_rw(run_size);
  if (!base) {
    return NULL;
  }
  uint64_t full_bits =
      slot_count == 64u ? UINT64_MAX : ((UINT64_C(1) << slot_count) - 1u);
  uint32_t page_id = h9_segment_entry_page_count++;
  H9SegmentEntryPage* page = &h9_segment_entry_pages[page_id];
  *page = (H9SegmentEntryPage){
      .base = base,
      .slot_size = slot_size,
      .run_size = run_size,
      .slot_count = slot_count,
      .class_id = (uint16_t)class_id,
      .free_bits = full_bits,
  };
  h9_segment_entry_active[class_id] = page_id;
  h9_segment_entry_handle[class_id] = (uintptr_t)page;
  return page;
}

static bool h9_segment_entry_cycle_page(H9SegmentEntryPage* page,
                                        void** ptr_out) {
  if (!page || page->free_bits == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(page->free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  page->free_bits &= ~bit;
  page->alloc_bits |= bit;
  if (ptr_out) {
    *ptr_out = (void*)((uintptr_t)page->base +
                       (uintptr_t)slot * (uintptr_t)page->slot_size);
  }
  page->alloc_bits &= ~bit;
  page->free_bits |= bit;
  return true;
}

static bool h9_segment_entry_alloc_page(H9SegmentEntryPage* page,
                                        void** ptr_out) {
  if (!page || page->free_bits == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(page->free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  page->free_bits &= ~bit;
  page->alloc_bits |= bit;
  if (ptr_out) {
    *ptr_out = (void*)((uintptr_t)page->base +
                       (uintptr_t)slot * (uintptr_t)page->slot_size);
  }
  return true;
}

static bool h9_segment_entry_alloc_page_slot(H9SegmentEntryPage* page,
                                             void** ptr_out,
                                             uint32_t* slot_out) {
  if (!page || page->free_bits == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(page->free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  page->free_bits &= ~bit;
  page->alloc_bits |= bit;
  if (ptr_out) {
    *ptr_out = (void*)((uintptr_t)page->base +
                       (uintptr_t)slot * (uintptr_t)page->slot_size);
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return true;
}

static bool h9_segment_entry_free_page(H9SegmentEntryPage* page, void* ptr,
                                       bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  if (!page || !ptr) {
    return false;
  }
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)page->base;
  if (!base || addr < base || addr >= base + (uintptr_t)page->run_size) {
    return false;
  }
  if (owned_out) {
    *owned_out = true;
  }
  uint32_t slot = 0u;
  if (!h9_segment_entry_slot_from_addr(page, addr, &slot)) {
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

static bool h9_segment_entry_free_page_slot(H9SegmentEntryPage* page,
                                            uint32_t slot, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  if (!page || slot >= page->slot_count) {
    return false;
  }
  if (owned_out) {
    *owned_out = true;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u) {
    return false;
  }
  page->alloc_bits &= ~bit;
  page->free_bits |= bit;
  return true;
}

void h9_segment_entry_debug_reset(void) {
  for (uint32_t i = 0u; i < h9_segment_entry_page_count; ++i) {
    H9SegmentEntryPage* page = &h9_segment_entry_pages[i];
    if (page->base) {
      h8_platform_release(page->base, page->run_size);
    }
  }
  memset(h9_segment_entry_pages, 0, sizeof(h9_segment_entry_pages));
  h9_segment_entry_page_count = 0u;
  for (uint32_t i = 0u; i < H8_MEDIUM_CLASS_COUNT; ++i) {
    h9_segment_entry_active[i] = UINT32_MAX;
    h9_segment_entry_handle[i] = 0u;
  }
}

bool h9_segment_entry_debug_alloc(uint32_t class_id, void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  uint32_t page_id = h9_segment_entry_active[class_id];
  H9SegmentEntryPage* page =
      page_id < h9_segment_entry_page_count ? &h9_segment_entry_pages[page_id]
                                            : NULL;
  if (!page || page->class_id != class_id || page->free_bits == 0u) {
    page = h9_segment_entry_create_page(class_id);
  }
  return h9_segment_entry_alloc_page(page, ptr_out);
}

bool h9_segment_entry_debug_cycle_fused(uint32_t class_id, void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  uint32_t page_id = h9_segment_entry_active[class_id];
  H9SegmentEntryPage* page =
      page_id < h9_segment_entry_page_count ? &h9_segment_entry_pages[page_id]
                                            : NULL;
  if (!page || page->class_id != class_id || page->free_bits == 0u) {
    page = h9_segment_entry_create_page(class_id);
  }
  return h9_segment_entry_cycle_page(page, ptr_out);
}

bool h9_segment_entry_debug_cycle_active_fast(uint32_t class_id,
                                              void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  uint32_t page_id = h9_segment_entry_active[class_id];
  H9SegmentEntryPage* page =
      page_id < h9_segment_entry_page_count ? &h9_segment_entry_pages[page_id]
                                            : NULL;
  if (!page || page->class_id != class_id || page->free_bits == 0u) {
    return h9_segment_entry_debug_cycle_fused(class_id, ptr_out);
  }
  return h9_segment_entry_cycle_page(page, ptr_out);
}

uint32_t h9_segment_entry_debug_prepare_active(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return UINT32_MAX;
  }
  uint32_t page_id = h9_segment_entry_active[class_id];
  H9SegmentEntryPage* page =
      page_id < h9_segment_entry_page_count ? &h9_segment_entry_pages[page_id]
                                            : NULL;
  if (!page || page->class_id != class_id || page->free_bits == 0u) {
    page = h9_segment_entry_create_page(class_id);
    page_id = page ? (uint32_t)(page - h9_segment_entry_pages) : UINT32_MAX;
  }
  return page ? page_id : UINT32_MAX;
}

bool h9_segment_entry_debug_cycle_page(uint32_t page_id, void** ptr_out) {
  if (page_id >= h9_segment_entry_page_count) {
    return false;
  }
  return h9_segment_entry_cycle_page(&h9_segment_entry_pages[page_id],
                                     ptr_out);
}

uintptr_t h9_segment_entry_debug_prepare_handle(uint32_t class_id) {
  uint32_t page_id = h9_segment_entry_debug_prepare_active(class_id);
  uintptr_t handle = page_id < h9_segment_entry_page_count
                         ? (uintptr_t)&h9_segment_entry_pages[page_id]
                         : 0u;
  if (class_id < H8_MEDIUM_CLASS_COUNT) {
    h9_segment_entry_handle[class_id] = handle;
  }
  return handle;
}

bool h9_segment_entry_debug_cycle_handle(uintptr_t handle, void** ptr_out) {
  if (handle == 0u) {
    return false;
  }
  return h9_segment_entry_cycle_page((H9SegmentEntryPage*)handle, ptr_out);
}

bool h9_segment_entry_debug_cycle_tls_handle(uint32_t class_id,
                                             void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SegmentEntryPage* page =
      (H9SegmentEntryPage*)h9_segment_entry_handle[class_id];
  if (!page || page->class_id != class_id || page->free_bits == 0u) {
    uintptr_t handle = h9_segment_entry_debug_prepare_handle(class_id);
    page = (H9SegmentEntryPage*)handle;
  }
  return h9_segment_entry_cycle_page(page, ptr_out);
}

bool h9_segment_entry_debug_alloc_tls_handle(uint32_t class_id,
                                             void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SegmentEntryPage* page =
      (H9SegmentEntryPage*)h9_segment_entry_handle[class_id];
  if (!page || page->class_id != class_id || page->free_bits == 0u) {
    uintptr_t handle = h9_segment_entry_debug_prepare_handle(class_id);
    page = (H9SegmentEntryPage*)handle;
  }
  return h9_segment_entry_alloc_page(page, ptr_out);
}

bool h9_segment_entry_debug_free_tls_handle(uint32_t class_id, void* ptr,
                                            bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SegmentEntryPage* page =
      (H9SegmentEntryPage*)h9_segment_entry_handle[class_id];
  bool owned = false;
  if (h9_segment_entry_free_page(page, ptr, &owned)) {
    if (owned_out) {
      *owned_out = true;
    }
    return true;
  }
  if (owned) {
    if (owned_out) {
      *owned_out = true;
    }
    return false;
  }
  return h9_segment_entry_debug_free(ptr, owned_out);
}

bool h9_segment_entry_debug_alloc_tls_slot(uint32_t class_id, void** ptr_out,
                                           uint32_t* slot_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SegmentEntryPage* page =
      (H9SegmentEntryPage*)h9_segment_entry_handle[class_id];
  if (!page || page->class_id != class_id || page->free_bits == 0u) {
    uintptr_t handle = h9_segment_entry_debug_prepare_handle(class_id);
    page = (H9SegmentEntryPage*)handle;
  }
  return h9_segment_entry_alloc_page_slot(page, ptr_out, slot_out);
}

bool h9_segment_entry_debug_free_tls_slot(uint32_t class_id, uint32_t slot,
                                          bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SegmentEntryPage* page =
      (H9SegmentEntryPage*)h9_segment_entry_handle[class_id];
  return h9_segment_entry_free_page_slot(page, slot, owned_out);
}

bool h9_segment_entry_debug_free(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  uint32_t slot = 0u;
  H9SegmentEntryPage* page = h9_segment_entry_page_for_ptr(ptr, NULL, &slot);
  if (!page) {
    return false;
  }
  if (owned_out) {
    *owned_out = true;
  }
  if (!h9_segment_entry_slot_from_addr(page, (uintptr_t)ptr, &slot)) {
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

H8RouteKind h9_segment_entry_debug_route(void* ptr) {
  uint32_t slot = 0u;
  H9SegmentEntryPage* page = h9_segment_entry_page_for_ptr(ptr, NULL, &slot);
  if (!page) {
    return H8_ROUTE_MISS;
  }
  if (!h9_segment_entry_slot_from_addr(page, (uintptr_t)ptr, &slot)) {
    return H8_ROUTE_INVALID;
  }
  uint64_t bit = UINT64_C(1) << slot;
  return (page->alloc_bits & bit) != 0u ? H8_ROUTE_VALID : H8_ROUTE_INVALID;
}

uint64_t h9_segment_entry_debug_free_bits(uint32_t page_id) {
  return page_id < h9_segment_entry_page_count
             ? h9_segment_entry_pages[page_id].free_bits
             : 0u;
}

uint64_t h9_segment_entry_debug_alloc_bits(uint32_t page_id) {
  return page_id < h9_segment_entry_page_count
             ? h9_segment_entry_pages[page_id].alloc_bits
             : 0u;
}

uint32_t h9_segment_entry_debug_page_count(void) {
  return h9_segment_entry_page_count;
}

#else
typedef int h9_segment_entry_translation_unit_silence;
#endif
