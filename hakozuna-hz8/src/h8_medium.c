#include "h8_internal.h"
#include "h8_medium.h"

#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#if defined(H8_ENABLE_DEBUG_STATS)
#include <time.h>
#endif

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

static uint64_t h8_medium_owner_word_for(const H8OwnerRecord* owner) {
  if (!owner || owner->slot >= H8_OWNER_MAX) {
    return 0;
  }
  H8OwnerWord word = h8_owner_word_make((uint8_t)owner->slot,
                                        (uint16_t)owner->generation,
                                        H8_SPAN_OWNED_ACTIVE, 0);
  return h8_owner_word_pack(word);
}

static void h8_medium_debug_lock_elide_candidate(const H8MediumRun* run,
                                                 const H8ThreadCtx* ctx,
                                                 bool is_free) {
#if defined(H8_ENABLE_DEBUG_STATS)
  if (!run || !ctx || !ctx->owner) {
    H8_DEBUG_INC(medium_lock_elide_owner_mismatch);
    return;
  }
  if (h8_medium_run_owned_by_ctx(run, ctx)) {
    if (is_free) {
      H8_DEBUG_INC(medium_lock_elide_free_candidate);
    } else {
      H8_DEBUG_INC(medium_lock_elide_alloc_candidate);
    }
  } else {
    H8_DEBUG_INC(medium_lock_elide_owner_mismatch);
  }
#else
  (void)run;
  (void)ctx;
  (void)is_free;
#endif
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

static void h8_medium_owner_add_run(H8ThreadCtx* ctx, H8MediumRun* run) {
  if (!ctx || !ctx->owner || !run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  run->owner_attached = true;
  atomic_store_explicit(&run->owner_word,
                        h8_medium_owner_word_for(ctx->owner),
                        memory_order_release);
  run->next_owner = ctx->owner->medium_by_class[run->class_id];
  ctx->owner->medium_by_class[run->class_id] = run;
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
  if (!run || !run->base || !ptr || run->slot_size == 0u || run->slot_count == 0u) {
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
  atomic_store_explicit(&run->qstate, H8_Q_IDLE, memory_order_relaxed);
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
  if (!run || atomic_load_explicit(&run->state, memory_order_acquire) != H8_MEDIUM_RUN_ACTIVE || run->free_mask == 0) {
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
      atomic_load_explicit(&run->pending_bits[slot >> 6u], memory_order_acquire) & (UINT64_C(1) << (slot & 63u))) {
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
  bool did_capacity_collect = false;
retry_owner_capacity:
  H8MediumRun* active = ctx ? ctx->active_medium_runs[class_id] : NULL;
  if (active) {
    if (!h8_medium_run_owned_by_ctx(active, ctx)) {
      H8_DEBUG_INC(medium_active_alloc_owner_mismatch);
      ctx->active_medium_runs[class_id] = NULL;
      active = NULL;
    }
  }
  if (active) {
    if (h8_medium_run_owned_by_ctx(active, ctx) &&
        h8_medium_run_usable_locked(active, class_id)) {
      h8_medium_debug_lock_elide_candidate(active, ctx, false);
      h8_medium_debug_writer_enter(active, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
      H8_DEBUG_INC(medium_run_reuse_active_count);
      void* ptr = h8_medium_run_alloc_local_scaffold(active);
      h8_medium_debug_writer_exit(active);
      h8_medium_collect_owner_pending_periodic(ctx);
      return ptr;
    }
    if (!h8_medium_run_owned_by_ctx(active, ctx)) {
      H8_DEBUG_INC(medium_active_alloc_owner_mismatch);
      ctx->active_medium_runs[class_id] = NULL;
    }
  }
  if (ctx && ctx->owner) {
    H8_DEBUG_INC(medium_owner_scan_count);
    for (H8MediumRun* run = ctx->owner->medium_by_class[class_id]; run;
         run = run->next_owner) {
      H8_DEBUG_INC(medium_owner_scan_step_count);
      if (!h8_medium_run_owned_by_ctx(run, ctx)) {
        H8_DEBUG_INC(medium_owner_list_owner_mismatch);
        continue;
      }
      h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
      h8_medium_lock_run(run);
      if (h8_medium_run_owned_by_ctx(run, ctx) &&
          h8_medium_run_usable_locked(run, class_id)) {
        h8_medium_debug_lock_elide_candidate(run, ctx, false);
        H8_DEBUG_INC(medium_run_reuse_owner_list_count);
        void* ptr = h8_medium_run_alloc_local_scaffold(run);
        ctx->active_medium_runs[class_id] = run;
        h8_medium_unlock_run(run);
        h8_medium_debug_writer_exit(run);
        h8_medium_collect_owner_pending_periodic(ctx);
        return ptr;
      }
      if (!h8_medium_run_owned_by_ctx(run, ctx)) {
        H8_DEBUG_INC(medium_owner_list_owner_mismatch);
      }
      h8_medium_unlock_run(run);
      h8_medium_debug_writer_exit(run);
    }
  }
  if (ctx && ctx->owner && !did_capacity_collect &&
      h8_medium_owner_has_pending(ctx->owner)) {
    did_capacity_collect = true;
    h8_medium_collect_owner_pending(ctx->owner);
    goto retry_owner_capacity;
  }
  h8_medium_lock_global();
  H8_DEBUG_INC(medium_global_scan_count);
  for (H8MediumRun* run = h8_medium_global_head(); run;
       run = run->next_global) {
    H8_DEBUG_INC(medium_global_scan_step_count);
    h8_medium_lock_run(run);
    if (run->owner_attached ||
        atomic_load_explicit(&run->owner_word, memory_order_acquire) != 0) {
      H8_DEBUG_INC(medium_global_skip_foreign_attached);
      h8_medium_unlock_run(run);
      continue;
    }
    if (h8_medium_run_usable_locked(run, class_id)) {
      H8_DEBUG_INC(medium_run_reuse_global_count);
      if (ctx && ctx->owner) {
        h8_medium_owner_add_run(ctx, run);
      }
      h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_GLOBAL_ATTACH);
      void* ptr = h8_medium_run_alloc_local_scaffold(run);
      h8_medium_debug_writer_exit(run);
      if (ctx && h8_medium_run_owned_by_ctx(run, ctx)) {
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
  h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                               H8_MEDIUM_WRITER_GLOBAL_ATTACH);
  h8_medium_lock_run(run);
  void* ptr = h8_medium_run_alloc_local_scaffold(run);
  if (ctx) {
    ctx->active_medium_runs[class_id] = run;
  }
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
  h8_medium_unlock_global();
  return ptr;
}

bool h8_medium_free_inner(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  H8_DEBUG_INC(medium_free_lookup_count);
  H8MediumRun* run = h8_medium_directory_find(ptr);
  H8ThreadCtx* ctx = h8_tls_ctx;
  if (run && !h8_medium_run_owned_by_ctx(run, ctx) &&
      atomic_load_explicit(&run->owner_word, memory_order_acquire) != 0) {
    H8_DEBUG_INC(medium_remote_free_owner_mismatch);
    if (owned_out) { *owned_out = true; }
    for (;;) {
      H8PublishResult res = h8_medium_remote_publish(run, ptr);
      if (res == H8_PUBLISH_OK) {
        return true;
      }
      if (res != H8_PUBLISH_OWNER_TRANSITION) {
        return false;
      }
      sched_yield();
    }
  }
  if (run) {
    h8_medium_debug_lock_elide_candidate(run, ctx, true);
    bool same_owner = h8_medium_run_owned_by_ctx(run, ctx);
    if (same_owner) {
      H8_DEBUG_INC(medium_local_free_owner_match);
      h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_FREE);
      if (owned_out) {
        *owned_out = true;
      }
      bool ok = h8_medium_run_free_local_scaffold(run, ptr);
      if (!ok) {
        H8_DEBUG_INC(medium_invalid_owned_count);
      }
      if (ok && ctx && run->class_id < H8_MEDIUM_CLASS_COUNT) {
        ctx->active_medium_runs[run->class_id] = run;
      }
      h8_medium_debug_writer_exit(run);
      return ok;
    } else {
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
    }
    h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                 H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE);
    h8_medium_lock_run(run);
  } else {
    h8_medium_lock_global();
    run = h8_medium_find_run_locked(ptr, false);
    if (!run) {
      h8_medium_unlock_global();
      return false;
    }
    if (!h8_medium_run_owned_by_ctx(run, ctx) &&
        atomic_load_explicit(&run->owner_word, memory_order_acquire) != 0) {
      h8_medium_unlock_global();
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
      if (owned_out) { *owned_out = true; }
      for (;;) {
        H8PublishResult res = h8_medium_remote_publish(run, ptr);
        if (res == H8_PUBLISH_OK) {
          return true;
        }
        if (res != H8_PUBLISH_OWNER_TRANSITION) {
          return false;
        }
        sched_yield();
      }
    }
    if (h8_medium_run_owned_by_ctx(run, ctx)) {
      H8_DEBUG_INC(medium_local_free_owner_match);
    } else {
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
    }
    h8_medium_debug_writer_enter(
        run, ctx ? ctx->owner : NULL,
        h8_medium_run_owned_by_ctx(run, ctx)
            ? H8_MEDIUM_WRITER_OWNER_LOCAL_FREE
            : H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE);
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
    if (ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
        h8_medium_run_owned_by_ctx(run, ctx)) {
      ctx->active_medium_runs[run->class_id] = run;
    }
  }
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
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
  bool pending = (atomic_load_explicit(&run->pending_bits[0],
                                       memory_order_acquire) &
                  bit) != 0;
  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
  bool slot_authority_valid =
      h8_slot_state_tag(state) == (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT) &&
      !pending;
#if defined(H8_ENABLE_DEBUG_STATS)
  bool mask_authority_valid =
      (run->allocated_mask & bit) != 0 && (run->free_mask & bit) == 0 &&
      !pending;
  if (slot_authority_valid != mask_authority_valid) {
    H8_DEBUG_INC(medium_route_authority_mismatch);
  }
#endif
  H8RouteKind route = slot_authority_valid ? H8_ROUTE_VALID : H8_ROUTE_INVALID;
  h8_medium_unlock_run(run);
  return route;
}

void h8_medium_owner_detach_all(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  h8_medium_collect_owner_pending(owner);
  h8_medium_lock_global();
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    H8MediumRun* run = owner->medium_by_class[c];
    owner->medium_by_class[c] = NULL;
    while (run) {
      H8MediumRun* next = run->next_owner;
      h8_medium_debug_writer_enter(run, owner, H8_MEDIUM_WRITER_OWNER_DETACH);
      h8_medium_lock_run(run);
      run->owner_attached = false;
      atomic_store_explicit(&run->owner_word, 0, memory_order_release);
      if (run->payload_state == H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT) {
        H8_DEBUG_INC(medium_owner_exit_drain_count);
        h8_medium_decommit_empty_locked(run);
      }
      h8_medium_unlock_run(run);
      h8_medium_debug_writer_exit(run);
      run->next_owner = NULL;
      run = next;
    }
  }
  h8_medium_unlock_global();
}
