#include "hz5_midfront.h"

#include "hz5_config.h"
#include "hz5_internal.h"
#include "hz5_ownerhub.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <sys/mman.h>

#if !defined(_WIN32)
void* _aligned_malloc(size_t size, size_t alignment);
void _aligned_free(void* ptr);
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_M1
#define BENCHLAB_HZ5_LINUX_MIDFRONT_M1 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_OWNER_FAST_STATE
#define BENCHLAB_HZ5_LINUX_MIDFRONT_OWNER_FAST_STATE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_ALL_ON_MISS
#define BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_ALL_ON_MISS 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_ON_MISS
#define BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_ON_MISS 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_HIT_STOP
#define BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_HIT_STOP 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_TAKE_FIRST
#define BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_TAKE_FIRST 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_EMPTY_GATED
#define BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_EMPTY_GATED 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE
#define BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_OUTBOX
#define BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_OUTBOX 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_OUTBOX_FLUSH_ON_MISS
#define BENCHLAB_HZ5_LINUX_MIDFRONT_OUTBOX_FLUSH_ON_MISS 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE
#define BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_TRUST_DRAIN_STATE
#define BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_TRUST_DRAIN_STATE 0
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_MIDFRONT_M1

#define HZ5_MIDFRONT_MAGIC UINT64_C(0x485A354D49444D31)
#define HZ5_MIDFRONT_PAGE_SIZE ((size_t)4096)
#define HZ5_MIDFRONT_CLASS_COUNT 5u

#ifndef HZ5_MIDFRONT_MAP_BITS
#define HZ5_MIDFRONT_MAP_BITS 21u
#endif

#define HZ5_MIDFRONT_MAP_CAP ((size_t)1u << HZ5_MIDFRONT_MAP_BITS)

#ifndef HZ5_MIDFRONT_REMOTE_BATCH_CAP
#define HZ5_MIDFRONT_REMOTE_BATCH_CAP 16u
#endif

#ifndef HZ5_MIDFRONT_REMOTE_OUTBOX_SLOTS
#define HZ5_MIDFRONT_REMOTE_OUTBOX_SLOTS 4u
#endif

#ifndef HZ5_MIDFRONT_SOURCE_BATCH_COUNT
#define HZ5_MIDFRONT_SOURCE_BATCH_COUNT 64u
#endif

typedef enum Hz5MidSpanState {
  HZ5_MIDSPAN_INVALID = 0,
  HZ5_MIDSPAN_ACTIVE = 1,
  HZ5_MIDSPAN_LOCAL_FREE = 2,
  HZ5_MIDSPAN_REMOTE_PENDING = 3,
  HZ5_MIDSPAN_ORPHAN = 4,
  HZ5_MIDSPAN_RETIRED = 5
} Hz5MidSpanState;

typedef struct Hz5MidSpan {
  uint64_t magic;
  void* raw;
  void* base;
  uint32_t class_bytes;
  uint16_t page_count;
  uint16_t class_index;
  Hz5OwnerToken owner;
  _Atomic unsigned char state;
  uint16_t generation;
  uint16_t flags;
  struct Hz5MidSpan* next;
} Hz5MidSpan;

typedef struct Hz5MidMapEntry {
  _Atomic uintptr_t page_base;
  _Atomic(Hz5MidSpan*) span;
} Hz5MidMapEntry;

typedef struct Hz5MidRemoteOutboxSlot {
  Hz5OwnerToken owner;
  uint32_t class_index;
  uint32_t count;
  Hz5MidSpan* head;
  Hz5MidSpan* tail;
} Hz5MidRemoteOutboxSlot;

