#ifndef H8_INTERNAL_H
#define H8_INTERNAL_H

#include "../include/h8.h"
#include "h8_class_map.h"
#include "h8_medium.h"
#include "h8_platform.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define H8_SMALL_ARENA_BYTES (1ull << 36)
#define H8_SPAN_BYTES 65536u
#define H8_OWNER_MAX 64u
#define H8_OWNER_SPAN_CHUNK 32u
#define H8_DIRECT_FALLBACK_LIMIT (128u * 1024u)
#define H8_CACHELINE_BYTES 64u

#if !defined(H8_LIKELY)
#if defined(__GNUC__) || defined(__clang__)
#define H8_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define H8_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#define H8_CACHELINE_ALIGNED __attribute__((aligned(H8_CACHELINE_BYTES)))
#else
#define H8_LIKELY(expr) (expr)
#define H8_UNLIKELY(expr) (expr)
#define H8_CACHELINE_ALIGNED
#endif
#endif

#if !defined(H8_CACHELINE_ALIGNED)
#if defined(__GNUC__) || defined(__clang__)
#define H8_CACHELINE_ALIGNED __attribute__((aligned(H8_CACHELINE_BYTES)))
#else
#define H8_CACHELINE_ALIGNED
#endif
#endif

#include "h8_runtime_types.h"

#if defined(__linux__) && defined(__ELF__) && defined(__x86_64__)
#define H8_TLS_FAST __attribute__((tls_model("initial-exec"), visibility("hidden")))
#else
#define H8_TLS_FAST
#endif
extern H8Global h8g;
extern _Thread_local H8ThreadCtx* h8_tls_ctx H8_TLS_FAST;

#if defined(H8_ENABLE_DEBUG_STATS)
#define H8_DEBUG_INC(field) \
  atomic_fetch_add_explicit(&h8g.field, 1, memory_order_relaxed)
#define H8_DEBUG_ADD(field, value) \
  atomic_fetch_add_explicit(&h8g.field, (value), memory_order_relaxed)
#else
#define H8_DEBUG_INC(field) ((void)0)
#define H8_DEBUG_ADD(field, value) ((void)0)
#endif

static inline uint32_t h8_round_up_u32(uint32_t value, uint32_t align) {
  return (value + align - 1u) & ~(align - 1u);
}

static inline size_t h8_round_up_size(size_t value, size_t align) {
  return (value + align - 1u) & ~(align - 1u);
}

static inline size_t h8_span_index_from_ptr(const void* ptr) {
  uintptr_t base = (uintptr_t)h8g.arena_base;
  uintptr_t addr = (uintptr_t)ptr;
  return (size_t)((addr - base) / H8_SPAN_BYTES);
}

static inline uint8_t* h8_span_base_from_index(size_t index) {
  return (uint8_t*)h8g.arena_base + index * H8_SPAN_BYTES;
}

static inline bool h8_arena_contains(const void* ptr) {
  uintptr_t base = (uintptr_t)h8g.arena_base;
  uintptr_t addr = (uintptr_t)ptr;
  return addr >= base && addr < base + h8g.arena_bytes;
}

static inline bool h8_direct_large_maybe_contains_hot(const void* ptr) {
  uintptr_t min =
      atomic_load_explicit(&h8g.direct_large_min_addr, memory_order_acquire);
  if (min == 0) {
    return false;
  }
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t max =
      atomic_load_explicit(&h8g.direct_large_max_addr, memory_order_acquire);
  return addr >= min && addr < max;
}

static inline size_t h8_slot_count_for_class(uint32_t class_id) {
  return h8_class_slot_count(class_id);
}

static inline size_t h8_word_count_for_slots(size_t slots) {
  return (slots + 63u) / 64u;
}

#include "h8_slot_state_inline.h"
#include "h8_hz9_local_mag_inline.inc"

static inline H8CtlWord h8_ctl_unpack(uint64_t raw) {
  H8CtlWord ctl;
  ctl.generation = (uint16_t)(raw & 0xFFFFu);
  ctl.state = (uint8_t)((raw >> 16) & 0x3u);
  ctl.publish_closed = (uint8_t)((raw >> 18) & 0x1u);
  ctl.publish_refs = (uint16_t)((raw >> 19) & 0xFFFFu);
  return ctl;
}

