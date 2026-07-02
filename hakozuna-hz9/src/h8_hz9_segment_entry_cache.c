#include "h8_hz9_segment_entry_internal.h"

#if defined(H9_SEGMENT_ENTRY_L1)

static bool h9_segment_entry_cache_pop(uint32_t class_id, void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT ||
      !h9_segment_entry_cache_ptr[class_id]) {
    return false;
  }
  H9SegmentEntryPage* page =
      (H9SegmentEntryPage*)h9_segment_entry_cache_page[class_id];
  uint32_t slot = h9_segment_entry_cache_slot[class_id];
  if (!page || page->class_id != class_id || slot >= page->slot_count) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u ||
      (page->cache_bits & bit) == 0u) {
    return false;
  }
  page->cache_bits &= ~bit;
  if (ptr_out) {
    *ptr_out = h9_segment_entry_cache_ptr[class_id];
  }
  h9_segment_entry_cache_ptr[class_id] = NULL;
  h9_segment_entry_cache_page[class_id] = 0u;
  h9_segment_entry_cache_slot[class_id] = UINT32_MAX;
  return true;
}

static bool h9_segment_entry_cache_pop_slot(uint32_t class_id,
                                            H9SegmentEntryPage** page_out,
                                            uint32_t* slot_out,
                                            void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT ||
      !h9_segment_entry_cache_ptr[class_id]) {
    return false;
  }
  H9SegmentEntryPage* page =
      (H9SegmentEntryPage*)h9_segment_entry_cache_page[class_id];
  uint32_t slot = h9_segment_entry_cache_slot[class_id];
  if (!page || page->class_id != class_id || slot >= page->slot_count) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u ||
      (page->cache_bits & bit) == 0u) {
    return false;
  }
  page->cache_bits &= ~bit;
  if (page_out) {
    *page_out = page;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  if (ptr_out) {
    *ptr_out = h9_segment_entry_cache_ptr[class_id];
  }
  h9_segment_entry_cache_ptr[class_id] = NULL;
  h9_segment_entry_cache_page[class_id] = 0u;
  h9_segment_entry_cache_slot[class_id] = UINT32_MAX;
  return true;
}

static bool h9_segment_entry_cache_push(uint32_t class_id, void* ptr) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT ||
      h9_segment_entry_cache_ptr[class_id] != NULL) {
    return false;
  }
  uint32_t slot = UINT32_MAX;
  H9SegmentEntryPage* page = h9_segment_entry_page_for_ptr(ptr, NULL, &slot);
  if (!page || page->class_id != class_id || slot == UINT32_MAX) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u ||
      (page->cache_bits & bit) != 0u) {
    return false;
  }
  page->cache_bits |= bit;
  h9_segment_entry_cache_ptr[class_id] = ptr;
  h9_segment_entry_cache_page[class_id] = (uintptr_t)page;
  h9_segment_entry_cache_slot[class_id] = slot;
  return true;
}

static bool h9_segment_entry_cache_push_slot(uint32_t class_id,
                                             H9SegmentEntryPage* page,
                                             uint32_t slot, void* ptr) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT ||
      h9_segment_entry_cache_ptr[class_id] != NULL || !page ||
      page->class_id != class_id || slot >= page->slot_count || !ptr) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u ||
      (page->cache_bits & bit) != 0u) {
    return false;
  }
  page->cache_bits |= bit;
  h9_segment_entry_cache_ptr[class_id] = ptr;
  h9_segment_entry_cache_page[class_id] = (uintptr_t)page;
  h9_segment_entry_cache_slot[class_id] = slot;
  return true;
}

bool h9_segment_entry_debug_cycle_tls_cache(uint32_t class_id, uint64_t value,
                                            bool touch, void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  void* ptr = NULL;
  if (!h9_segment_entry_cache_pop(class_id, &ptr) &&
      !h9_segment_entry_debug_alloc_tls_handle(class_id, &ptr)) {
    return false;
  }
  if (ptr_out) {
    *ptr_out = ptr;
  }
  if (touch) {
    const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
    if (!spec || spec->slot_size == 0u) {
      return false;
    }
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    p[0] = (unsigned char)value;
    p[spec->slot_size - 1u] = (unsigned char)(value >> 8);
  }
  return h9_segment_entry_cache_push(class_id, ptr);
}

static bool h9_segment_entry_tls_token_cache_ensure(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SegmentEntryTokenCache* cache =
      &h9_segment_entry_token_cache_state[class_id];
  if (h9_segment_entry_debug_token_current(&cache->token)) {
    return true;
  }
  if (cache->cache_ptr &&
      !h9_segment_entry_retire_token_cache_state_inline(cache)) {
    return false;
  }
  h9_segment_entry_token_cache_reset(cache);
  return h9_segment_entry_debug_acquire_token(class_id, &cache->token);
}

