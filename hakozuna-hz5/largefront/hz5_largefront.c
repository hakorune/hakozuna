#include "hz5_largefront.h"

#include "hz5_config.h"
#include "hz5_internal.h"
#include "hz5_ownerhub.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <sys/mman.h>

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_L1
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_L1 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_FAST_STATE
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_FAST_STATE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_BATCH
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_BATCH 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TAKE_FIRST
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TAKE_FIRST 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_EMPTY_GATED
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_EMPTY_GATED 0
#endif

#ifndef HZ5_LARGEFRONT_REMOTE_BATCH_CAP
#define HZ5_LARGEFRONT_REMOTE_BATCH_CAP 16u
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LARGEFRONT_L1

#define HZ5_LARGEFRONT_MAGIC UINT64_C(0x485A354C41524731)
#define HZ5_LARGEFRONT_PAGE_SIZE ((size_t)4096)
#define HZ5_LARGEFRONT_CLASS_COUNT 4u

#ifndef HZ5_LARGEFRONT_MAP_BITS
#define HZ5_LARGEFRONT_MAP_BITS 22u
#endif

#define HZ5_LARGEFRONT_MAP_CAP ((size_t)1u << HZ5_LARGEFRONT_MAP_BITS)

#ifndef HZ5_LARGEFRONT_SOURCE_BATCH_COUNT
#define HZ5_LARGEFRONT_SOURCE_BATCH_COUNT 16u
#endif

typedef enum Hz5LargeSpanState {
  HZ5_LARGESPAN_INVALID = 0,
  HZ5_LARGESPAN_ACTIVE = 1,
  HZ5_LARGESPAN_LOCAL_FREE = 2,
  HZ5_LARGESPAN_REMOTE_PENDING = 3,
  HZ5_LARGESPAN_ORPHAN = 4
} Hz5LargeSpanState;

typedef struct Hz5LargeSpan {
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
  struct Hz5LargeSpan* next;
} Hz5LargeSpan;

typedef struct Hz5LargeMapEntry {
  _Atomic uintptr_t page_base;
  _Atomic(Hz5LargeSpan*) span;
} Hz5LargeMapEntry;

typedef struct Hz5LargeTls {
  Hz5OwnerToken owner;
  Hz5LargeSpan* free_head[HZ5_LARGEFRONT_CLASS_COUNT];
  Hz5OwnerToken remote_batch_owner;
  uint32_t remote_batch_class;
  uint32_t remote_batch_count;
  Hz5LargeSpan* remote_batch_head;
  Hz5LargeSpan* remote_batch_tail;
} Hz5LargeTls;

typedef struct Hz5LargeRawNode {
  struct Hz5LargeRawNode* next;
} Hz5LargeRawNode;

static const uint32_t g_hz5_largefront_classes[HZ5_LARGEFRONT_CLASS_COUNT] = {
    131072u, 262144u, 524288u, 1048576u};

static const Hz5OwnerToken k_hz5_largefront_no_owner = {0, 0};

static Hz5LargeMapEntry g_hz5_largefront_map[HZ5_LARGEFRONT_MAP_CAP];
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
static _Atomic(void*) g_hz5_largefront_owner_inbox[UINT16_MAX + 1u]
                                                 [HZ5_LARGEFRONT_CLASS_COUNT];
#endif
static Hz5LargeSpan* g_hz5_largefront_global_free[HZ5_LARGEFRONT_CLASS_COUNT];
static pthread_mutex_t g_hz5_largefront_global_lock =
    PTHREAD_MUTEX_INITIALIZER;
static Hz5LargeRawNode* g_hz5_largefront_source_free[HZ5_LARGEFRONT_CLASS_COUNT];
static pthread_mutex_t g_hz5_largefront_source_lock =
    PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_hz5_largefront_map_lock = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local Hz5LargeTls g_hz5_largefront_tls;