static inline uint64_t h8_ctl_pack(H8CtlWord ctl) {
  return ((uint64_t)ctl.generation) |
         ((uint64_t)ctl.state << 16) |
         ((uint64_t)ctl.publish_closed << 18) |
         ((uint64_t)ctl.publish_refs << 19);
}

static inline uint64_t h8_owner_word_pack(H8OwnerWord ow) {
  return ((uint64_t)ow.slot) |
         ((uint64_t)ow.generation << 6) |
         ((uint64_t)ow.state << 22) |
         ((uint64_t)ow.span_epoch << 25);
}

static inline H8OwnerWord h8_owner_word_unpack(uint64_t raw) {
  H8OwnerWord ow;
  ow.slot = (uint8_t)(raw & 0x3Fu);
  ow.generation = (uint16_t)((raw >> 6) & 0xFFFFu);
  ow.state = (uint8_t)((raw >> 22) & 0x7u);
  ow.span_epoch = (uint32_t)(raw >> 25);
  return ow;
}

static inline H8CtlWord h8_ctl_set_state(H8CtlWord ctl, H8OwnerState state) {
  ctl.state = (uint8_t)state;
  return ctl;
}

static inline H8CtlWord h8_ctl_close_publish(H8CtlWord ctl) {
  ctl.publish_closed = 1;
  return ctl;
}

static inline H8CtlWord h8_ctl_open_publish(H8CtlWord ctl) {
  ctl.publish_closed = 0;
  return ctl;
}

static inline H8CtlWord h8_ctl_refs_inc(H8CtlWord ctl) {
  ctl.publish_refs++;
  return ctl;
}

static inline H8CtlWord h8_ctl_refs_dec(H8CtlWord ctl) {
  ctl.publish_refs--;
  return ctl;
}

static inline H8OwnerWord h8_owner_word_make(uint8_t slot, uint16_t generation,
                                             H8SpanState state, uint32_t epoch) {
  H8OwnerWord ow;
  ow.slot = slot;
  ow.generation = generation;
  ow.state = (uint8_t)state;
  ow.span_epoch = epoch;
  return ow;
}

static inline H8OwnerWord h8_span_owner_word_load(const H8Span* span) {
  return h8_owner_word_unpack(
      atomic_load_explicit(&span->owner_word, memory_order_acquire));
}

static inline void h8_span_owner_word_store(H8Span* span, H8OwnerWord word,
                                            memory_order order) {
  atomic_store_explicit(&span->owner_word, h8_owner_word_pack(word), order);
  span->owner_slot = word.slot;
  span->owner_generation = word.generation;
  atomic_store_explicit(&span->span_state, (uint8_t)word.state, order);
  atomic_store_explicit(&span->span_epoch, word.span_epoch, order);
}

static inline H8SpanState h8_span_state_load(const H8Span* span) {
  return (H8SpanState)atomic_load_explicit(&span->span_state, memory_order_acquire);
}

static inline void h8_span_state_store(H8Span* span, H8SpanState state,
                                       memory_order order) {
  atomic_store_explicit(&span->span_state, (uint8_t)state, order);
  uint64_t raw = atomic_load_explicit(&span->owner_word, memory_order_relaxed);
  H8OwnerWord word = h8_owner_word_unpack(raw);
  word.state = (uint8_t)state;
  atomic_store_explicit(&span->owner_word, h8_owner_word_pack(word), order);
}

static inline bool h8_owner_word_equal(H8OwnerWord a, H8OwnerWord b) {
  return a.slot == b.slot && a.generation == b.generation &&
         a.state == b.state && a.span_epoch == b.span_epoch;
}

static inline bool h8_regular_adoption_enabled(void) {
  return atomic_load_explicit(&h8g.regular_adoption_enabled, memory_order_acquire);
}

static inline bool h8_remote_lease_elision_enabled(void) {
#if defined(H8_ENABLE_UNSAFE_EVIDENCE_KNOBS)
  return atomic_load_explicit(&h8g.remote_lease_elision_enabled,
                              memory_order_acquire);
#else
  return false;
#endif
}

