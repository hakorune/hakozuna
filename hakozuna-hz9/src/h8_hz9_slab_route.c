#include "h8_internal.h"

#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1) && \
    !defined(H9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY)
#error "H9_MEDIUM_LOCAL_SLAB_PAGE_L1 requires H9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY"
#endif

#if defined(H9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY)

#include <string.h>

#include "h8_hz9_slab_route_internal.h"

static h8_platform_once_t h9_slab_route_once = H8_PLATFORM_ONCE_INIT;
static h8_platform_mutex_t h9_slab_route_lock;
static H9MediumSlabRoutePage h9_slab_route_pages[H9_SLAB_ROUTE_MAX_PAGES];
static _Atomic uint32_t h9_slab_route_highwater;
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
static _Atomic uintptr_t h9_slab_base_hash[H9_SLAB_ROUTE_HASH_CAP];
_Atomic bool h9_slab_route_has_pages;
_Atomic uintptr_t h9_slab_route_min_addr;
_Atomic uintptr_t h9_slab_route_max_addr;
#endif

static void h9_slab_route_init_once(void) {
  h8_platform_mutex_init(&h9_slab_route_lock);
}

static void h9_slab_route_init(void) {
  h8_platform_once(&h9_slab_route_once, h9_slab_route_init_once);
}

#if defined(H8_ENABLE_DEBUG_STATS) && defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
static void h9_slab_debug_update_peak(atomic_size_t* counter, size_t value) {
  size_t peak = atomic_load_explicit(counter, memory_order_relaxed);
  while (value > peak &&
         !atomic_compare_exchange_weak_explicit(
             counter, &peak, value, memory_order_release,
             memory_order_relaxed)) {
  }
}

static void h9_slab_debug_note_register(size_t bytes, size_t raw_bytes) {
  H8_DEBUG_INC(h9_slab_registered_pages);
  size_t registered = atomic_fetch_add_explicit(
                          &h8g.h9_slab_registered_bytes, bytes,
                          memory_order_relaxed) +
                      bytes;
  h9_slab_debug_update_peak(&h8g.h9_slab_registered_peak_bytes, registered);
  size_t raw_registered = atomic_fetch_add_explicit(
                              &h8g.h9_slab_raw_reserved_bytes, raw_bytes,
                              memory_order_relaxed) +
                          raw_bytes;
  h9_slab_debug_update_peak(&h8g.h9_slab_raw_reserved_peak_bytes,
                            raw_registered);
}
#endif

static void h9_slab_raise_highwater(uint32_t wanted_highwater) {
  uint32_t old_highwater =
      atomic_load_explicit(&h9_slab_route_highwater, memory_order_acquire);
  while (wanted_highwater > old_highwater &&
         !atomic_compare_exchange_weak_explicit(
             &h9_slab_route_highwater, &old_highwater, wanted_highwater,
             memory_order_acq_rel, memory_order_acquire)) {
  }
}

static uint64_t h9_slab_owner_word_for(const H8OwnerRecord* owner) {
  if (!owner) {
    return 0;
  }
  H8OwnerWord word = h8_owner_word_make((uint8_t)owner->slot,
                                        (uint16_t)owner->generation,
                                        H8_SPAN_OWNED_ACTIVE, 0);
  return h8_owner_word_pack(word);
}

#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
static uint64_t h9_slab_hash_base(uintptr_t base) {
  return (uint64_t)(base / H9_SLAB_PAGE_BYTES) * UINT64_C(11400714819323198485);
}

static void h9_slab_note_range(uintptr_t base, uintptr_t end) {
  uintptr_t min_addr =
      atomic_load_explicit(&h9_slab_route_min_addr, memory_order_acquire);
  while ((min_addr == 0 || base < min_addr) &&
         !atomic_compare_exchange_weak_explicit(
             &h9_slab_route_min_addr, &min_addr, base, memory_order_acq_rel,
             memory_order_acquire)) {
  }

  uintptr_t max_addr =
      atomic_load_explicit(&h9_slab_route_max_addr, memory_order_acquire);
  while (end > max_addr &&
         !atomic_compare_exchange_weak_explicit(
             &h9_slab_route_max_addr, &max_addr, end, memory_order_acq_rel,
             memory_order_acquire)) {
  }
}

