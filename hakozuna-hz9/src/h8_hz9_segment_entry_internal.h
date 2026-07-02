#ifndef H8_HZ9_SEGMENT_ENTRY_INTERNAL_H
#define H8_HZ9_SEGMENT_ENTRY_INTERNAL_H

#include "h8_internal.h"
#include "h8_hz9_segment_entry.h"

#if defined(H9_SEGMENT_ENTRY_L1)

#ifndef H9_SEGMENT_ENTRY_PAGE_CAP
#define H9_SEGMENT_ENTRY_PAGE_CAP 64u
#endif

typedef struct H9SegmentEntryPage {
  void* base;
  uint32_t slot_size;
  uint32_t run_size;
  uint32_t generation;
  uint16_t slot_count;
  uint16_t class_id;
  uint64_t free_bits;
  uint64_t alloc_bits;
  uint64_t cache_bits;
} H9SegmentEntryPage;

typedef struct H9SegmentEntryTokenCache {
  H9SegmentEntryToken token;
  uint32_t cache_slot;
  void* cache_ptr;
} H9SegmentEntryTokenCache;

extern H9SegmentEntryPage h9_segment_entry_pages[H9_SEGMENT_ENTRY_PAGE_CAP];
extern uint32_t h9_segment_entry_page_count;
extern _Thread_local uint32_t
    h9_segment_entry_active[H8_MEDIUM_CLASS_COUNT];
extern _Thread_local uintptr_t
    h9_segment_entry_handle[H8_MEDIUM_CLASS_COUNT];
extern _Thread_local uint32_t
    h9_segment_entry_handle_generation[H8_MEDIUM_CLASS_COUNT];
extern _Thread_local void*
    h9_segment_entry_cache_ptr[H8_MEDIUM_CLASS_COUNT];
extern _Thread_local uintptr_t
    h9_segment_entry_cache_page[H8_MEDIUM_CLASS_COUNT];
extern _Thread_local uint32_t
    h9_segment_entry_cache_slot[H8_MEDIUM_CLASS_COUNT];
extern _Thread_local H9SegmentEntryTokenCache
    h9_segment_entry_token_cache_state[H8_MEDIUM_CLASS_COUNT];

static inline bool h9_segment_entry_cycle_page_checked_touch_inline(
    H9SegmentEntryPage* page, uint64_t value, bool touch, void** ptr_out) {
  if (!page || page->free_bits == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(page->free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  page->free_bits &= ~bit;
  page->alloc_bits |= bit;
  void* ptr = (void*)((uintptr_t)page->base +
                      (uintptr_t)slot * (uintptr_t)page->slot_size);
  if (ptr_out) {
    *ptr_out = ptr;
  }
  if (touch) {
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    p[0] = (unsigned char)value;
    p[page->slot_size - 1u] = (unsigned char)(value >> 8);
  }
  if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u) {
    return false;
  }
  page->alloc_bits &= ~bit;
  page->free_bits |= bit;
  return true;
}

static inline bool h9_segment_entry_cycle_token_hot_inline(
    const H9SegmentEntryToken* token, uint64_t value, bool touch,
    void** ptr_out) {
  return token && token->handle != 0u &&
         h9_segment_entry_cycle_page_checked_touch_inline(
             (H9SegmentEntryPage*)token->handle, value, touch, ptr_out);
}

static inline bool h9_segment_entry_cycle_token_cache_inline(
    const H9SegmentEntryToken* token, uint32_t* cache_slot, void** cache_ptr,
    uint64_t value, bool touch, void** ptr_out) {
  if (!token || token->handle == 0u || !cache_slot || !cache_ptr) {
    return false;
  }
  H9SegmentEntryPage* page = (H9SegmentEntryPage*)token->handle;
  uint32_t slot = *cache_slot;
  void* ptr = *cache_ptr;
  if (ptr && slot < page->slot_count) {
    page->cache_bits &= ~(UINT64_C(1) << slot);
    *cache_ptr = NULL;
    *cache_slot = UINT32_MAX;
  } else {
    if (page->free_bits == 0u) {
      return false;
    }
    slot = (uint32_t)__builtin_ctzll(page->free_bits);
    uint64_t bit = UINT64_C(1) << slot;
    page->free_bits &= ~bit;
    page->alloc_bits |= bit;
    ptr = (void*)((uintptr_t)page->base +
                  (uintptr_t)slot * (uintptr_t)page->slot_size);
  }
  if (ptr_out) {
    *ptr_out = ptr;
  }
  if (touch) {
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    p[0] = (unsigned char)value;
    p[page->slot_size - 1u] = (unsigned char)(value >> 8);
  }
  page->cache_bits |= UINT64_C(1) << slot;
  *cache_ptr = ptr;
  *cache_slot = slot;
  return true;
}

static inline bool h9_segment_entry_token_cache_pop_slot_inline(
    H9SegmentEntryTokenCache* cache, uint32_t* slot_out, void** ptr_out) {
  if (!cache || cache->token.handle == 0u) {
    return false;
  }
  H9SegmentEntryPage* page = (H9SegmentEntryPage*)cache->token.handle;
  uint32_t slot = cache->cache_slot;
  void* ptr = cache->cache_ptr;
  if (ptr && slot < page->slot_count) {
    uint64_t bit = UINT64_C(1) << slot;
    if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u ||
        (page->cache_bits & bit) == 0u) {
      return false;
    }
    page->cache_bits &= ~bit;
    cache->cache_ptr = NULL;
    cache->cache_slot = UINT32_MAX;
  } else {
    if (page->free_bits == 0u) {
      return false;
    }
    slot = (uint32_t)__builtin_ctzll(page->free_bits);
    uint64_t bit = UINT64_C(1) << slot;
    page->free_bits &= ~bit;
    page->alloc_bits |= bit;
    ptr = (void*)((uintptr_t)page->base +
                  (uintptr_t)slot * (uintptr_t)page->slot_size);
  }
  if (slot_out) {
    *slot_out = slot;
  }
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return true;
}