typedef struct Hz5MidTls {
  Hz5OwnerToken owner;
  Hz5MidSpan* free_head[HZ5_MIDFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_OUTBOX
  Hz5MidRemoteOutboxSlot remote_outbox[HZ5_MIDFRONT_REMOTE_OUTBOX_SLOTS];
  uint32_t remote_outbox_victim;
#else
  Hz5OwnerToken remote_batch_owner;
  uint32_t remote_batch_class;
  uint32_t remote_batch_count;
  Hz5MidSpan* remote_batch_head;
  Hz5MidSpan* remote_batch_tail;
#endif
} Hz5MidTls;

typedef struct Hz5MidRawNode {
  struct Hz5MidRawNode* next;
} Hz5MidRawNode;

static const uint32_t g_hz5_midfront_classes[HZ5_MIDFRONT_CLASS_COUNT] = {
    4096u, 8192u, 16384u, 32768u, 65536u};

static const Hz5OwnerToken k_hz5_midfront_no_owner = {0, 0};

static Hz5MidMapEntry g_hz5_midfront_map[HZ5_MIDFRONT_MAP_CAP];
static _Atomic(void*) g_hz5_midfront_owner_inbox[UINT16_MAX + 1u]
                                             [HZ5_MIDFRONT_CLASS_COUNT];
static Hz5MidSpan* g_hz5_midfront_global_free[HZ5_MIDFRONT_CLASS_COUNT];
static pthread_mutex_t g_hz5_midfront_global_lock = PTHREAD_MUTEX_INITIALIZER;
static Hz5MidRawNode* g_hz5_midfront_source_free[HZ5_MIDFRONT_CLASS_COUNT];
static pthread_mutex_t g_hz5_midfront_source_lock = PTHREAD_MUTEX_INITIALIZER;
#if BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_ON_MISS
static _Atomic uint32_t g_hz5_midfront_owner_inbox_mask[UINT16_MAX + 1u];
#endif
static pthread_mutex_t g_hz5_midfront_map_lock = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local Hz5MidTls g_hz5_midfront_tls;

static Hz5MidTls* hz5_midfront_tls(void) {
  Hz5MidTls* tls = &g_hz5_midfront_tls;
  if (tls->owner.slot == 0) {
    tls->owner = hz5_owner_current();
  }
  return tls;
}

static uint32_t hz5_midfront_class_bytes(uint32_t class_index) {
  return g_hz5_midfront_classes[class_index];
}

static int hz5_midfront_class_valid(uint32_t class_index) {
  return class_index < HZ5_MIDFRONT_CLASS_COUNT;
}

static int hz5_midfront_class_index(size_t size) {
  if (size <= 2048u || size > 65536u) {
    return -1;
  }
  for (uint32_t i = 0; i < HZ5_MIDFRONT_CLASS_COUNT; ++i) {
    if (size <= g_hz5_midfront_classes[i]) {
      return (int)i;
    }
  }
  return -1;
}

static size_t hz5_midfront_span_stride(uint32_t class_index) {
  return HZ5_MIDFRONT_PAGE_SIZE +
         (size_t)hz5_midfront_class_bytes(class_index);
}

static size_t hz5_midfront_hash(uintptr_t page_base) {
  uintptr_t x = page_base >> 12;
  x ^= x >> HZ5_MIDFRONT_MAP_BITS;
  x ^= x >> (HZ5_MIDFRONT_MAP_BITS * 2u);
  return (size_t)x & (HZ5_MIDFRONT_MAP_CAP - 1u);
}

static int hz5_midfront_source_refill_locked(uint32_t class_index) {
  if (!hz5_midfront_class_valid(class_index)) {
    return 0;
  }
  size_t stride = hz5_midfront_span_stride(class_index);
  if (stride == 0 ||
      HZ5_MIDFRONT_SOURCE_BATCH_COUNT > SIZE_MAX / stride) {
    return 0;
  }
  size_t bytes = stride * (size_t)HZ5_MIDFRONT_SOURCE_BATCH_COUNT;
  void* block = mmap(NULL,
                     bytes,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1,
                     0);
  if (block == MAP_FAILED) {
    return 0;
  }

  uintptr_t base = (uintptr_t)block;
  for (uint32_t i = 0; i < HZ5_MIDFRONT_SOURCE_BATCH_COUNT; ++i) {
    Hz5MidRawNode* node = (Hz5MidRawNode*)(base + (uintptr_t)i * stride);
    node->next = g_hz5_midfront_source_free[class_index];
    g_hz5_midfront_source_free[class_index] = node;
  }
  return 1;
}

static void* hz5_midfront_source_alloc_raw(uint32_t class_index) {
  if (!hz5_midfront_class_valid(class_index)) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_midfront_source_lock);
  if (!g_hz5_midfront_source_free[class_index] &&
      !hz5_midfront_source_refill_locked(class_index)) {
    pthread_mutex_unlock(&g_hz5_midfront_source_lock);
    return NULL;
  }
  Hz5MidRawNode* node = g_hz5_midfront_source_free[class_index];
  g_hz5_midfront_source_free[class_index] = node->next;
  pthread_mutex_unlock(&g_hz5_midfront_source_lock);
  return node;
}

