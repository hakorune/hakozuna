#include "h8_internal.h"
#include "h8_medium.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#if defined(H8_ENABLE_DEBUG_STATS)
#include <time.h>
#endif

static pthread_mutex_t h8_medium_lock = PTHREAD_MUTEX_INITIALIZER;
static H8MediumRun* h8_medium_runs;
static _Atomic uintptr_t h8_medium_directory_addr;
static _Atomic uintptr_t h8_medium_min_addr;
static _Atomic uintptr_t h8_medium_max_addr;
#define H8_MEDIUM_DIRECTORY_CAP 4096u
static const size_t k_h8_medium_empty_resident_budget =
    (size_t)H8_OWNER_MAX * H8_MEDIUM_CLASS_COUNT * H8_MEDIUM_RUN_BYTES;

static const H8MediumClassSpec k_h8_medium_classes[H8_MEDIUM_CLASS_COUNT] = {
    {8192u, H8_MEDIUM_RUN_BYTES, 13u, 8u, 1u},
    {16384u, H8_MEDIUM_RUN_BYTES, 14u, 4u, 1u},
    {32768u, H8_MEDIUM_RUN_BYTES, 15u, 2u, 1u},
    {65536u, H8_MEDIUM_RUN_BYTES, 16u, 1u, 1u},
};

_Static_assert(H8_MEDIUM_MIN_SIZE == 4097u,
               "medium range must start immediately after small");
_Static_assert(H8_MEDIUM_MAX_SIZE == 65536u,
               "medium v1 scaffold currently ends at 64KiB");
_Static_assert(H8_MEDIUM_CLASS_COUNT == 4u,
               "medium v1 scaffold expects four coarse classes");

#if defined(H8_ENABLE_DEBUG_STATS)
static uint64_t h8_medium_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}
#endif

static void h8_medium_lock_global(void) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_now_ns();
#endif
  pthread_mutex_lock(&h8_medium_lock);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_global_lock_wait_ns,
               (size_t)(h8_medium_now_ns() - start));
#endif
}

static void h8_medium_unlock_global(void) {
  pthread_mutex_unlock(&h8_medium_lock);
}

static void h8_medium_lock_run(H8MediumRun* run) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_now_ns();
#endif
  pthread_mutex_lock(&run->lock);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_run_lock_wait_ns,
               (size_t)(h8_medium_now_ns() - start));
#endif
}

static void h8_medium_unlock_run(H8MediumRun* run) {
  pthread_mutex_unlock(&run->lock);
}

static uint64_t h8_medium_hash_base(uintptr_t base) {
  return (uint64_t)(base >> 16) * UINT64_C(11400714819323198485);
}

static void h8_medium_directory_note_range_locked(H8MediumRun* run) {
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t end = base + run->run_size;
  uintptr_t min =
      atomic_load_explicit(&h8_medium_min_addr, memory_order_relaxed);
  if (min == 0 || base < min) {
    atomic_store_explicit(&h8_medium_min_addr, base, memory_order_release);
  }
  uintptr_t max =
      atomic_load_explicit(&h8_medium_max_addr, memory_order_relaxed);
  if (end > max) {
    atomic_store_explicit(&h8_medium_max_addr, end, memory_order_release);
  }
}

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

static void* h8_medium_mmap_aligned_run(size_t run_size) {
  size_t reserve = run_size * 2u;
  uint8_t* raw = mmap(NULL, reserve, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS,
                      -1, 0);
  if (raw == MAP_FAILED) {
    return MAP_FAILED;
  }
  uintptr_t aligned =
      ((uintptr_t)raw + run_size - 1u) & ~(uintptr_t)(run_size - 1u);
  size_t prefix = (size_t)((uint8_t*)aligned - raw);
  size_t suffix = reserve - prefix - run_size;
  if (prefix != 0) {
    munmap(raw, prefix);
  }
  if (suffix != 0) {
    munmap((uint8_t*)aligned + run_size, suffix);
  }
  if (mprotect((void*)aligned, run_size, PROT_READ | PROT_WRITE) != 0) {
    munmap((void*)aligned, run_size);
    return MAP_FAILED;
  }
  return (void*)aligned;
}

