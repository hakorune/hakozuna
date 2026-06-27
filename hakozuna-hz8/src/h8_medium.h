#ifndef H8_MEDIUM_H
#define H8_MEDIUM_H

#include "../include/h8.h"
#include "h8_class_map.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <stdint.h>

#define H8_MEDIUM_MIN_SIZE (H8_MAX_SMALL_SIZE + 1u)
#define H8_MEDIUM_MAX_SIZE 65536u
#define H8_MEDIUM_QUANTUM_BYTES 65536u
#define H8_MEDIUM_RUN_BYTES 65536u
#if defined(H8_MEDIUM_64K_ONE_SLOT)
#define H8_MEDIUM_64K_RUN_BYTES H8_MEDIUM_RUN_BYTES
#define H8_MEDIUM_64K_SLOT_COUNT 1u
#else
#define H8_MEDIUM_64K_RUN_BYTES (2u * H8_MEDIUM_QUANTUM_BYTES)
#define H8_MEDIUM_64K_SLOT_COUNT 2u
#endif
#define H8_MEDIUM_PAGE_BYTES 4096u
#define H8_MEDIUM_RETENTION_STACK_DEPTH 8u
#if !defined(H8_MEDIUM_RESIDENT_BUDGET_CLASSES)
#define H8_MEDIUM_RESIDENT_BUDGET_CLASSES 16u
#endif
#if defined(H8_MEDIUM_V12_48K2_CLASS) && defined(H8_MEDIUM_LEGACY_Q64_RUN64K2)
#error "H8_MEDIUM_V12_48K2_CLASS and H8_MEDIUM_LEGACY_Q64_RUN64K2 are mutually exclusive"
#endif
#if defined(H8_MEDIUM_V12_48K2_CLASS) && defined(H8_MEDIUM_UPPER48_CLASS)
#error "H8_MEDIUM_V12_48K2_CLASS and H8_MEDIUM_UPPER48_CLASS are mutually exclusive"
#endif
#if !defined(H8_MEDIUM_LEGACY_Q64_RUN64K2) && \
    !defined(H8_MEDIUM_UPPER48_CLASS)
#define H8_MEDIUM_V12_48K2_CLASS 1
#endif
#if defined(H8_MEDIUM_V12_48K2_CLASS) && defined(H8_MEDIUM_64K_ONE_SLOT)
#error "H8_MEDIUM_V12_48K2_CLASS requires the default 64K two-slot geometry"
#endif
#if defined(H8_MEDIUM_V12_48K2_CLASS)
#define H8_MEDIUM_CLASS_COUNT 6u
#elif defined(H8_MEDIUM_UPPER48_CLASS)
#define H8_MEDIUM_CLASS_COUNT 5u
#else
#define H8_MEDIUM_CLASS_COUNT 4u
#endif
typedef enum H8MediumRunState {
  H8_MEDIUM_RUN_UNUSED = 0,
  H8_MEDIUM_RUN_ACTIVE = 1,
  H8_MEDIUM_RUN_RETIRING = 2,
  H8_MEDIUM_RUN_RETIRED = 3
} H8MediumRunState;

typedef enum H8MediumPayloadState {
  H8_MEDIUM_PAYLOAD_LIVE = 0,
  H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT = 1,
  H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED = 2
} H8MediumPayloadState;

typedef enum H8MediumWriterKind {
  H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC = 1,
  H8_MEDIUM_WRITER_OWNER_LOCAL_FREE = 2,
  H8_MEDIUM_WRITER_OWNER_COLLECT = 3,
  H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE = 4,
  H8_MEDIUM_WRITER_GLOBAL_ATTACH = 5,
  H8_MEDIUM_WRITER_OWNER_DETACH = 6
} H8MediumWriterKind;

typedef enum H8MediumDecommitReason {
  H8_MEDIUM_DECOMMIT_COLD = 1,
  H8_MEDIUM_DECOMMIT_BUDGET_REJECT = 2,
  H8_MEDIUM_DECOMMIT_OWNER_EXIT = 3
} H8MediumDecommitReason;

typedef enum H8MediumCollectSource {
  H8_MEDIUM_COLLECT_PERIODIC_ACTIVE = 1,
  H8_MEDIUM_COLLECT_PERIODIC_OWNER_LIST = 2,
  H8_MEDIUM_COLLECT_CAPACITY_MISS = 3,
  H8_MEDIUM_COLLECT_OWNER_EXIT = 4,
  H8_MEDIUM_COLLECT_ACTIVE_MISS_DEMAND = 5
} H8MediumCollectSource;

