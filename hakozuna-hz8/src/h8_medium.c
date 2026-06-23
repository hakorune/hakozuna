#include "h8_internal.h"
#include "h8_medium.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

static pthread_mutex_t h8_medium_lock = PTHREAD_MUTEX_INITIALIZER;
static H8MediumRun* h8_medium_runs;

static const H8MediumClassSpec k_h8_medium_classes[H8_MEDIUM_CLASS_COUNT] = {
    {8192u, H8_MEDIUM_RUN_BYTES, 8u, 1u},
    {16384u, H8_MEDIUM_RUN_BYTES, 4u, 1u},
    {32768u, H8_MEDIUM_RUN_BYTES, 2u, 1u},
    {65536u, H8_MEDIUM_RUN_BYTES, 1u, 1u},
};

_Static_assert(H8_MEDIUM_MIN_SIZE == 4097u,
               "medium range must start immediately after small");
_Static_assert(H8_MEDIUM_MAX_SIZE == 65536u,
               "medium v1 scaffold currently ends at 64KiB");
_Static_assert(H8_MEDIUM_CLASS_COUNT == 4u,
               "medium v1 scaffold expects four coarse classes");

bool h8_medium_size_supported(size_t size) {
  return size >= H8_MEDIUM_MIN_SIZE && size <= H8_MEDIUM_MAX_SIZE;
}

uint32_t h8_medium_class_for_size(size_t size) {
  if (size <= 8192u) {
    return 0u;
  }
  if (size <= 16384u) {
    return 1u;
  }
  if (size <= 32768u) {
    return 2u;
  }
  return 3u;
}

const H8MediumClassSpec* h8_medium_class_spec(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  return &k_h8_medium_classes[class_id];
}

uint32_t h8_medium_rounded_size(size_t size) {
  if (!h8_medium_size_supported(size)) {
    return 0u;
  }
  return k_h8_medium_classes[h8_medium_class_for_size(size)].slot_size;
}

static bool h8_medium_ptr_in_run(const H8MediumRun* run, const void* ptr) {
  if (!run || !run->base || !ptr) {
    return false;
  }
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t addr = (uintptr_t)ptr;
  size_t payload = (size_t)run->slot_size * (size_t)run->slot_count;
  return addr >= base && addr < base + payload;
}

static H8MediumRun* h8_medium_find_run_locked(const void* ptr) {
  for (H8MediumRun* run = h8_medium_runs; run; run = run->next_global) {
    if (h8_medium_ptr_in_run(run, ptr)) {
      return run;
    }
  }
  return NULL;
}

static void h8_medium_register_locked(H8MediumRun* run) {
  run->next_global = h8_medium_runs;
  h8_medium_runs = run;
}