static inline bool h8_remote_pending_publish_elision_enabled(void) {
#if defined(H8_ENABLE_UNSAFE_EVIDENCE_KNOBS)
  return atomic_load_explicit(&h8g.remote_pending_publish_elision_enabled,
                              memory_order_acquire);
#else
  return false;
#endif
}

#include "h8_slot_geometry_inline.h"

static inline uint64_t h8_bit_mask(size_t slot) {
  return 1ull << (slot & 63u);
}

static inline size_t h8_bit_word(size_t slot) {
  return slot >> 6u;
}

static inline void h8_debug_local_live_word(size_t slot) {
#if defined(H8_ENABLE_DEBUG_STATS)
  size_t word = h8_bit_word(slot);
  if (word == 0) {
    H8_DEBUG_INC(local_live_word_0);
  } else if (word == 1) {
    H8_DEBUG_INC(local_live_word_1);
  } else if (word <= 7) {
    H8_DEBUG_INC(local_live_word_2_7);
  } else if (word <= 31) {
    H8_DEBUG_INC(local_live_word_8_31);
  } else {
    H8_DEBUG_INC(local_live_word_32_63);
  }
#else
  (void)slot;
#endif
}

static inline bool h8_bitmap_test(_Atomic uint64_t* bits, size_t slot) {
  return (atomic_load_explicit(&bits[h8_bit_word(slot)], memory_order_acquire) &
          h8_bit_mask(slot)) != 0;
}

static inline bool h8_bitmap_test_and_set(_Atomic uint64_t* bits, size_t slot) {
  uint64_t mask = h8_bit_mask(slot);
  uint64_t old = atomic_fetch_or_explicit(
      &bits[h8_bit_word(slot)], mask, memory_order_acq_rel);
  return (old & mask) != 0;
}

static inline bool h8_bitmap_test_and_set_relaxed(_Atomic uint64_t* bits,
                                                  size_t slot) {
  uint64_t mask = h8_bit_mask(slot);
  uint64_t old =
      atomic_fetch_or_explicit(&bits[h8_bit_word(slot)], mask, memory_order_relaxed);
  return (old & mask) != 0;
}

static inline void h8_bitmap_clear(_Atomic uint64_t* bits, size_t slot) {
  uint64_t mask = ~h8_bit_mask(slot);
  atomic_fetch_and_explicit(&bits[h8_bit_word(slot)], mask, memory_order_release);
}

static inline void h8_bitmap_clear_relaxed(_Atomic uint64_t* bits, size_t slot) {
  uint64_t mask = ~h8_bit_mask(slot);
  atomic_fetch_and_explicit(&bits[h8_bit_word(slot)], mask, memory_order_relaxed);
}

static inline bool h8_owner_live_set(H8Span* span, size_t slot) {
  _Atomic uint64_t* word = &span->live_bits[h8_bit_word(slot)];
  uint64_t bit = h8_bit_mask(slot);
  uint64_t old = atomic_load_explicit(word, memory_order_relaxed);
  if ((old & bit) != 0) {
    H8_DEBUG_INC(owner_live_set_already_live);
    return false;
  }
  atomic_store_explicit(word, old | bit, memory_order_release);
  return true;
}

static inline bool h8_owner_live_clear(H8Span* span, size_t slot) {
  _Atomic uint64_t* word = &span->live_bits[h8_bit_word(slot)];
  uint64_t bit = h8_bit_mask(slot);
  uint64_t old = atomic_load_explicit(word, memory_order_relaxed);
  if ((old & bit) == 0) {
    H8_DEBUG_INC(owner_live_clear_already_free);
    return false;
  }
  atomic_store_explicit(word, old & ~bit, memory_order_release);
  return true;
}

static inline size_t h8_bitmap_popcount(const _Atomic uint64_t* bits, size_t words) {
  size_t total = 0;
  for (size_t i = 0; i < words; ++i) {
    total += (size_t)__builtin_popcountll(
        atomic_load_explicit(&bits[i], memory_order_acquire));
  }
  return total;
}