static void hz5_midfront_source_free_raw(uint32_t class_index, void* raw) {
  if (!hz5_midfront_class_valid(class_index) || !raw) {
    return;
  }
  Hz5MidRawNode* node = (Hz5MidRawNode*)raw;
  pthread_mutex_lock(&g_hz5_midfront_source_lock);
  node->next = g_hz5_midfront_source_free[class_index];
  g_hz5_midfront_source_free[class_index] = node;
  pthread_mutex_unlock(&g_hz5_midfront_source_lock);
}

static Hz5MidSpan* hz5_midfront_lookup_page(uintptr_t page_base) {
  size_t idx = hz5_midfront_hash(page_base);
  for (size_t probe = 0; probe < HZ5_MIDFRONT_MAP_CAP; ++probe) {
    Hz5MidMapEntry* entry =
        &g_hz5_midfront_map[(idx + probe) & (HZ5_MIDFRONT_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->page_base, memory_order_acquire);
    if (current == page_base) {
      return atomic_load_explicit(&entry->span, memory_order_acquire);
    }
    if (current == 0) {
      return NULL;
    }
  }
  return NULL;
}

static int hz5_midfront_map_insert_one(uintptr_t page_base, Hz5MidSpan* span) {
  size_t idx = hz5_midfront_hash(page_base);
  for (size_t probe = 0; probe < HZ5_MIDFRONT_MAP_CAP; ++probe) {
    Hz5MidMapEntry* entry =
        &g_hz5_midfront_map[(idx + probe) & (HZ5_MIDFRONT_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->page_base, memory_order_acquire);
    if (current == page_base) {
      atomic_store_explicit(&entry->span, span, memory_order_release);
      return 1;
    }
    if (current == 0) {
      atomic_store_explicit(&entry->span, span, memory_order_relaxed);
      atomic_store_explicit(&entry->page_base, page_base,
                            memory_order_release);
      return 1;
    }
  }
  return 0;
}

static int hz5_midfront_map_insert(Hz5MidSpan* span) {
  uintptr_t base = (uintptr_t)span->base;
  pthread_mutex_lock(&g_hz5_midfront_map_lock);
  for (uint32_t i = 0; i < span->page_count; ++i) {
    uintptr_t page_base = base + ((uintptr_t)i * HZ5_MIDFRONT_PAGE_SIZE);
    if (!hz5_midfront_map_insert_one(page_base, span)) {
      pthread_mutex_unlock(&g_hz5_midfront_map_lock);
      return 0;
    }
  }
  pthread_mutex_unlock(&g_hz5_midfront_map_lock);
  return 1;
}

static Hz5MidSpan* hz5_midfront_span_for_ptr(void* ptr) {
  if (!ptr) {
    return NULL;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t page_base = p & ~(uintptr_t)(HZ5_MIDFRONT_PAGE_SIZE - 1u);
  Hz5MidSpan* span = hz5_midfront_lookup_page(page_base);
  if (!span || span->magic != HZ5_MIDFRONT_MAGIC) {
    return NULL;
  }
  uintptr_t base = (uintptr_t)span->base;
  uintptr_t end = base + (uintptr_t)span->class_bytes;
  if (p < base || p >= end) {
    return NULL;
  }
  return span;
}

static void hz5_midfront_local_push(Hz5MidTls* tls,
                                    uint32_t class_index,
                                    Hz5MidSpan* span) {
  span->next = tls->free_head[class_index];
  tls->free_head[class_index] = span;
}

static Hz5MidSpan* hz5_midfront_local_pop(Hz5MidTls* tls,
                                          uint32_t class_index) {
  Hz5MidSpan* span = tls->free_head[class_index];
  if (!span) {
    return NULL;
  }
  tls->free_head[class_index] = span->next;
  span->next = NULL;
  return span;
}

static void hz5_midfront_global_push(uint32_t class_index, Hz5MidSpan* span) {
  if (!hz5_midfront_class_valid(class_index) || !span) {
    return;
  }
  pthread_mutex_lock(&g_hz5_midfront_global_lock);
  span->next = g_hz5_midfront_global_free[class_index];
  g_hz5_midfront_global_free[class_index] = span;
  pthread_mutex_unlock(&g_hz5_midfront_global_lock);
}

static Hz5MidSpan* hz5_midfront_global_pop(uint32_t class_index) {
  if (!hz5_midfront_class_valid(class_index)) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_midfront_global_lock);
  Hz5MidSpan* span = g_hz5_midfront_global_free[class_index];
  if (span) {
    g_hz5_midfront_global_free[class_index] = span->next;
    span->next = NULL;
  }
  pthread_mutex_unlock(&g_hz5_midfront_global_lock);
  return span;
}

static int hz5_midfront_state_cas(Hz5MidSpan* span,
                                  unsigned char from,
                                  unsigned char to) {
  unsigned char expected = from;
  return atomic_compare_exchange_strong_explicit(&span->state,
                                                 &expected,
                                                 to,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire);
}

static int hz5_midfront_owner_local_state_transition(Hz5MidSpan* span,
                                                     unsigned char from,
                                                     unsigned char to) {
#if BENCHLAB_HZ5_LINUX_MIDFRONT_OWNER_FAST_STATE
  if (atomic_load_explicit(&span->state, memory_order_acquire) != from) {
    return 0;
  }
  atomic_store_explicit(&span->state, to, memory_order_release);
  return 1;
#else
  return hz5_midfront_state_cas(span, from, to);
#endif
}

static int hz5_midfront_activate_local_for_owner(Hz5MidTls* tls,
                                                 Hz5MidSpan* span) {
  if (!hz5_midfront_owner_local_state_transition(
          span,
          (unsigned char)HZ5_MIDSPAN_LOCAL_FREE,
          (unsigned char)HZ5_MIDSPAN_ACTIVE)) {
    return 0;
  }
  span->owner = tls->owner;
  return 1;
}

static int hz5_midfront_activate_global_for_owner(Hz5MidTls* tls,
                                                  Hz5MidSpan* span) {
  if (!hz5_midfront_state_cas(span,
                              (unsigned char)HZ5_MIDSPAN_LOCAL_FREE,
                              (unsigned char)HZ5_MIDSPAN_ACTIVE)) {
    return 0;
  }
  span->owner = tls->owner;
  return 1;
}

static void hz5_midfront_mark_orphan(Hz5MidSpan* span) {
  if (!span) {
    return;
  }
  atomic_store_explicit(&span->state,
                        (unsigned char)HZ5_MIDSPAN_ORPHAN,
                        memory_order_release);
}

static int hz5_midfront_can_publish_to_owner(Hz5OwnerToken owner,
                                             uint32_t class_index,
                                             const Hz5MidSpan* head,
                                             const Hz5MidSpan* tail) {
  return owner.slot != 0 && hz5_midfront_class_valid(class_index) && head &&
         tail && hz5_owner_is_alive(owner);
}

static void hz5_midfront_remote_publish_list(Hz5OwnerToken owner,
                                             uint32_t class_index,
                                             Hz5MidSpan* head,
                                             Hz5MidSpan* tail) {
  if (!hz5_midfront_can_publish_to_owner(owner, class_index, head, tail)) {
    return;
  }

  void* old_head = NULL;
  _Atomic(void*)* inbox =
      &g_hz5_midfront_owner_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    tail->next = (Hz5MidSpan*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, head, memory_order_release, memory_order_acquire));
  hz5_ownerhub_mark_pending(owner, HZ5_OWNERHUB_FRONT_MID, class_index);
#if BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_ON_MISS
  atomic_fetch_or_explicit(&g_hz5_midfront_owner_inbox_mask[owner.slot],
                           UINT32_C(1) << class_index,
                           memory_order_release);
#endif
}

#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_OUTBOX
static void hz5_midfront_remote_outbox_flush_slot(
    Hz5MidRemoteOutboxSlot* slot) {
  if (!slot || slot->count == 0u) {
    return;
  }

  hz5_midfront_remote_publish_list(
      slot->owner, slot->class_index, slot->head, slot->tail);
  slot->owner = k_hz5_midfront_no_owner;
  slot->class_index = 0;
  slot->count = 0;
  slot->head = NULL;
  slot->tail = NULL;
}

static void hz5_midfront_remote_outbox_flush_class(Hz5MidTls* tls,
                                                   uint32_t class_index) {
  if (!tls || !hz5_midfront_class_valid(class_index)) {
    return;
  }
  for (uint32_t i = 0; i < HZ5_MIDFRONT_REMOTE_OUTBOX_SLOTS; ++i) {
    Hz5MidRemoteOutboxSlot* slot = &tls->remote_outbox[i];
    if (slot->count != 0u && slot->class_index == class_index) {
      hz5_midfront_remote_outbox_flush_slot(slot);
    }
  }
}

static Hz5MidRemoteOutboxSlot* hz5_midfront_remote_outbox_slot(
    Hz5MidTls* tls,
    Hz5OwnerToken owner,
    uint32_t class_index) {
  Hz5MidRemoteOutboxSlot* empty = NULL;
  for (uint32_t i = 0; i < HZ5_MIDFRONT_REMOTE_OUTBOX_SLOTS; ++i) {
    Hz5MidRemoteOutboxSlot* slot = &tls->remote_outbox[i];
    if (slot->count != 0u && slot->class_index == class_index &&
        hz5_owner_equal(slot->owner, owner)) {
      return slot;
    }
    if (slot->count == 0u && !empty) {
      empty = slot;
    }
  }
  if (empty) {
    return empty;
  }

  uint32_t victim =
      tls->remote_outbox_victim++ % HZ5_MIDFRONT_REMOTE_OUTBOX_SLOTS;
  Hz5MidRemoteOutboxSlot* slot = &tls->remote_outbox[victim];
  hz5_midfront_remote_outbox_flush_slot(slot);
  return slot;
}

static void hz5_midfront_remote_batch_push(Hz5MidTls* tls, Hz5MidSpan* span) {
  if (!tls || !span || !hz5_owner_is_alive(span->owner)) {
    if (span) {
      hz5_midfront_mark_orphan(span);
    }
    return;
  }

  uint32_t class_index = span->class_index;
  Hz5MidRemoteOutboxSlot* slot =
      hz5_midfront_remote_outbox_slot(tls, span->owner, class_index);
  span->next = NULL;

  if (slot->count == 0u) {
    slot->owner = span->owner;
    slot->class_index = class_index;
    slot->head = span;
  } else {
    slot->tail->next = span;
  }
  slot->tail = span;
  ++slot->count;

  if (slot->count >= HZ5_MIDFRONT_REMOTE_BATCH_CAP) {
    hz5_midfront_remote_outbox_flush_slot(slot);
  }
}
#else
static void hz5_midfront_remote_batch_flush(Hz5MidTls* tls) {
  if (!tls || tls->remote_batch_count == 0u) {
    return;
  }

  hz5_midfront_remote_publish_list(tls->remote_batch_owner,
                                   tls->remote_batch_class,
                                   tls->remote_batch_head,
                                   tls->remote_batch_tail);
  tls->remote_batch_owner = k_hz5_midfront_no_owner;
  tls->remote_batch_class = 0;
  tls->remote_batch_count = 0;
  tls->remote_batch_head = NULL;
  tls->remote_batch_tail = NULL;
}

static void hz5_midfront_remote_batch_push(Hz5MidTls* tls, Hz5MidSpan* span) {
  if (!hz5_owner_is_alive(span->owner)) {
    hz5_midfront_mark_orphan(span);
    return;
  }
  uint32_t class_index = span->class_index;
  if (tls->remote_batch_count != 0u &&
      (!hz5_owner_equal(tls->remote_batch_owner, span->owner) ||
       tls->remote_batch_class != class_index)) {
    hz5_midfront_remote_batch_flush(tls);
  }

  span->next = NULL;
  if (tls->remote_batch_count == 0u) {
    tls->remote_batch_owner = span->owner;
    tls->remote_batch_class = class_index;
    tls->remote_batch_head = span;
  } else {
    tls->remote_batch_tail->next = span;
  }
  tls->remote_batch_tail = span;
  ++tls->remote_batch_count;

  if (tls->remote_batch_count >= HZ5_MIDFRONT_REMOTE_BATCH_CAP) {
    hz5_midfront_remote_batch_flush(tls);
  }
}
#endif

static Hz5MidSpan* hz5_midfront_drain_remote_class_budget(Hz5MidTls* tls,
                                                          uint32_t class_index,
                                                          int take_first,
                                                          uint32_t budget,
                                                          uint32_t* drained_out) {
  uint32_t drained = 0;
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_MIDFRONT_CLASS_COUNT) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }

#if BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_ON_MISS
  atomic_fetch_and_explicit(&g_hz5_midfront_owner_inbox_mask[tls->owner.slot],
                            ~(UINT32_C(1) << class_index),
                            memory_order_acq_rel);
#endif
  hz5_ownerhub_clear_pending(tls->owner,
                             HZ5_OWNERHUB_FRONT_MID,
                             class_index);
  _Atomic(void*)* inbox =
      &g_hz5_midfront_owner_inbox[tls->owner.slot][class_index];
#if BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_EMPTY_GATED
  if (!atomic_load_explicit(inbox, memory_order_acquire)) {
    return NULL;
  }
#endif
  void* head = atomic_exchange_explicit(inbox, NULL, memory_order_acq_rel);
  Hz5MidSpan* taken = NULL;
  while (head) {
    Hz5MidSpan* span = (Hz5MidSpan*)head;
    head = span->next;
    if (budget != 0u && drained >= budget) {
      Hz5MidSpan* tail = span;
      while (tail->next) {
        tail = tail->next;
      }
      hz5_midfront_remote_publish_list(tls->owner, class_index, span, tail);
      if (drained_out) {
        *drained_out = drained;
      }
      return taken;
    }
    if (!hz5_owner_equal(span->owner, tls->owner)) {
      hz5_midfront_mark_orphan(span);
      continue;
    }
#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE
    if (take_first && !taken &&
        hz5_midfront_state_cas(span,
                               (unsigned char)HZ5_MIDSPAN_LOCAL_FREE,
                               (unsigned char)HZ5_MIDSPAN_ACTIVE)) {
      span->next = NULL;
      taken = span;
      continue;
    }
#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_TRUST_DRAIN_STATE
    hz5_midfront_local_push(tls, class_index, span);
#else
    if (atomic_load_explicit(&span->state, memory_order_acquire) ==
        (unsigned char)HZ5_MIDSPAN_LOCAL_FREE) {
      hz5_midfront_local_push(tls, class_index, span);
    }
#endif
#else
    if (take_first && !taken &&
        hz5_midfront_state_cas(span,
                               (unsigned char)HZ5_MIDSPAN_REMOTE_PENDING,
                               (unsigned char)HZ5_MIDSPAN_ACTIVE)) {
      span->next = NULL;
      taken = span;
      continue;
    }
    if (hz5_midfront_state_cas(span,
                               (unsigned char)HZ5_MIDSPAN_REMOTE_PENDING,
                               (unsigned char)HZ5_MIDSPAN_LOCAL_FREE)) {
      hz5_midfront_local_push(tls, class_index, span);
    }
#endif
    ++drained;
  }
  if (drained_out) {
    *drained_out = drained;
  }
  return taken;
}

