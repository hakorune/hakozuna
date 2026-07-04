#include "h8_internal.h"

#if defined(H9_OWNER_LOCAL_PAGE_POOL_L0)

typedef enum H9OwnerPageMode {
  H9_OWNER_PAGE_PURE_LOCAL = 0,
  H9_OWNER_PAGE_REMOTE_SEEN = 1,
  H9_OWNER_PAGE_DRAINING = 2,
  H9_OWNER_PAGE_DETACHED = 3
} H9OwnerPageMode;

typedef enum H9OwnerPageRoute {
  H9_OWNER_PAGE_ROUTE_MISS = 0,
  H9_OWNER_PAGE_ROUTE_VALID = 1,
  H9_OWNER_PAGE_ROUTE_INVALID = 2
} H9OwnerPageRoute;

typedef struct H9OwnerLocalPage {
  H8OwnerRecord* owner;
  uint64_t owner_word;
  void* base;
  size_t page_bytes;
  uint16_t class_id;
  uint16_t slot_count;
  _Atomic uint64_t local_free_bits;
  _Atomic uint64_t pending_bits;
  _Atomic uint32_t mode;
  struct H9OwnerLocalPage* next_global;
} H9OwnerLocalPage;

typedef struct H9OwnerPageThreadState {
  H9OwnerLocalPage* active[H8_MEDIUM_CLASS_COUNT];
  uint64_t disabled_class_bits;
  size_t retained_bytes;
} H9OwnerPageThreadState;

typedef struct H9OwnerPageSidecar {
  H8OwnerRecord* owner;
  _Atomic(H9OwnerLocalPage*) pending_head;
  _Atomic size_t pending_count;
} H9OwnerPageSidecar;

static _Thread_local H9OwnerPageThreadState* h9_owner_page_tls_state;
#if defined(H9_OWNER_LOCAL_PAGE_POOL_PURE_LOCAL_L1)
static h8_platform_mutex_t h9_owner_page_global_lock = H8_PLATFORM_MUTEX_INIT;
static H9OwnerLocalPage* h9_owner_page_global_head;

#if defined(H9_OWNER_LOCAL_PAGE_POOL_OWNER_FAST_BITS_L1) && \
    !defined(H9_OWNER_LOCAL_PAGE_POOL_OWNER_FAST_BITS_MAX_CLASS_ID)
#define H9_OWNER_LOCAL_PAGE_POOL_OWNER_FAST_BITS_MAX_CLASS_ID 5u
#endif

static void h9_owner_page_release_thread_pages(H9OwnerPageThreadState* state);

static uint64_t h9_owner_page_owner_word_for(const H8OwnerRecord* owner) {
  if (!owner || owner->slot >= H8_OWNER_MAX) {
    return 0;
  }
  H8OwnerWord word = h8_owner_word_make((uint8_t)owner->slot,
                                        (uint16_t)owner->generation,
                                        H8_SPAN_OWNED_ACTIVE, 0);
  return h8_owner_word_pack(word);
}

static bool h9_owner_page_owner_matches_ctx(const H9OwnerLocalPage* page,
                                            const H8ThreadCtx* ctx) {
  if (!page || !ctx || !ctx->owner || page->owner_word == 0) {
    return false;
  }
  return page->owner_word == h9_owner_page_owner_word_for(ctx->owner);
}

/* TLS state is private to the owner thread; no H8ThreadCtx field is added. */
static H9OwnerPageThreadState* h9_owner_page_ensure_thread_state(void) {
  H8_DEBUG_INC(h9_owner_page_api_state_ensure);
  if (h9_owner_page_tls_state) {
    return h9_owner_page_tls_state;
  }
  H9OwnerPageThreadState* state =
      h8_sys_calloc(1, sizeof(H9OwnerPageThreadState));
  if (!state) {
    H8_DEBUG_INC(h9_owner_page_api_state_oom);
    return NULL;
  }
  h9_owner_page_tls_state = state;
  H8_DEBUG_INC(h9_owner_page_api_state_create);
  return state;
}