void h8_system_init(void);
void* h8_sys_malloc(size_t size);
void* h8_sys_calloc(size_t count, size_t size);
void* h8_sys_realloc(void* ptr, size_t size);
void h8_sys_free(void* ptr);

bool h8_direct_large_size_supported(size_t size);
void* h8_direct_large_malloc(size_t size);
bool h8_direct_large_free_exact_inner(void* ptr, bool* owned_out);
bool h8_direct_large_free_inner(void* ptr, bool* owned_out);
bool h8_direct_large_usable_size_exact_inner(void* ptr, size_t* usable_out,
                                             bool* owned_out);
bool h8_direct_large_usable_size_inner(void* ptr, size_t* usable_out,
                                       bool* owned_out);
H8RouteKind h8_direct_large_route_exact_inner(void* ptr);
H8RouteKind h8_direct_large_route_inner(void* ptr);

H8ThreadCtx* h8_thread_ctx_get_slow(void);
H8ThreadCtx* h8_thread_ctx_get(void);
H8OwnerRecord* h8_owner_current(void);
H8OwnerRecord* h8_orphan_owner(void);
void h8_thread_shutdown(void* arg);

static inline H8ThreadCtx* h8_thread_ctx_fast(void) {
  H8ThreadCtx* ctx = h8_tls_ctx;
  if (H8_LIKELY(ctx != NULL)) {
    return ctx;
  }
  return h8_thread_ctx_get_slow();
}

H8Span* h8_span_from_ptr_checked(void* ptr, size_t* slot_out);
H8Span* h8_span_commit_for_class(H8OwnerRecord* owner, uint32_t class_id);
void h8_span_retire(H8Span* span);
H8Span* h8_span_retire_logical(H8Span* span);
void h8_span_purge_retired_batch(H8Span* spans);
void h8_span_collect_remote(H8OwnerRecord* owner, H8Span* span);
void h8_span_notify(H8OwnerRecord* owner, H8Span* span);
typedef enum H8RemotePressureCollectSource {
  H8_REMOTE_PRESSURE_COLLECT_SOURCE_ACTIVE_HIT_FULL = 0,
  H8_REMOTE_PRESSURE_COLLECT_SOURCE_ACTIVE_MISS = 1,
  H8_REMOTE_PRESSURE_COLLECT_SOURCE_OWNER_EXIT = 2,
  H8_REMOTE_PRESSURE_COLLECT_SOURCE_COUNT = 3
} H8RemotePressureCollectSource;
void h8_pressure_owner_collect(H8OwnerRecord* owner);
void h8_pressure_owner_collect_remote_pressure(H8OwnerRecord* owner,
                                              H8RemotePressureCollectSource source);
void h8_collect_owner_pending(H8OwnerRecord* owner);
void h8_owner_exit(H8OwnerRecord* owner);
bool h8_owner_lifecycle_enter(H8OwnerRecord* owner, uint16_t expected_generation);
void h8_owner_lifecycle_exit(H8OwnerRecord* owner);
bool h8_owner_publish_enter(H8OwnerRecord* owner, uint16_t expected_generation);
void h8_owner_publish_exit(H8OwnerRecord* owner);
void h8_owner_mark_alive(H8OwnerRecord* owner, uint32_t slot, uint16_t generation,
                         bool permanent);
void h8_owner_mark_dying(H8OwnerRecord* owner);
void h8_owner_mark_dead(H8OwnerRecord* owner);
bool h8_owner_is_alive_and_open(H8OwnerRecord* owner);
bool h8_orphan_adoption_dry_run(H8OwnerRecord* adopter, uint32_t class_id);
void h8_owner_free_stack_push(H8OwnerRecord* owner);
void h8_owner_add_owned_span(H8OwnerRecord* owner, H8Span* span);
void h8_owner_remove_owned_span(H8OwnerRecord* owner, H8Span* span);
void h8_owner_add_orphan_span(H8OwnerRecord* owner, H8Span* span);
void h8_owner_remove_orphan_span(H8OwnerRecord* owner, H8Span* span);
bool h8_owner_wait_publishers_zero(H8OwnerRecord* owner);
bool h8_span_publish_enter(H8Span* span);
void h8_span_publish_exit(H8Span* span);
void h8_span_close_publish(H8Span* span);
void h8_span_wait_publishers_zero(H8Span* span);
void h8_span_mark_orphan_quiescing(H8Span* span);
void h8_span_mark_orphan_ready(H8Span* span);
bool h8_span_handoff(H8Span* span, H8OwnerWord expected_old_token,
                     H8OwnerRecord* target_owner);
