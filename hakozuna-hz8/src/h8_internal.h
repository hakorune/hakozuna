#ifndef H8_INTERNAL_H
#define H8_INTERNAL_H

#include "../include/h8.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#define H8_SMALL_ARENA_BYTES (1ull << 36)
#define H8_SPAN_BYTES 65536u
#define H8_MAX_SMALL_SIZE 4096u
#define H8_OWNER_MAX 64u
#define H8_CLASS_COUNT 9u
#define H8_DIRECT_FALLBACK_LIMIT (128u * 1024u)

typedef enum H8OwnerState {
  H8_OWNER_ALIVE = 0,
  H8_OWNER_DYING = 1,
  H8_OWNER_DEAD = 2
} H8OwnerState;

typedef enum H8SpanQueueState {
  H8_Q_IDLE = 0,
  H8_Q_QUEUED = 1,
  H8_Q_DRAINING = 2
} H8SpanQueueState;

typedef enum H8SpanState {
  H8_SPAN_OWNED_ACTIVE = 0,
  H8_SPAN_ORPHAN_QUIESCING = 1,
  H8_SPAN_ORPHAN_READY = 2,
  H8_SPAN_ADOPTING = 3,
  H8_SPAN_RETIRING = 4,
  H8_SPAN_RETIRED = 5
} H8SpanState;

typedef enum H8OwnerPlacement {
  H8_OWNER_PLACEMENT_OWNED = 0,
  H8_OWNER_PLACEMENT_ORPHAN = 1
} H8OwnerPlacement;

typedef struct H8OwnerRecord H8OwnerRecord;
typedef struct H8Span H8Span;
typedef struct H8ThreadCtx H8ThreadCtx;

typedef struct H8OwnerWord {
  uint8_t slot;
  uint16_t generation;
  uint8_t state;
  uint32_t span_epoch;
} H8OwnerWord;

typedef struct H8CtlWord {
  uint16_t generation;
  uint8_t state;
  uint8_t publish_closed;
  uint16_t publish_refs;
} H8CtlWord;

struct H8Span {
  uint8_t* base;
  uint16_t class_id;
  uint16_t slot_count;
  uint32_t owner_slot;
  uint16_t owner_generation;
  _Atomic uint8_t span_state;
  _Atomic uint8_t publish_closed;
  _Atomic uint16_t publish_refs;
  _Atomic uint8_t qstate;
  _Atomic uint32_t span_epoch;
  _Atomic uint32_t bump_index;
  _Atomic uint32_t local_free_head;
  _Atomic size_t used_count;
  uint64_t* live_bits;
  uint64_t* pending_bits;
  uint32_t* next_free;
  struct H8Span* next_owned;
  struct H8Span* next_pending;
  struct H8Span* next_orphan;
};

struct H8OwnerRecord {
  _Atomic uint64_t control;
  uint32_t slot;
  uint32_t generation;
  bool permanent;
  H8OwnerPlacement placement;
  _Atomic(H8Span*) pending_head;
  H8Span* owned_head;
  H8Span* orphan_head;
  pthread_mutex_t owned_lock;
  struct H8OwnerRecord* free_next;
};

struct H8ThreadCtx {
  H8OwnerRecord* owner;
  H8Span* active_spans[H8_CLASS_COUNT];
};

typedef struct H8Global {
  pthread_once_t once;
  pthread_key_t thread_key;
  _Atomic bool ready;
  void* arena_base;
  size_t arena_bytes;
  size_t span_count;
  atomic_size_t arena_committed_bytes;
  H8Span** spans;
  H8OwnerRecord owners[H8_OWNER_MAX];
  H8OwnerRecord* owner_free;
  pthread_mutex_t owner_lock;
  H8OwnerRecord* orphan_owner;
  H8OwnerRecord* current_owner;
  atomic_size_t owner_count;
  atomic_size_t orphan_span_count;
  atomic_size_t local_alloc_count;
  atomic_size_t local_free_count;
  atomic_size_t remote_publish_count;
  atomic_size_t remote_collect_count;
  atomic_size_t owner_publish_enter_count;
  atomic_size_t owner_publish_exit_count;
  atomic_size_t owner_exit_count;
  atomic_size_t pending_enqueue_count;
  atomic_size_t pending_dequeue_count;
  atomic_size_t orphan_handoff_count;
  atomic_size_t handoff_success_count;
  atomic_size_t handoff_fail_count;
  atomic_size_t invalid_count;
  atomic_size_t miss_count;
  atomic_size_t owner_transition_count;
} H8Global;

