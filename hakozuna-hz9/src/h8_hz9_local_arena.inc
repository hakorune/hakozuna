#if defined(H9_LOCAL_ARENA_L0)
#ifndef H9_LOCAL_ARENA_PAGE_BYTES
#define H9_LOCAL_ARENA_PAGE_BYTES H8_MEDIUM_QUANTUM_BYTES
#endif
#ifndef H9_LOCAL_ARENA_PAGE_COUNT
#define H9_LOCAL_ARENA_PAGE_COUNT 1024u
#endif
#ifndef H9_LOCAL_ARENA_MAX_SLOTS
#define H9_LOCAL_ARENA_MAX_SLOTS 8u
#endif
#ifndef H9_LOCAL_ARENA_MIN_CLASS_ID
#define H9_LOCAL_ARENA_MIN_CLASS_ID 0u
#endif
#ifndef H9_LOCAL_ARENA_REMOTE_PROBATION_ALLOCS
#define H9_LOCAL_ARENA_REMOTE_PROBATION_ALLOCS 8u
#endif

typedef struct H9LocalArenaPage {
  uint32_t class_id;
  uint32_t slot_size;
  uint16_t slot_count;
  uint64_t owner_word;
  _Atomic uint64_t free_bits;
  _Atomic uint32_t slot_state[H9_LOCAL_ARENA_MAX_SLOTS];
  _Atomic uint32_t local_alloc_count;
  _Atomic bool remote_observed;
  _Atomic bool remote_seen;
  _Atomic bool remote_safe;
  _Atomic bool registered;
#if defined(H8_ENABLE_DEBUG_STATS)
  uint32_t creator_page_seq;
  uint32_t creator_alloc_history;
  _Atomic uint32_t local_free_count;
  _Atomic uint32_t remote_free_count;
#endif
} H9LocalArenaPage;

static h8_platform_once_t h9_local_arena_once = H8_PLATFORM_ONCE_INIT;
static h8_platform_mutex_t h9_local_arena_lock;
static uint8_t* h9_local_arena_base;
static uint32_t h9_local_arena_next_page;
static H9LocalArenaPage h9_local_arena_pages[H9_LOCAL_ARENA_PAGE_COUNT];
static _Atomic uintptr_t h9_local_arena_min_addr;
static _Atomic uintptr_t h9_local_arena_max_addr;
static _Atomic bool h9_local_arena_remote_class_seen[H8_MEDIUM_CLASS_COUNT];
static _Thread_local H9LocalArenaPage*
    h9_local_arena_active[H8_MEDIUM_CLASS_COUNT];
#if defined(H9_LOCAL_ARENA_REMOTE_DISABLE_L1)
static _Thread_local uint8_t h9_local_arena_disabled[H8_MEDIUM_CLASS_COUNT];
#endif
#if defined(H9_LOCAL_ARENA_REMOTE_PROBATION_L1)
static _Thread_local uint8_t
    h9_local_arena_remote_probation[H8_MEDIUM_CLASS_COUNT];
#endif
#if defined(H8_ENABLE_DEBUG_STATS)
static _Thread_local uint32_t
    h9_local_arena_thread_page_create_count[H8_MEDIUM_CLASS_COUNT];
static _Thread_local uint32_t
    h9_local_arena_thread_alloc_count[H8_MEDIUM_CLASS_COUNT];
#endif

static void h9_local_arena_init_once(void) {
  h8_platform_mutex_init(&h9_local_arena_lock);
  size_t bytes = (size_t)H9_LOCAL_ARENA_PAGE_BYTES *
                 (size_t)H9_LOCAL_ARENA_PAGE_COUNT;
  h9_local_arena_base = h8_platform_reserve(bytes);
  if (h9_local_arena_base) {
    atomic_store_explicit(&h9_local_arena_min_addr,
                          (uintptr_t)h9_local_arena_base,
                          memory_order_release);
    atomic_store_explicit(&h9_local_arena_max_addr,
                          (uintptr_t)h9_local_arena_base + bytes,
                          memory_order_release);
  }
}

static bool h9_local_arena_init(void) {
  h8_platform_once(&h9_local_arena_once, h9_local_arena_init_once);
  return h9_local_arena_base != NULL;
}

static bool h9_local_arena_class_enabled(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
#if H9_LOCAL_ARENA_MIN_CLASS_ID > 0u
  if (class_id < H9_LOCAL_ARENA_MIN_CLASS_ID) {
    return false;
  }
#endif
#if defined(H9_LOCAL_ARENA_MAX_CLASS_ID)
  if (class_id > H9_LOCAL_ARENA_MAX_CLASS_ID) {
    return false;
  }
#endif
  return true;
}