static Hz5MidSpan* hz5_midfront_drain_remote_class(Hz5MidTls* tls,
                                                   uint32_t class_index,
                                                   int take_first) {
  return hz5_midfront_drain_remote_class_budget(tls,
                                                class_index,
                                                take_first,
                                                0u,
                                                NULL);
}

static Hz5MidSpan* hz5_midfront_drain_remote_on_miss(Hz5MidTls* tls,
                                                     uint32_t class_index) {
  Hz5MidSpan* taken = NULL;
#if BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_TAKE_FIRST
  int take_first = 1;
#else
  int take_first = 0;
#endif
#if BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_ALL_ON_MISS
  for (uint32_t i = 0; i < HZ5_MIDFRONT_CLASS_COUNT; ++i) {
    Hz5MidSpan* candidate =
        hz5_midfront_drain_remote_class(tls, i, i == class_index && take_first);
    if (candidate) {
      taken = candidate;
    }
  }
#elif BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_ON_MISS
  taken = hz5_midfront_drain_remote_class(tls, class_index, take_first);
  if (taken) {
    return taken;
  }
  if (BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_MASK_HIT_STOP && tls &&
      class_index < HZ5_MIDFRONT_CLASS_COUNT &&
      tls->free_head[class_index] != NULL) {
    return NULL;
  }
  if (!tls || tls->owner.slot == 0) {
    return NULL;
  }
  uint32_t mask = atomic_load_explicit(
      &g_hz5_midfront_owner_inbox_mask[tls->owner.slot],
      memory_order_acquire);
  mask &= (UINT32_C(1) << HZ5_MIDFRONT_CLASS_COUNT) - UINT32_C(1);
  for (uint32_t i = 0; i < HZ5_MIDFRONT_CLASS_COUNT; ++i) {
    if (i != class_index && (mask & (UINT32_C(1) << i))) {
      hz5_midfront_drain_remote_class(tls, i, 0);
    }
  }
#else
  taken = hz5_midfront_drain_remote_class(tls, class_index, take_first);
#endif
  return taken;
}