static Hz5LargeTls* hz5_largefront_tls(void) {
  Hz5LargeTls* tls = &g_hz5_largefront_tls;
  if (tls->owner.slot == 0) {
    tls->owner = hz5_owner_current();
  }
  return tls;
}

static int hz5_largefront_class_valid(uint32_t class_index) {
  return class_index < HZ5_LARGEFRONT_CLASS_COUNT;
}

static uint32_t hz5_largefront_class_bytes(uint32_t class_index) {
  return g_hz5_largefront_classes[class_index];
}

static int hz5_largefront_class_index(size_t size) {
  if (size <= 65536u || size > 1048576u) {
    return -1;
  }
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_CLASS_COUNT; ++i) {
    if (size <= g_hz5_largefront_classes[i]) {
      return (int)i;
    }
  }
  return -1;
}

static size_t hz5_largefront_span_stride(uint32_t class_index) {
  return HZ5_LARGEFRONT_PAGE_SIZE +
         (size_t)hz5_largefront_class_bytes(class_index);
}

static size_t hz5_largefront_hash(uintptr_t page_base) {
  uintptr_t x = page_base >> 12;
  x ^= x >> HZ5_LARGEFRONT_MAP_BITS;
  x ^= x >> (HZ5_LARGEFRONT_MAP_BITS * 2u);
  return (size_t)x & (HZ5_LARGEFRONT_MAP_CAP - 1u);
}

static int hz5_largefront_source_refill_locked(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return 0;
  }
  size_t stride = hz5_largefront_span_stride(class_index);
  if (stride == 0 ||
      HZ5_LARGEFRONT_SOURCE_BATCH_COUNT > SIZE_MAX / stride) {
    return 0;
  }
  size_t bytes = stride * (size_t)HZ5_LARGEFRONT_SOURCE_BATCH_COUNT;
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
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_SOURCE_BATCH_COUNT; ++i) {
    Hz5LargeRawNode* node = (Hz5LargeRawNode*)(base + (uintptr_t)i * stride);
    node->next = g_hz5_largefront_source_free[class_index];
    g_hz5_largefront_source_free[class_index] = node;
  }
  return 1;
}

static void* hz5_largefront_source_alloc_raw(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_largefront_source_lock);
  if (!g_hz5_largefront_source_free[class_index] &&
      !hz5_largefront_source_refill_locked(class_index)) {
    pthread_mutex_unlock(&g_hz5_largefront_source_lock);
    return NULL;
  }
  Hz5LargeRawNode* node = g_hz5_largefront_source_free[class_index];
  g_hz5_largefront_source_free[class_index] = node->next;
  pthread_mutex_unlock(&g_hz5_largefront_source_lock);
  return node;
}

static void hz5_largefront_source_free_raw(uint32_t class_index, void* raw) {
  if (!hz5_largefront_class_valid(class_index) || !raw) {
    return;
  }
  Hz5LargeRawNode* node = (Hz5LargeRawNode*)raw;
  pthread_mutex_lock(&g_hz5_largefront_source_lock);
  node->next = g_hz5_largefront_source_free[class_index];
  g_hz5_largefront_source_free[class_index] = node;
  pthread_mutex_unlock(&g_hz5_largefront_source_lock);
}