static void h9_owner_page_free_thread_state(void) {
  H9OwnerPageThreadState* state = h9_owner_page_tls_state;
  h9_owner_page_tls_state = NULL;
  if (state) {
    h9_owner_page_release_thread_pages(state);
    h8_sys_free(state);
    H8_DEBUG_INC(h9_owner_page_api_state_free);
  }
}

/* Route only searches HZ9 pages already installed in this thread state. */
static H9OwnerPageRoute __attribute__((unused)) h9_owner_page_route_thread(
    H9OwnerPageThreadState* state, const void* ptr,
    H9OwnerLocalPage** page_out, uint32_t* slot_out) {
  if (page_out) {
    *page_out = NULL;
  }
  if (slot_out) {
    *slot_out = 0;
  }
  if (!state || !ptr) {
    return H9_OWNER_PAGE_ROUTE_MISS;
  }
  uintptr_t addr = (uintptr_t)ptr;
  for (uint32_t i = 0; i < H8_MEDIUM_CLASS_COUNT; ++i) {
    H9OwnerLocalPage* page = state->active[i];
    if (!page || !page->base || page->class_id >= H8_MEDIUM_CLASS_COUNT) {
      continue;
    }
    uintptr_t base = (uintptr_t)page->base;
    uintptr_t end = base + page->page_bytes;
    if (end <= base || addr < base || addr >= end) {
      continue;
    }
    const H8MediumClassSpec* spec = h8_medium_class_spec(page->class_id);
    if (!spec || spec->slot_size == 0u || page->slot_count == 0u) {
      return H9_OWNER_PAGE_ROUTE_INVALID;
    }
    uintptr_t offset = addr - base;
    uintptr_t slot_size = (uintptr_t)spec->slot_size;
    uintptr_t payload = slot_size * (uintptr_t)page->slot_count;
    if (offset >= payload || (offset % slot_size) != 0u) {
      return H9_OWNER_PAGE_ROUTE_INVALID;
    }
    uint32_t slot = (uint32_t)(offset / slot_size);
    if (slot >= page->slot_count || slot >= 64u) {
      return H9_OWNER_PAGE_ROUTE_INVALID;
    }
    if (page_out) {
      *page_out = page;
    }
    if (slot_out) {
      *slot_out = slot;
    }
    return H9_OWNER_PAGE_ROUTE_VALID;
  }
  return H9_OWNER_PAGE_ROUTE_MISS;
}

static void h9_owner_page_register_global(H9OwnerLocalPage* page) {
  h8_platform_mutex_lock(&h9_owner_page_global_lock);
  page->next_global = h9_owner_page_global_head;
  h9_owner_page_global_head = page;
  h8_platform_mutex_unlock(&h9_owner_page_global_lock);
}

static void h9_owner_page_unregister_global_locked(H9OwnerLocalPage* page) {
  H9OwnerLocalPage** cur = &h9_owner_page_global_head;
  while (*cur) {
    if (*cur == page) {
      *cur = page->next_global;
      page->next_global = NULL;
      return;
    }
    cur = &(*cur)->next_global;
  }
}

