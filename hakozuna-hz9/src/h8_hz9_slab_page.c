#include "h8_internal.h"

#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)

#include "h8_hz9_slab_route_internal.h"

#if defined(H9_SLAB_CLASS_COVERAGE_L1) && !defined(H9_SLAB_CLASS_MIN_ID)
#define H9_SLAB_CLASS_MIN_ID 2u
#endif

typedef enum H9SlabDirectSource {
  H9_SLAB_DIRECT_ACTIVE = 0,
  H9_SLAB_DIRECT_PARTIAL = 1,
  H9_SLAB_DIRECT_AFTER_OWNER = 2,
  H9_SLAB_DIRECT_NEW_PAGE = 3
} H9SlabDirectSource;

static void h9_slab_note_direct_try(H9SlabDirectSource source) {
  switch (source) {
    case H9_SLAB_DIRECT_ACTIVE:
      H8_DEBUG_INC(h9_slab_direct_try_active);
      break;
    case H9_SLAB_DIRECT_PARTIAL:
      H8_DEBUG_INC(h9_slab_direct_try_partial);
      break;
    case H9_SLAB_DIRECT_AFTER_OWNER:
      H8_DEBUG_INC(h9_slab_direct_try_after_owner);
      break;
    case H9_SLAB_DIRECT_NEW_PAGE:
      H8_DEBUG_INC(h9_slab_direct_try_new_page);
      break;
  }
}

static void h9_slab_note_direct_pending(H9SlabDirectSource source,
                                        size_t slots) {
  (void)slots;
  switch (source) {
    case H9_SLAB_DIRECT_ACTIVE:
      H8_DEBUG_ADD(h9_slab_direct_pending_active_slot, slots);
      break;
    case H9_SLAB_DIRECT_PARTIAL:
      H8_DEBUG_ADD(h9_slab_direct_pending_partial_slot, slots);
      break;
    case H9_SLAB_DIRECT_AFTER_OWNER:
      H8_DEBUG_ADD(h9_slab_direct_pending_after_owner_slot, slots);
      break;
    case H9_SLAB_DIRECT_NEW_PAGE:
      H8_DEBUG_ADD(h9_slab_direct_pending_new_page_slot, slots);
      break;
  }
}

static uint8_t* h9_slab_reserve_aligned_rw(size_t size, size_t align,
                                           void** raw_out,
                                           size_t* raw_bytes_out) {
  if (size == 0 || align == 0 || (align & (align - 1u)) != 0) {
    return NULL;
  }
  size_t reserve = size + align;
  void* raw = h8_platform_reserve(reserve);
  if (!raw) {
    return NULL;
  }
  uintptr_t aligned =
      ((uintptr_t)raw + (uintptr_t)align - 1u) & ~((uintptr_t)align - 1u);
  if (h8_platform_commit((void*)aligned, size) != 0) {
    h8_platform_release(raw, reserve);
    return NULL;
  }
  if (raw_out) {
    *raw_out = raw;
  }
  if (raw_bytes_out) {
    *raw_bytes_out = reserve;
  }
  return (uint8_t*)aligned;
}

#if defined(H9_SLAB_THREAD_SIDECAR_L1) && \
    !defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
static H9SlabThreadState* h9_slab_thread_state(H8ThreadCtx* ctx,
                                               bool create) {
  if (!ctx) {
    return NULL;
  }
  if (!ctx->h9_slab_state && create) {
    ctx->h9_slab_state = h8_sys_calloc(1, sizeof(*ctx->h9_slab_state));
  }
  return ctx->h9_slab_state;
}
#endif

H9MediumSlabRoutePage* h9_slab_thread_active_page(H8ThreadCtx* ctx,
                                                  uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
#if defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
  (void)ctx;
  return NULL;
#elif defined(H9_SLAB_THREAD_SIDECAR_L1)
  H9SlabThreadState* state = h9_slab_thread_state(ctx, false);
  return state ? state->active[class_id] : NULL;
#else
  return ctx ? ctx->h9_slab_active[class_id] : NULL;
#endif
}

H9MediumSlabRoutePage* h9_slab_thread_page_at(H8ThreadCtx* ctx,
                                              uint32_t class_id,
                                              uint32_t index) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT || index >= H9_SLAB_LOCAL_PAGE_CAP) {
    return NULL;
  }
#if defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
  (void)ctx;
  return NULL;
#elif defined(H9_SLAB_THREAD_SIDECAR_L1)
  H9SlabThreadState* state = h9_slab_thread_state(ctx, false);
  return state ? state->pages[class_id][index] : NULL;
#else
  return ctx ? ctx->h9_slab_pages[class_id][index] : NULL;
#endif
}