static void h9_slab_hash_insert(H9MediumSlabRoutePage* page) {
  if (!page || page->bytes != H9_SLAB_PAGE_BYTES ||
      (((uintptr_t)page->base & (uintptr_t)(H9_SLAB_PAGE_BYTES - 1u)) != 0)) {
    return;
  }
  size_t mask = H9_SLAB_ROUTE_HASH_CAP - 1u;
  size_t pos = (size_t)h9_slab_hash_base((uintptr_t)page->base) & mask;
  uintptr_t wanted = (uintptr_t)page;
  for (size_t n = 0; n < H9_SLAB_ROUTE_HASH_CAP; ++n) {
    _Atomic uintptr_t* slot = &h9_slab_base_hash[(pos + n) & mask];
    uintptr_t current = atomic_load_explicit(slot, memory_order_acquire);
    if (current == wanted) {
      return;
    }
    if (current != 0) {
      continue;
    }
    uintptr_t expected = 0;
    if (atomic_compare_exchange_strong_explicit(
            slot, &expected, wanted, memory_order_acq_rel,
            memory_order_acquire)) {
      return;
    }
  }
}

static H9MediumSlabRoutePage* h9_slab_hash_find(uintptr_t base) {
  size_t mask = H9_SLAB_ROUTE_HASH_CAP - 1u;
  size_t pos = (size_t)h9_slab_hash_base(base) & mask;
  for (size_t n = 0; n < H9_SLAB_ROUTE_HASH_CAP; ++n) {
    uintptr_t current = atomic_load_explicit(
        &h9_slab_base_hash[(pos + n) & mask], memory_order_acquire);
    if (current == 0) {
      return NULL;
    }
    H9MediumSlabRoutePage* page = (H9MediumSlabRoutePage*)current;
    if (atomic_load_explicit(&page->registered, memory_order_acquire) &&
        (uintptr_t)page->base == base) {
      return page;
    }
  }
  return NULL;
}

static H9MediumSlabRoutePage* h9_slab_find_active_thread_page(uintptr_t base,
                                                              uintptr_t addr) {
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  if (!ctx || H8_MEDIUM_CLASS_COUNT == 0) {
    return NULL;
  }
#if defined(H9_SLAB_CLASS_COVERAGE_L1)
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    H9MediumSlabRoutePage* page = h9_slab_thread_active_page(ctx, c);
    if (page && page != H9_SLAB_DISABLED_PAGE &&
        (uintptr_t)page->base == base && addr < base + page->bytes) {
      return page;
    }
  }
  return NULL;
#else
  uint32_t class_id = H8_MEDIUM_CLASS_COUNT - 1u;
  H9MediumSlabRoutePage* page = h9_slab_thread_active_page(ctx, class_id);
  if (!page || page == H9_SLAB_DISABLED_PAGE || (uintptr_t)page->base != base ||
      addr >= base + page->bytes) {
    return NULL;
  }
  return page;
#endif
}
#endif

static bool h9_slab_owner_matches_current(const H9MediumSlabRoutePage* page) {
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  if (!ctx || !ctx->owner || page->owner_word == 0) {
    return false;
  }
  return page->owner_word == h9_slab_owner_word_for(ctx->owner);
}

#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
static H8OwnerRecord* h9_slab_owner_for_page(const H9MediumSlabRoutePage* page,
                                             uint16_t* generation_out) {
  if (!page || page->owner_word == 0) {
    return NULL;
  }
  H8OwnerWord word = h8_owner_word_unpack(page->owner_word);
  if (generation_out) {
    *generation_out = word.generation;
  }
  return h8_owner_by_slot(word.slot);
}

#endif