static H9OwnerPageRoute h9_owner_page_route_global_locked(
    const void* ptr, H9OwnerLocalPage** page_out, uint32_t* slot_out) {
  if (page_out) {
    *page_out = NULL;
  }
  if (slot_out) {
    *slot_out = 0;
  }
  if (!ptr) {
    return H9_OWNER_PAGE_ROUTE_MISS;
  }
  uintptr_t addr = (uintptr_t)ptr;
  for (H9OwnerLocalPage* page = h9_owner_page_global_head; page;
       page = page->next_global) {
    if (!page->base || page->class_id >= H8_MEDIUM_CLASS_COUNT) {
      continue;
    }
    uintptr_t base = (uintptr_t)page->base;
    uintptr_t end = base + page->page_bytes;
    if (end <= base || addr < base || addr >= end) {
      continue;
    }
    const H8MediumClassSpec* spec = h8_medium_class_spec(page->class_id);
    if (!spec || spec->slot_size == 0u || page->slot_count == 0u) {
      return H9_OWNER_PAGE_ROUTE_INVALID;
    }
    uintptr_t offset = addr - base;
    uintptr_t slot_size = (uintptr_t)spec->slot_size;
    uintptr_t payload = slot_size * (uintptr_t)page->slot_count;
    if (offset >= payload || (offset % slot_size) != 0u) {
      return H9_OWNER_PAGE_ROUTE_INVALID;
    }
    uint32_t slot = (uint32_t)(offset / slot_size);
    if (slot >= page->slot_count || slot >= 64u) {
      return H9_OWNER_PAGE_ROUTE_INVALID;
    }
    if (page_out) {
      *page_out = page;
    }
    if (slot_out) {
      *slot_out = slot;
    }
    return H9_OWNER_PAGE_ROUTE_VALID;
  }
  return H9_OWNER_PAGE_ROUTE_MISS;
}

/* Page mode transitions must not mutate owner-local free bits. */
static bool __attribute__((unused))
h9_owner_page_mark_remote_seen(H9OwnerLocalPage* page) {
  if (!page) {
    return false;
  }
  H8_DEBUG_INC(h9_owner_page_api_remote_mark);
  uint64_t before_bits =
      atomic_load_explicit(&page->local_free_bits, memory_order_acquire);
  uint32_t expected = H9_OWNER_PAGE_PURE_LOCAL;
  bool first = atomic_compare_exchange_strong_explicit(
      &page->mode, &expected, H9_OWNER_PAGE_REMOTE_SEEN,
      memory_order_acq_rel, memory_order_acquire);
  if (first) {
    H8_DEBUG_INC(h9_owner_page_api_remote_mark_first);
  } else if (expected == H9_OWNER_PAGE_REMOTE_SEEN) {
    H8_DEBUG_INC(h9_owner_page_api_remote_mark_repeat);
  }
  if (atomic_load_explicit(&page->local_free_bits, memory_order_acquire) !=
      before_bits) {
    H8_DEBUG_INC(h9_owner_page_api_local_bits_mutation);
  }
  return first;
}

static bool __attribute__((unused))
h9_owner_page_start_drain(H9OwnerLocalPage* page) {
  if (!page) {
    return false;
  }
  uint64_t before_bits =
      atomic_load_explicit(&page->local_free_bits, memory_order_acquire);
  uint32_t mode = atomic_load_explicit(&page->mode, memory_order_acquire);
  for (;;) {
    if (mode == H9_OWNER_PAGE_DRAINING ||
        mode == H9_OWNER_PAGE_DETACHED) {
      return false;
    }
    if (atomic_compare_exchange_weak_explicit(
            &page->mode, &mode, H9_OWNER_PAGE_DRAINING,
            memory_order_acq_rel, memory_order_acquire)) {
      H8_DEBUG_INC(h9_owner_page_api_drain_start);
      if (atomic_load_explicit(&page->local_free_bits, memory_order_acquire) !=
          before_bits) {
        H8_DEBUG_INC(h9_owner_page_api_local_bits_mutation);
      }
      return true;
    }
  }
}

static bool __attribute__((unused)) h9_owner_page_detach(
    H9OwnerLocalPage* page) {
  if (!page) {
    return false;
  }
  uint64_t before_bits =
      atomic_load_explicit(&page->local_free_bits, memory_order_acquire);
  atomic_store_explicit(&page->mode, H9_OWNER_PAGE_DETACHED,
                        memory_order_release);
  H8_DEBUG_INC(h9_owner_page_api_detach);
  if (atomic_load_explicit(&page->local_free_bits, memory_order_acquire) !=
      before_bits) {
    H8_DEBUG_INC(h9_owner_page_api_local_bits_mutation);
  }
  return true;
}