H8Span* h8_orphan_adopt_span(H8OwnerRecord* adopter, uint32_t class_id);
static inline H8OwnerRecord* h8_owner_by_slot(uint32_t slot) {
  if (slot >= H8_OWNER_MAX) {
    return NULL;
  }
  return &h8g.owners[slot];
}
void h8_stats_snapshot(H8Stats* out);
void h8_debug_stats_snapshot(H8DebugStats* out);

H8PublishResult h8_remote_free_publish(void* ptr);
H8PublishResult h8_remote_free_publish_known(H8Span* span, size_t slot);
H8RouteKind h8_route_inner(void* ptr);
void* h8_malloc_inner(size_t size);
void* h8_realloc_inner(void* ptr, size_t size);
void h8_free_arena_inner(void* ptr);
void h8_free_inner(void* ptr);
#if defined(H9_LOCAL_ENTRY_SPLIT_L1)
void* h9_local_malloc_medium_inner(size_t size);
void h9_local_free_outer(void* ptr);
void h9_local_note_medium_remote_free(uint32_t class_id);
bool h9_local_maybe_contains_hot(const void* ptr);
bool h9_local_usable_size_inner(void* ptr, size_t* usable_out,
                                bool* owned_out);
H8RouteKind h9_local_route_inner(void* ptr);
void h9_local_flush_owner(H8OwnerRecord* owner);
#else
static inline void h9_local_note_medium_remote_free(uint32_t class_id) {
  (void)class_id;
}
#endif
void* h9_slab_malloc_medium_inner(size_t size);
void h9_slab_free_outer(void* ptr);
#if defined(H9_OWNER_LOCAL_PAGE_POOL_PURE_LOCAL_L1)
void* h9_owner_page_try_alloc(H8ThreadCtx* ctx, uint32_t class_id);
bool h9_owner_page_try_free(H8ThreadCtx* ctx, void* ptr, bool* owned_out);
void h9_owner_page_note_remote_medium_free(H8ThreadCtx* ctx,
                                           H8MediumRun* run);
void h9_owner_page_flush_thread(H8ThreadCtx* ctx);
void h9_owner_page_flush_owner(H8OwnerRecord* owner);
#if defined(H9_OWNER_LOCAL_PAGE_POOL_ROUTE_SMOKE)
bool h9_owner_page_route_smoke_run(void);
#endif
#else
static inline void* h9_owner_page_try_alloc(H8ThreadCtx* ctx,
                                            uint32_t class_id) {
  (void)ctx;
  (void)class_id;
  return NULL;
}
static inline bool h9_owner_page_try_free(H8ThreadCtx* ctx, void* ptr,
                                          bool* owned_out) {
  (void)ctx;
  (void)ptr;
  if (owned_out) {
    *owned_out = false;
  }
  return false;
}
static inline void h9_owner_page_note_remote_medium_free(H8ThreadCtx* ctx,
                                                         H8MediumRun* run) {
  (void)ctx;
  (void)run;
}
static inline void h9_owner_page_flush_thread(H8ThreadCtx* ctx) {
  (void)ctx;
}
static inline void h9_owner_page_flush_owner(H8OwnerRecord* owner) {
  (void)owner;
}
#endif
#if defined(H9_OWNER_LOCAL_PAGE_POOL_SHADOW_L0) && \
    defined(H8_ENABLE_DEBUG_STATS)
void h9_owner_page_shadow_note_alloc(H8ThreadCtx* ctx, uint32_t class_id);
void h9_owner_page_shadow_note_local_free(H8ThreadCtx* ctx,
                                          H8MediumRun* run);
void h9_owner_page_shadow_note_remote_free(H8ThreadCtx* ctx,
                                           H8MediumRun* run);