void hz5_midfront_ownerhub_drain_some(uint32_t budget) {
  Hz5MidTls* tls = hz5_midfront_tls();
  uint32_t remaining = budget;
  for (uint32_t i = 0; i < HZ5_MIDFRONT_CLASS_COUNT; ++i) {
    if (remaining == 0u) {
      break;
    }
    uint32_t drained = 0;
    (void)hz5_midfront_drain_remote_class_budget(tls,
                                                 i,
                                                 0,
                                                 remaining,
                                                 &drained);
    remaining -= drained > remaining ? remaining : drained;
  }
}

static Hz5MidSpan* hz5_midfront_new_span(Hz5MidTls* tls,
                                         uint32_t class_index) {
  uint32_t class_bytes = hz5_midfront_class_bytes(class_index);
  void* raw = hz5_midfront_source_alloc_raw(class_index);
  if (!raw) {
    return NULL;
  }

  Hz5MidSpan* span = (Hz5MidSpan*)raw;
  span->magic = HZ5_MIDFRONT_MAGIC;
  span->raw = raw;
  span->base = (void*)((uintptr_t)raw + HZ5_MIDFRONT_PAGE_SIZE);
  span->class_bytes = class_bytes;
  span->page_count = (uint16_t)(class_bytes / HZ5_MIDFRONT_PAGE_SIZE);
  span->class_index = (uint16_t)class_index;
  span->owner = tls->owner;
  atomic_store_explicit(&span->state,
                        (unsigned char)HZ5_MIDSPAN_ACTIVE,
                        memory_order_release);
  span->generation = 1;
  span->flags = 0;
  span->next = NULL;

  if (!hz5_midfront_map_insert(span)) {
    hz5_midfront_source_free_raw(class_index, raw);
    return NULL;
  }
  return span;
}