static uint64_t h9_local_owner_word_for(const H8OwnerRecord* owner) {
  if (!owner) {
    return 0;
  }
  H8OwnerWord word = h8_owner_word_make((uint8_t)owner->slot,
                                        (uint16_t)owner->generation,
                                        H8_SPAN_OWNED_ACTIVE, 0);
  return h8_owner_word_pack(word);
}

static bool h9_local_addr_in_arena(uintptr_t addr) {
  uintptr_t min =
      atomic_load_explicit(&h9_local_arena_min_addr, memory_order_acquire);
  uintptr_t max =
      atomic_load_explicit(&h9_local_arena_max_addr, memory_order_acquire);
  return min != 0 && addr >= min && addr < max;
}

static H9LocalArenaPage* h9_local_page_for_ptr(const void* ptr,
                                               uintptr_t* offset_out) {
  uintptr_t addr = (uintptr_t)ptr;
  if (!h9_local_addr_in_arena(addr) || !h9_local_arena_base) {
    return NULL;
  }
  uintptr_t offset = addr - (uintptr_t)h9_local_arena_base;
  size_t page_index = (size_t)(offset / H9_LOCAL_ARENA_PAGE_BYTES);
  if (page_index >= H9_LOCAL_ARENA_PAGE_COUNT) {
    return NULL;
  }
  if (offset_out) {
    *offset_out = offset % H9_LOCAL_ARENA_PAGE_BYTES;
  }
  return &h9_local_arena_pages[page_index];
}