/* L0 creates/release pages but still falls back to HZ8 for allocations. */
static uint64_t h9_owner_page_slot_mask(uint16_t slot_count) {
  if (slot_count == 0u || slot_count > 64u) {
    return 0;
  }
  return slot_count == 64u ? UINT64_MAX : ((UINT64_C(1) << slot_count) - 1u);
}

static bool h9_owner_page_is_all_free(const H9OwnerLocalPage* page) {
  if (!page) {
    return false;
  }
  uint64_t full = h9_owner_page_slot_mask(page->slot_count);
  uint64_t pending =
      atomic_load_explicit(&page->pending_bits, memory_order_acquire) & full;
  uint64_t local =
      atomic_load_explicit(&page->local_free_bits, memory_order_acquire);
  return full != 0 && (local & full) == full && pending == 0;
}

static void h9_owner_page_collect_pending_locked(H9OwnerLocalPage* page) {
  if (!page) {
    return;
  }
  uint64_t full = h9_owner_page_slot_mask(page->slot_count);
  uint64_t pending =
      atomic_exchange_explicit(&page->pending_bits, 0, memory_order_acq_rel);
  pending &= full;
  if (pending) {
    atomic_fetch_or_explicit(&page->local_free_bits, pending,
                             memory_order_acq_rel);
    H8_DEBUG_INC(h9_owner_page_api_page_pending_collect);
  }
}

#include "h8_hz9_owner_page_bits.inc"