typedef struct H8MediumClassSpec {
  uint32_t slot_size;
  uint32_t run_size;
  uint16_t slot_shift;
  uint16_t slot_count;
  uint16_t bitmap_words;
} H8MediumClassSpec;

typedef struct H8ThreadCtx H8ThreadCtx;

typedef struct H8MediumRun {
  uint8_t* base;
  uint16_t class_id;
  uint16_t slot_count;
  uint32_t slot_size;
  uint16_t slot_shift;
  uint32_t run_size;
  _Atomic uint64_t owner_word;
  _Atomic uint8_t state;
  _Atomic uint8_t qstate;
  _Atomic uint64_t pending_word_mask;
  _Atomic uint64_t* pending_bits;
  _Atomic uint32_t* slot_state;
  pthread_mutex_t lock;
  uint64_t free_mask;
  uint64_t allocated_mask;
  uint8_t payload_state;
  bool resident_charge;
  bool active_live_empty_charge;
  bool lazy_purge_charge;
  bool owner_attached;
  bool payload_chunk_backed;
  void* meta_alloc_base;
  struct H8MediumRun* next_owner;
  struct H8MediumRun* next_global;
  struct H8MediumRun* next_detached;
  struct H8MediumRun* next_pending;
  bool detached_indexed;
#if defined(H8_ENABLE_DEBUG_STATS) || defined(H8_MEDIUM_ENABLE_AVAILABLE_INDEX)
  struct H8MediumRun* debug_available_prev;
  struct H8MediumRun* debug_available_next;
  bool debug_available_indexed;
#endif
#if defined(H8_ENABLE_DEBUG_STATS)
  atomic_uint debug_writer_active;
  atomic_uint debug_writer_kind;
  atomic_uint_fast64_t debug_writer_token;
  uint64_t debug_retention_empty_epoch;
  uint64_t debug_retention_decommit_epoch;
  uint8_t debug_retention_empty_distance;
  uint8_t debug_retention_decommit_distance;
  uint8_t debug_retention_decommit_reason;
  bool debug_retention_decommitted_ghost;
  bool debug_lazy_purge_candidate;
  uint8_t debug_retention_l3_state[4];
  struct H8MediumRun* debug_retention_l3_next[4];
  bool debug_retention_l3_reuse_evidence;
  bool debug_retention_l3_m0_expect_decommit;
  uint32_t debug_collect_generation;
  uint32_t debug_collect_free_credits;
  uint64_t debug_collect_owner_alloc_epoch;
  uint64_t debug_local_fast_shadow_mask;
#endif
} H8MediumRun;

typedef struct H8OwnerRecord H8OwnerRecord;

static inline bool h8_medium_size_supported_fast(size_t size) {
  return size >= H8_MEDIUM_MIN_SIZE && size <= H8_MEDIUM_MAX_SIZE;
}

static inline uint32_t h8_medium_class_for_size_fast(size_t size) {
  if (size <= 8192u) {
    return 0u;
  }
  if (size <= 16384u) {
    return 1u;
  }
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  if (size <= 24576u) {
    return 2u;
  }
  if (size <= 32768u) {
    return 3u;
  }
  if (size <= 49152u) {
    return 4u;
  }
  return 5u;
#else
  if (size <= 32768u) {
    return 2u;
  }
#if defined(H8_MEDIUM_UPPER48_CLASS)
  if (size <= 49152u) {
    return 3u;
  }
  return 4u;
#else
  return 3u;
#endif
#endif
}

bool h8_medium_size_supported(size_t size);
uint32_t h8_medium_class_for_size(size_t size);
const H8MediumClassSpec* h8_medium_class_spec(uint32_t class_id);
uint32_t h8_medium_rounded_size(size_t size);
bool h8_medium_slot_index_from_ptr_checked(const H8MediumRun* run,
                                           const void* ptr,
                                           size_t* slot_out);
