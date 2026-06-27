#ifndef H8_RUNTIME_TYPES_H
#define H8_RUNTIME_TYPES_H

/* Included by h8_internal.h after core constants and medium/class types. */
typedef enum H8OwnerState {
  H8_OWNER_ALIVE = 0,
  H8_OWNER_DYING = 1,
  H8_OWNER_DEAD = 2
} H8OwnerState;

typedef enum H8SpanQueueState {
  H8_Q_IDLE = 0,
  H8_Q_QUEUED = 1,
  H8_Q_DRAINING = 2,
  H8_Q_DRAINING_DIRTY = 3
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
  union {
    struct {
      uint8_t* base;
      uint16_t class_id;
      uint16_t slot_count;
      _Atomic uint64_t* live_bits;
      _Atomic uint64_t* pending_bits;
      _Atomic uint32_t* slot_state;
      void* meta_alloc_base;
      bool meta_bundled;
    };
    struct {
      uint8_t* immutable_base;
      uint16_t immutable_class_id;
      uint16_t immutable_slot_count;
      _Atomic uint64_t* immutable_live_bits;
      _Atomic uint64_t* immutable_pending_bits;
      _Atomic uint32_t* immutable_slot_state;
      void* immutable_meta_alloc_base;
      bool immutable_meta_bundled;
    } immutable;
  };
  union {
    struct {
      _Atomic uint64_t owner_word;
      uint32_t owner_slot;
      uint16_t owner_generation;
      _Atomic uint8_t span_state;
      _Atomic uint8_t publish_closed;
      _Atomic uint16_t publish_refs;
      _Atomic uint8_t qstate;
      _Atomic size_t pending_count;
      _Atomic uint64_t pending_word_mask;
      _Atomic uint32_t span_epoch;
    };
    struct {
      _Atomic uint64_t remote_owner_word;
      uint32_t remote_owner_slot;
      uint16_t remote_owner_generation;
      _Atomic uint8_t remote_span_state;
      _Atomic uint8_t remote_publish_closed;
      _Atomic uint16_t remote_publish_refs;
      _Atomic uint8_t remote_qstate;
      _Atomic size_t remote_pending_count;
      _Atomic uint64_t remote_pending_word_mask;
      _Atomic uint32_t remote_span_epoch;
    } remote_hot;
  } H8_CACHELINE_ALIGNED;
  struct {
    _Atomic uint32_t local_bump_index;
    _Atomic uint32_t local_free_head_word;
#if defined(H8_ENABLE_DEBUG_STATS)
    size_t local_used_mirror;
#endif
  } local_hot H8_CACHELINE_ALIGNED;
  struct H8Span* next_owned;
  struct H8Span* next_owned_class;
  struct H8Span* next_pending;
  struct H8Span* next_orphan;
  struct H8Span* next_orphan_class;
};

_Static_assert(_Alignof(H8Span) >= H8_CACHELINE_BYTES,
               "H8Span must be cacheline aligned");
_Static_assert(offsetof(H8Span, remote_hot) % H8_CACHELINE_BYTES == 0,
               "remote span hot fields must start on a cacheline");
_Static_assert(offsetof(H8Span, local_hot) % H8_CACHELINE_BYTES == 0,
               "local span hot fields must start on a cacheline");

struct H8OwnerRecord {
  _Atomic uint64_t control;
  _Atomic uint32_t medium_publish_ctl;
  uint32_t slot;
  uint32_t generation;
  bool permanent;
  H8OwnerPlacement placement;
  H8Span* owned_by_class[H8_CLASS_COUNT];
  H8MediumRun* medium_by_class[H8_MEDIUM_CLASS_COUNT];
#if defined(H8_ENABLE_DEBUG_STATS) || defined(H8_MEDIUM_ENABLE_AVAILABLE_INDEX)
  H8MediumRun* medium_available_shadow[H8_MEDIUM_CLASS_COUNT];
#endif
#if defined(H8_ENABLE_DEBUG_STATS) || \
    defined(H8_MEDIUM_ENABLE_REFILL_CANDIDATE)
  H8MediumRun* medium_refill_candidate[H8_MEDIUM_CLASS_COUNT];
#endif
#if defined(H8_ENABLE_DEBUG_STATS)
  H8MediumRun* medium_warm_shadow1[H8_MEDIUM_CLASS_COUNT];
  H8MediumRun* medium_warm_shadow2[H8_MEDIUM_CLASS_COUNT][2];
  uint64_t medium_retention_epoch[H8_MEDIUM_CLASS_COUNT];
  H8MediumRun* medium_retention_stack[H8_MEDIUM_CLASS_COUNT]
                                      [H8_MEDIUM_RETENTION_STACK_DEPTH];
  H8MediumRun* medium_retention_l3_protected1[H8_MEDIUM_CLASS_COUNT];
  H8MediumRun* medium_retention_l3_protected2[H8_MEDIUM_CLASS_COUNT][2];
  uint64_t debug_medium_alloc_epoch;
#endif
  _Atomic(H8Span*) pending_head;
  _Atomic size_t pending_span_count;
  H8Span* pending_carry;
  _Atomic(H8MediumRun*) medium_pending_head;
  H8MediumRun* medium_pending_carry;
  H8Span* owned_head;
  H8Span* orphan_head;
  H8Span* orphan_by_class[H8_CLASS_COUNT];
  size_t span_chunk_next;
  size_t span_chunk_end;
  atomic_size_t medium_pending_count;
  pthread_mutex_t owned_lock;
  pthread_mutex_t pending_lock;
  struct H8OwnerRecord* free_next;
};

struct H8ThreadCtx {
  H8OwnerRecord* owner;
  H8Span* active_spans[H8_CLASS_COUNT];
  H8MediumRun* active_medium_runs[H8_MEDIUM_CLASS_COUNT];
  H8MediumRun* medium_last_alloc_run;
  uint8_t medium_collect_credit;
#if defined(H8_HZ9_MEDIUM_LOCAL_MAG_SHADOW)
  H8MediumRun* medium_local_mag_shadow_run[H8_MEDIUM_CLASS_COUNT];
  uint64_t medium_local_mag_shadow_mask[H8_MEDIUM_CLASS_COUNT];
#endif
};

#include "h8_runtime_types_global.inc"

#endif
