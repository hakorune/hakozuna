#include "hz5_midpagefront.h"

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

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M2
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M2 0
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M2

#define HZ5_MIDPAGEFRONT_MAGIC UINT64_C(0x485A354D50414732)
#define HZ5_MIDPAGEFRONT_SLAB_BYTES ((size_t)65536)
#define HZ5_MIDPAGEFRONT_CLASS_COUNT 5u
#define HZ5_MIDPAGEFRONT_MAP_BITS 18u
#define HZ5_MIDPAGEFRONT_MAP_CAP \
  ((size_t)1u << HZ5_MIDPAGEFRONT_MAP_BITS)

#ifndef HZ5_MIDPAGEFRONT_REMOTE_BATCH_CAP
#define HZ5_MIDPAGEFRONT_REMOTE_BATCH_CAP 16u
#endif

typedef struct Hz5MidPage {
  uint64_t magic;
  void* slab_base;
  uint32_t class_size;
  uint16_t class_index;
  uint16_t slot_count;
  Hz5OwnerToken owner;
  _Atomic uint64_t active_bits;
  struct Hz5MidPage* next_owned;
} Hz5MidPage;

typedef struct Hz5MidPageNode {
  struct Hz5MidPageNode* next;
  Hz5MidPage* page;
} Hz5MidPageNode;

typedef struct Hz5MidPageMapEntry {
  _Atomic uintptr_t slab_base;
  _Atomic(Hz5MidPage*) page;
} Hz5MidPageMapEntry;