extern H8Global h8g;

static inline uint32_t h8_round_up_u32(uint32_t value, uint32_t align) {
  return (value + align - 1u) & ~(align - 1u);
}

static inline size_t h8_round_up_size(size_t value, size_t align) {
  return (value + align - 1u) & ~(align - 1u);
}

static inline uint32_t h8_class_size(uint32_t class_id) {
  static const uint32_t sizes[H8_CLASS_COUNT] = {
      16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
  return sizes[class_id];
}

static inline uint32_t h8_class_for_size(size_t size) {
  if (size <= 16u) return 0;
  if (size <= 32u) return 1;
  if (size <= 64u) return 2;
  if (size <= 128u) return 3;
  if (size <= 256u) return 4;
  if (size <= 512u) return 5;
  if (size <= 1024u) return 6;
  if (size <= 2048u) return 7;
  return 8;
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

static inline size_t h8_slot_count_for_class(uint32_t class_id) {
  return H8_SPAN_BYTES / h8_class_size(class_id);
}

static inline size_t h8_word_count_for_slots(size_t slots) {
  return (slots + 63u) / 64u;
}

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

static inline H8SpanState h8_span_state_load(const H8Span* span) {
  return (H8SpanState)atomic_load_explicit(&span->span_state, memory_order_acquire);
}

static inline void h8_span_state_store(H8Span* span, H8SpanState state,
                                       memory_order order) {
  atomic_store_explicit(&span->span_state, (uint8_t)state, order);
}

static inline bool h8_owner_word_equal(H8OwnerWord a, H8OwnerWord b) {
  return a.slot == b.slot && a.generation == b.generation &&
         a.state == b.state && a.span_epoch == b.span_epoch;
}

static inline size_t h8_slot_index_from_ptr(const H8Span* span, const void* ptr) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)span->base;
  size_t class_size = h8_class_size(span->class_id);
  return (size_t)((addr - base) / class_size);
}

static inline void* h8_slot_ptr(const H8Span* span, size_t slot) {
  return span->base + slot * h8_class_size(span->class_id);
}

static inline uint64_t h8_bit_mask(size_t slot) {
  return 1ull << (slot & 63u);
}

static inline size_t h8_bit_word(size_t slot) {
  return slot >> 6u;
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

static inline void h8_bitmap_clear(_Atomic uint64_t* bits, size_t slot) {
  uint64_t mask = ~h8_bit_mask(slot);
  atomic_fetch_and_explicit(&bits[h8_bit_word(slot)], mask, memory_order_release);
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
void h8_sys_free(void* ptr);

H8ThreadCtx* h8_thread_ctx_get(void);
H8OwnerRecord* h8_owner_current(void);
H8OwnerRecord* h8_orphan_owner(void);
void h8_thread_shutdown(void* arg);

H8Span* h8_span_from_ptr_checked(void* ptr, size_t* slot_out);
H8Span* h8_span_commit_for_class(H8OwnerRecord* owner, uint32_t class_id);
void h8_span_retire(H8Span* span);
void h8_span_collect_remote(H8OwnerRecord* owner, H8Span* span);
void h8_span_notify(H8OwnerRecord* owner, H8Span* span);
void h8_pressure_owner_collect(H8OwnerRecord* owner);
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
H8RouteKind h8_route_inner(void* ptr);
void* h8_malloc_inner(size_t size);
void h8_free_inner(void* ptr);

#endif