static bool h9_local_slot_index(const H9LocalArenaPage* page,
                                uintptr_t page_offset, size_t* slot_out) {
  if (!page || !atomic_load_explicit(&page->registered, memory_order_acquire) ||
      page->slot_size == 0 || page_offset >= H9_LOCAL_ARENA_PAGE_BYTES) {
    return false;
  }
  size_t slot = 0;
  uintptr_t slot_size = (uintptr_t)page->slot_size;
  if (page->slot_count == 2u) {
    if (page_offset == 0) {
      slot = 0;
    } else if (page_offset == slot_size) {
      slot = 1;
    } else {
      return false;
    }
  } else if ((slot_size & (slot_size - 1u)) == 0) {
    if ((page_offset & (slot_size - 1u)) != 0) {
      return false;
    }
    slot = (size_t)(page_offset >> __builtin_ctzl(slot_size));
  } else {
    if ((page_offset % slot_size) != 0) {
      return false;
    }
    slot = (size_t)(page_offset / slot_size);
  }
  if (slot >= page->slot_count) {
    return false;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return true;
}

static void* h9_local_slot_ptr(const H9LocalArenaPage* page, size_t slot) {
  size_t page_index = (size_t)(page - h9_local_arena_pages);
  return h9_local_arena_base +
         page_index * (size_t)H9_LOCAL_ARENA_PAGE_BYTES +
         slot * (size_t)page->slot_size;
}

#if defined(H8_ENABLE_DEBUG_STATS)
static size_t h9_local_arena_age_bucket(uint32_t count) {
  if (count == 0u) {
    return 0u;
  }
  if (count == 1u) {
    return 1u;
  }
  if (count < 4u) {
    return 2u;
  }
  if (count < 8u) {
    return 3u;
  }
  return 4u;
}

static void h9_local_arena_note_alloc(H9LocalArenaPage* page) {
  atomic_fetch_add_explicit(&page->local_alloc_count, 1, memory_order_relaxed);
  if (page->class_id < H8_MEDIUM_CLASS_COUNT) {
    ++h9_local_arena_thread_alloc_count[page->class_id];
  }
}

static void h9_local_arena_note_local_free(H9LocalArenaPage* page) {
  atomic_fetch_add_explicit(&page->local_free_count, 1, memory_order_relaxed);
  if (atomic_load_explicit(&page->remote_observed, memory_order_acquire)) {
    H8_DEBUG_INC(h9_local_arena_local_free_after_remote_class[page->class_id]);
  }
}

static uint32_t h9_local_arena_note_remote_free(H9LocalArenaPage* page) {
  atomic_fetch_add_explicit(&page->remote_free_count, 1, memory_order_relaxed);
  uint32_t local =
      atomic_load_explicit(&page->local_free_count, memory_order_acquire);
  if (local != 0u) {
    H8_DEBUG_INC(h9_local_arena_remote_free_after_local_class[page->class_id]);
  }
  return local;
}
#else
static void h9_local_arena_note_alloc(H9LocalArenaPage* page) {
  atomic_fetch_add_explicit(&page->local_alloc_count, 1, memory_order_relaxed);
}

static void h9_local_arena_note_local_free(H9LocalArenaPage* page) {
  (void)page;
}

static uint32_t h9_local_arena_note_remote_free(H9LocalArenaPage* page) {
  (void)page;
  return 0u;
}
#endif

#if defined(H8_ENABLE_DEBUG_STATS)
static void h9_local_arena_note_admission_shadow(H8ThreadCtx* ctx,
                                                 uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8_DEBUG_INC(h9_local_arena_admission_attempt_class[class_id]);
  bool remote_seen =
      atomic_load_explicit(&h9_local_arena_remote_class_seen[class_id],
                           memory_order_acquire);
  if (remote_seen) {
    H8_DEBUG_INC(h9_local_arena_admission_remote_seen_class[class_id]);
  }
  H9LocalArenaPage* page = h9_local_arena_active[class_id];
  bool active_owner =
      ctx && ctx->owner && page && page->class_id == class_id &&
      page->owner_word == h9_local_owner_word_for(ctx->owner);
  if (active_owner) {
    H8_DEBUG_INC(h9_local_arena_admission_active_owner_class[class_id]);
  }
  bool active_nonfull =
      active_owner &&
      atomic_load_explicit(&page->free_bits, memory_order_acquire) != 0;
  if (active_nonfull) {
    H8_DEBUG_INC(h9_local_arena_admission_active_nonfull_class[class_id]);
  }
  if (remote_seen && active_nonfull) {
    H8_DEBUG_INC(
        h9_local_arena_admission_remote_seen_active_nonfull_class[class_id]);
  } else if (remote_seen) {
    H8_DEBUG_INC(
        h9_local_arena_admission_remote_seen_no_capacity_class[class_id]);
  }
}
#endif

static bool h9_local_arena_is_remote_safe(const H9LocalArenaPage* page) {
#if defined(H9_LOCAL_ARENA_REMOTE_SAFE_AFTER_CLASS_L1)
  return page && atomic_load_explicit(&page->remote_safe, memory_order_acquire);
#else
  (void)page;
  return false;
#endif
}

#if defined(H9_LOCAL_ARENA_OWNER_FAST_MUTATION_L1)
static bool h9_local_arena_owner_fast_enabled(const H9LocalArenaPage* page) {
  return page && !h9_local_arena_is_remote_safe(page) &&
         !atomic_load_explicit(&page->remote_seen, memory_order_acquire);
}
#endif

static H9LocalArenaPage* h9_local_create_page(H8ThreadCtx* ctx,
                                              uint32_t class_id) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!ctx || !ctx->owner || !spec || spec->slot_size == 0 ||
      spec->slot_size > H9_LOCAL_ARENA_PAGE_BYTES ||
      !h9_local_arena_init()) {
    return NULL;
  }
  uint16_t slot_count =
      (uint16_t)(H9_LOCAL_ARENA_PAGE_BYTES / spec->slot_size);
  if (slot_count == 0 || slot_count > H9_LOCAL_ARENA_MAX_SLOTS) {
    return NULL;
  }

  h8_platform_mutex_lock(&h9_local_arena_lock);
  uint32_t page_index = h9_local_arena_next_page;
  if (page_index >= H9_LOCAL_ARENA_PAGE_COUNT) {
    h8_platform_mutex_unlock(&h9_local_arena_lock);
    return NULL;
  }
  h9_local_arena_next_page = page_index + 1u;
  H9LocalArenaPage* page = &h9_local_arena_pages[page_index];
  uint8_t* base = h9_local_arena_base +
                  (size_t)page_index * (size_t)H9_LOCAL_ARENA_PAGE_BYTES;
  if (h8_platform_commit(base, H9_LOCAL_ARENA_PAGE_BYTES) != 0) {
    h8_platform_mutex_unlock(&h9_local_arena_lock);
    return NULL;
  }
  page->class_id = class_id;
  page->slot_size = spec->slot_size;
  page->slot_count = slot_count;
  page->owner_word = h9_local_owner_word_for(ctx->owner);
  bool class_remote_seen =
      atomic_load_explicit(&h9_local_arena_remote_class_seen[class_id],
                           memory_order_acquire);
  bool remote_safe = false;