static H9MediumSlabRoutePage* h9_slab_find(void* ptr) {
  uintptr_t addr = (uintptr_t)ptr;
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
  if (!atomic_load_explicit(&h9_slab_route_has_pages, memory_order_acquire)) {
    H8_DEBUG_INC(h9_slab_find_no_pages);
    return NULL;
  }
  uintptr_t base = addr & ~(uintptr_t)(H9_SLAB_PAGE_BYTES - 1u);
  H9MediumSlabRoutePage* page = h9_slab_find_active_thread_page(base, addr);
  if (page) {
    H8_DEBUG_INC(h9_slab_find_active_hit);
    return page;
  }
  page = h9_slab_hash_find(base);
  if (page && addr >= base && addr < base + page->bytes) {
    H8_DEBUG_INC(h9_slab_find_hash_hit);
    return page;
  }
  H8_DEBUG_INC(h9_slab_find_miss);
  return NULL;
#else
  uint32_t highwater =
      atomic_load_explicit(&h9_slab_route_highwater, memory_order_acquire);
  if (highwater > H9_SLAB_ROUTE_MAX_PAGES) {
    highwater = H9_SLAB_ROUTE_MAX_PAGES;
  }
  for (uint32_t i = 0; i < highwater; ++i) {
    H9MediumSlabRoutePage* page = &h9_slab_route_pages[i];
    if (!atomic_load_explicit(&page->registered, memory_order_acquire) ||
        !page->base || page->bytes == 0) {
      continue;
    }
    uintptr_t base = (uintptr_t)page->base;
    if (addr >= base && addr < base + page->bytes) {
      return page;
    }
  }
  return NULL;
#endif
}

