#include "hz5_midfront.h"

#include "hz5_config.h"
#include "hz5_internal.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>

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

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_MIDFRONT_M1

#define HZ5_MIDFRONT_MAGIC UINT64_C(0x485A354D49444D31)
#define HZ5_MIDFRONT_PAGE_SIZE ((size_t)4096)
#define HZ5_MIDFRONT_CLASS_COUNT 5u
#define HZ5_MIDFRONT_MAP_BITS 18u
#define HZ5_MIDFRONT_MAP_CAP ((size_t)1u << HZ5_MIDFRONT_MAP_BITS)

#ifndef HZ5_MIDFRONT_REMOTE_BATCH_CAP
#define HZ5_MIDFRONT_REMOTE_BATCH_CAP 16u
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

typedef struct Hz5MidTls {
  Hz5OwnerToken owner;
  Hz5MidSpan* free_head[HZ5_MIDFRONT_CLASS_COUNT];
  Hz5OwnerToken remote_batch_owner;
  uint32_t remote_batch_class;
  uint32_t remote_batch_count;
  Hz5MidSpan* remote_batch_head;
  Hz5MidSpan* remote_batch_tail;
} Hz5MidTls;

static const uint32_t g_hz5_midfront_classes[HZ5_MIDFRONT_CLASS_COUNT] = {
    4096u, 8192u, 16384u, 32768u, 65536u};

static const Hz5OwnerToken k_hz5_midfront_no_owner = {0, 0};

static Hz5MidMapEntry g_hz5_midfront_map[HZ5_MIDFRONT_MAP_CAP];
static _Atomic(void*) g_hz5_midfront_owner_inbox[UINT16_MAX + 1u]
                                             [HZ5_MIDFRONT_CLASS_COUNT];
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

static size_t hz5_midfront_hash(uintptr_t page_base) {
  uintptr_t x = page_base >> 12;
  x ^= x >> HZ5_MIDFRONT_MAP_BITS;
  x ^= x >> (HZ5_MIDFRONT_MAP_BITS * 2u);
  return (size_t)x & (HZ5_MIDFRONT_MAP_CAP - 1u);
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
}

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

static void hz5_midfront_drain_remote_class(Hz5MidTls* tls,
                                            uint32_t class_index) {
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_MIDFRONT_CLASS_COUNT) {
    return;
  }

  void* head = atomic_exchange_explicit(
      &g_hz5_midfront_owner_inbox[tls->owner.slot][class_index], NULL,
      memory_order_acq_rel);
  while (head) {
    Hz5MidSpan* span = (Hz5MidSpan*)head;
    head = span->next;
    if (!hz5_owner_equal(span->owner, tls->owner)) {
      hz5_midfront_mark_orphan(span);
      continue;
    }
    if (hz5_midfront_state_cas(span,
                               (unsigned char)HZ5_MIDSPAN_REMOTE_PENDING,
                               (unsigned char)HZ5_MIDSPAN_LOCAL_FREE)) {
      hz5_midfront_local_push(tls, class_index, span);
    }
  }
}

static void hz5_midfront_drain_remote_on_miss(Hz5MidTls* tls,
                                              uint32_t class_index) {
#if BENCHLAB_HZ5_LINUX_MIDFRONT_DRAIN_ALL_ON_MISS
  for (uint32_t i = 0; i < HZ5_MIDFRONT_CLASS_COUNT; ++i) {
    hz5_midfront_drain_remote_class(tls, i);
  }
#else
  hz5_midfront_drain_remote_class(tls, class_index);
#endif
}

static Hz5MidSpan* hz5_midfront_new_span(Hz5MidTls* tls,
                                         uint32_t class_index) {
  uint32_t class_bytes = hz5_midfront_class_bytes(class_index);
  size_t raw_bytes = HZ5_MIDFRONT_PAGE_SIZE + (size_t)class_bytes;
  void* raw = _aligned_malloc(raw_bytes, HZ5_MIDFRONT_PAGE_SIZE);
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
    _aligned_free(raw);
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
    hz5_midfront_drain_remote_on_miss(tls, ci);
    span = hz5_midfront_local_pop(tls, ci);
  }
  if (span) {
    if (!hz5_midfront_owner_local_state_transition(
            span,
            (unsigned char)HZ5_MIDSPAN_LOCAL_FREE,
            (unsigned char)HZ5_MIDSPAN_ACTIVE)) {
      return NULL;
    }
    return span->base;
  }

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
    if (!hz5_midfront_state_cas(span,
                                (unsigned char)HZ5_MIDSPAN_ACTIVE,
                                (unsigned char)HZ5_MIDSPAN_REMOTE_PENDING)) {
      return HZ5_MIDFRONT_FREE_INVALID;
    }
    hz5_midfront_remote_batch_push(tls, span);
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