#if defined(H9_LOCAL_ARENA_REMOTE_SAFE_AFTER_CLASS_L1)
  remote_safe = class_remote_seen;
#endif
#if defined(H8_ENABLE_DEBUG_STATS)
  uint32_t create_seq = h9_local_arena_thread_page_create_count[class_id]++;
  uint32_t alloc_history = h9_local_arena_thread_alloc_count[class_id];
  page->creator_page_seq = create_seq;
  page->creator_alloc_history = alloc_history;
  atomic_store_explicit(&page->local_free_count, 0, memory_order_release);
  atomic_store_explicit(&page->remote_free_count, 0, memory_order_release);
  H8_DEBUG_INC(
      h9_local_arena_create_thread_page_seq[h9_local_arena_age_bucket(create_seq)]);
  H8_DEBUG_INC(h9_local_arena_create_thread_alloc_history
                   [h9_local_arena_age_bucket(alloc_history)]);
#endif
  uint64_t free_bits = (UINT64_C(1) << slot_count) - 1u;
  atomic_store_explicit(&page->free_bits, free_bits, memory_order_release);
  atomic_store_explicit(&page->local_alloc_count, 0, memory_order_release);
  atomic_store_explicit(&page->remote_observed, false, memory_order_release);
  atomic_store_explicit(&page->remote_seen, false, memory_order_release);
  atomic_store_explicit(&page->remote_safe, remote_safe, memory_order_release);
  for (uint32_t i = 0; i < H9_LOCAL_ARENA_MAX_SLOTS; ++i) {
    uint32_t state = i < slot_count ? H8_SLOT_FREE | H8_SLOT_NONE
                                    : H8_SLOT_NEVER_USED;
    atomic_store_explicit(&page->slot_state[i], state, memory_order_release);
  }
  atomic_store_explicit(&page->registered, true, memory_order_release);
  h8_platform_mutex_unlock(&h9_local_arena_lock);
  if (class_remote_seen) {
    H8_DEBUG_INC(h9_local_arena_admit_create_after_remote_class[class_id]);
  } else {
    H8_DEBUG_INC(h9_local_arena_admit_create_before_remote_class[class_id]);
  }
  if (remote_safe) {
    H8_DEBUG_INC(h9_local_arena_remote_safe_page_create_class[class_id]);
  }
  H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_PAGE_CREATE]);
  H8_DEBUG_INC(h9_local_arena_page_create_class[class_id]);
  return page;
}

static void* h9_local_alloc_from_page(H9LocalArenaPage* page) {
  if (!page || !atomic_load_explicit(&page->registered, memory_order_acquire)) {
    return NULL;
  }
#if defined(H9_LOCAL_ARENA_OWNER_FAST_MUTATION_L1)
  if (h9_local_arena_owner_fast_enabled(page)) {
    uint64_t bits = atomic_load_explicit(&page->free_bits, memory_order_acquire);
    while (bits != 0) {
      size_t slot = (size_t)__builtin_ctzll(bits);
      uint64_t next = bits & ~(UINT64_C(1) << slot);
      atomic_store_explicit(&page->free_bits, next, memory_order_release);
      uint32_t state =
          atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
      if (state == (H8_SLOT_FREE | H8_SLOT_NONE)) {
        atomic_store_explicit(&page->slot_state[slot], H8_SLOT_ALLOCATED,
                              memory_order_release);
        H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_HIT]);
        H8_DEBUG_INC(h9_local_arena_alloc_hit_class[page->class_id]);
        H8_DEBUG_INC(h9_local_arena_ownerfast_alloc_class[page->class_id]);
        h9_local_arena_note_alloc(page);
        return h9_local_slot_ptr(page, slot);
      }
      bits = next;
    }
    return NULL;
  }
#endif
  uint64_t bits = atomic_load_explicit(&page->free_bits, memory_order_acquire);
  while (bits != 0) {
    size_t slot = (size_t)__builtin_ctzll(bits);
    uint64_t bit = UINT64_C(1) << slot;
    uint64_t next = bits & ~bit;
    if (!atomic_compare_exchange_weak_explicit(
            &page->free_bits, &bits, next, memory_order_acq_rel,
            memory_order_acquire)) {
      continue;
    }
    uint32_t expected = H8_SLOT_FREE | H8_SLOT_NONE;
    if (atomic_compare_exchange_strong_explicit(
            &page->slot_state[slot], &expected, H8_SLOT_ALLOCATED,
            memory_order_acq_rel, memory_order_acquire)) {
      H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_HIT]);
      H8_DEBUG_INC(h9_local_arena_alloc_hit_class[page->class_id]);
      if (h9_local_arena_is_remote_safe(page)) {
        H8_DEBUG_INC(h9_local_arena_remote_safe_alloc_hit_class[page->class_id]);
      }
      h9_local_arena_note_alloc(page);
      return h9_local_slot_ptr(page, slot);
    }
    bits = atomic_load_explicit(&page->free_bits, memory_order_acquire);
  }
  return NULL;
}