#else
static inline void h9_owner_page_shadow_note_alloc(H8ThreadCtx* ctx,
                                                   uint32_t class_id) {
  (void)ctx;
  (void)class_id;
}
static inline void h9_owner_page_shadow_note_local_free(H8ThreadCtx* ctx,
                                                        H8MediumRun* run) {
  (void)ctx;
  (void)run;
}
static inline void h9_owner_page_shadow_note_remote_free(H8ThreadCtx* ctx,
                                                         H8MediumRun* run) {
  (void)ctx;
  (void)run;
}
#endif
#if defined(H9_STATIC_LOCAL_PAGE_SCAFFOLD_L0)
void h9_static_local_page_debug_reset(void);
size_t h9_static_local_page_debug_state_size(void);
bool h9_static_local_page_debug_put(uint32_t class_id, uint32_t slot);
bool h9_static_local_page_debug_take(uint32_t class_id, uint32_t* slot_out);
bool h9_static_local_page_debug_free_allocated(uint32_t class_id,
                                               uint32_t slot);
uint64_t h9_static_local_page_debug_free_bits(uint32_t class_id);
uint64_t h9_static_local_page_debug_alloc_bits(uint32_t class_id);
uint64_t h9_static_local_page_debug_touched_classes(void);
#endif
#if defined(H9_SEGMENT_LOCAL_CACHE_L0)
void h9_segment_local_cache_debug_reset(void);
size_t h9_segment_local_cache_debug_state_size(void);
bool h9_segment_local_cache_debug_put(uint32_t class_id, uint32_t slot);
bool h9_segment_local_cache_debug_take(uint32_t class_id, uint32_t* slot_out);
bool h9_segment_local_cache_debug_free_allocated(uint32_t class_id,
                                                 uint32_t slot);
bool h9_segment_local_cache_debug_remote_mark(uint32_t class_id,
                                              uint32_t slot);
bool h9_segment_local_cache_debug_drain_remote(uint32_t class_id,
                                               uint64_t* pending_out);