static bool h8_medium_directory_ensure_locked(void) {
  if (atomic_load_explicit(&h8_medium_directory_addr, memory_order_acquire) != 0) {
    return true;
  }
  _Atomic(H8MediumRun*)* directory =
      h8_sys_calloc(H8_MEDIUM_DIRECTORY_CAP, sizeof(*directory));
  if (!directory) {
    return false;
  }
  atomic_store_explicit(&h8_medium_directory_addr, (uintptr_t)directory,
                        memory_order_release);
  return true;
}

static void h8_medium_directory_insert_locked(H8MediumRun* run) {
  if (!run || !run->base || !h8_medium_directory_ensure_locked()) {
    return;
  }
  h8_medium_directory_note_range_locked(run);
  _Atomic(H8MediumRun*)* directory = (_Atomic(H8MediumRun*)*)atomic_load_explicit(
      &h8_medium_directory_addr, memory_order_acquire);
  size_t mask = H8_MEDIUM_DIRECTORY_CAP - 1u;
  size_t pos = (size_t)h8_medium_hash_base((uintptr_t)run->base) & mask;
  for (size_t n = 0; n < H8_MEDIUM_DIRECTORY_CAP; ++n) {
    _Atomic(H8MediumRun*)* slot = &directory[(pos + n) & mask];
    H8MediumRun* cur = atomic_load_explicit(slot, memory_order_acquire);
    if (!cur || cur == run) {
      atomic_store_explicit(slot, run, memory_order_release);
      return;
    }
  }
}

static void h8_medium_directory_remove_locked(H8MediumRun* run) {
  _Atomic(H8MediumRun*)* directory = (_Atomic(H8MediumRun*)*)atomic_load_explicit(
      &h8_medium_directory_addr, memory_order_acquire);
  if (!run || !run->base || !directory) {
    return;
  }
  size_t mask = H8_MEDIUM_DIRECTORY_CAP - 1u;
  size_t pos = (size_t)h8_medium_hash_base((uintptr_t)run->base) & mask;
  for (size_t n = 0; n < H8_MEDIUM_DIRECTORY_CAP; ++n) {
    _Atomic(H8MediumRun*)* slot = &directory[(pos + n) & mask];
    H8MediumRun* cur = atomic_load_explicit(slot, memory_order_acquire);
    if (cur == run) {
      atomic_store_explicit(slot, NULL, memory_order_release);
      return;
    }
  }
}

static H8MediumRun* h8_medium_directory_find(const void* ptr) {
  _Atomic(H8MediumRun*)* directory = (_Atomic(H8MediumRun*)*)atomic_load_explicit(
      &h8_medium_directory_addr, memory_order_acquire);
  if (!directory || !ptr) {
    return NULL;
  }
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t min = atomic_load_explicit(&h8_medium_min_addr, memory_order_acquire);
  uintptr_t max = atomic_load_explicit(&h8_medium_max_addr, memory_order_acquire);
  if (min != 0 && (addr < min || addr >= max)) {
    return NULL;
  }
  uintptr_t base = addr & ~(uintptr_t)(H8_MEDIUM_RUN_BYTES - 1u);
  size_t mask = H8_MEDIUM_DIRECTORY_CAP - 1u;
  size_t pos = (size_t)h8_medium_hash_base(base) & mask;
  for (size_t n = 0; n < H8_MEDIUM_DIRECTORY_CAP; ++n) {
    H8MediumRun* run = atomic_load_explicit(&directory[(pos + n) & mask],
                                            memory_order_acquire);
    if (run && (uintptr_t)run->base == base && h8_medium_ptr_in_run(run, ptr)) {
      return run;
    }
  }
  return NULL;
}

static void h8_medium_update_resident_peak(size_t value) {
  size_t cur =
      atomic_load_explicit(&h8g.medium_resident_empty_peak, memory_order_relaxed);
  while (value > cur &&
         !atomic_compare_exchange_weak_explicit(
             &h8g.medium_resident_empty_peak, &cur, value,
             memory_order_relaxed, memory_order_relaxed)) {
  }
}