static bool h9_slab_thread_prepare(H8ThreadCtx* ctx) {
#if defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
  (void)ctx;
  return false;
#elif defined(H9_SLAB_THREAD_SIDECAR_L1)
  return h9_slab_thread_state(ctx, true) != NULL;
#else
  return ctx != NULL;
#endif
}

void h9_slab_thread_set_active_page(H8ThreadCtx* ctx, uint32_t class_id,
                                    H9MediumSlabRoutePage* page) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
#if defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
  (void)ctx;
  (void)page;
#elif defined(H9_SLAB_THREAD_SIDECAR_L1)
  H9SlabThreadState* state = h9_slab_thread_state(ctx, page != NULL);
  if (state) {
    state->active[class_id] = page;
  }
#else
  if (ctx) {
    ctx->h9_slab_active[class_id] = page;
  }
#endif
}

bool h9_slab_thread_install_page(H8ThreadCtx* ctx, uint32_t class_id,
                                 H9MediumSlabRoutePage* page, uint32_t cap) {
  if (!page || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  if (cap > H9_SLAB_LOCAL_PAGE_CAP) {
    cap = H9_SLAB_LOCAL_PAGE_CAP;
  }
#if defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
  (void)ctx;
  (void)cap;
  return false;
#elif defined(H9_SLAB_THREAD_SIDECAR_L1)
  H9SlabThreadState* state = h9_slab_thread_state(ctx, true);
  if (!state) {
    return false;
  }
  for (uint32_t i = 0; i < cap; ++i) {
    if (!state->pages[class_id][i]) {
      state->pages[class_id][i] = page;
      state->active[class_id] = page;
      H8_DEBUG_INC(h9_slab_thread_page_install);
      H8_DEBUG_INC(h9_slab_thread_page_install_class[class_id]);
      return true;
    }
  }
#else
  if (!ctx) {
    return false;
  }
  for (uint32_t i = 0; i < cap; ++i) {
    if (!ctx->h9_slab_pages[class_id][i]) {
      ctx->h9_slab_pages[class_id][i] = page;
      ctx->h9_slab_active[class_id] = page;
      H8_DEBUG_INC(h9_slab_thread_page_install);
      H8_DEBUG_INC(h9_slab_thread_page_install_class[class_id]);
      return true;
    }
  }
#endif
  return false;
}

static H9MediumSlabRoutePage* h9_slab_create_page(H8ThreadCtx* ctx,
                                                  uint32_t class_id) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!ctx || !ctx->owner || !spec) {
    return NULL;
  }
#if defined(H9_SLAB_CLASS_COVERAGE_L1)
#if H9_SLAB_CLASS_MIN_ID > 0u
  if (class_id < H9_SLAB_CLASS_MIN_ID) {
    return NULL;
  }
#endif
  if (spec->slot_size == 0u || spec->slot_size > H9_SLAB_PAGE_BYTES) {
    return NULL;
  }
  uint16_t slab_slots = (uint16_t)(H9_SLAB_PAGE_BYTES / spec->slot_size);
#else
  if (spec->slot_size != 65536u) {
    return NULL;
  }
  uint16_t slab_slots = spec->slot_count;
#endif
  if (slab_slots == 0u || slab_slots > H9_SLAB_ROUTE_MAX_SLOTS) {
    return NULL;
  }
  if (!h9_slab_owner_prepare(ctx->owner)) {
    return NULL;
  }
  void* raw = NULL;
  size_t raw_bytes = 0;
  uint8_t* base =
      h9_slab_reserve_aligned_rw(H9_SLAB_PAGE_BYTES, H9_SLAB_PAGE_BYTES, &raw,
                                 &raw_bytes);
  if (!base) {
    return NULL;
  }
  H9MediumSlabRoutePage* page = h9_slab_route_register_page(
      raw, raw_bytes, base, H9_SLAB_PAGE_BYTES, class_id, spec->slot_size,
      slab_slots, ctx->owner, H8_SLOT_FREE | H8_SLOT_NONE);
  if (!page) {
    h8_platform_release(raw, raw_bytes);
    return NULL;
  }
  H8_DEBUG_INC(h9_slab_alloc_page_create);
  return page;
}