bool h9_segment_entry_debug_acquire_tls_token_cache(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SegmentEntryTokenCache* cache =
      &h9_segment_entry_token_cache_state[class_id];
  if (cache->cache_ptr &&
      !h9_segment_entry_retire_token_cache_state_inline(cache)) {
    return false;
  }
  h9_segment_entry_token_cache_reset(cache);
  return h9_segment_entry_debug_acquire_token(class_id, &cache->token);
}

bool h9_segment_entry_debug_cycle_tls_token_cache_body(uint32_t class_id,
                                                       uint64_t value,
                                                       bool touch,
                                                       void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  return h9_segment_entry_cycle_token_cache_state_inline(
      &h9_segment_entry_token_cache_state[class_id], value, touch, ptr_out);
}

bool h9_segment_entry_debug_cycle_tls_token_cache(uint32_t class_id,
                                                  uint64_t value, bool touch,
                                                  void** ptr_out) {
  if (!h9_segment_entry_tls_token_cache_ensure(class_id)) {
    return false;
  }
  return h9_segment_entry_cycle_token_cache_state_inline(
      &h9_segment_entry_token_cache_state[class_id], value, touch, ptr_out);
}

bool h9_segment_entry_debug_retire_tls_token_cache(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SegmentEntryTokenCache* cache =
      &h9_segment_entry_token_cache_state[class_id];
  bool ok = h9_segment_entry_retire_token_cache_state_inline(cache);
  if (ok) {
    h9_segment_entry_token_cache_reset(cache);
  }
  return ok;
}

bool h9_segment_entry_debug_cycle_tls_ledger(uint32_t class_id, uint64_t value,
                                             bool touch, void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SegmentEntryPage* page = NULL;
  uint32_t slot = UINT32_MAX;
  void* ptr = NULL;
  if (!h9_segment_entry_cache_pop_slot(class_id, &page, &slot, &ptr)) {
    page = (H9SegmentEntryPage*)h9_segment_entry_handle[class_id];
    if (!page || page->class_id != class_id || page->free_bits == 0u) {
      uintptr_t handle = h9_segment_entry_debug_prepare_handle(class_id);
      page = (H9SegmentEntryPage*)handle;
    }
    if (!h9_segment_entry_alloc_page_slot(page, &ptr, &slot)) {
      return false;
    }
  }
  if (ptr_out) {
    *ptr_out = ptr;
  }
  if (touch) {
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    p[0] = (unsigned char)value;
    p[page->slot_size - 1u] = (unsigned char)(value >> 8);
  }
  return h9_segment_entry_cache_push_slot(class_id, page, slot, ptr);
}

bool h9_segment_entry_debug_cycle_tls_ledger_body(uint32_t class_id,
                                                  uint64_t value, bool touch,
                                                  void** ptr_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9SegmentEntryPage* page =
      (H9SegmentEntryPage*)h9_segment_entry_cache_page[class_id];
  uint32_t slot = h9_segment_entry_cache_slot[class_id];
  void* ptr = h9_segment_entry_cache_ptr[class_id];
  if (ptr && page && page->class_id == class_id && slot < page->slot_count) {
    uint64_t bit = UINT64_C(1) << slot;
    if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u ||
        (page->cache_bits & bit) == 0u) {
      return false;
    }
    page->cache_bits &= ~bit;
    h9_segment_entry_cache_ptr[class_id] = NULL;
    h9_segment_entry_cache_page[class_id] = 0u;
    h9_segment_entry_cache_slot[class_id] = UINT32_MAX;
  } else {
    page = (H9SegmentEntryPage*)h9_segment_entry_handle[class_id];
    if (!page || page->class_id != class_id || page->free_bits == 0u) {
      uintptr_t handle = h9_segment_entry_debug_prepare_handle(class_id);
      page = (H9SegmentEntryPage*)handle;
    }
    if (!h9_segment_entry_alloc_page_slot(page, &ptr, &slot)) {
      return false;
    }
  }
  if (ptr_out) {
    *ptr_out = ptr;
  }
  if (touch) {
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    p[0] = (unsigned char)value;
    p[page->slot_size - 1u] = (unsigned char)(value >> 8);
  }
  uint64_t bit = UINT64_C(1) << slot;
  if (h9_segment_entry_cache_ptr[class_id] != NULL ||
      (page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u ||
      (page->cache_bits & bit) != 0u) {
    return false;
  }
  page->cache_bits |= bit;
  h9_segment_entry_cache_ptr[class_id] = ptr;
  h9_segment_entry_cache_page[class_id] = (uintptr_t)page;
  h9_segment_entry_cache_slot[class_id] = slot;
  return true;
}

#else
typedef int h9_segment_entry_cache_translation_unit_silence;
#endif
