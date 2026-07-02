#include "h8_internal.h"

#if defined(H9_LOCAL_ENTRY_SPLIT_L1)

#include "h8_hz9_local_arena.inc"

void* h9_local_malloc_medium_inner(size_t size) {
  uint32_t class_id = h8_medium_class_for_size_fast(size);
#if defined(H9_LOCAL_ARENA_L0)
  if (h9_local_arena_class_enabled(class_id)) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_MALLOC_CLASS_ENABLED]);
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_ATTEMPT]);
    void* ptr = h9_local_arena_malloc(class_id);
    if (ptr) {
      return ptr;
    }
  } else {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_MALLOC_CLASS_DISABLED]);
  }
#endif
  H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_MALLOC_FALLBACK]);
  return h8_medium_malloc_class_inner(class_id);
}

void h9_local_note_medium_remote_free(uint32_t class_id) {
#if defined(H9_LOCAL_ARENA_L0)
  h9_local_arena_note_remote_class(class_id);
#else
  (void)class_id;
#endif
}

void h9_local_free_outer(void* ptr) {
#if defined(H9_LOCAL_ARENA_L0)
  bool owned = false;
  if (h9_local_arena_free(ptr, &owned)) {
    return;
  }
  if (owned) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_FREE_OWNED_INVALID_OUTER]);
    H8_DEBUG_INC(invalid_count);
    return;
  }
#endif
  H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_FREE_FALLBACK_OUTER]);
  h8_free_inner(ptr);
}

bool h9_local_maybe_contains_hot(const void* ptr) {
#if defined(H9_LOCAL_ARENA_L0)
  return ptr && h9_local_addr_in_arena((uintptr_t)ptr);
#else
  (void)ptr;
  return false;
#endif
}

bool h9_local_usable_size_inner(void* ptr, size_t* usable_out,
                                bool* owned_out) {
  H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_USABLE_CHECK]);
  if (owned_out) {
    *owned_out = false;
  }
#if defined(H9_LOCAL_ARENA_L0)
  uintptr_t page_offset = 0;
  H9LocalArenaPage* page = h9_local_page_for_ptr(ptr, &page_offset);
  if (!page) {
    return false;
  }
  if (owned_out) {
    *owned_out = true;
  }
  size_t slot = 0;
  if (!h9_local_slot_index(page, page_offset, &slot)) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_USABLE_OWNED_INVALID]);
    return false;
  }
  uint32_t state =
      atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
  if (!h8_slot_state_is_allocated_tag(state)) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_USABLE_OWNED_INVALID]);
    return false;
  }
  if (usable_out) {
    *usable_out = page->slot_size;
  }
  H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_USABLE_VALID]);
  return true;
#else
  (void)ptr;
  (void)usable_out;
  return false;
#endif
}

H8RouteKind h9_local_route_inner(void* ptr) {
  H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ROUTE_CHECK]);
#if defined(H9_LOCAL_ARENA_L0)
  bool owned = false;
  size_t usable = 0;
  bool valid = h9_local_usable_size_inner(ptr, &usable, &owned);
  (void)usable;
  if (valid) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ROUTE_VALID]);
    return H8_ROUTE_VALID;
  }
  if (owned) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ROUTE_INVALID]);
    return H8_ROUTE_INVALID;
  }
  return H8_ROUTE_MISS;
#else
  (void)ptr;
  return H8_ROUTE_MISS;
#endif
}

void h9_local_flush_owner(H8OwnerRecord* owner) {
#if defined(H9_LOCAL_ARENA_L0)
  if (!owner || !h9_local_arena_base) {
    return;
  }
  uint64_t owner_word = h9_local_owner_word_for(owner);
  for (uint32_t i = 0; i < h9_local_arena_next_page; ++i) {
    H9LocalArenaPage* page = &h9_local_arena_pages[i];
    if (!atomic_load_explicit(&page->registered, memory_order_acquire) ||
        page->owner_word != owner_word) {
      continue;
    }
    uint64_t full = (UINT64_C(1) << page->slot_count) - 1u;
    uint64_t free_bits =
        atomic_load_explicit(&page->free_bits, memory_order_acquire) & full;
    if (free_bits == full) {
      uint8_t* base = h9_local_arena_base +
                      (size_t)i * (size_t)H9_LOCAL_ARENA_PAGE_BYTES;
      h8_platform_purge(base, H9_LOCAL_ARENA_PAGE_BYTES);
    }
  }
#else
  (void)owner;
#endif
}

#else
typedef int h9_local_entry_translation_unit_silence;
#endif