static bool h9_slab_collect_pending(H9MediumSlabRoutePage* page,
                                    H9SlabDirectSource source) {
  uint64_t pending =
      atomic_load_explicit(&page->pending_bits, memory_order_acquire);
  if (pending == 0) {
    return false;
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
  if (collected != 0) {
    H8_DEBUG_INC(h9_slab_pending_collect_direct_call);
    H8_DEBUG_ADD(h9_slab_pending_collect_direct_slot, collected);
    h9_slab_note_direct_pending(source, collected);
  }
  return collected != 0;
}

static void* h9_slab_alloc_free_slot(H9MediumSlabRoutePage* page,
                                     bool free_first,
                                     bool pending_retry) {
  (void)free_first;
  (void)pending_retry;
  const uint32_t count = page->slot_count;
#if defined(H9_SLAB_LOCAL_FREE_BITS_L1)
  uint64_t bits = count == 64u ? UINT64_MAX : ((UINT64_C(1) << count) - 1u);
  bits &= page->local_free_bits;
  if (bits == 0) {
    H8_DEBUG_ADD(h9_slab_alloc_scan_steps, 1u);
    H8_DEBUG_INC(h9_slab_alloc_scan_miss);
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(bits);
  uint64_t bit = UINT64_C(1) << slot;
  page->local_free_bits &= ~bit;
#if defined(H8_ENABLE_DEBUG_STATS)
  uint32_t state =
      atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
  if (state != (H8_SLOT_FREE | H8_SLOT_NONE)) {
    H8_DEBUG_INC(h9_slab_free_invalid);
    return NULL;
  }
#endif
  atomic_store_explicit(&page->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
  H8_DEBUG_ADD(h9_slab_alloc_scan_steps, 1u);
  H8_DEBUG_INC(h9_slab_alloc_hit);
  if (pending_retry) {
    H8_DEBUG_INC(h9_slab_alloc_pending_retry_hit);
  } else if (free_first) {
    H8_DEBUG_INC(h9_slab_alloc_free_first_hit);
  }
  return page->base + ((size_t)slot * (size_t)page->slot_size);
#else
#if defined(H9_SLAB_ALLOC_CURSOR_L1)
  uint32_t start = page->alloc_cursor;
  if (start >= count) {
    start = 0;
  }
#else
  const uint32_t start = 0;
#endif
  size_t scanned = 0;
  for (uint32_t n = 0; n < count; ++n) {
    ++scanned;
    uint32_t slot = start + n;
    if (slot >= count) {
      slot -= count;
    }
#if defined(H9_SLAB_LOCAL_STORE_MUTATION_L1)
    uint32_t state =
        atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
    if (state != (H8_SLOT_FREE | H8_SLOT_NONE)) {
      continue;
    }
    atomic_store_explicit(&page->slot_state[slot], H8_SLOT_ALLOCATED,
                          memory_order_release);
#else
    uint32_t expected = H8_SLOT_FREE | H8_SLOT_NONE;
    if (!atomic_compare_exchange_strong_explicit(
            &page->slot_state[slot], &expected, H8_SLOT_ALLOCATED,
            memory_order_acq_rel, memory_order_acquire)) {
      continue;
    }
#endif
    H8_DEBUG_ADD(h9_slab_alloc_scan_steps, scanned);
    H8_DEBUG_INC(h9_slab_alloc_hit);
    if (pending_retry) {
      H8_DEBUG_INC(h9_slab_alloc_pending_retry_hit);
    } else if (free_first) {
      H8_DEBUG_INC(h9_slab_alloc_free_first_hit);
    }
#if defined(H9_SLAB_ALLOC_CURSOR_L1)
    uint32_t next = slot + 1u;
    page->alloc_cursor = (uint16_t)(next < count ? next : 0u);
#endif
    return page->base + ((size_t)slot * (size_t)page->slot_size);
  }
  H8_DEBUG_ADD(h9_slab_alloc_scan_steps, scanned);
  H8_DEBUG_INC(h9_slab_alloc_scan_miss);
  return NULL;
#endif
}

static void* h9_slab_alloc_from_page(H9MediumSlabRoutePage* page,
                                     H9SlabDirectSource source) {
  h9_slab_note_direct_try(source);
#if defined(H9_SLAB_ALLOC_FREE_FIRST_L1)
  void* ptr = h9_slab_alloc_free_slot(page, true, false);
  if (ptr) {
    return ptr;
  }
  if (h9_slab_collect_pending(page, source)) {
    ptr = h9_slab_alloc_free_slot(page, false, true);
    if (ptr) {
      return ptr;
    }
  }
#else
  h9_slab_collect_pending(page, source);
  void* ptr = h9_slab_alloc_free_slot(page, false, false);
  if (ptr) {
    return ptr;
  }
#endif
  H8_DEBUG_INC(h9_slab_alloc_fallback);
  return NULL;
}

static void* h9_slab_try_page(H8ThreadCtx* ctx, uint32_t class_id,
                              H9MediumSlabRoutePage* page,
                              H9SlabDirectSource source) {
  if (!page || page == H9_SLAB_DISABLED_PAGE ||
      !atomic_load_explicit(&page->registered, memory_order_acquire)) {
    return NULL;
  }
  void* ptr = h9_slab_alloc_from_page(page, source);
  if (ptr) {
    h9_slab_thread_set_active_page(ctx, class_id, page);
  }
  return ptr;
}

static void h9_slab_collect_owner_pending_if_needed(H8OwnerRecord* owner) {
  if (!owner || h9_slab_owner_pending_count(owner) == 0) {
    H8_DEBUG_INC(h9_slab_owner_collect_skip);
    return;
  }
  H8_DEBUG_INC(h9_slab_owner_collect_probe);
  if (h9_slab_collect_owner_pending(owner, H9_SLAB_LOCAL_PAGE_CAP) != 0) {
    H8_DEBUG_INC(h9_slab_owner_collect_hit);
  }
}

void* h9_slab_alloc(H8ThreadCtx* ctx, uint32_t class_id) {
  if (!ctx || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
#if defined(H9_SLAB_CLASS_COVERAGE_L1)
  if (!spec) {
    return NULL;
  }
#if H9_SLAB_CLASS_MIN_ID > 0u
  if (class_id < H9_SLAB_CLASS_MIN_ID) {
    return NULL;
  }
#endif
#else
  if (!spec || spec->slot_size != 65536u) {
    return NULL;
  }
#endif
  H9MediumSlabRoutePage* page = h9_slab_thread_active_page(ctx, class_id);
  if (page == H9_SLAB_DISABLED_PAGE) {
    return NULL;
  }
  uint32_t cap = H9_SLAB_LOCAL_PAGE_CAP;
#if defined(H9_SLAB_MIDCLASS_PAGE_CAP_L1)
  if (class_id != 2u && class_id != 3u && cap > 4u) {
    cap = 4u;
  }
#endif
  void* ptr = h9_slab_try_page(ctx, class_id, page, H9_SLAB_DIRECT_ACTIVE);
  if (ptr) {
    return ptr;
  }
  for (uint32_t i = 0; i < cap; ++i) {
    page = h9_slab_thread_page_at(ctx, class_id, i);
    ptr = h9_slab_try_page(ctx, class_id, page, H9_SLAB_DIRECT_PARTIAL);
    if (ptr) {
      return ptr;
    }
  }
  h9_slab_collect_owner_pending_if_needed(ctx->owner);
  for (uint32_t i = 0; i < cap; ++i) {
    page = h9_slab_thread_page_at(ctx, class_id, i);
    ptr = h9_slab_try_page(ctx, class_id, page,
                           H9_SLAB_DIRECT_AFTER_OWNER);
    if (ptr) {
      return ptr;
    }
  }
  if (h9_slab_thread_page_at(ctx, class_id, cap - 1u)) {
    H8_DEBUG_INC(h9_slab_alloc_fallback);
    H8_DEBUG_INC(h9_slab_thread_cap_fallback);
    H8_DEBUG_INC(h9_slab_thread_cap_fallback_class[class_id]);
    return NULL;
  }
  if (!h9_slab_thread_prepare(ctx)) {
    H8_DEBUG_INC(h9_slab_alloc_fallback);
    return NULL;
  }
  page = h9_slab_create_page(ctx, class_id);
  if (!h9_slab_thread_install_page(ctx, class_id, page, cap)) {
    h9_slab_thread_set_active_page(ctx, class_id, H9_SLAB_DISABLED_PAGE);
    return NULL;
  }
  return h9_slab_alloc_from_page(page, H9_SLAB_DIRECT_NEW_PAGE);
}

void h9_slab_flush_thread(H8ThreadCtx* ctx) {
  if (!ctx) {
    return;
  }
  bool has_pages = h9_slab_maybe_has_pages_hot();
  if (!has_pages) {
#if defined(H9_SLAB_THREAD_SIDECAR_L1) && \
    !defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
    h8_sys_free(ctx->h9_slab_state);
    ctx->h9_slab_state = NULL;
#endif
    return;
  }
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    for (uint32_t i = 0; i < H9_SLAB_LOCAL_PAGE_CAP; ++i) {
      H9MediumSlabRoutePage* page = h9_slab_thread_page_at(ctx, c, i);
      if (page && page != H9_SLAB_DISABLED_PAGE) {
        h9_slab_route_purge_empty_page(page);
      }
#if defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
      (void)ctx;
#elif defined(H9_SLAB_THREAD_SIDECAR_L1)
      H9SlabThreadState* state = h9_slab_thread_state(ctx, false);
      if (state) {
        state->pages[c][i] = NULL;
      }
#else
      ctx->h9_slab_pages[c][i] = NULL;
#endif
    }
    h9_slab_thread_set_active_page(ctx, c, NULL);
  }
#if defined(H9_SLAB_THREAD_SIDECAR_L1) && \
    !defined(H9_SLAB_LAYOUT_NEUTRAL_PROOF_L1)
  h8_sys_free(ctx->h9_slab_state);
  ctx->h9_slab_state = NULL;
#endif
}

#else
typedef int h9_slab_page_translation_unit_silence;
#endif