static bool h8_medium_try_reserve_empty_payload(H8MediumRun* run) {
  if (!run || run->resident_charge) {
    return true;
  }
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  size_t cur =
      atomic_load_explicit(&h8g.medium_resident_empty_bytes, memory_order_relaxed);
  for (;;) {
    if (cur > k_h8_medium_empty_resident_budget ||
        bytes > k_h8_medium_empty_resident_budget - cur) {
      H8_DEBUG_INC(medium_empty_budget_reject_count);
      return false;
    }
    size_t next = cur + bytes;
    if (atomic_compare_exchange_weak_explicit(
            &h8g.medium_resident_empty_bytes, &cur, next,
            memory_order_acq_rel, memory_order_relaxed)) {
      run->resident_charge = true;
      h8_medium_update_resident_peak(next);
      return true;
    }
  }
}

static void h8_medium_release_empty_payload(H8MediumRun* run) {
  if (!run || !run->resident_charge) {
    return;
  }
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  atomic_fetch_sub_explicit(&h8g.medium_resident_empty_bytes, bytes,
                            memory_order_acq_rel);
  run->resident_charge = false;
}

static void h8_medium_madvise_run(H8MediumRun* run) {
  if (!run || !run->base || run->run_size == 0) {
    return;
  }
  H8_DEBUG_INC(medium_run_madvise_count);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_now_ns();
#endif
  if (madvise(run->base, run->run_size, MADV_DONTNEED) != 0) {
    H8_DEBUG_INC(medium_madvise_fail_count);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_madvise_ns, (size_t)(h8_medium_now_ns() - start));
#endif
}

static void h8_medium_decommit_empty_locked(H8MediumRun* run) {
  if (!run || run->allocated_mask != 0) {
    return;
  }
  h8_medium_madvise_run(run);
  h8_medium_release_empty_payload(run);
  run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED;
}

static void h8_medium_mark_live_on_alloc(H8MediumRun* run) {
  if (!run || run->allocated_mask != 0) {
    return;
  }
  if (run->payload_state == H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT) {
    H8_DEBUG_INC(medium_empty_reactivate_count);
    h8_medium_release_empty_payload(run);
  }
  run->payload_state = H8_MEDIUM_PAYLOAD_LIVE;
}

static void h8_medium_mark_empty_locked(H8MediumRun* run) {
  if (!run || run->allocated_mask != 0) {
    return;
  }
  H8_DEBUG_INC(medium_empty_transition_count);
  if (run->owner_attached && h8_medium_try_reserve_empty_payload(run)) {
    H8_DEBUG_INC(medium_empty_retain_count);
    run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT;
    return;
  }
  h8_medium_decommit_empty_locked(run);
}

static H8MediumRun* h8_medium_find_run_locked(const void* ptr,
                                              bool route_lookup) {
  for (H8MediumRun* run = h8_medium_runs; run; run = run->next_global) {
    if (route_lookup) {
      H8_DEBUG_INC(medium_route_lookup_step_count);
    } else {
      H8_DEBUG_INC(medium_free_lookup_step_count);
    }
    if (h8_medium_ptr_in_run(run, ptr)) {
      return run;
    }
  }
  return NULL;
}

static void h8_medium_register_locked(H8MediumRun* run) {
  run->next_global = h8_medium_runs;
  h8_medium_runs = run;
  h8_medium_directory_insert_locked(run);
}