static void h8_medium_owner_add_run(H8ThreadCtx* ctx, H8MediumRun* run) {
  if (!ctx || !ctx->owner || !run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  run->next_owner = ctx->owner->medium_by_class[run->class_id];
  ctx->owner->medium_by_class[run->class_id] = run;
}

static void h8_medium_unregister_locked(H8MediumRun* run) {
  H8MediumRun** cur = &h8_medium_runs;
  while (*cur) {
    if (*cur == run) {
      *cur = run->next_global;
      run->next_global = NULL;
      return;
    }
    cur = &(*cur)->next_global;
  }
}

static bool h8_medium_run_usable_locked(const H8MediumRun* run,
                                        uint32_t class_id) {
  return run && run->class_id == class_id && run->free_mask != 0 &&
         atomic_load_explicit(&run->state, memory_order_acquire) ==
             H8_MEDIUM_RUN_ACTIVE;
}

bool h8_medium_slot_index_from_ptr_checked(const H8MediumRun* run,
                                           const void* ptr,
                                           size_t* slot_out) {
  if (!run || !run->base || !ptr || run->slot_size == 0u ||
      run->slot_count == 0u) {
    return false;
  }
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t addr = (uintptr_t)ptr;
  if (addr < base) {
    return false;
  }
  uintptr_t offset = addr - base;
  size_t payload = (size_t)run->slot_size * (size_t)run->slot_count;
  if (offset >= payload || (offset % run->slot_size) != 0u) {
    return false;
  }
  size_t slot = (size_t)(offset / run->slot_size);
  if (slot >= run->slot_count) {
    return false;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return true;
}

void* h8_medium_slot_ptr(const H8MediumRun* run, size_t slot) {
  if (!run || !run->base || slot >= run->slot_count) {
    return NULL;
  }
  return run->base + slot * (size_t)run->slot_size;
}

H8MediumRun* h8_medium_run_create_scaffold(uint32_t class_id) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec) {
    return NULL;
  }
  H8MediumRun* run = h8_sys_calloc(1, sizeof(*run));
  if (!run) {
    return NULL;
  }
  pthread_mutex_init(&run->lock, NULL);
  run->slot_state = h8_sys_calloc(spec->slot_count, sizeof(*run->slot_state));
  run->pending_bits = h8_sys_calloc(spec->bitmap_words, sizeof(*run->pending_bits));
  if (!run->slot_state || !run->pending_bits) {
    h8_medium_run_destroy_scaffold(run);
    return NULL;
  }
  void* payload = mmap(NULL, spec->run_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (payload == MAP_FAILED) {
    h8_medium_run_destroy_scaffold(run);
    return NULL;
  }
  run->base = payload;
  run->class_id = (uint16_t)class_id;
  run->slot_size = spec->slot_size;
  run->slot_count = spec->slot_count;
  run->run_size = spec->run_size;
  run->free_mask = (run->slot_count == 64u)
                       ? UINT64_MAX
                       : ((UINT64_C(1) << run->slot_count) - 1u);
  run->allocated_mask = 0;
  atomic_store_explicit(&run->state, H8_MEDIUM_RUN_ACTIVE,
                        memory_order_relaxed);
  atomic_store_explicit(&run->pending_word_mask, 0, memory_order_relaxed);
  for (uint16_t i = 0; i < run->slot_count; ++i) {
    atomic_store_explicit(&run->slot_state[i], H8_SLOT_FREE | H8_SLOT_NONE,
                          memory_order_relaxed);
  }
  return run;
}

void h8_medium_run_destroy_scaffold(H8MediumRun* run) {
  if (!run) {
    return;
  }
  pthread_mutex_lock(&h8_medium_lock);
  h8_medium_unregister_locked(run);
  pthread_mutex_unlock(&h8_medium_lock);
  if (run->base) {
    munmap(run->base, run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES);
  }
  h8_sys_free(run->pending_bits);
  h8_sys_free(run->slot_state);
  pthread_mutex_destroy(&run->lock);
  h8_sys_free(run);
}

void* h8_medium_run_alloc_local_scaffold(H8MediumRun* run) {
  if (!run || atomic_load_explicit(&run->state, memory_order_acquire) !=
                  H8_MEDIUM_RUN_ACTIVE ||
      run->free_mask == 0) {
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(run->free_mask);
  uint64_t bit = UINT64_C(1) << slot;
  run->free_mask &= ~bit;
  run->allocated_mask |= bit;
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
  return h8_medium_slot_ptr(run, slot);
}

bool h8_medium_run_free_local_scaffold(H8MediumRun* run, void* ptr) {
  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked(run, ptr, &slot)) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((run->allocated_mask & bit) == 0 || (run->free_mask & bit) != 0 ||
      atomic_load_explicit(&run->pending_bits[slot >> 6u],
                           memory_order_acquire) &
          (UINT64_C(1) << (slot & 63u))) {
    return false;
  }
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_FREE | H8_SLOT_NONE,
                        memory_order_release);
  run->allocated_mask &= ~bit;
  run->free_mask |= bit;
  if (run->allocated_mask == 0 && run->base && run->run_size != 0) {
    madvise(run->base, run->run_size, MADV_DONTNEED);
  }
  return true;
}