static void* h9_local_arena_malloc(uint32_t class_id) {
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  if (!ctx || !ctx->owner || !h9_local_arena_class_enabled(class_id)) {
    return NULL;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  h9_local_arena_note_admission_shadow(ctx, class_id);
#endif
#if defined(H9_LOCAL_ARENA_REMOTE_DISABLE_L1)
  if (h9_local_arena_disabled[class_id]) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_FALLBACK]);
    return NULL;
  }
#endif
  H9LocalArenaPage* page = h9_local_arena_active[class_id];
  if (page && page->class_id == class_id &&
      page->owner_word == h9_local_owner_word_for(ctx->owner)) {
#if defined(H9_LOCAL_ARENA_REMOTE_DISABLE_L1)
    if (atomic_load_explicit(&page->remote_seen, memory_order_acquire)) {
      h9_local_arena_disabled[class_id] = 1u;
      h9_local_arena_active[class_id] = NULL;
      H8_DEBUG_INC(h9_local_arena_remote_disable_class[class_id]);
      H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_FALLBACK]);
      return NULL;
    }
#elif defined(H9_LOCAL_ARENA_REMOTE_RETIRE_L1)
    if (atomic_load_explicit(&page->remote_seen, memory_order_acquire)) {
      h9_local_arena_active[class_id] = NULL;
      H8_DEBUG_INC(h9_local_arena_remote_retire_class[class_id]);
      page = NULL;
    }
#endif
    if (page) {
      void* ptr = h9_local_alloc_from_page(page);
      if (ptr) {
        return ptr;
      }
    }
  }
#if defined(H9_LOCAL_ARENA_REMOTE_SEEN_ACTIVE_ONLY_L1)
  if (atomic_load_explicit(&h9_local_arena_remote_class_seen[class_id],
                           memory_order_acquire)) {
    H8_DEBUG_INC(h9_local_arena_remote_admit_block_class[class_id]);
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_FALLBACK]);
    return NULL;
  }
#endif
#if defined(H9_LOCAL_ARENA_REMOTE_ADMISSION_L1)
  if (atomic_load_explicit(&h9_local_arena_remote_class_seen[class_id],
                           memory_order_acquire)) {
    H8_DEBUG_INC(h9_local_arena_remote_admit_block_class[class_id]);
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_FALLBACK]);
    return NULL;
  }
#endif
#if defined(H9_LOCAL_ARENA_REMOTE_PROBATION_L1)
  if (atomic_load_explicit(&h9_local_arena_remote_class_seen[class_id],
                           memory_order_acquire)) {
    if (h9_local_arena_remote_probation[class_id] <
        (uint8_t)H9_LOCAL_ARENA_REMOTE_PROBATION_ALLOCS) {
      ++h9_local_arena_remote_probation[class_id];
      H8_DEBUG_INC(h9_local_arena_remote_probation_block_class[class_id]);
      H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_FALLBACK]);
      return NULL;
    }
    h9_local_arena_remote_probation[class_id] = 0u;
    H8_DEBUG_INC(h9_local_arena_remote_probation_allow_class[class_id]);
  }
#endif
  page = h9_local_create_page(ctx, class_id);
  if (!page) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_ALLOC_FALLBACK]);
    return NULL;
  }
  h9_local_arena_active[class_id] = page;
  return h9_local_alloc_from_page(page);
}

static bool h9_local_arena_free(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
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
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_INVALID]);
    return false;
  }
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  bool owner_match =
      ctx && ctx->owner && page->owner_word == h9_local_owner_word_for(ctx->owner);
  uint64_t bit = UINT64_C(1) << slot;