void* hz5_midfront_alloc(size_t size, size_t align) {
  if (align > 16u) {
    return NULL;
  }
  int class_index = hz5_midfront_class_index(size);
  if (class_index < 0) {
    return NULL;
  }

  Hz5MidTls* tls = hz5_midfront_tls();
  uint32_t ci = (uint32_t)class_index;
  Hz5MidSpan* span = hz5_midfront_local_pop(tls, ci);
  if (!span) {
#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_OUTBOX && \
    BENCHLAB_HZ5_LINUX_MIDFRONT_OUTBOX_FLUSH_ON_MISS
    hz5_midfront_remote_outbox_flush_class(tls, ci);
#endif
    hz5_ownerhub_note_alloc_miss(tls->owner, HZ5_OWNERHUB_FRONT_MID, ci);
    span = hz5_midfront_drain_remote_on_miss(tls, ci);
    if (span) {
      return span->base;
    }
    hz5_ownerhub_drain_cross_fronts(tls->owner, HZ5_OWNERHUB_FRONT_MID);
    span = hz5_midfront_local_pop(tls, ci);
  }
  if (span) {
    if (!hz5_midfront_activate_local_for_owner(tls, span)) {
      return NULL;
    }
    return span->base;
  }

#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE
  while ((span = hz5_midfront_global_pop(ci)) != NULL) {
    if (hz5_midfront_activate_global_for_owner(tls, span)) {
      return span->base;
    }
  }
#endif

  span = hz5_midfront_new_span(tls, ci);
  return span ? span->base : NULL;
}