static bool h9_slab_slot_index(const H9MediumSlabRoutePage* page, void* ptr,
                               size_t* slot_out) {
  uintptr_t offset = (uintptr_t)ptr - (uintptr_t)page->base;
  if (page->slot_count == 2u && page->slot_size == 65536u) {
    if (offset == 0u || offset == (uintptr_t)page->slot_size) {
      if (slot_out) {
        *slot_out = offset == 0u ? 0u : 1u;
      }
      return true;
    }
    return false;
  }
  size_t payload = (size_t)page->slot_size * (size_t)page->slot_count;
  if (offset >= payload || page->slot_size == 0 ||
      (offset % (uintptr_t)page->slot_size) != 0) {
    return false;
  }
  size_t slot = (size_t)(offset / (uintptr_t)page->slot_size);
  if (slot >= page->slot_count) {
    return false;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return true;
}

static bool h9_slab_slot_valid(const H9MediumSlabRoutePage* page,
                               size_t slot) {
  uint64_t bit = UINT64_C(1) << slot;
  bool pending =
      (atomic_load_explicit(&page->pending_bits, memory_order_acquire) & bit) !=
      0;
  uint32_t state =
      atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
  return h8_slot_state_is_allocated_tag(state) && !pending;
}

size_t h9_slab_collect_pending_page(H9MediumSlabRoutePage* page) {
  uint64_t pending =
      atomic_load_explicit(&page->pending_bits, memory_order_acquire);
  if (pending == 0) {
    return 0;
  }
  pending = atomic_exchange_explicit(&page->pending_bits, 0,
                                     memory_order_acq_rel);
  size_t collected = 0;
  for (uint32_t slot = 0; slot < page->slot_count; ++slot) {
    uint64_t bit = UINT64_C(1) << slot;
    if ((pending & bit) == 0) {
      continue;
    }
    ++collected;
    uint32_t expected = H8_SLOT_ALLOCATED;
    if (!atomic_compare_exchange_strong_explicit(
            &page->slot_state[slot], &expected, H8_SLOT_FREE | H8_SLOT_NONE,
            memory_order_acq_rel, memory_order_acquire)) {
      H8_DEBUG_INC(h9_slab_free_invalid);
    } else {
#if defined(H9_SLAB_LOCAL_FREE_BITS_L1)
      page->local_free_bits |= bit;
#endif
    }
  }
  return collected;
}

static bool h9_slab_page_empty_for_purge(H9MediumSlabRoutePage* page) {
  if (!page || !atomic_load_explicit(&page->registered, memory_order_acquire) ||
      atomic_load_explicit(&page->pending_bits, memory_order_acquire) != 0) {
    return false;
  }
  for (uint32_t slot = 0; slot < page->slot_count; ++slot) {
    uint32_t state =
        atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
    if (h8_slot_state_tag(state) != (H8_SLOT_FREE >> H8_SLOT_TAG_SHIFT)) {
      return false;
    }
  }
  return true;
}

void h9_slab_route_purge_empty_page(H9MediumSlabRoutePage* page) {
  size_t collected = h9_slab_collect_pending_page(page);
  if (collected != 0) {
    H8_DEBUG_ADD(h9_slab_pending_collect_purge_slot, collected);
  }
  if (h9_slab_page_empty_for_purge(page) &&
      h8_platform_purge(page->base, page->bytes) == 0) {
    H8_DEBUG_INC(h9_slab_page_release);
    H8_DEBUG_ADD(h9_slab_purge_bytes, page->bytes);
  }
}

static H9MediumSlabRoutePage* h9_slab_register_locked(
    void* raw_base, size_t raw_bytes, void* base, size_t bytes,
    uint32_t class_id, uint32_t slot_size, uint16_t slot_count,
    H8OwnerRecord* owner, uint32_t initial_state) {
  if (!base || slot_size == 0 || slot_count == 0 ||
      slot_count > H9_SLAB_ROUTE_MAX_SLOTS ||
      (size_t)slot_size * (size_t)slot_count > bytes) {
    return NULL;
  }
  for (uint32_t i = 0; i < H9_SLAB_ROUTE_MAX_PAGES; ++i) {
    H9MediumSlabRoutePage* page = &h9_slab_route_pages[i];
    if (atomic_load_explicit(&page->registered, memory_order_acquire)) {
      continue;
    }
    page->raw_base = raw_base ? raw_base : base;
    page->raw_bytes = raw_bytes ? raw_bytes : bytes;
    page->base = (uint8_t*)base;
    page->bytes = bytes;
    page->class_id = class_id;
    page->slot_size = slot_size;
    page->slot_count = slot_count;
#if defined(H9_SLAB_ALLOC_CURSOR_L1)
    page->alloc_cursor = 0;
#endif
#if defined(H9_SLAB_LOCAL_FREE_BITS_L1)
    page->local_free_bits =
        initial_state == (H8_SLOT_FREE | H8_SLOT_NONE)
            ? (slot_count == 64u ? UINT64_MAX
                                 : ((UINT64_C(1) << slot_count) - 1u))
            : 0u;
#endif
    page->owner_word = h9_slab_owner_word_for(owner);
    atomic_store_explicit(&page->pending_bits, 0, memory_order_release);
    atomic_store_explicit(&page->qstate, H8_Q_IDLE, memory_order_release);
    page->next_pending = NULL;
    for (uint32_t s = 0; s < H9_SLAB_ROUTE_MAX_SLOTS; ++s) {
      uint32_t state = s < slot_count ? initial_state : H8_SLOT_NEVER_USED;
      atomic_store_explicit(&page->slot_state[s], state, memory_order_release);
    }
    atomic_store_explicit(&page->registered, true, memory_order_release);
#if defined(H8_ENABLE_DEBUG_STATS) && defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
    h9_slab_debug_note_register(bytes, raw_bytes ? raw_bytes : bytes);
#endif
    h9_slab_raise_highwater(i + 1u);
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
    h9_slab_note_range((uintptr_t)page->base,
                       (uintptr_t)page->base + page->bytes);
    h9_slab_hash_insert(page);
    atomic_store_explicit(&h9_slab_route_has_pages, true,
                          memory_order_release);
#endif
    return page;
  }
  return NULL;
}

H9MediumSlabRoutePage* h9_slab_route_register_page(
    void* raw_base, size_t raw_bytes, void* base, size_t bytes,
    uint32_t class_id, uint32_t slot_size, uint16_t slot_count,
    H8OwnerRecord* owner, uint32_t initial_state) {
  h9_slab_route_init();
  h8_platform_mutex_lock(&h9_slab_route_lock);
  H9MediumSlabRoutePage* page = h9_slab_register_locked(
      raw_base, raw_bytes, base, bytes, class_id, slot_size, slot_count, owner,
      initial_state);
  h8_platform_mutex_unlock(&h9_slab_route_lock);
  return page;
}

H8RouteKind h9_slab_route_inner(void* ptr) {
  if (!ptr) {
    return H8_ROUTE_INVALID;
  }
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1) && \
    defined(H9_SLAB_NO_PAGE_FAST_REJECT_L1)
  if (!atomic_load_explicit(&h9_slab_route_has_pages, memory_order_acquire)) {
    return H8_ROUTE_MISS;
  }
#endif
  h9_slab_route_init();
  H9MediumSlabRoutePage* page = h9_slab_find(ptr);
  if (!page) {
    return H8_ROUTE_MISS;
  }
  size_t slot = 0;
  H8RouteKind route = H8_ROUTE_INVALID;
  if (h9_slab_slot_index(page, ptr, &slot) && h9_slab_slot_valid(page, slot)) {
    H8_DEBUG_INC(h9_slab_route_valid);
    route = H8_ROUTE_VALID;
  } else {
    H8_DEBUG_INC(h9_slab_route_invalid);
  }
  return route;
}