static Hz5LargeSpan* hz5_largefront_lookup_page(uintptr_t page_base) {
  size_t idx = hz5_largefront_hash(page_base);
  for (size_t probe = 0; probe < HZ5_LARGEFRONT_MAP_CAP; ++probe) {
    Hz5LargeMapEntry* entry =
        &g_hz5_largefront_map[(idx + probe) & (HZ5_LARGEFRONT_MAP_CAP - 1u)];
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

static int hz5_largefront_map_insert_one(uintptr_t page_base,
                                         Hz5LargeSpan* span) {
  size_t idx = hz5_largefront_hash(page_base);
  for (size_t probe = 0; probe < HZ5_LARGEFRONT_MAP_CAP; ++probe) {
    Hz5LargeMapEntry* entry =
        &g_hz5_largefront_map[(idx + probe) & (HZ5_LARGEFRONT_MAP_CAP - 1u)];
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

static int hz5_largefront_map_insert(Hz5LargeSpan* span) {
  uintptr_t base = (uintptr_t)span->base;
  pthread_mutex_lock(&g_hz5_largefront_map_lock);
  for (uint32_t i = 0; i < span->page_count; ++i) {
    uintptr_t page_base = base + ((uintptr_t)i * HZ5_LARGEFRONT_PAGE_SIZE);
    if (!hz5_largefront_map_insert_one(page_base, span)) {
      pthread_mutex_unlock(&g_hz5_largefront_map_lock);
      return 0;
    }
  }
  pthread_mutex_unlock(&g_hz5_largefront_map_lock);
  return 1;
}

static Hz5LargeSpan* hz5_largefront_span_for_ptr(void* ptr) {
  if (!ptr) {
    return NULL;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t page_base = p & ~(uintptr_t)(HZ5_LARGEFRONT_PAGE_SIZE - 1u);
  Hz5LargeSpan* span = hz5_largefront_lookup_page(page_base);
  if (!span || span->magic != HZ5_LARGEFRONT_MAGIC) {
    return NULL;
  }
  uintptr_t base = (uintptr_t)span->base;
  uintptr_t end = base + (uintptr_t)span->class_bytes;
  if (p < base || p >= end) {
    return NULL;
  }
  return span;
}

static void hz5_largefront_local_push(Hz5LargeTls* tls,
                                      uint32_t class_index,
                                      Hz5LargeSpan* span) {
  span->next = tls->free_head[class_index];
  tls->free_head[class_index] = span;
}

static Hz5LargeSpan* hz5_largefront_local_pop(Hz5LargeTls* tls,
                                              uint32_t class_index) {
  Hz5LargeSpan* span = tls->free_head[class_index];
  if (!span) {
    return NULL;
  }
  tls->free_head[class_index] = span->next;
  span->next = NULL;
  return span;
}

static void hz5_largefront_global_push(uint32_t class_index,
                                       Hz5LargeSpan* span) {
  if (!hz5_largefront_class_valid(class_index) || !span) {
    return;
  }
  pthread_mutex_lock(&g_hz5_largefront_global_lock);
  span->next = g_hz5_largefront_global_free[class_index];
  g_hz5_largefront_global_free[class_index] = span;
  pthread_mutex_unlock(&g_hz5_largefront_global_lock);
}

static Hz5LargeSpan* hz5_largefront_global_pop(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_largefront_global_lock);
  Hz5LargeSpan* span = g_hz5_largefront_global_free[class_index];
  if (span) {
    g_hz5_largefront_global_free[class_index] = span->next;
    span->next = NULL;
  }
  pthread_mutex_unlock(&g_hz5_largefront_global_lock);
  return span;
}

static int hz5_largefront_state_cas(Hz5LargeSpan* span,
                                    unsigned char from,
                                    unsigned char to) {
  unsigned char expected = from;
  return atomic_compare_exchange_strong_explicit(&span->state,
                                                 &expected,
                                                 to,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire);
}

static int hz5_largefront_owner_local_state_transition(Hz5LargeSpan* span,
                                                       unsigned char from,
                                                       unsigned char to) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_FAST_STATE
  if (atomic_load_explicit(&span->state, memory_order_acquire) != from) {
    return 0;
  }
  atomic_store_explicit(&span->state, to, memory_order_release);
  return 1;
#else
  return hz5_largefront_state_cas(span, from, to);
#endif
}

static int hz5_largefront_activate_local_for_owner(Hz5LargeTls* tls,
                                                   Hz5LargeSpan* span) {
  if (!hz5_largefront_owner_local_state_transition(
          span,
          (unsigned char)HZ5_LARGESPAN_LOCAL_FREE,
          (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
    return 0;
  }
  span->owner = tls->owner;
  return 1;
}

static int hz5_largefront_activate_global_for_owner(Hz5LargeTls* tls,
                                                    Hz5LargeSpan* span) {
  if (!hz5_largefront_state_cas(span,
                                (unsigned char)HZ5_LARGESPAN_LOCAL_FREE,
                                (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
    return 0;
  }
  span->owner = tls->owner;
  return 1;
}

static void hz5_largefront_mark_orphan(Hz5LargeSpan* span) {
  if (!span) {
    return;
  }
  atomic_store_explicit(&span->state,
                        (unsigned char)HZ5_LARGESPAN_ORPHAN,
                        memory_order_release);
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
static int hz5_largefront_can_publish_to_owner(Hz5OwnerToken owner,
                                               uint32_t class_index,
                                               const Hz5LargeSpan* span) {
  return owner.slot != 0 && hz5_largefront_class_valid(class_index) && span &&
         hz5_owner_is_alive(owner);
}

static int hz5_largefront_remote_publish_one(Hz5OwnerToken owner,
                                             uint32_t class_index,
                                             Hz5LargeSpan* span) {
  if (!hz5_largefront_can_publish_to_owner(owner, class_index, span)) {
    return 0;
  }

  void* old_head = NULL;
  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    span->next = (Hz5LargeSpan*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, span, memory_order_release, memory_order_acquire));
  hz5_ownerhub_mark_pending(owner, HZ5_OWNERHUB_FRONT_LARGE, class_index);
  return 1;
}

static int hz5_largefront_remote_publish_list(Hz5OwnerToken owner,
                                              uint32_t class_index,
                                              Hz5LargeSpan* head,
                                              Hz5LargeSpan* tail) {
  if (!hz5_largefront_can_publish_to_owner(owner, class_index, head) || !tail) {
    return 0;
  }

  void* old_head = NULL;
  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    tail->next = (Hz5LargeSpan*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, head, memory_order_release, memory_order_acquire));
  hz5_ownerhub_mark_pending(owner, HZ5_OWNERHUB_FRONT_LARGE, class_index);
  return 1;
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_BATCH
static void hz5_largefront_remote_batch_flush(Hz5LargeTls* tls) {
  if (!tls || tls->remote_batch_count == 0u) {
    return;
  }

  if (!hz5_largefront_remote_publish_list(tls->remote_batch_owner,
                                          tls->remote_batch_class,
                                          tls->remote_batch_head,
                                          tls->remote_batch_tail)) {
    Hz5LargeSpan* span = tls->remote_batch_head;
    while (span) {
      Hz5LargeSpan* next = span->next;
      hz5_largefront_mark_orphan(span);
      span = next;
    }
  }
  tls->remote_batch_owner = k_hz5_largefront_no_owner;
  tls->remote_batch_class = 0;
  tls->remote_batch_count = 0;
  tls->remote_batch_head = NULL;
  tls->remote_batch_tail = NULL;
}

static void hz5_largefront_remote_batch_push(Hz5LargeTls* tls,
                                             Hz5LargeSpan* span) {
  if (!hz5_owner_is_alive(span->owner)) {
    hz5_largefront_mark_orphan(span);
    return;
  }
  uint32_t class_index = span->class_index;
  if (tls->remote_batch_count != 0u &&
      (!hz5_owner_equal(tls->remote_batch_owner, span->owner) ||
       tls->remote_batch_class != class_index)) {
    hz5_largefront_remote_batch_flush(tls);
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

  if (tls->remote_batch_count >= HZ5_LARGEFRONT_REMOTE_BATCH_CAP) {
    hz5_largefront_remote_batch_flush(tls);
  }
}
#endif

static Hz5LargeSpan* hz5_largefront_drain_remote_class_budget(
    Hz5LargeTls* tls,
    uint32_t class_index,
    uint32_t budget,
    uint32_t* drained_out) {
  uint32_t drained = 0;
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }

  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[tls->owner.slot][class_index];
  hz5_ownerhub_clear_pending(tls->owner,
                             HZ5_OWNERHUB_FRONT_LARGE,
                             class_index);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_EMPTY_GATED
  if (!atomic_load_explicit(inbox, memory_order_acquire)) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }
#endif
  void* head = atomic_exchange_explicit(inbox, NULL, memory_order_acq_rel);
  Hz5LargeSpan* taken = NULL;
  while (head) {
    Hz5LargeSpan* span = (Hz5LargeSpan*)head;
    head = span->next;
    if (budget != 0u && drained >= budget) {
      Hz5LargeSpan* tail = span;
      while (tail->next) {
        tail = tail->next;
      }
      (void)hz5_largefront_remote_publish_list(tls->owner,
                                               class_index,
                                               span,
                                               tail);
      if (drained_out) {
        *drained_out = drained;
      }
      return taken;
    }
    if (!hz5_owner_equal(span->owner, tls->owner)) {
      hz5_largefront_mark_orphan(span);
      continue;
    }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TAKE_FIRST
    if (!taken &&
        hz5_largefront_state_cas(span,
                                 (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                 (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
      span->next = NULL;
      taken = span;
      continue;
    }
#endif
    if (hz5_largefront_state_cas(span,
                                 (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                 (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
      hz5_largefront_local_push(tls, class_index, span);
    }
    ++drained;
  }
  if (drained_out) {
    *drained_out = drained;
  }
  return taken;
}

static Hz5LargeSpan* hz5_largefront_drain_remote_class(Hz5LargeTls* tls,
                                                       uint32_t class_index) {
  return hz5_largefront_drain_remote_class_budget(tls,
                                                  class_index,
                                                  0u,
                                                  NULL);
}

void hz5_largefront_ownerhub_drain_some(uint32_t budget) {
  Hz5LargeTls* tls = hz5_largefront_tls();
  uint32_t remaining = budget;
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_CLASS_COUNT; ++i) {
    if (remaining == 0u) {
      break;
    }
    uint32_t drained = 0;
    (void)hz5_largefront_drain_remote_class_budget(tls,
                                                   i,
                                                   remaining,
                                                   &drained);
    remaining -= drained > remaining ? remaining : drained;
  }
}
#endif

static Hz5LargeSpan* hz5_largefront_new_span(Hz5LargeTls* tls,
                                             uint32_t class_index) {
  uint32_t class_bytes = hz5_largefront_class_bytes(class_index);
  void* raw = hz5_largefront_source_alloc_raw(class_index);
  if (!raw) {
    return NULL;
  }

  Hz5LargeSpan* span = (Hz5LargeSpan*)raw;
  span->magic = 0;
  span->raw = raw;
  span->base = (void*)((uintptr_t)raw + HZ5_LARGEFRONT_PAGE_SIZE);
  span->class_bytes = class_bytes;
  span->page_count = (uint16_t)(class_bytes / HZ5_LARGEFRONT_PAGE_SIZE);
  span->class_index = (uint16_t)class_index;
  span->owner = tls->owner;
  atomic_store_explicit(&span->state,
                        (unsigned char)HZ5_LARGESPAN_ACTIVE,
                        memory_order_release);
  span->generation = 1;
  span->flags = 0;
  span->next = NULL;

  if (!hz5_largefront_map_insert(span)) {
    hz5_largefront_source_free_raw(class_index, raw);
    return NULL;
  }
  span->magic = HZ5_LARGEFRONT_MAGIC;
  return span;
}

void* hz5_largefront_alloc(size_t size, size_t align) {
  if (align > 16u) {
    return NULL;
  }
  int class_index = hz5_largefront_class_index(size);
  if (class_index < 0) {
    return NULL;
  }

  Hz5LargeTls* tls = hz5_largefront_tls();
  uint32_t ci = (uint32_t)class_index;
  Hz5LargeSpan* span = hz5_largefront_local_pop(tls, ci);
  if (span) {
    if (hz5_largefront_activate_local_for_owner(tls, span)) {
      return span->base;
    }
    return NULL;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
  hz5_ownerhub_note_alloc_miss(tls->owner, HZ5_OWNERHUB_FRONT_LARGE, ci);
  span = hz5_largefront_drain_remote_class(tls, ci);
  if (span) {
    return span->base;
  }
  hz5_ownerhub_drain_cross_fronts(tls->owner, HZ5_OWNERHUB_FRONT_LARGE);
  span = hz5_largefront_local_pop(tls, ci);
  if (span) {
    if (hz5_largefront_activate_local_for_owner(tls, span)) {
      return span->base;
    }
    return NULL;
  }
#endif

  while ((span = hz5_largefront_global_pop(ci)) != NULL) {
    if (hz5_largefront_activate_global_for_owner(tls, span)) {
      return span->base;
    }
  }

  span = hz5_largefront_new_span(tls, ci);
  return span ? span->base : NULL;
}

Hz5LargeFrontFreeResult hz5_largefront_free(void* ptr) {
  Hz5LargeSpan* span = hz5_largefront_span_for_ptr(ptr);
  if (!span) {
    return HZ5_LARGEFRONT_FREE_NOT_OWNED;
  }
  if (ptr != span->base) {
    return HZ5_LARGEFRONT_FREE_INVALID;
  }

  Hz5LargeTls* tls = hz5_largefront_tls();
  if (hz5_owner_equal(span->owner, tls->owner)) {
    if (!hz5_largefront_owner_local_state_transition(
            span,
            (unsigned char)HZ5_LARGESPAN_ACTIVE,
            (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
      return HZ5_LARGEFRONT_FREE_INVALID;
    }
    hz5_largefront_local_push(tls, span->class_index, span);
  } else {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
    if (hz5_owner_is_alive(span->owner)) {
      if (!hz5_largefront_state_cas(
              span,
              (unsigned char)HZ5_LARGESPAN_ACTIVE,
              (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING)) {
        return HZ5_LARGEFRONT_FREE_INVALID;
      }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_BATCH
      hz5_largefront_remote_batch_push(tls, span);
#else
      if (!hz5_largefront_remote_publish_one(span->owner,
                                             span->class_index,
                                             span)) {
        hz5_largefront_mark_orphan(span);
      }
#endif
      return HZ5_LARGEFRONT_FREE_OK;
    }
#endif
    if (!hz5_largefront_state_cas(span,
                                  (unsigned char)HZ5_LARGESPAN_ACTIVE,
                                  (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
      return HZ5_LARGEFRONT_FREE_INVALID;
    }
    hz5_largefront_global_push(span->class_index, span);
  }
  return HZ5_LARGEFRONT_FREE_OK;
}

int hz5_largefront_can_handle(size_t size, size_t align) {
  return align <= 16u && hz5_largefront_class_index(size) >= 0;
}

int hz5_largefront_owns(void* ptr) {
  return hz5_largefront_span_for_ptr(ptr) != NULL;
}

size_t hz5_largefront_usable_size(void* ptr) {
  Hz5LargeSpan* span = hz5_largefront_span_for_ptr(ptr);
  if (!span || ptr != span->base) {
    return 0;
  }
  return span->class_bytes;
}

#else

void hz5_largefront_ownerhub_drain_some(uint32_t budget) {
  (void)budget;
}

void* hz5_largefront_alloc(size_t size, size_t align) {
  (void)size;
  (void)align;
  return NULL;
}

Hz5LargeFrontFreeResult hz5_largefront_free(void* ptr) {
  (void)ptr;
  return HZ5_LARGEFRONT_FREE_NOT_OWNED;
}

int hz5_largefront_can_handle(size_t size, size_t align) {
  (void)size;
  (void)align;
  return 0;
}

int hz5_largefront_owns(void* ptr) {
  (void)ptr;
  return 0;
}

size_t hz5_largefront_usable_size(void* ptr) {
  (void)ptr;
  return 0;
}

#endif