Hz5MidFrontFreeResult hz5_midfront_free(void* ptr) {
  Hz5MidSpan* span = hz5_midfront_span_for_ptr(ptr);
  if (!span) {
    return HZ5_MIDFRONT_FREE_NOT_OWNED;
  }
  if (ptr != span->base) {
    return HZ5_MIDFRONT_FREE_INVALID;
  }

  Hz5MidTls* tls = hz5_midfront_tls();
  if (hz5_owner_equal(span->owner, tls->owner)) {
    if (!hz5_midfront_owner_local_state_transition(
            span,
            (unsigned char)HZ5_MIDSPAN_ACTIVE,
            (unsigned char)HZ5_MIDSPAN_LOCAL_FREE)) {
      return HZ5_MIDFRONT_FREE_INVALID;
    }
    hz5_midfront_local_push(tls, span->class_index, span);
  } else {
#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE
    if (!hz5_midfront_state_cas(span,
                                (unsigned char)HZ5_MIDSPAN_ACTIVE,
                                (unsigned char)HZ5_MIDSPAN_LOCAL_FREE)) {
      return HZ5_MIDFRONT_FREE_INVALID;
    }
    hz5_midfront_global_push(span->class_index, span);
#else
#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE
    if (!hz5_midfront_state_cas(span,
                                (unsigned char)HZ5_MIDSPAN_ACTIVE,
                                (unsigned char)HZ5_MIDSPAN_LOCAL_FREE)) {
      return HZ5_MIDFRONT_FREE_INVALID;
    }
#else
    if (!hz5_midfront_state_cas(span,
                                (unsigned char)HZ5_MIDSPAN_ACTIVE,
                                (unsigned char)HZ5_MIDSPAN_REMOTE_PENDING)) {
      return HZ5_MIDFRONT_FREE_INVALID;
    }
#endif
    hz5_midfront_remote_batch_push(tls, span);
#endif
  }
  return HZ5_MIDFRONT_FREE_OK;
}

int hz5_midfront_can_handle(size_t size, size_t align) {
  return align <= 16u && hz5_midfront_class_index(size) >= 0;
}

int hz5_midfront_owns(void* ptr) {
  return hz5_midfront_span_for_ptr(ptr) != NULL;
}

size_t hz5_midfront_usable_size(void* ptr) {
  Hz5MidSpan* span = hz5_midfront_span_for_ptr(ptr);
  if (!span || ptr != span->base) {
    return 0;
  }
  return span->class_bytes;
}

#else

void hz5_midfront_ownerhub_drain_some(uint32_t budget) {
  (void)budget;
}

void* hz5_midfront_alloc(size_t size, size_t align) {
  (void)size;
  (void)align;
  return NULL;
}

Hz5MidFrontFreeResult hz5_midfront_free(void* ptr) {
  (void)ptr;
  return HZ5_MIDFRONT_FREE_NOT_OWNED;
}

int hz5_midfront_can_handle(size_t size, size_t align) {
  (void)size;
  (void)align;
  return 0;
}

int hz5_midfront_owns(void* ptr) {
  (void)ptr;
  return 0;
}

size_t hz5_midfront_usable_size(void* ptr) {
  (void)ptr;
  return 0;
}

#endif