#if defined(H9_LOCAL_ARENA_OWNER_FAST_MUTATION_L1)
  if (owner_match && h9_local_arena_owner_fast_enabled(page)) {
    uint32_t state =
        atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
    if (state != H8_SLOT_ALLOCATED) {
      H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_INVALID]);
      return false;
    }
    atomic_store_explicit(&page->slot_state[slot], H8_SLOT_FREE | H8_SLOT_NONE,
                          memory_order_release);
    uint64_t old =
        atomic_load_explicit(&page->free_bits, memory_order_acquire);
    if ((old & bit) != 0) {
      H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_INVALID]);
      return false;
    }
    atomic_store_explicit(&page->free_bits, old | bit, memory_order_release);
    if (h9_local_arena_class_enabled(page->class_id)) {
      h9_local_arena_active[page->class_id] = page;
    }
    h9_local_arena_note_local_free(page);
    H8_DEBUG_INC(h9_local_arena_free_local_class[page->class_id]);
    H8_DEBUG_INC(h9_local_arena_ownerfast_free_class[page->class_id]);
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_VALID]);
    return true;
  }
#endif
  uint32_t expected = H8_SLOT_ALLOCATED;
  if (!atomic_compare_exchange_strong_explicit(
          &page->slot_state[slot], &expected, H8_SLOT_FREE | H8_SLOT_NONE,
          memory_order_acq_rel, memory_order_acquire)) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_INVALID]);
    return false;
  }
  uint64_t old =
      atomic_fetch_or_explicit(&page->free_bits, bit, memory_order_acq_rel);
  if ((old & bit) != 0) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_INVALID]);
  }
  if (!owner_match) {
    H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_REMOTE]);
    H8_DEBUG_INC(h9_local_arena_free_remote_class[page->class_id]);
    if (h9_local_arena_is_remote_safe(page)) {
      H8_DEBUG_INC(h9_local_arena_remote_safe_free_remote_class[page->class_id]);
    }
    uint32_t local_before_remote = h9_local_arena_note_remote_free(page);
    (void)local_before_remote;
    bool was_remote = atomic_exchange_explicit(
        &page->remote_observed, true, memory_order_acq_rel);
    if (!was_remote) {
      H8_DEBUG_INC(h9_local_arena_remote_first_class[page->class_id]);
#if defined(H8_ENABLE_DEBUG_STATS)
      uint32_t age =
          atomic_load_explicit(&page->local_alloc_count, memory_order_acquire);
      H8_DEBUG_INC(h9_local_arena_remote_first_alloc_age
                       [h9_local_arena_age_bucket(age)]);
      H8_DEBUG_INC(h9_local_arena_remote_first_local_free_history
                       [h9_local_arena_age_bucket(local_before_remote)]);
      H8_DEBUG_INC(h9_local_arena_remote_first_thread_page_seq
                       [h9_local_arena_age_bucket(page->creator_page_seq)]);
      H8_DEBUG_INC(h9_local_arena_remote_first_thread_alloc_history
                       [h9_local_arena_age_bucket(
                           page->creator_alloc_history)]);
#endif
    }
    bool was_class_remote = atomic_exchange_explicit(
        &h9_local_arena_remote_class_seen[page->class_id], true,
        memory_order_acq_rel);
    if (!was_class_remote) {
      H8_DEBUG_INC(h9_local_arena_admit_remote_class_first[page->class_id]);
    }
#if defined(H9_LOCAL_ARENA_REMOTE_DISABLE_L1) || \
    defined(H9_LOCAL_ARENA_REMOTE_RETIRE_L1) ||   \
    defined(H9_LOCAL_ARENA_REMOTE_SAFE_AFTER_CLASS_L1)
    atomic_store_explicit(&page->remote_seen, true, memory_order_release);
#endif
  } else if (h9_local_arena_class_enabled(page->class_id)) {
    h9_local_arena_note_local_free(page);
    H8_DEBUG_INC(h9_local_arena_free_local_class[page->class_id]);
    if (h9_local_arena_is_remote_safe(page)) {
      H8_DEBUG_INC(h9_local_arena_remote_safe_free_local_class[page->class_id]);
    }
#if defined(H9_LOCAL_ARENA_REMOTE_DISABLE_L1)
    if (!h9_local_arena_disabled[page->class_id] &&
        !atomic_load_explicit(&page->remote_seen, memory_order_acquire)) {
      h9_local_arena_active[page->class_id] = page;
    }
#elif defined(H9_LOCAL_ARENA_REMOTE_RETIRE_L1)
    if (!atomic_load_explicit(&page->remote_seen, memory_order_acquire)) {
      h9_local_arena_active[page->class_id] = page;
    }
#else
    h9_local_arena_active[page->class_id] = page;
#endif
  }
  H8_DEBUG_INC(h9_local_entry_event[H9_LOCAL_ENTRY_ARENA_FREE_VALID]);
  return true;
}
#endif
