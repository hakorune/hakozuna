#include "hz5_smallfront.h"

#include "hz5_config.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>

#if !defined(_WIN32)
void* _aligned_malloc(size_t size, size_t alignment);
void _aligned_free(void* ptr);
#endif

#ifndef BENCHLAB_HZ5_LINUX_SMALLFRONT_S1
#define BENCHLAB_HZ5_LINUX_SMALLFRONT_S1 0
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_SMALLFRONT_S1

#define HZ5_SMALLFRONT_MAGIC UINT64_C(0x485A35534D4C5331)
#define HZ5_SMALLFRONT_PAGE_SIZE ((size_t)4096)
#define HZ5_SMALLFRONT_RAW_BYTES (HZ5_SMALLFRONT_PAGE_SIZE * (size_t)2)
#define HZ5_SMALLFRONT_CLASS_COUNT 14u
#define HZ5_SMALLFRONT_PAGE_MAP_BITS 18u
#define HZ5_SMALLFRONT_PAGE_MAP_CAP \
  ((size_t)1u << HZ5_SMALLFRONT_PAGE_MAP_BITS)

typedef struct Hz5SmallFrontPage {
  uint64_t magic;
  void* raw;
  void* page_base;
  uint16_t class_index;
  uint16_t class_size;
  uint16_t slot_count;
  uint16_t reserved0;
  uintptr_t owner_token;
  _Atomic unsigned char slot_state[256];
  _Atomic(void*) remote_head;
  struct Hz5SmallFrontPage* owner_next;
} Hz5SmallFrontPage;

typedef struct Hz5SmallFrontNode {
  struct Hz5SmallFrontNode* next;
  Hz5SmallFrontPage* page;
} Hz5SmallFrontNode;

typedef struct Hz5SmallFrontPageMapEntry {
  _Atomic uintptr_t page_base;
  Hz5SmallFrontPage* page;
} Hz5SmallFrontPageMapEntry;

typedef struct Hz5SmallFrontTls {
  uintptr_t owner_token;
  void* free_head[HZ5_SMALLFRONT_CLASS_COUNT];
  _Atomic(void*) remote_head[HZ5_SMALLFRONT_CLASS_COUNT];
  Hz5SmallFrontPage* owned_pages[HZ5_SMALLFRONT_CLASS_COUNT];
} Hz5SmallFrontTls;

static const uint16_t g_hz5_smallfront_classes[HZ5_SMALLFRONT_CLASS_COUNT] = {
    16, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048};

static Hz5SmallFrontPageMapEntry
    g_hz5_smallfront_page_map[HZ5_SMALLFRONT_PAGE_MAP_CAP];
static pthread_mutex_t g_hz5_smallfront_page_map_lock =
    PTHREAD_MUTEX_INITIALIZER;
static _Thread_local Hz5SmallFrontTls g_hz5_smallfront_tls;

static Hz5SmallFrontTls* hz5_smallfront_tls(void) {
  Hz5SmallFrontTls* tls = &g_hz5_smallfront_tls;
  if (!tls->owner_token) {
    tls->owner_token = (uintptr_t)tls;
  }
  return tls;
}

static int hz5_smallfront_class_index(size_t size) {
  size_t request = size == 0 ? 1u : size;
  for (uint32_t i = 0; i < HZ5_SMALLFRONT_CLASS_COUNT; ++i) {
    if (request <= g_hz5_smallfront_classes[i]) {
      return (int)i;
    }
  }
  return -1;
}

static size_t hz5_smallfront_page_hash(uintptr_t page_base) {
  uintptr_t x = page_base >> 12;
  x ^= x >> 33;
  x *= UINT64_C(0xff51afd7ed558ccd);
  x ^= x >> 33;
  return (size_t)x & (HZ5_SMALLFRONT_PAGE_MAP_CAP - 1u);
}