static inline bool h9_segment_entry_token_cache_push_slot_inline(
    H9SegmentEntryTokenCache* cache, uint32_t slot, void* ptr) {
  if (!cache || cache->token.handle == 0u || !ptr) {
    return false;
  }
  H9SegmentEntryPage* page = (H9SegmentEntryPage*)cache->token.handle;
  if (slot >= page->slot_count || cache->cache_ptr != NULL) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u ||
      (page->cache_bits & bit) != 0u) {
    return false;
  }
  page->cache_bits |= bit;
  cache->cache_ptr = ptr;
  cache->cache_slot = slot;
  return true;
}

static inline void h9_segment_entry_token_cache_reset(
    H9SegmentEntryTokenCache* cache) {
  if (!cache) {
    return;
  }
  cache->token = (H9SegmentEntryToken){0};
  cache->cache_slot = UINT32_MAX;
  cache->cache_ptr = NULL;
}

static inline bool h9_segment_entry_cycle_token_cache_state_inline(
    H9SegmentEntryTokenCache* cache, uint64_t value, bool touch,
    void** ptr_out) {
  if (!cache) {
    return false;
  }
  return h9_segment_entry_cycle_token_cache_inline(
      &cache->token, &cache->cache_slot, &cache->cache_ptr, value, touch,
      ptr_out);
}

static inline bool h9_segment_entry_retire_token_cache_inline(
    const H9SegmentEntryToken* token, uint32_t* cache_slot, void** cache_ptr) {
  if (!token || token->handle == 0u || !cache_slot || !cache_ptr) {
    return false;
  }
  if (!*cache_ptr) {
    *cache_slot = UINT32_MAX;
    return true;
  }
  H9SegmentEntryPage* page = (H9SegmentEntryPage*)token->handle;
  uint32_t slot = *cache_slot;
  if (slot >= page->slot_count) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((page->alloc_bits & bit) == 0u || (page->free_bits & bit) != 0u ||
      (page->cache_bits & bit) == 0u) {
    return false;
  }
  page->cache_bits &= ~bit;
  page->alloc_bits &= ~bit;
  page->free_bits |= bit;
  *cache_ptr = NULL;
  *cache_slot = UINT32_MAX;
  return true;
}

static inline bool h9_segment_entry_retire_token_cache_state_inline(
    H9SegmentEntryTokenCache* cache) {
  if (!cache) {
    return false;
  }
  return h9_segment_entry_retire_token_cache_inline(
      &cache->token, &cache->cache_slot, &cache->cache_ptr);
}

H9SegmentEntryPage* h9_segment_entry_page_for_ptr(void* ptr,
                                                  uint32_t* page_out,
                                                  uint32_t* slot_out);
bool h9_segment_entry_alloc_page_slot(H9SegmentEntryPage* page,
                                      void** ptr_out,
                                      uint32_t* slot_out);
bool h9_segment_entry_cycle_page_checked_touch(H9SegmentEntryPage* page,
                                               uint64_t value,
                                               bool touch,
                                               void** ptr_out);

#endif

#endif