bool h9_segment_local_cache_debug_retire(uint32_t class_id);
uint32_t h9_segment_local_cache_debug_state(uint32_t class_id);
uint64_t h9_segment_local_cache_debug_free_bits(uint32_t class_id);
uint64_t h9_segment_local_cache_debug_alloc_bits(uint32_t class_id);
uint64_t h9_segment_local_cache_debug_remote_bits(uint32_t class_id);
uint64_t h9_segment_local_cache_debug_touched_classes(void);
#endif
typedef enum H9LocalEntryEvent {
  H9_LOCAL_ENTRY_MALLOC_MEDIUM = 0,
  H9_LOCAL_ENTRY_MALLOC_FALLBACK = 1,
  H9_LOCAL_ENTRY_FREE_CHECK = 2,
  H9_LOCAL_ENTRY_FREE_MAYBE_FALSE = 3,
  H9_LOCAL_ENTRY_FREE_LOCAL_OUTER = 4,
  H9_LOCAL_ENTRY_FREE_ARENA_SKIP = 5,
  H9_LOCAL_ENTRY_ARENA_ALLOC_HIT = 6,
  H9_LOCAL_ENTRY_ARENA_PAGE_CREATE = 7,
  H9_LOCAL_ENTRY_ARENA_ALLOC_FALLBACK = 8,
  H9_LOCAL_ENTRY_ARENA_FREE_VALID = 9,
  H9_LOCAL_ENTRY_ARENA_FREE_INVALID = 10,
  H9_LOCAL_ENTRY_ARENA_FREE_REMOTE = 11,
  H9_LOCAL_ENTRY_MALLOC_CLASS_ENABLED = 12,
  H9_LOCAL_ENTRY_MALLOC_CLASS_DISABLED = 13,
  H9_LOCAL_ENTRY_ARENA_ALLOC_ATTEMPT = 14,
  H9_LOCAL_ENTRY_FREE_READY = 15,
  H9_LOCAL_ENTRY_FREE_FALLBACK_OUTER = 16,
  H9_LOCAL_ENTRY_FREE_OWNED_INVALID_OUTER = 17,
  H9_LOCAL_ENTRY_USABLE_CHECK = 18,
  H9_LOCAL_ENTRY_USABLE_VALID = 19,
  H9_LOCAL_ENTRY_USABLE_OWNED_INVALID = 20,
  H9_LOCAL_ENTRY_ROUTE_CHECK = 21,
  H9_LOCAL_ENTRY_ROUTE_VALID = 22,
  H9_LOCAL_ENTRY_ROUTE_INVALID = 23,
  H9_LOCAL_ENTRY_EVENT_COUNT = 24
} H9LocalEntryEvent;
typedef enum H9SlabEntryEvent {
  H9_SLAB_ENTRY_MALLOC_MEDIUM = 0,
  H9_SLAB_ENTRY_MALLOC_FALLBACK = 1,
  H9_SLAB_ENTRY_FREE_READY = 2,
  H9_SLAB_ENTRY_FREE_ARENA = 3,
  H9_SLAB_ENTRY_FREE_MAYBE_FALSE = 4,
  H9_SLAB_ENTRY_FREE_SLAB_OUTER = 5,
  H9_SLAB_ENTRY_REALLOC_CHECK = 6,
  H9_SLAB_ENTRY_REALLOC_SLAB = 7
} H9SlabEntryEvent;
#if defined(H9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY)
typedef struct H9MediumSlabRoutePage H9MediumSlabRoutePage;
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
extern _Atomic bool h9_slab_route_has_pages;
extern _Atomic uintptr_t h9_slab_route_min_addr;
extern _Atomic uintptr_t h9_slab_route_max_addr;
static inline bool h9_slab_maybe_has_pages_hot(void) {
  return atomic_load_explicit(&h9_slab_route_has_pages, memory_order_acquire);
}
static inline bool h9_slab_maybe_contains_hot(const void* ptr) {
  if (!h9_slab_maybe_has_pages_hot()) {
    H8_DEBUG_INC(h9_slab_maybe_no_pages);
    return false;
  }
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t min_addr =
      atomic_load_explicit(&h9_slab_route_min_addr, memory_order_acquire);
  uintptr_t max_addr =
      atomic_load_explicit(&h9_slab_route_max_addr, memory_order_acquire);
  bool hit = min_addr != 0 && addr >= min_addr && addr < max_addr;
  if (hit) {
    H8_DEBUG_INC(h9_slab_maybe_range_hit);
  } else {
    H8_DEBUG_INC(h9_slab_maybe_range_miss);
  }
  return hit;
}
#else
static inline bool h9_slab_maybe_has_pages_hot(void) {
  return true;
}
static inline bool h9_slab_maybe_contains_hot(const void* ptr) {
  (void)ptr;
  return true;
}
#endif
H8RouteKind h9_slab_route_inner(void* ptr);
bool h9_slab_free_inner(void* ptr, bool* owned_out);
bool h9_slab_usable_size_inner(void* ptr, size_t* usable_out,
                               bool* owned_out);
#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
void* h9_slab_alloc(H8ThreadCtx* ctx, uint32_t class_id);
void h9_slab_flush_thread(H8ThreadCtx* ctx);
void h9_slab_flush_owner(H8OwnerRecord* owner);
size_t h9_slab_collect_owner_pending(H8OwnerRecord* owner, size_t budget);
H9SlabOwnerState* h9_slab_owner_state(H8OwnerRecord* owner, bool create);
bool h9_slab_owner_prepare(H8OwnerRecord* owner);
void h9_slab_owner_reset(H8OwnerRecord* owner);
void h9_slab_owner_destroy(H8OwnerRecord* owner);
size_t h9_slab_owner_pending_count(H8OwnerRecord* owner);
#if defined(H9_SLAB_REMOTE_ADAPTIVE_L1)
void h9_slab_owner_mark_remote_hot(H8OwnerRecord* owner, uint16_t class_id);
uint8_t h9_slab_owner_remote_hot_mask(H8OwnerRecord* owner);
bool h9_slab_owner_remote_hot(H8OwnerRecord* owner, uint16_t class_id,
                              bool use_mask);
#endif
H9MediumSlabRoutePage* h9_slab_thread_active_page(H8ThreadCtx* ctx,
                                                  uint32_t class_id);