static Hz5SmallFrontPage* hz5_smallfront_page_lookup_base(
    uintptr_t page_base) {
  size_t idx = hz5_smallfront_page_hash(page_base);
  for (size_t probe = 0; probe < HZ5_SMALLFRONT_PAGE_MAP_CAP; ++probe) {
    Hz5SmallFrontPageMapEntry* entry =
        &g_hz5_smallfront_page_map[(idx + probe) &
                                   (HZ5_SMALLFRONT_PAGE_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->page_base, memory_order_acquire);
    if (current == page_base) {
      return entry->page;
    }
    if (current == 0) {
      return NULL;
    }
  }
  return NULL;
}

static int hz5_smallfront_page_map_insert(uintptr_t page_base,
                                          Hz5SmallFrontPage* page) {
  size_t idx = hz5_smallfront_page_hash(page_base);
  pthread_mutex_lock(&g_hz5_smallfront_page_map_lock);
  for (size_t probe = 0; probe < HZ5_SMALLFRONT_PAGE_MAP_CAP; ++probe) {
    Hz5SmallFrontPageMapEntry* entry =
        &g_hz5_smallfront_page_map[(idx + probe) &
                                   (HZ5_SMALLFRONT_PAGE_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->page_base, memory_order_acquire);
    if (current == page_base) {
      entry->page = page;
      pthread_mutex_unlock(&g_hz5_smallfront_page_map_lock);
      return 1;
    }
    if (current == 0) {
      entry->page = page;
      atomic_store_explicit(&entry->page_base, page_base,
                            memory_order_release);
      pthread_mutex_unlock(&g_hz5_smallfront_page_map_lock);
      return 1;
    }
  }
  pthread_mutex_unlock(&g_hz5_smallfront_page_map_lock);
  return 0;
}

static Hz5SmallFrontPage* hz5_smallfront_page_for_ptr(void* ptr) {
  if (!ptr) {
    return NULL;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t page_base = p & ~(uintptr_t)(HZ5_SMALLFRONT_PAGE_SIZE - 1u);
  Hz5SmallFrontPage* page = hz5_smallfront_page_lookup_base(page_base);
  if (!page || page->magic != HZ5_SMALLFRONT_MAGIC ||
      (uintptr_t)page->page_base != page_base) {
    return NULL;
  }
  return page;
}

static int hz5_smallfront_slot_index(Hz5SmallFrontPage* page,
                                     void* ptr,
                                     uint32_t* slot_out) {
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)page->page_base;
  if (p < base || p >= base + HZ5_SMALLFRONT_PAGE_SIZE) {
    return 0;
  }
  size_t offset = (size_t)(p - base);
  if (page->class_size == 0 || offset % page->class_size != 0) {
    return 0;
  }
  uint32_t slot = (uint32_t)(offset / page->class_size);
  if (slot >= page->slot_count) {
    return 0;
  }
  *slot_out = slot;
  return 1;
}

static void hz5_smallfront_local_push(Hz5SmallFrontTls* tls,
                                      uint32_t class_index,
                                      void* ptr,
                                      Hz5SmallFrontPage* page) {
  Hz5SmallFrontNode* node = (Hz5SmallFrontNode*)ptr;
  node->page = page;
  node->next = (Hz5SmallFrontNode*)tls->free_head[class_index];
  tls->free_head[class_index] = node;
}

static void* hz5_smallfront_local_pop(Hz5SmallFrontTls* tls,
                                      uint32_t class_index,
                                      Hz5SmallFrontPage** page_out) {
  Hz5SmallFrontNode* node =
      (Hz5SmallFrontNode*)tls->free_head[class_index];
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

static void hz5_smallfront_remote_push(Hz5SmallFrontPage* page, void* ptr) {
  Hz5SmallFrontTls* owner_tls = (Hz5SmallFrontTls*)page->owner_token;
  Hz5SmallFrontNode* node = (Hz5SmallFrontNode*)ptr;
  node->page = page;
  void* old_head = NULL;
  do {
    old_head = atomic_load_explicit(
        &owner_tls->remote_head[page->class_index], memory_order_acquire);
    node->next = (Hz5SmallFrontNode*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      &owner_tls->remote_head[page->class_index], &old_head, node,
      memory_order_release, memory_order_acquire));
}

static void hz5_smallfront_drain_remote_class(Hz5SmallFrontTls* tls,
                                              uint32_t class_index) {
  void* head = atomic_exchange_explicit(&tls->remote_head[class_index], NULL,
                                        memory_order_acq_rel);
  while (head) {
    Hz5SmallFrontNode* node = (Hz5SmallFrontNode*)head;
    head = node->next;
    hz5_smallfront_local_push(tls, class_index, node, node->page);
  }
}

static int hz5_smallfront_mark_active_local(Hz5SmallFrontPage* page,
                                            uint32_t slot) {
  unsigned char old =
      atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
  if (old != 0) {
    return 0;
  }
  atomic_store_explicit(&page->slot_state[slot], 1u, memory_order_release);
  return 1;
}

static int hz5_smallfront_mark_free_local(Hz5SmallFrontPage* page,
                                          uint32_t slot) {
  unsigned char old =
      atomic_load_explicit(&page->slot_state[slot], memory_order_acquire);
  if (old != 1u) {
    return 0;
  }
  atomic_store_explicit(&page->slot_state[slot], 0u, memory_order_release);
  return 1;
}

static int hz5_smallfront_mark_free_remote(Hz5SmallFrontPage* page,
                                           uint32_t slot) {
  unsigned char expected = 1u;
  return atomic_compare_exchange_strong_explicit(
      &page->slot_state[slot], &expected, 0u, memory_order_acq_rel,
      memory_order_acquire);
}

static Hz5SmallFrontPage* hz5_smallfront_new_page(Hz5SmallFrontTls* tls,
                                                  uint32_t class_index) {
  void* raw = _aligned_malloc(HZ5_SMALLFRONT_RAW_BYTES,
                              HZ5_SMALLFRONT_PAGE_SIZE);
  if (!raw) {
    return NULL;
  }

  Hz5SmallFrontPage* page = (Hz5SmallFrontPage*)raw;
  uintptr_t page_base = (uintptr_t)raw + HZ5_SMALLFRONT_PAGE_SIZE;
  uint16_t class_size = g_hz5_smallfront_classes[class_index];
  uint16_t slot_count =
      (uint16_t)(HZ5_SMALLFRONT_PAGE_SIZE / (size_t)class_size);

  page->magic = HZ5_SMALLFRONT_MAGIC;
  page->raw = raw;
  page->page_base = (void*)page_base;
  page->class_index = (uint16_t)class_index;
  page->class_size = class_size;
  page->slot_count = slot_count;
  page->reserved0 = 0;
  page->owner_token = tls->owner_token;
  for (uint32_t i = 0; i < 256u; ++i) {
    atomic_store_explicit(&page->slot_state[i], 0, memory_order_relaxed);
  }
  atomic_store_explicit(&page->remote_head, NULL, memory_order_relaxed);
  page->owner_next = tls->owned_pages[class_index];

  if (!hz5_smallfront_page_map_insert(page_base, page)) {
    _aligned_free(raw);
    return NULL;
  }

  tls->owned_pages[class_index] = page;
  for (uint32_t slot = slot_count; slot > 0; --slot) {
    void* ptr = (void*)(page_base + (uintptr_t)(slot - 1u) * class_size);
    hz5_smallfront_local_push(tls, class_index, ptr, page);
  }
  return page;
}

void* hz5_smallfront_alloc(size_t size, size_t align) {
  int class_index = hz5_smallfront_class_index(size);
  if (align > 16u || class_index < 0) {
    return NULL;
  }

  Hz5SmallFrontTls* tls = hz5_smallfront_tls();
  uint32_t ci = (uint32_t)class_index;
  Hz5SmallFrontPage* page = NULL;
  void* ptr = hz5_smallfront_local_pop(tls, ci, &page);
  if (!ptr) {
    hz5_smallfront_drain_remote_class(tls, ci);
    ptr = hz5_smallfront_local_pop(tls, ci, &page);
  }
  if (!ptr) {
    if (!hz5_smallfront_new_page(tls, ci)) {
      return NULL;
    }
    ptr = hz5_smallfront_local_pop(tls, ci, &page);
  }
  if (!ptr) {
    return NULL;
  }

  if (!page || page->magic != HZ5_SMALLFRONT_MAGIC) {
    page = hz5_smallfront_page_for_ptr(ptr);
  }
  uint32_t slot = 0;
  if (!page || !hz5_smallfront_slot_index(page, ptr, &slot) ||
      !hz5_smallfront_mark_active_local(page, slot)) {
    return NULL;
  }
  return ptr;
}

Hz5SmallFrontFreeResult hz5_smallfront_free(void* ptr) {
  Hz5SmallFrontPage* page = hz5_smallfront_page_for_ptr(ptr);
  if (!page) {
    return HZ5_SMALLFRONT_FREE_NOT_OWNED;
  }

  uint32_t slot = 0;
  if (!hz5_smallfront_slot_index(page, ptr, &slot)) {
    return HZ5_SMALLFRONT_FREE_INVALID;
  }

  Hz5SmallFrontTls* tls = hz5_smallfront_tls();
  if (page->owner_token == tls->owner_token) {
    if (!hz5_smallfront_mark_free_local(page, slot)) {
      return HZ5_SMALLFRONT_FREE_INVALID;
    }
    hz5_smallfront_local_push(tls, page->class_index, ptr, page);
  } else {
    if (!hz5_smallfront_mark_free_remote(page, slot)) {
      return HZ5_SMALLFRONT_FREE_INVALID;
    }
    hz5_smallfront_remote_push(page, ptr);
  }
  return HZ5_SMALLFRONT_FREE_OK;
}

int hz5_smallfront_can_handle(size_t size, size_t align) {
  return align <= 16u && hz5_smallfront_class_index(size) >= 0;
}

int hz5_smallfront_owns(void* ptr) {
  return hz5_smallfront_page_for_ptr(ptr) != NULL;
}

size_t hz5_smallfront_usable_size(void* ptr) {
  Hz5SmallFrontPage* page = hz5_smallfront_page_for_ptr(ptr);
  if (!page) {
    return 0;
  }
  uint32_t slot = 0;
  if (!hz5_smallfront_slot_index(page, ptr, &slot)) {
    return 0;
  }
  return page->class_size;
}

#else

void* hz5_smallfront_alloc(size_t size, size_t align) {
  (void)size;
  (void)align;
  return NULL;
}

Hz5SmallFrontFreeResult hz5_smallfront_free(void* ptr) {
  (void)ptr;
  return HZ5_SMALLFRONT_FREE_NOT_OWNED;
}

int hz5_smallfront_can_handle(size_t size, size_t align) {
  (void)size;
  (void)align;
  return 0;
}

int hz5_smallfront_owns(void* ptr) {
  (void)ptr;
  return 0;
}

size_t hz5_smallfront_usable_size(void* ptr) {
  (void)ptr;
  return 0;
}

#endif