static void h8_medium_owner_add_run(H8ThreadCtx* ctx, H8MediumRun* run) {
  if (!ctx || !ctx->owner || !run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  run->owner_attached = true;
  run->next_owner = ctx->owner->medium_by_class[run->class_id];
  ctx->owner->medium_by_class[run->class_id] = run;
}

static void h8_medium_unregister_locked(H8MediumRun* run) {
  H8MediumRun** cur = &h8_medium_runs;
  while (*cur) {
    if (*cur == run) {
      *cur = run->next_global;
      run->next_global = NULL;
      h8_medium_directory_remove_locked(run);
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
  uintptr_t slot_mask = ((uintptr_t)1u << run->slot_shift) - 1u;
  if (offset >= payload || (offset & slot_mask) != 0u) {
    return false;
  }
  size_t slot = (size_t)(offset >> run->slot_shift);
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
  return run->base + (slot << run->slot_shift);
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
  void* payload = h8_medium_mmap_aligned_run(spec->run_size);
  if (payload == MAP_FAILED) {
    h8_medium_run_destroy_scaffold(run);
    return NULL;
  }
  run->base = payload;
  run->class_id = (uint16_t)class_id;
  run->slot_size = spec->slot_size;
  run->slot_shift = spec->slot_shift;
  run->slot_count = spec->slot_count;
  run->run_size = spec->run_size;
  run->free_mask = (run->slot_count == 64u)
                       ? UINT64_MAX
                       : ((UINT64_C(1) << run->slot_count) - 1u);
  run->allocated_mask = 0;
  run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED;
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
  h8_medium_lock_global();
  h8_medium_unregister_locked(run);
  h8_medium_unlock_global();
  if (run->base) {
    h8_medium_release_empty_payload(run);
    munmap(run->base, run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES);
  }
  h8_sys_free(run->pending_bits);
  h8_sys_free(run->slot_state);
  pthread_mutex_destroy(&run->lock);
  h8_sys_free(run);
}

void* h8_medium_run_alloc_local_scaffold(H8MediumRun* run) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_now_ns();
#endif
  if (!run || atomic_load_explicit(&run->state, memory_order_acquire) !=
                  H8_MEDIUM_RUN_ACTIVE ||
      run->free_mask == 0) {
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(run->free_mask);
  uint64_t bit = UINT64_C(1) << slot;
  h8_medium_mark_live_on_alloc(run);
  run->free_mask &= ~bit;
  run->allocated_mask |= bit;
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(medium_alloc_slot_count);
  H8_DEBUG_ADD(medium_alloc_slot_ns, (size_t)(h8_medium_now_ns() - start));
#endif
  return h8_medium_slot_ptr(run, slot);
}

bool h8_medium_run_free_local_scaffold(H8MediumRun* run, void* ptr) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_now_ns();
#endif
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
  h8_medium_mark_empty_locked(run);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(medium_free_slot_count);
  H8_DEBUG_ADD(medium_free_slot_ns, (size_t)(h8_medium_now_ns() - start));
#endif
  return true;
}

void* h8_medium_malloc_inner(size_t size) {
  if (!h8_medium_size_supported(size)) {
    return NULL;
  }
  H8_DEBUG_INC(medium_malloc_count);
  uint32_t class_id = h8_medium_class_for_size(size);
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  H8MediumRun* active = ctx ? ctx->active_medium_runs[class_id] : NULL;
  if (active) {
    h8_medium_lock_run(active);
    if (h8_medium_run_usable_locked(active, class_id)) {
      H8_DEBUG_INC(medium_run_reuse_active_count);
      void* ptr = h8_medium_run_alloc_local_scaffold(active);
      h8_medium_unlock_run(active);
      return ptr;
    }
    h8_medium_unlock_run(active);
  }
  if (ctx && ctx->owner) {
    H8_DEBUG_INC(medium_owner_scan_count);
    for (H8MediumRun* run = ctx->owner->medium_by_class[class_id]; run;
         run = run->next_owner) {
      H8_DEBUG_INC(medium_owner_scan_step_count);
      h8_medium_lock_run(run);
      if (h8_medium_run_usable_locked(run, class_id)) {
        H8_DEBUG_INC(medium_run_reuse_owner_list_count);
        void* ptr = h8_medium_run_alloc_local_scaffold(run);
        ctx->active_medium_runs[class_id] = run;
        h8_medium_unlock_run(run);
        return ptr;
      }
      h8_medium_unlock_run(run);
    }
  }
  h8_medium_lock_global();
  H8_DEBUG_INC(medium_global_scan_count);
  for (H8MediumRun* run = h8_medium_runs; run; run = run->next_global) {
    H8_DEBUG_INC(medium_global_scan_step_count);
    h8_medium_lock_run(run);
    if (h8_medium_run_usable_locked(run, class_id)) {
      H8_DEBUG_INC(medium_run_reuse_global_count);
      if (ctx && ctx->owner && !run->owner_attached) {
        h8_medium_owner_add_run(ctx, run);
      }
      void* ptr = h8_medium_run_alloc_local_scaffold(run);
      if (ctx) {
        ctx->active_medium_runs[class_id] = run;
      }
      h8_medium_unlock_run(run);
      h8_medium_unlock_global();
      return ptr;
    }
    h8_medium_unlock_run(run);
  }
  h8_medium_unlock_global();

  H8MediumRun* run = h8_medium_run_create_scaffold(class_id);
  if (!run) {
    return NULL;
  }
  H8_DEBUG_INC(medium_run_create_count);
  h8_medium_lock_global();
  h8_medium_register_locked(run);
  h8_medium_owner_add_run(ctx, run);
  h8_medium_lock_run(run);
  void* ptr = h8_medium_run_alloc_local_scaffold(run);
  if (ctx) {
    ctx->active_medium_runs[class_id] = run;
  }
  h8_medium_unlock_run(run);
  h8_medium_unlock_global();
  return ptr;
}

bool h8_medium_free_inner(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  H8_DEBUG_INC(medium_free_lookup_count);
  H8MediumRun* run = h8_medium_directory_find(ptr);
  if (run) {
    h8_medium_lock_run(run);
  } else {
    h8_medium_lock_global();
    run = h8_medium_find_run_locked(ptr, false);
    if (!run) {
      h8_medium_unlock_global();
      return false;
    }
    h8_medium_lock_run(run);
    h8_medium_unlock_global();
  }
  if (owned_out) {
    *owned_out = true;
  }
  bool ok = h8_medium_run_free_local_scaffold(run, ptr);
  if (!ok) {
    H8_DEBUG_INC(medium_invalid_owned_count);
  }
  if (ok) {
    H8ThreadCtx* ctx = h8_tls_ctx;
    if (ctx && run->class_id < H8_MEDIUM_CLASS_COUNT) {
      ctx->active_medium_runs[run->class_id] = run;
    }
  }
  h8_medium_unlock_run(run);
  return ok;
}

H8RouteKind h8_medium_route_inner(void* ptr) {
  H8_DEBUG_INC(medium_route_lookup_count);
  H8MediumRun* run = h8_medium_directory_find(ptr);
  if (run) {
    h8_medium_lock_run(run);
  } else {
    h8_medium_lock_global();
    run = h8_medium_find_run_locked(ptr, true);
    if (!run) {
      h8_medium_unlock_global();
      return H8_ROUTE_MISS;
    }
    h8_medium_lock_run(run);
    h8_medium_unlock_global();
  }
  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked(run, ptr, &slot)) {
    h8_medium_unlock_run(run);
    return H8_ROUTE_INVALID;
  }
  uint64_t bit = UINT64_C(1) << slot;
  H8RouteKind route =
      ((run->allocated_mask & bit) != 0 && (run->free_mask & bit) == 0)
          ? H8_ROUTE_VALID
          : H8_ROUTE_INVALID;
  h8_medium_unlock_run(run);
  return route;
}

void h8_medium_owner_detach_all(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  h8_medium_lock_global();
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    H8MediumRun* run = owner->medium_by_class[c];
    owner->medium_by_class[c] = NULL;
    while (run) {
      H8MediumRun* next = run->next_owner;
      h8_medium_lock_run(run);
      run->owner_attached = false;
      if (run->payload_state == H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT) {
        H8_DEBUG_INC(medium_owner_exit_drain_count);
        h8_medium_decommit_empty_locked(run);
      }
      h8_medium_unlock_run(run);
      run->next_owner = NULL;
      run = next;
    }
  }
  h8_medium_unlock_global();
}