void* h8_medium_slot_ptr(const H8MediumRun* run, size_t slot);
static inline bool h8_medium_slot_index_from_ptr_checked_fast(
    const H8MediumRun* run, const void* ptr, size_t* slot_out) {
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
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  if ((run->slot_size & (run->slot_size - 1u)) != 0u) {
    if (offset >= payload || (offset % (uintptr_t)run->slot_size) != 0u) {
      return false;
    }
    size_t slot = (size_t)(offset / (uintptr_t)run->slot_size);
    if (slot >= run->slot_count) {
      return false;
    }
    if (slot_out) {
      *slot_out = slot;
    }
    return true;
  }
#endif
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
static inline void* h8_medium_slot_ptr_fast(const H8MediumRun* run,
                                            size_t slot) {
  if (!run || !run->base || slot >= run->slot_count) {
    return NULL;
  }
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  if ((run->slot_size & (run->slot_size - 1u)) != 0u) {
    return run->base + (slot * (size_t)run->slot_size);
  }
#endif
  return run->base + (slot << run->slot_shift);
}

static inline void* h8_medium_slot_ptr_known(const H8MediumRun* run,
                                             size_t slot) {
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  if ((run->slot_size & (run->slot_size - 1u)) != 0u) {
    return run->base + (slot * (size_t)run->slot_size);
  }
#endif
  return run->base + (slot << run->slot_shift);
}
#if defined(H8_MEDIUM_V12_48K2_CLASS)
static inline bool h8_medium_24k_local_free_slot_index_fast(
    const H8MediumRun* run, const void* ptr, size_t* slot_out) {
  if (!run || !run->base || !ptr || run->class_id != 2u ||
      run->slot_size != 24576u || run->slot_count != 2u) {
    return false;
  }
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t addr = (uintptr_t)ptr;
  if (addr < base) {
    return false;
  }
  uintptr_t offset = addr - base;
  if (offset == 0u) {
    if (slot_out) {
      *slot_out = 0u;
    }
    return true;
  }
  if (offset == (uintptr_t)run->slot_size) {
    if (slot_out) {
      *slot_out = 1u;
    }
    return true;
  }
  return false;
}
#else
static inline bool h8_medium_24k_local_free_slot_index_fast(
    const H8MediumRun* run, const void* ptr, size_t* slot_out) {
  (void)run;
  (void)ptr;
  (void)slot_out;
  return false;
}
#endif
bool h8_medium_run_owned_by_ctx(const H8MediumRun* run,
                                const H8ThreadCtx* ctx);
#if defined(H8_ENABLE_DEBUG_STATS)
void h8_medium_debug_writer_enter(H8MediumRun* run, H8OwnerRecord* owner,
                                  H8MediumWriterKind kind);
void h8_medium_debug_writer_exit(H8MediumRun* run);
#else
#define h8_medium_debug_writer_enter(run, owner, kind) ((void)0)
#define h8_medium_debug_writer_exit(run) ((void)0)
#endif
H8MediumRun* h8_medium_run_create_scaffold(uint32_t class_id);
void h8_medium_run_destroy_scaffold(H8MediumRun* run);
void* h8_medium_payload_alloc(size_t run_size, bool* chunk_backed_out);
void h8_medium_payload_free(void* ptr, size_t run_size, bool chunk_backed);
void* h8_medium_run_alloc_local_scaffold(H8MediumRun* run);
bool h8_medium_run_free_local_scaffold(H8MediumRun* run, void* ptr,
                                       bool keep_empty_live);
#if defined(H8_ENABLE_DEBUG_STATS) || defined(H8_MEDIUM_ENABLE_AVAILABLE_INDEX)
void h8_medium_available_shadow_attach(H8OwnerRecord* owner,
                                       H8MediumRun* run);
void h8_medium_available_shadow_remove(H8OwnerRecord* owner,
                                       H8MediumRun* run);
void h8_medium_available_shadow_after_mask_change(H8MediumRun* run);
void h8_medium_available_shadow_after_mask_change_ctx(H8MediumRun* run,
                                                      H8ThreadCtx* ctx);
#else
#define h8_medium_available_shadow_attach(owner, run) \
  do {                                                \
    (void)(owner);                                    \
    (void)(run);                                      \
  } while (0)
#define h8_medium_available_shadow_remove(owner, run) \
  do {                                                \
    (void)(owner);                                    \
    (void)(run);                                      \
  } while (0)
#define h8_medium_available_shadow_after_mask_change(run) \
  do {                                                    \
    (void)(run);                                          \
  } while (0)
#define h8_medium_available_shadow_after_mask_change_ctx(run, ctx) \
  do {                                                             \
    (void)(run);                                                   \
    (void)(ctx);                                                   \
  } while (0)
#endif
void h8_medium_release_empty_payload(H8MediumRun* run);
void h8_medium_decommit_empty_locked(H8MediumRun* run);
void h8_medium_decommit_empty_owner_exit_locked(H8MediumRun* run);
void h8_medium_mark_live_on_alloc(H8MediumRun* run);
void h8_medium_note_active_live_empty(H8MediumRun* run);
void h8_medium_clear_active_live_empty(H8MediumRun* run);
void h8_medium_mark_empty_locked(H8MediumRun* run);
void h8_medium_lazy_purge_shadow_drop(H8MediumRun* run);
#if defined(H8_ENABLE_DEBUG_STATS)
void h8_medium_debug_note_owner_medium_alloc(H8OwnerRecord* owner);
void h8_medium_debug_note_active_miss_pending(H8ThreadCtx* ctx,
                                              uint32_t class_id,
                                              H8MediumRun* active);
void h8_medium_debug_note_owner_list_hit_position(size_t position);
void h8_medium_debug_note_owner_list_hit_class(uint32_t class_id);
void h8_medium_debug_note_collect_capacity(H8OwnerRecord* owner,
                                           H8MediumRun* run,
                                           uint64_t accepted,
                                           uint64_t old_free_mask,
                                           H8MediumCollectSource source);
void h8_medium_debug_note_alloc_collect_credit(H8OwnerRecord* owner,
                                               H8MediumRun* run);
void h8_medium_debug_discard_collect_credit(H8MediumRun* run);
void h8_medium_debug_note_local_fast_eligible_free(H8MediumRun* run,
                                                   uint64_t bit);
void h8_medium_debug_note_local_fast_alloc(H8MediumRun* run);
void h8_medium_debug_note_local_fast_flush(H8MediumRun* run, bool owner_exit);
#else
#define h8_medium_debug_note_owner_medium_alloc(owner) ((void)0)
#define h8_medium_debug_note_active_miss_pending(ctx, class_id, active) \
  ((void)(ctx), (void)(class_id), (void)(active))
#define h8_medium_debug_note_owner_list_hit_position(position) ((void)0)
#define h8_medium_debug_note_owner_list_hit_class(class_id) ((void)(class_id))
#define h8_medium_debug_note_collect_capacity(owner, run, accepted, old_free_mask, source) \
  ((void)(owner), (void)(run), (void)(accepted), (void)(old_free_mask), \
   (void)(source))
#define h8_medium_debug_note_alloc_collect_credit(owner, run) \
  ((void)(owner), (void)(run))
#define h8_medium_debug_discard_collect_credit(run) ((void)(run))
#define h8_medium_debug_note_local_fast_eligible_free(run, bit) \
  ((void)(run), (void)(bit))
#define h8_medium_debug_note_local_fast_alloc(run) ((void)(run))
#define h8_medium_debug_note_local_fast_flush(run, owner_exit) \
  ((void)(run), (void)(owner_exit))
#endif
static inline void h8_medium_note_active_live_empty_fast(H8MediumRun* run) {
#if defined(H8_ENABLE_DEBUG_STATS)
  h8_medium_note_active_live_empty(run);
#elif defined(H8_MEDIUM_ELIDE_ACTIVE_EMPTY_CHARGE)
  (void)run;
#else
  if (!run || run->active_live_empty_charge) {
    return;
  }
  run->active_live_empty_charge = true;
#endif
}
static inline void h8_medium_mark_live_on_alloc_fast(H8MediumRun* run) {
#if defined(H8_ENABLE_DEBUG_STATS)
  h8_medium_mark_live_on_alloc(run);
#else
  if (!run || run->allocated_mask != 0) {
    return;
  }
  if (run->payload_state == H8_MEDIUM_PAYLOAD_LIVE) {
#if defined(H8_MEDIUM_ELIDE_ACTIVE_EMPTY_CHARGE)
    return;
#else
    run->active_live_empty_charge = false;
#endif
  } else if (run->payload_state == H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT) {
    h8_medium_release_empty_payload(run);
  }
  run->payload_state = H8_MEDIUM_PAYLOAD_LIVE;
#endif
}
static inline void* h8_medium_run_alloc_local_hot(H8MediumRun* run) {
#if defined(H8_MEDIUM_ENABLE_INLINE_OWNER_ALLOC) && \
    !defined(H8_ENABLE_DEBUG_STATS)
  if (!run) {
    return NULL;
  }
  if (atomic_load_explicit(&run->state, memory_order_acquire) !=
      H8_MEDIUM_RUN_ACTIVE) {
    return NULL;
  }
  if (run->free_mask == 0) {
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(run->free_mask);
  uint64_t bit = UINT64_C(1) << slot;
  h8_medium_mark_live_on_alloc_fast(run);
  run->free_mask &= ~bit;
  run->allocated_mask |= bit;
#if !defined(H8_MEDIUM_CEILING_ALLOC_NO_SLOT_STATE)
  atomic_store_explicit(&run->slot_state[slot], (UINT32_C(1) << 30u),
                        memory_order_release);
#endif
  return h8_medium_slot_ptr_fast(run, slot);
#else
  return h8_medium_run_alloc_local_scaffold(run);
#endif
}
void h8_medium_owner_lease_shadow_open(H8OwnerRecord* owner,
                                       uint16_t generation);
void h8_medium_owner_lease_shadow_close(H8OwnerRecord* owner);
bool h8_medium_owner_lease_shadow_enter(H8OwnerRecord* owner,
                                        uint16_t generation);
void h8_medium_owner_lease_shadow_exit(H8OwnerRecord* owner);
void h8_medium_owner_lease_shadow_check_exit(H8OwnerRecord* owner);
#if defined(H8_ENABLE_DEBUG_STATS)
void h8_medium_retention_shadow_owner_init(H8OwnerRecord* owner);
void h8_medium_retention_shadow_note_empty(H8MediumRun* run);
void h8_medium_retention_shadow_note_retain(H8MediumRun* run);
void h8_medium_retention_shadow_note_decommit(H8MediumRun* run,
                                              H8MediumDecommitReason reason);
void h8_medium_retention_shadow_note_alloc(H8MediumRun* run);
#else
#define h8_medium_retention_shadow_owner_init(owner) ((void)0)
#define h8_medium_retention_shadow_note_empty(run) ((void)0)
#define h8_medium_retention_shadow_note_retain(run) ((void)0)
#define h8_medium_retention_shadow_note_decommit(run, reason) ((void)0)
#define h8_medium_retention_shadow_note_alloc(run) ((void)0)
#endif
H8PublishResult h8_medium_remote_publish(H8MediumRun* run, void* ptr);
bool h8_medium_owner_has_pending(H8OwnerRecord* owner);
size_t h8_medium_collect_current_pending_budget(H8ThreadCtx* ctx,
                                                size_t run_budget);
size_t h8_medium_collect_current_pending_budget_source(
    H8ThreadCtx* ctx, size_t run_budget, H8MediumCollectSource source);
size_t h8_medium_collect_owner_pending_budget(H8OwnerRecord* owner,
                                              size_t run_budget);
void h8_medium_collect_owner_pending_periodic(H8ThreadCtx* ctx);
void h8_medium_collect_owner_pending_periodic_owner_list(H8ThreadCtx* ctx);
void h8_medium_collect_owner_pending(H8OwnerRecord* owner);
void h8_medium_lock_global(void);
void h8_medium_unlock_global(void);
H8MediumRun* h8_medium_global_head(void);
H8MediumRun* h8_medium_detached_head_locked(uint32_t class_id);
void h8_medium_detached_add_locked(H8MediumRun* run);
void h8_medium_detached_remove_locked(H8MediumRun* run);
bool h8_medium_ptr_in_run(const H8MediumRun* run, const void* ptr);
H8MediumRun* h8_medium_directory_find(const void* ptr);
H8MediumRun* h8_medium_find_run_locked(const void* ptr, bool route_lookup);
void h8_medium_register_locked(H8MediumRun* run);
void h8_medium_unregister_locked(H8MediumRun* run);
void* h8_medium_malloc_class_inner(uint32_t class_id);
void* h8_medium_malloc_inner(size_t size);
bool h8_medium_free_inner(void* ptr, bool* owned_out);
bool h8_medium_usable_size_inner(void* ptr, size_t* usable_out,
                                 bool* owned_out);
H8RouteKind h8_medium_route_inner(void* ptr);
void h8_medium_owner_detach_all(H8OwnerRecord* owner);

#endif