bool h9_slab_free_inner(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  if (!ptr) {
    return false;
  }
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1) && \
    defined(H9_SLAB_NO_PAGE_FAST_REJECT_L1)
  if (!atomic_load_explicit(&h9_slab_route_has_pages, memory_order_acquire)) {
    return false;
  }
#endif
  h9_slab_route_init();
  H9MediumSlabRoutePage* page = h9_slab_find(ptr);
  if (!page) {
    return false;
  }
  if (owned_out) {
    *owned_out = true;
  }
  size_t slot = 0;
  if (!h9_slab_slot_index(page, ptr, &slot)) {
    H8_DEBUG_INC(h9_slab_free_invalid);
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((atomic_load_explicit(&page->pending_bits, memory_order_acquire) & bit) !=
      0) {
    H8_DEBUG_INC(h9_slab_free_invalid);
    return false;
  }
  if (!h9_slab_owner_matches_current(page)) {
    uint32_t state =
        atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
    if (!h8_slot_state_is_allocated_tag(state)) {
      H8_DEBUG_INC(h9_slab_free_invalid);
      return false;
    }
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
    uint16_t owner_generation = 0;
    H8OwnerRecord* owner = h9_slab_owner_for_page(page, &owner_generation);
    bool lease_entered =
        owner && h8_owner_publish_enter(owner, owner_generation);
    if (!lease_entered) {
      uint32_t expected = H8_SLOT_ALLOCATED;
      if (!atomic_compare_exchange_strong_explicit(
              &page->slot_state[slot], &expected, H8_SLOT_FREE | H8_SLOT_NONE,
              memory_order_acq_rel, memory_order_acquire)) {
        H8_DEBUG_INC(h9_slab_free_invalid);
        return false;
      }
      H8_DEBUG_INC(h9_slab_free_valid);
      H8_DEBUG_INC(h9_slab_owner_death_direct_free);
      h9_slab_route_purge_empty_page(page);
      return true;
    }
    uint64_t old_pending =
        atomic_fetch_or_explicit(&page->pending_bits, bit, memory_order_acq_rel);
    if ((old_pending & bit) != 0) {
      H8_DEBUG_INC(h9_slab_free_invalid);
      h8_owner_publish_exit(owner);
      return false;
    }
    H8_DEBUG_INC(h9_slab_remote_claim);
    if (old_pending == 0) {
      H8_DEBUG_INC(h9_slab_remote_claim_first);
    } else {
      H8_DEBUG_INC(h9_slab_remote_claim_coalesced);
    }
    state = atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
    if (!h8_slot_state_is_allocated_tag(state)) {
      uint8_t qstate = atomic_load_explicit(&page->qstate,
                                            memory_order_acquire);
      bool draining = qstate == H8_Q_DRAINING ||
                      qstate == H8_Q_DRAINING_DIRTY;
      uint64_t rollback = atomic_fetch_and_explicit(&page->pending_bits, ~bit,
                                                    memory_order_acq_rel);
      if ((rollback & bit) != 0 && !draining) {
        H8_DEBUG_INC(h9_slab_free_invalid);
        h8_owner_publish_exit(owner);
        return false;
      }
      H8_DEBUG_INC(h9_slab_free_remote);
      H8_DEBUG_INC(h9_slab_free_valid);
      h8_owner_publish_exit(owner);
      return true;
    }
    if (old_pending == 0) {
      h9_slab_signal_work(owner, page);
    }
    H8_DEBUG_INC(h9_slab_free_remote);
    H8_DEBUG_INC(h9_slab_free_valid);
    h8_owner_publish_exit(owner);
    return true;
#else
    uint64_t old_pending =
        atomic_fetch_or_explicit(&page->pending_bits, bit, memory_order_acq_rel);
    if ((old_pending & bit) != 0) {
      H8_DEBUG_INC(h9_slab_free_invalid);
      return false;
    }
    state = atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
    if (!h8_slot_state_is_allocated_tag(state)) {
      atomic_fetch_and_explicit(&page->pending_bits, ~bit,
                                memory_order_acq_rel);
      H8_DEBUG_INC(h9_slab_free_invalid);
      return false;
    }
    H8_DEBUG_INC(h9_slab_free_remote);
    H8_DEBUG_INC(h9_slab_free_valid);
    return true;
#endif
  }
#if defined(H9_SLAB_LOCAL_STORE_MUTATION_L1)
  uint32_t state =
      atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
  if (state != H8_SLOT_ALLOCATED) {
    H8_DEBUG_INC(h9_slab_free_invalid);
    return false;
  }
  atomic_store_explicit(&page->slot_state[slot], H8_SLOT_FREE | H8_SLOT_NONE,
                        memory_order_release);
#else
  uint32_t expected = H8_SLOT_ALLOCATED;
  if (!atomic_compare_exchange_strong_explicit(
          &page->slot_state[slot], &expected, H8_SLOT_FREE | H8_SLOT_NONE,
          memory_order_acq_rel, memory_order_acquire)) {
    H8_DEBUG_INC(h9_slab_free_invalid);
    return false;
  }
#endif
#if defined(H9_SLAB_LOCAL_FREE_BITS_L1)
  page->local_free_bits |= bit;
#endif
  H8_DEBUG_INC(h9_slab_free_local);
  H8_DEBUG_INC(h9_slab_free_valid);
  return true;
}