H9MediumSlabRoutePage* h9_slab_thread_page_at(H8ThreadCtx* ctx,
                                              uint32_t class_id,
                                              uint32_t index);
void h9_slab_thread_set_active_page(H8ThreadCtx* ctx, uint32_t class_id,
                                    H9MediumSlabRoutePage* page);
bool h9_slab_thread_install_page(H8ThreadCtx* ctx, uint32_t class_id,
                                 H9MediumSlabRoutePage* page, uint32_t cap);
#else
static inline void* h9_slab_alloc(H8ThreadCtx* ctx, uint32_t class_id) {
  (void)ctx;
  (void)class_id;
  return NULL;
}
static inline void h9_slab_flush_thread(H8ThreadCtx* ctx) {
  (void)ctx;
}
static inline void h9_slab_flush_owner(H8OwnerRecord* owner) {
  (void)owner;
}
static inline size_t h9_slab_collect_owner_pending(H8OwnerRecord* owner,
                                                   size_t budget) {
  (void)owner;
  (void)budget;
  return 0;
}
static inline void h9_slab_owner_reset(H8OwnerRecord* owner) {
  (void)owner;
}
static inline void h9_slab_owner_destroy(H8OwnerRecord* owner) {
  (void)owner;
}
static inline H9MediumSlabRoutePage* h9_slab_thread_active_page(
    H8ThreadCtx* ctx, uint32_t class_id) {
  (void)ctx;
  (void)class_id;
  return NULL;
}
static inline H9MediumSlabRoutePage* h9_slab_thread_page_at(
    H8ThreadCtx* ctx, uint32_t class_id, uint32_t index) {
  (void)ctx;
  (void)class_id;
  (void)index;
  return NULL;
}
static inline void h9_slab_thread_set_active_page(
    H8ThreadCtx* ctx, uint32_t class_id, H9MediumSlabRoutePage* page) {
  (void)ctx;
  (void)class_id;
  (void)page;
}
static inline bool h9_slab_thread_install_page(
    H8ThreadCtx* ctx, uint32_t class_id, H9MediumSlabRoutePage* page,
    uint32_t cap) {
  (void)ctx;
  (void)class_id;
  (void)page;
  (void)cap;
  return false;
}
#endif
H9MediumSlabRoutePage* h9_slab_route_test_register(
    void* base, size_t bytes, uint32_t class_id, uint32_t slot_size,
    uint16_t slot_count, H8OwnerRecord* owner);
void h9_slab_route_test_unregister(H9MediumSlabRoutePage* page);
#else
static inline H8RouteKind h9_slab_route_inner(void* ptr) {
  (void)ptr;
  return H8_ROUTE_MISS;
}
static inline bool h9_slab_free_inner(void* ptr, bool* owned_out) {
  (void)ptr;
  if (owned_out) {
    *owned_out = false;
  }
  return false;
}
static inline bool h9_slab_usable_size_inner(void* ptr, size_t* usable_out,
                                             bool* owned_out) {
  (void)ptr;
  (void)usable_out;
  if (owned_out) {
    *owned_out = false;
  }
  return false;
}
static inline bool h9_slab_maybe_has_pages_hot(void) {
  return false;
}
static inline bool h9_slab_maybe_contains_hot(const void* ptr) {
  (void)ptr;
  return false;
}
static inline void h9_slab_flush_owner(H8OwnerRecord* owner) {
  (void)owner;
}
#endif
size_t h8_collect_owner_pending_budget(H8OwnerRecord* owner, size_t budget);
bool h8_span_pending_quiescent(H8Span* span);
bool h8_span_repair_pending_mask(H8OwnerRecord* owner, H8Span* span);
void h8_slot_shadow_set_allocated(H8Span* span, size_t slot);
void h8_slot_shadow_set_free(H8Span* span, size_t slot, uint32_t next);
void h8_slot_shadow_expect(H8Span* span, size_t slot, uint32_t tag);
void h8_slot_shadow_verify_span(H8Span* span);
void h8_slot_shadow_verify_span_quiescent(H8Span* span);
size_t h8_slot_allocated_count_quiescent(H8Span* span);
uint32_t h8_slot_state_load_acquire(H8Span* span, size_t slot);

#endif