typedef struct Hz5MidPageTls {
  Hz5OwnerToken owner;
  Hz5MidPageNode* free_head[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  Hz5MidPage* owned_pages[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  Hz5OwnerToken remote_batch_owner;
  uint32_t remote_batch_class;
  uint32_t remote_batch_count;
  Hz5MidPageNode* remote_batch_head;
  Hz5MidPageNode* remote_batch_tail;
} Hz5MidPageTls;

static const uint32_t g_hz5_midpagefront_classes
    [HZ5_MIDPAGEFRONT_CLASS_COUNT] = {3072u, 4096u, 8192u, 16384u, 32768u};

static const Hz5OwnerToken k_hz5_midpagefront_no_owner = {0, 0};

static Hz5MidPageMapEntry
    g_hz5_midpagefront_map[HZ5_MIDPAGEFRONT_MAP_CAP];
static _Atomic(void*) g_hz5_midpagefront_owner_inbox[UINT16_MAX + 1u]
                                                  [HZ5_MIDPAGEFRONT_CLASS_COUNT];
static pthread_mutex_t g_hz5_midpagefront_map_lock =
    PTHREAD_MUTEX_INITIALIZER;
static _Thread_local Hz5MidPageTls g_hz5_midpagefront_tls;

static Hz5MidPageTls* hz5_midpagefront_tls(void) {
  Hz5MidPageTls* tls = &g_hz5_midpagefront_tls;
  if (tls->owner.slot == 0) {
    tls->owner = hz5_owner_current();
  }
  return tls;
}

static int hz5_midpagefront_class_valid(uint32_t class_index) {
  return class_index < HZ5_MIDPAGEFRONT_CLASS_COUNT;
}

static int hz5_midpagefront_class_index(size_t size) {
  if (size <= 2048u || size > 32768u) {
    return -1;
  }
  for (uint32_t i = 0; i < HZ5_MIDPAGEFRONT_CLASS_COUNT; ++i) {
    if (size <= g_hz5_midpagefront_classes[i]) {
      return (int)i;
    }
  }
  return -1;
}

static size_t hz5_midpagefront_hash(uintptr_t slab_base) {
  uintptr_t x = slab_base >> 16;
  x ^= x >> HZ5_MIDPAGEFRONT_MAP_BITS;
  x ^= x >> (HZ5_MIDPAGEFRONT_MAP_BITS * 2u);
  return (size_t)x & (HZ5_MIDPAGEFRONT_MAP_CAP - 1u);
}

static Hz5MidPage* hz5_midpagefront_lookup_slab(uintptr_t slab_base) {
  size_t idx = hz5_midpagefront_hash(slab_base);
  for (size_t probe = 0; probe < HZ5_MIDPAGEFRONT_MAP_CAP; ++probe) {
    Hz5MidPageMapEntry* entry =
        &g_hz5_midpagefront_map[(idx + probe) &
                                (HZ5_MIDPAGEFRONT_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->slab_base, memory_order_acquire);
    if (current == slab_base) {
      return atomic_load_explicit(&entry->page, memory_order_acquire);
    }
    if (current == 0) {
      return NULL;
    }
  }
  return NULL;
}

static int hz5_midpagefront_map_insert(uintptr_t slab_base,
                                       Hz5MidPage* page) {
  size_t idx = hz5_midpagefront_hash(slab_base);
  pthread_mutex_lock(&g_hz5_midpagefront_map_lock);
  for (size_t probe = 0; probe < HZ5_MIDPAGEFRONT_MAP_CAP; ++probe) {
    Hz5MidPageMapEntry* entry =
        &g_hz5_midpagefront_map[(idx + probe) &
                                (HZ5_MIDPAGEFRONT_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->slab_base, memory_order_acquire);
    if (current == slab_base) {
      atomic_store_explicit(&entry->page, page, memory_order_release);
      pthread_mutex_unlock(&g_hz5_midpagefront_map_lock);
      return 1;
    }
    if (current == 0) {
      atomic_store_explicit(&entry->page, page, memory_order_relaxed);
      atomic_store_explicit(&entry->slab_base, slab_base,
                            memory_order_release);
      pthread_mutex_unlock(&g_hz5_midpagefront_map_lock);
      return 1;
    }
  }
  pthread_mutex_unlock(&g_hz5_midpagefront_map_lock);
  return 0;
}

static Hz5MidPage* hz5_midpagefront_page_for_ptr(void* ptr) {
  if (!ptr) {
    return NULL;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t slab_base =
      p & ~(uintptr_t)(HZ5_MIDPAGEFRONT_SLAB_BYTES - 1u);
  Hz5MidPage* page = hz5_midpagefront_lookup_slab(slab_base);
  if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
      (uintptr_t)page->slab_base != slab_base) {
    return NULL;
  }
  return page;
}

static int hz5_midpagefront_slot_index(Hz5MidPage* page,
                                       void* ptr,
                                       uint32_t* slot_out) {
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)page->slab_base;
  if (p < base || p >= base + HZ5_MIDPAGEFRONT_SLAB_BYTES) {
    return 0;
  }
  size_t offset = (size_t)(p - base);
  if (page->class_size == 0 || offset % page->class_size != 0) {
    return 0;
  }
  uint32_t slot = (uint32_t)(offset / page->class_size);
  if (slot >= page->slot_count || slot >= 64u) {
    return 0;
  }
  *slot_out = slot;
  return 1;
}

static uint64_t hz5_midpagefront_slot_mask(uint32_t slot) {
  return UINT64_C(1) << slot;
}

static void hz5_midpagefront_local_push(Hz5MidPageTls* tls,
                                        uint32_t class_index,
                                        void* ptr,
                                        Hz5MidPage* page) {
  Hz5MidPageNode* node = (Hz5MidPageNode*)ptr;
  node->page = page;
  node->next = tls->free_head[class_index];
  tls->free_head[class_index] = node;
}

static void* hz5_midpagefront_local_pop(Hz5MidPageTls* tls,
                                        uint32_t class_index,
                                        Hz5MidPage** page_out) {
  Hz5MidPageNode* node = tls->free_head[class_index];
  if (!node) {
    if (page_out) {
      *page_out = NULL;
    }
    return NULL;
  }
  tls->free_head[class_index] = node->next;
  if (page_out) {
    *page_out = node->page;
  }
  return node;
}

static int hz5_midpagefront_can_publish_to_owner(Hz5OwnerToken owner,
                                                 uint32_t class_index,
                                                 const Hz5MidPageNode* head,
                                                 const Hz5MidPageNode* tail) {
  return owner.slot != 0 && hz5_midpagefront_class_valid(class_index) &&
         head && tail && hz5_owner_is_alive(owner);
}

static void hz5_midpagefront_remote_publish_list(Hz5OwnerToken owner,
                                                 uint32_t class_index,
                                                 Hz5MidPageNode* head,
                                                 Hz5MidPageNode* tail) {
  if (!hz5_midpagefront_can_publish_to_owner(
          owner, class_index, head, tail)) {
    return;
  }
  void* old_head = NULL;
  _Atomic(void*)* inbox =
      &g_hz5_midpagefront_owner_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    tail->next = (Hz5MidPageNode*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, head, memory_order_release, memory_order_acquire));
}

static void hz5_midpagefront_remote_batch_flush(Hz5MidPageTls* tls) {
  if (!tls || tls->remote_batch_count == 0u) {
    return;
  }
  hz5_midpagefront_remote_publish_list(tls->remote_batch_owner,
                                       tls->remote_batch_class,
                                       tls->remote_batch_head,
                                       tls->remote_batch_tail);
  tls->remote_batch_owner = k_hz5_midpagefront_no_owner;
  tls->remote_batch_class = 0;
  tls->remote_batch_count = 0;
  tls->remote_batch_head = NULL;
  tls->remote_batch_tail = NULL;
}

static void hz5_midpagefront_remote_batch_push(Hz5MidPageTls* tls,
                                               Hz5MidPage* page,
                                               void* ptr) {
  if (!tls || !page || !ptr || !hz5_owner_is_alive(page->owner)) {
    return;
  }
  uint32_t class_index = page->class_index;
  if (tls->remote_batch_count != 0u &&
      (!hz5_owner_equal(tls->remote_batch_owner, page->owner) ||
       tls->remote_batch_class != class_index)) {
    hz5_midpagefront_remote_batch_flush(tls);
  }

  Hz5MidPageNode* node = (Hz5MidPageNode*)ptr;
  node->page = page;
  node->next = NULL;
  if (tls->remote_batch_count == 0u) {
    tls->remote_batch_owner = page->owner;
    tls->remote_batch_class = class_index;
    tls->remote_batch_head = node;
  } else {
    tls->remote_batch_tail->next = node;
  }
  tls->remote_batch_tail = node;
  ++tls->remote_batch_count;

  if (tls->remote_batch_count >= HZ5_MIDPAGEFRONT_REMOTE_BATCH_CAP) {
    hz5_midpagefront_remote_batch_flush(tls);
  }
}

static uint32_t hz5_midpagefront_drain_remote_class(Hz5MidPageTls* tls,
                                                    uint32_t class_index) {
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return 0;
  }
  void* head = atomic_exchange_explicit(
      &g_hz5_midpagefront_owner_inbox[tls->owner.slot][class_index],
      NULL,
      memory_order_acq_rel);
  uint32_t drained = 0;
  while (head) {
    Hz5MidPageNode* node = (Hz5MidPageNode*)head;
    head = node->next;
    if (node->page && hz5_owner_equal(node->page->owner, tls->owner)) {
      hz5_midpagefront_local_push(tls, class_index, node, node->page);
      ++drained;
    }
  }
  return drained;
}

static int hz5_midpagefront_mark_active_local(Hz5MidPage* page,
                                              uint32_t slot) {
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
  uint64_t old = atomic_load_explicit(&page->active_bits,
                                      memory_order_acquire);
  for (;;) {
    if ((old & mask) != 0) {
      return 0;
    }
    uint64_t next = old | mask;
    if (atomic_compare_exchange_weak_explicit(&page->active_bits,
                                              &old,
                                              next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      return 1;
    }
  }
}

static int hz5_midpagefront_mark_free_local(Hz5MidPage* page,
                                            uint32_t slot) {
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
  uint64_t old = atomic_load_explicit(&page->active_bits,
                                      memory_order_acquire);
  for (;;) {
    if ((old & mask) == 0) {
      return 0;
    }
    uint64_t next = old & ~mask;
    if (atomic_compare_exchange_weak_explicit(&page->active_bits,
                                              &old,
                                              next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      return 1;
    }
  }
}

static int hz5_midpagefront_mark_free_remote(Hz5MidPage* page,
                                             uint32_t slot) {
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
  uint64_t old = atomic_load_explicit(&page->active_bits,
                                      memory_order_acquire);
  for (;;) {
    if ((old & mask) == 0) {
      return 0;
    }
    uint64_t next = old & ~mask;
    if (atomic_compare_exchange_weak_explicit(&page->active_bits,
                                              &old,
                                              next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      return 1;
    }
  }
}

static Hz5MidPage* hz5_midpagefront_new_page(Hz5MidPageTls* tls,
                                             uint32_t class_index) {
  void* slab = _aligned_malloc(HZ5_MIDPAGEFRONT_SLAB_BYTES,
                               HZ5_MIDPAGEFRONT_SLAB_BYTES);
  if (!slab) {
    return NULL;
  }
  Hz5MidPage* page =
      (Hz5MidPage*)_aligned_malloc(sizeof(Hz5MidPage), 64u);
  if (!page) {
    _aligned_free(slab);
    return NULL;
  }

  uint32_t class_size = g_hz5_midpagefront_classes[class_index];
  uint16_t slot_count =
      (uint16_t)(HZ5_MIDPAGEFRONT_SLAB_BYTES / (size_t)class_size);
  page->magic = HZ5_MIDPAGEFRONT_MAGIC;
  page->slab_base = slab;
  page->class_size = class_size;
  page->class_index = (uint16_t)class_index;
  page->slot_count = slot_count;
  page->owner = tls->owner;
  atomic_store_explicit(&page->active_bits, 0, memory_order_relaxed);
  page->next_owned = tls->owned_pages[class_index];

  if (!hz5_midpagefront_map_insert((uintptr_t)slab, page)) {
    _aligned_free(page);
    _aligned_free(slab);
    return NULL;
  }

  tls->owned_pages[class_index] = page;
  for (uint32_t slot = slot_count; slot > 0; --slot) {
    void* ptr = (void*)((uintptr_t)slab +
                        (uintptr_t)(slot - 1u) * class_size);
    hz5_midpagefront_local_push(tls, class_index, ptr, page);
  }
  return page;
}

void* hz5_midpagefront_alloc(size_t size, size_t align) {
  if (align > 16u) {
    return NULL;
  }
  int class_index = hz5_midpagefront_class_index(size);
  if (class_index < 0) {
    return NULL;
  }
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  uint32_t ci = (uint32_t)class_index;
  Hz5MidPage* page = NULL;
  void* ptr = hz5_midpagefront_local_pop(tls, ci, &page);
  if (!ptr) {
    (void)hz5_midpagefront_drain_remote_class(tls, ci);
    ptr = hz5_midpagefront_local_pop(tls, ci, &page);
  }
  if (!ptr) {
    if (!hz5_midpagefront_new_page(tls, ci)) {
      return NULL;
    }
    ptr = hz5_midpagefront_local_pop(tls, ci, &page);
  }
  if (!ptr) {
    return NULL;
  }
  if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC) {
    page = hz5_midpagefront_page_for_ptr(ptr);
  }
  uint32_t slot = 0;
  if (!page || !hz5_midpagefront_slot_index(page, ptr, &slot) ||
      !hz5_midpagefront_mark_active_local(page, slot)) {
    return NULL;
  }
  return ptr;
}

Hz5MidPageFrontFreeResult hz5_midpagefront_free(void* ptr) {
  Hz5MidPage* page = hz5_midpagefront_page_for_ptr(ptr);
  if (!page) {
    return HZ5_MIDPAGEFRONT_FREE_NOT_OWNED;
  }
  uint32_t slot = 0;
  if (!hz5_midpagefront_slot_index(page, ptr, &slot)) {
    return HZ5_MIDPAGEFRONT_FREE_INVALID;
  }

  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  if (hz5_owner_equal(page->owner, tls->owner)) {
    if (!hz5_midpagefront_mark_free_local(page, slot)) {
      return HZ5_MIDPAGEFRONT_FREE_INVALID;
    }
    hz5_midpagefront_local_push(tls, page->class_index, ptr, page);
  } else {
    if (!hz5_midpagefront_mark_free_remote(page, slot)) {
      return HZ5_MIDPAGEFRONT_FREE_INVALID;
    }
    hz5_midpagefront_remote_batch_push(tls, page, ptr);
  }
  return HZ5_MIDPAGEFRONT_FREE_OK;
}

int hz5_midpagefront_can_handle(size_t size, size_t align) {
  return align <= 16u && hz5_midpagefront_class_index(size) >= 0;
}

int hz5_midpagefront_owns(void* ptr) {
  return hz5_midpagefront_page_for_ptr(ptr) != NULL;
}

size_t hz5_midpagefront_usable_size(void* ptr) {
  Hz5MidPage* page = hz5_midpagefront_page_for_ptr(ptr);
  if (!page) {
    return 0;
  }
  uint32_t slot = 0;
  if (!hz5_midpagefront_slot_index(page, ptr, &slot)) {
    return 0;
  }
  return page->class_size;
}

#else

void* hz5_midpagefront_alloc(size_t size, size_t align) {
  (void)size;
  (void)align;
  return NULL;
}

Hz5MidPageFrontFreeResult hz5_midpagefront_free(void* ptr) {
  (void)ptr;
  return HZ5_MIDPAGEFRONT_FREE_NOT_OWNED;
}

int hz5_midpagefront_can_handle(size_t size, size_t align) {
  (void)size;
  (void)align;
  return 0;
}

int hz5_midpagefront_owns(void* ptr) {
  (void)ptr;
  return 0;
}

size_t hz5_midpagefront_usable_size(void* ptr) {
  (void)ptr;
  return 0;
}

#endif