bool h9_slab_usable_size_inner(void* ptr, size_t* usable_out,
                               bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  if (!ptr) {
    return false;
  }
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1) && \
    defined(H9_SLAB_NO_PAGE_FAST_REJECT_L1)
  if (!atomic_load_explicit(&h9_slab_route_has_pages, memory_order_acquire)) {
    return false;
  }
#endif
  h9_slab_route_init();
  H9MediumSlabRoutePage* page = h9_slab_find(ptr);
  if (!page) {
    return false;
  }
  if (owned_out) {
    *owned_out = true;
  }
  size_t slot = 0;
  bool valid =
      h9_slab_slot_index(page, ptr, &slot) && h9_slab_slot_valid(page, slot);
  if (valid && usable_out) {
    *usable_out = page->slot_size;
    H8_DEBUG_INC(h9_slab_usable_valid);
  }
  return valid;
}

H9MediumSlabRoutePage* h9_slab_route_test_register(
    void* base, size_t bytes, uint32_t class_id, uint32_t slot_size,
    uint16_t slot_count, H8OwnerRecord* owner) {
  if (!base || slot_size == 0 || slot_count == 0 ||
      slot_count > H9_SLAB_ROUTE_MAX_SLOTS ||
      (size_t)slot_size * (size_t)slot_count > bytes) {
    return NULL;
  }
  h9_slab_route_init();
  return h9_slab_route_register_page(base, bytes, base, bytes, class_id,
                                     slot_size, slot_count, owner,
                                     H8_SLOT_ALLOCATED);
}

#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
void h9_slab_flush_owner(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  h9_slab_route_init();
  uint64_t owner_word = h9_slab_owner_word_for(owner);
  uint32_t highwater =
      atomic_load_explicit(&h9_slab_route_highwater, memory_order_acquire);
  if (highwater > H9_SLAB_ROUTE_MAX_PAGES) {
    highwater = H9_SLAB_ROUTE_MAX_PAGES;
  }
  for (uint32_t i = 0; i < highwater; ++i) {
    H9MediumSlabRoutePage* page = &h9_slab_route_pages[i];
    if (atomic_load_explicit(&page->registered, memory_order_acquire) &&
        page->owner_word == owner_word) {
      h9_slab_route_purge_empty_page(page);
    }
  }
  h9_slab_owner_destroy(owner);
}
#endif

void h9_slab_route_test_unregister(H9MediumSlabRoutePage* page) {
  if (!page) {
    return;
  }
  h9_slab_route_init();
  h8_platform_mutex_lock(&h9_slab_route_lock);
  atomic_store_explicit(&page->registered, false, memory_order_release);
  memset(page, 0, sizeof(*page));
  h8_platform_mutex_unlock(&h9_slab_route_lock);
}

#else
typedef int h9_slab_route_translation_unit_silence;
#endif