void* h8_medium_malloc_inner(size_t size) {
  if (!h8_medium_size_supported(size)) {
    return NULL;
  }
  uint32_t class_id = h8_medium_class_for_size(size);
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  H8MediumRun* active = ctx ? ctx->active_medium_runs[class_id] : NULL;
  if (active) {
    pthread_mutex_lock(&active->lock);
    if (h8_medium_run_usable_locked(active, class_id)) {
      void* ptr = h8_medium_run_alloc_local_scaffold(active);
      pthread_mutex_unlock(&active->lock);
      return ptr;
    }
    pthread_mutex_unlock(&active->lock);
  }
  if (ctx && ctx->owner) {
    for (H8MediumRun* run = ctx->owner->medium_by_class[class_id]; run;
         run = run->next_owner) {
      pthread_mutex_lock(&run->lock);
      if (h8_medium_run_usable_locked(run, class_id)) {
        void* ptr = h8_medium_run_alloc_local_scaffold(run);
        ctx->active_medium_runs[class_id] = run;
        pthread_mutex_unlock(&run->lock);
        return ptr;
      }
      pthread_mutex_unlock(&run->lock);
    }
  }
  pthread_mutex_lock(&h8_medium_lock);
  for (H8MediumRun* run = h8_medium_runs; run; run = run->next_global) {
    pthread_mutex_lock(&run->lock);
    if (h8_medium_run_usable_locked(run, class_id)) {
      void* ptr = h8_medium_run_alloc_local_scaffold(run);
      if (ctx) {
        ctx->active_medium_runs[class_id] = run;
      }
      pthread_mutex_unlock(&run->lock);
      pthread_mutex_unlock(&h8_medium_lock);
      return ptr;
    }
    pthread_mutex_unlock(&run->lock);
  }
  pthread_mutex_unlock(&h8_medium_lock);

  H8MediumRun* run = h8_medium_run_create_scaffold(class_id);
  if (!run) {
    return NULL;
  }
  pthread_mutex_lock(&h8_medium_lock);
  h8_medium_register_locked(run);
  h8_medium_owner_add_run(ctx, run);
  pthread_mutex_lock(&run->lock);
  void* ptr = h8_medium_run_alloc_local_scaffold(run);
  if (ctx) {
    ctx->active_medium_runs[class_id] = run;
  }
  pthread_mutex_unlock(&run->lock);
  pthread_mutex_unlock(&h8_medium_lock);
  return ptr;
}

bool h8_medium_free_inner(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  pthread_mutex_lock(&h8_medium_lock);
  H8MediumRun* run = h8_medium_find_run_locked(ptr);
  if (!run) {
    pthread_mutex_unlock(&h8_medium_lock);
    return false;
  }
  if (owned_out) {
    *owned_out = true;
  }
  pthread_mutex_lock(&run->lock);
  pthread_mutex_unlock(&h8_medium_lock);
  bool ok = h8_medium_run_free_local_scaffold(run, ptr);
  if (ok) {
    H8ThreadCtx* ctx = h8_tls_ctx;
    if (ctx && run->class_id < H8_MEDIUM_CLASS_COUNT) {
      ctx->active_medium_runs[run->class_id] = run;
    }
  }
  pthread_mutex_unlock(&run->lock);
  return ok;
}

H8RouteKind h8_medium_route_inner(void* ptr) {
  pthread_mutex_lock(&h8_medium_lock);
  H8MediumRun* run = h8_medium_find_run_locked(ptr);
  if (!run) {
    pthread_mutex_unlock(&h8_medium_lock);
    return H8_ROUTE_MISS;
  }
  pthread_mutex_lock(&run->lock);
  pthread_mutex_unlock(&h8_medium_lock);
  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked(run, ptr, &slot)) {
    pthread_mutex_unlock(&run->lock);
    return H8_ROUTE_INVALID;
  }
  uint64_t bit = UINT64_C(1) << slot;
  H8RouteKind route =
      ((run->allocated_mask & bit) != 0 && (run->free_mask & bit) == 0)
          ? H8_ROUTE_VALID
          : H8_ROUTE_INVALID;
  pthread_mutex_unlock(&run->lock);
  return route;
}

void h8_medium_owner_detach_all(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  pthread_mutex_lock(&h8_medium_lock);
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    H8MediumRun* run = owner->medium_by_class[c];
    owner->medium_by_class[c] = NULL;
    while (run) {
      H8MediumRun* next = run->next_owner;
      run->next_owner = NULL;
      run = next;
    }
  }
  pthread_mutex_unlock(&h8_medium_lock);
}