static void* h9_owner_page_try_pop(H9OwnerLocalPage* page) {
  if (!page || !page->base) {
    return NULL;
  }
  uint32_t mode = atomic_load_explicit(&page->mode, memory_order_acquire);
  if (mode != H9_OWNER_PAGE_PURE_LOCAL) {
    H8_DEBUG_INC(h9_owner_page_api_alloc_mode_block);
    return NULL;
  }
  uint64_t bit = 0;
  if (!h9_owner_page_local_take(page, &bit)) {
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(bit);
  const H8MediumClassSpec* spec = h8_medium_class_spec(page->class_id);
  if (!spec || spec->slot_size == 0u) {
    (void)h9_owner_page_local_put(page, bit);
    return NULL;
  }
  void* ptr = (void*)((uintptr_t)page->base +
                      (uintptr_t)slot * (uintptr_t)spec->slot_size);
  H8_DEBUG_INC(h9_owner_page_api_alloc_hit);
  return ptr;
}

static H9OwnerLocalPage* h9_owner_page_create_active(
    H9OwnerPageThreadState* state, H8OwnerRecord* owner, uint32_t class_id) {
  if (!state || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H9OwnerLocalPage* active = state->active[class_id];
  if (active) {
    return active;
  }
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec || spec->run_size == 0u || spec->slot_count == 0u ||
      spec->slot_count > 64u) {
    H8_DEBUG_INC(h9_owner_page_api_page_create_fail);
    return NULL;
  }
  H9OwnerLocalPage* page = h8_sys_calloc(1, sizeof(*page));
  if (!page) {
    H8_DEBUG_INC(h9_owner_page_api_page_create_fail);
    return NULL;
  }
  void* payload = h8_platform_reserve_rw(spec->run_size);
  if (!payload) {
    h8_sys_free(page);
    H8_DEBUG_INC(h9_owner_page_api_page_create_fail);
    return NULL;
  }
  page->owner = owner;
  page->owner_word = h9_owner_page_owner_word_for(owner);
  page->base = payload;
  page->page_bytes = spec->run_size;
  page->class_id = (uint16_t)class_id;
  page->slot_count = spec->slot_count;
  atomic_store_explicit(&page->local_free_bits,
                        h9_owner_page_slot_mask(spec->slot_count),
                        memory_order_relaxed);
  atomic_store_explicit(&page->pending_bits, 0, memory_order_relaxed);
  atomic_store_explicit(&page->mode, H9_OWNER_PAGE_PURE_LOCAL,
                        memory_order_release);
  h9_owner_page_register_global(page);
  state->active[class_id] = page;
  state->retained_bytes += page->page_bytes;
  H8_DEBUG_INC(h9_owner_page_api_page_create);
  H8_DEBUG_INC(h9_owner_page_api_page_active_install);
  H8_DEBUG_ADD(h9_owner_page_api_page_bytes_create, page->page_bytes);
  return page;
}

static size_t h9_owner_page_destroy_unlocked(H9OwnerLocalPage* page) {
  if (!page) {
    return 0;
  }
  size_t bytes = page->page_bytes;
  if (page->base && bytes) {
    h8_platform_release(page->base, bytes);
  }
  H8_DEBUG_INC(h9_owner_page_api_page_release);
  H8_DEBUG_ADD(h9_owner_page_api_page_bytes_release, bytes);
  h8_sys_free(page);
  return bytes;
}

static void h9_owner_page_release_page(H9OwnerPageThreadState* state,
                                       uint32_t class_id) {
  if (!state || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H9OwnerLocalPage* page = state->active[class_id];
  state->active[class_id] = NULL;
  if (!page) {
    return;
  }
  h8_platform_mutex_lock(&h9_owner_page_global_lock);
  h9_owner_page_collect_pending_locked(page);
  if (!h9_owner_page_is_all_free(page)) {
    (void)h9_owner_page_detach(page);
    H8_DEBUG_INC(h9_owner_page_api_page_release_defer);
    h8_platform_mutex_unlock(&h9_owner_page_global_lock);
    if (state->retained_bytes >= page->page_bytes) {
      state->retained_bytes -= page->page_bytes;
    } else {
      state->retained_bytes = 0;
    }
    return;
  }
  h9_owner_page_unregister_global_locked(page);
  h8_platform_mutex_unlock(&h9_owner_page_global_lock);
  (void)h9_owner_page_start_drain(page);
  (void)h9_owner_page_detach(page);
  size_t bytes = h9_owner_page_destroy_unlocked(page);
  if (state->retained_bytes >= bytes) {
    state->retained_bytes -= bytes;
  } else {
    state->retained_bytes = 0;
  }
}

static void h9_owner_page_release_thread_pages(
    H9OwnerPageThreadState* state) {
  if (!state) {
    return;
  }
  for (uint32_t i = 0; i < H8_MEDIUM_CLASS_COUNT; ++i) {
    h9_owner_page_release_page(state, i);
  }
}
#endif

bool h9_owner_page_pool_scaffold_enabled(void) {
  return true;
}

size_t h9_owner_page_pool_scaffold_layout_bytes(void) {
  return sizeof(H9OwnerLocalPage) + sizeof(H9OwnerPageThreadState) +
         sizeof(H9OwnerPageSidecar);
}

void h9_owner_page_pool_scaffold_flush_thread(void) {
#if defined(H9_OWNER_LOCAL_PAGE_POOL_PURE_LOCAL_L1)
  h9_owner_page_free_thread_state();
#else
  h9_owner_page_tls_state = NULL;
#endif
}

#if defined(H9_OWNER_LOCAL_PAGE_POOL_PURE_LOCAL_L1)
void* h9_owner_page_try_alloc(H8ThreadCtx* ctx, uint32_t class_id) {
  if (class_id < H8_MEDIUM_CLASS_COUNT) {
    H8_DEBUG_INC(h9_owner_page_api_alloc_call);
#if defined(H9_OWNER_LOCAL_PAGE_POOL_DISABLED_FAST_REJECT_L1)
    H9OwnerPageThreadState* cached = h9_owner_page_tls_state;
    uint64_t class_bit = UINT64_C(1) << class_id;
    if (cached && (cached->disabled_class_bits & class_bit) != 0) {
      H8_DEBUG_INC(h9_owner_page_api_alloc_mode_block);
      return NULL;
    }
#endif
    H9OwnerPageThreadState* state = h9_owner_page_ensure_thread_state();
    if (state) {
#if !defined(H9_OWNER_LOCAL_PAGE_POOL_DISABLED_FAST_REJECT_L1)
      uint64_t class_bit = UINT64_C(1) << class_id;
#endif
      if ((state->disabled_class_bits & class_bit) != 0) {
        H8_DEBUG_INC(h9_owner_page_api_alloc_mode_block);
        return NULL;
      }
      H9OwnerLocalPage* page = state->active[class_id];
      if (page && atomic_load_explicit(&page->mode, memory_order_acquire) !=
                      H9_OWNER_PAGE_PURE_LOCAL) {
        state->disabled_class_bits |= class_bit;
        H8_DEBUG_INC(h9_owner_page_api_alloc_mode_block);
        return NULL;
      }
      page = h9_owner_page_create_active(state, ctx ? ctx->owner : NULL,
                                         class_id);
      void* ptr = h9_owner_page_try_pop(page);
      if (!ptr && page &&
          atomic_load_explicit(&page->mode, memory_order_acquire) !=
              H9_OWNER_PAGE_PURE_LOCAL) {
        state->disabled_class_bits |= class_bit;
      }
      return ptr;
    }
  }
  return NULL;
}

bool h9_owner_page_try_free(H8ThreadCtx* ctx, void* ptr, bool* owned_out) {
  H8_DEBUG_INC(h9_owner_page_api_free_call);
  if (owned_out) {
    *owned_out = false;
  }
  if (ptr) {
    H8_DEBUG_INC(h9_owner_page_api_route_attempt);
    H9OwnerLocalPage* page = NULL;
    uint32_t slot = 0;
    H9OwnerPageThreadState* state = h9_owner_page_tls_state;
    H9OwnerPageRoute local_route =
        h9_owner_page_route_thread(state, ptr, &page, &slot);
    if (local_route == H9_OWNER_PAGE_ROUTE_VALID &&
        h9_owner_page_owner_matches_ctx(page, ctx)) {
      if (owned_out) {
        *owned_out = true;
      }
      uint64_t bit = UINT64_C(1) << slot;
      uint64_t pending =
          atomic_load_explicit(&page->pending_bits, memory_order_acquire);
      if ((pending & bit) != 0) {
        H8_DEBUG_INC(h9_owner_page_api_free_double);
        return false;
      }
      if (!h9_owner_page_local_put(page, bit)) {
        H8_DEBUG_INC(h9_owner_page_api_free_double);
        return false;
      }
      H8_DEBUG_INC(h9_owner_page_api_route_hit);
      H8_DEBUG_INC(h9_owner_page_api_free_push);
      return true;
    }
    if (local_route == H9_OWNER_PAGE_ROUTE_INVALID) {
      H8_DEBUG_INC(h9_owner_page_api_route_invalid);
      if (owned_out) {
        *owned_out = true;
      }
      return false;
    }
    h8_platform_mutex_lock(&h9_owner_page_global_lock);
    H9OwnerPageRoute route =
        h9_owner_page_route_global_locked(ptr, &page, &slot);
    if (route == H9_OWNER_PAGE_ROUTE_VALID) {
      H8_DEBUG_INC(h9_owner_page_api_route_hit);
      if (owned_out) {
        *owned_out = true;
      }
      uint64_t bit = UINT64_C(1) << slot;
      uint32_t mode = atomic_load_explicit(&page->mode, memory_order_acquire);
      bool same_owner = h9_owner_page_owner_matches_ctx(page, ctx);
      uint64_t local =
          atomic_load_explicit(&page->local_free_bits, memory_order_acquire);
      if ((local & bit) != 0) {
        H8_DEBUG_INC(h9_owner_page_api_free_double);
        h8_platform_mutex_unlock(&h9_owner_page_global_lock);
        return false;
      }
      if (same_owner || mode == H9_OWNER_PAGE_DETACHED) {
        h9_owner_page_collect_pending_locked(page);
        (void)h9_owner_page_local_put(page, bit);
        H8_DEBUG_INC(h9_owner_page_api_free_push);
        if (mode == H9_OWNER_PAGE_DETACHED &&
            h9_owner_page_is_all_free(page)) {
          h9_owner_page_unregister_global_locked(page);
          h8_platform_mutex_unlock(&h9_owner_page_global_lock);
          (void)h9_owner_page_destroy_unlocked(page);
          return true;
        }
        h8_platform_mutex_unlock(&h9_owner_page_global_lock);
        return true;
      }
      (void)h9_owner_page_mark_remote_seen(page);
      uint64_t old = atomic_fetch_or_explicit(
          &page->pending_bits, bit, memory_order_acq_rel);
      if ((old & bit) != 0) {
        H8_DEBUG_INC(h9_owner_page_api_remote_pending_repeat);
        h8_platform_mutex_unlock(&h9_owner_page_global_lock);
        return false;
      }
      H8_DEBUG_INC(h9_owner_page_api_remote_pending_first);
      h8_platform_mutex_unlock(&h9_owner_page_global_lock);
      return true;
    } else if (route == H9_OWNER_PAGE_ROUTE_INVALID) {
      H8_DEBUG_INC(h9_owner_page_api_route_invalid);
      if (owned_out) {
        *owned_out = true;
      }
      h8_platform_mutex_unlock(&h9_owner_page_global_lock);
    } else {
      H8_DEBUG_INC(h9_owner_page_api_route_miss);
      h8_platform_mutex_unlock(&h9_owner_page_global_lock);
    }
  }
  return false;
}

void h9_owner_page_note_remote_medium_free(H8ThreadCtx* ctx,
                                           H8MediumRun* run) {
  (void)ctx;
  H8_DEBUG_INC(h9_owner_page_api_remote_note_call);
  H8_DEBUG_INC(h9_owner_page_api_remote_fallback);
  if (run && run->class_id < H8_MEDIUM_CLASS_COUNT) {
    H8_DEBUG_INC(h9_owner_page_api_remote_class[run->class_id]);
  }
}

void h9_owner_page_flush_thread(H8ThreadCtx* ctx) {
  (void)ctx;
  H8_DEBUG_INC(h9_owner_page_api_flush_thread);
  h9_owner_page_free_thread_state();
}

void h9_owner_page_flush_owner(H8OwnerRecord* owner) {
  H8_DEBUG_INC(h9_owner_page_api_flush_owner);
  uint64_t owner_word = h9_owner_page_owner_word_for(owner);
  if (owner_word == 0) {
    return;
  }
  H9OwnerLocalPage* release_head = NULL;
  h8_platform_mutex_lock(&h9_owner_page_global_lock);
  H9OwnerLocalPage* page = h9_owner_page_global_head;
  while (page) {
    H9OwnerLocalPage* next = page->next_global;
    if (page->owner_word == owner_word) {
      h9_owner_page_collect_pending_locked(page);
      if (h9_owner_page_is_all_free(page)) {
        h9_owner_page_unregister_global_locked(page);
        (void)h9_owner_page_start_drain(page);
        (void)h9_owner_page_detach(page);
        page->next_global = release_head;
        release_head = page;
      } else {
        (void)h9_owner_page_detach(page);
      }
    }
    page = next;
  }
  h8_platform_mutex_unlock(&h9_owner_page_global_lock);
  while (release_head) {
    H9OwnerLocalPage* next = release_head->next_global;
    release_head->next_global = NULL;
    (void)h9_owner_page_destroy_unlocked(release_head);
    release_head = next;
  }
}

#if defined(H9_OWNER_LOCAL_PAGE_POOL_ROUTE_SMOKE)
bool h9_owner_page_route_smoke_run(void) {
  unsigned char storage[H8_MEDIUM_QUANTUM_BYTES + H8_MEDIUM_PAGE_BYTES] = {0};
  uintptr_t raw = (uintptr_t)storage;
  uintptr_t aligned =
      (raw + (uintptr_t)H8_MEDIUM_PAGE_BYTES - 1u) &
      ~((uintptr_t)H8_MEDIUM_PAGE_BYTES - 1u);
  H9OwnerLocalPage page = {
      .owner = NULL,
      .base = (void*)aligned,
      .page_bytes = H8_MEDIUM_QUANTUM_BYTES,
      .class_id = 2u,
      .slot_count = 2u,
      .local_free_bits = ATOMIC_VAR_INIT(UINT64_C(3)),
      .pending_bits = 0,
      .mode = H9_OWNER_PAGE_PURE_LOCAL,
  };
  H9OwnerPageThreadState state = {0};
  state.active[page.class_id] = &page;

  H9OwnerLocalPage* out = NULL;
  uint32_t slot = 99u;
  if (h9_owner_page_route_thread(&state, page.base, &out, &slot) !=
          H9_OWNER_PAGE_ROUTE_VALID ||
      out != &page || slot != 0u) {
    return false;
  }
  void* slot1 = (void*)((uintptr_t)page.base + 24576u);
  if (h9_owner_page_route_thread(&state, slot1, &out, &slot) !=
          H9_OWNER_PAGE_ROUTE_VALID ||
      out != &page || slot != 1u) {
    return false;
  }
  void* interior = (void*)((uintptr_t)page.base + 1u);
  if (h9_owner_page_route_thread(&state, interior, &out, &slot) !=
      H9_OWNER_PAGE_ROUTE_INVALID) {
    return false;
  }
  void* tail = (void*)((uintptr_t)page.base + 49152u);
  if (h9_owner_page_route_thread(&state, tail, &out, &slot) !=
      H9_OWNER_PAGE_ROUTE_INVALID) {
    return false;
  }
  void* miss = (void*)((uintptr_t)page.base + H8_MEDIUM_QUANTUM_BYTES);
  if (h9_owner_page_route_thread(&state, miss, &out, &slot) !=
      H9_OWNER_PAGE_ROUTE_MISS) {
    return false;
  }

  uint64_t before_bits =
      atomic_load_explicit(&page.local_free_bits, memory_order_acquire);
  if (!h9_owner_page_mark_remote_seen(&page) ||
      atomic_load_explicit(&page.mode, memory_order_acquire) !=
          H9_OWNER_PAGE_REMOTE_SEEN ||
      atomic_load_explicit(&page.local_free_bits, memory_order_acquire) !=
          before_bits) {
    return false;
  }
  if (h9_owner_page_mark_remote_seen(&page) ||
      atomic_load_explicit(&page.local_free_bits, memory_order_acquire) !=
          before_bits) {
    return false;
  }
  if (!h9_owner_page_start_drain(&page) ||
      atomic_load_explicit(&page.mode, memory_order_acquire) !=
          H9_OWNER_PAGE_DRAINING ||
      atomic_load_explicit(&page.local_free_bits, memory_order_acquire) !=
          before_bits) {
    return false;
  }
  if (!h9_owner_page_detach(&page) ||
      atomic_load_explicit(&page.mode, memory_order_acquire) !=
          H9_OWNER_PAGE_DETACHED ||
      atomic_load_explicit(&page.local_free_bits, memory_order_acquire) !=
          before_bits) {
    return false;
  }

  H9OwnerPageThreadState* tls = h9_owner_page_ensure_thread_state();
  H9OwnerLocalPage* real_page =
      h9_owner_page_create_active(tls, NULL, page.class_id);
  if (!tls || !real_page || !real_page->base) {
    h9_owner_page_free_thread_state();
    return false;
  }
  if (h9_owner_page_route_thread(tls, real_page->base, &out, &slot) !=
          H9_OWNER_PAGE_ROUTE_VALID ||
      out != real_page || slot != 0u) {
    h9_owner_page_free_thread_state();
    return false;
  }
  void* real_miss =
      (void*)((uintptr_t)real_page->base + real_page->page_bytes);
  if (h9_owner_page_route_thread(tls, real_miss, &out, &slot) !=
      H9_OWNER_PAGE_ROUTE_MISS) {
    h9_owner_page_free_thread_state();
    return false;
  }
  h9_owner_page_free_thread_state();
  return h9_owner_page_tls_state == NULL;
}
#endif
#endif

#else
typedef int h9_owner_page_pool_translation_unit_silence;
#endif
