#include "hz5_midpagefront.h"

#include "hz5_config.h"
#include "hz5_internal.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>

#if !defined(_WIN32)
void* _aligned_malloc(size_t size, size_t alignment);
void _aligned_free(void* ptr);
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M2
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M2 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_LOCAL_FAST_STATE
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_LOCAL_FAST_STATE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_REGION_CACHE
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_REGION_CACHE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_LOCAL_ACTIVE_TRUST
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_LOCAL_ACTIVE_TRUST 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_SLOT_SWITCH
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_SLOT_SWITCH 0
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M2

#define HZ5_MIDPAGEFRONT_MAGIC UINT64_C(0x485A354D50414732)
#define HZ5_MIDPAGEFRONT_SLAB_BYTES ((size_t)65536)
#define HZ5_MIDPAGEFRONT_CLASS_COUNT 5u
#define HZ5_MIDPAGEFRONT_MAP_BITS 18u
#define HZ5_MIDPAGEFRONT_MAP_CAP \
  ((size_t)1u << HZ5_MIDPAGEFRONT_MAP_BITS)
#define HZ5_MIDPAGEFRONT_REGION_BYTES ((size_t)64u * 1024u * 1024u)
#define HZ5_MIDPAGEFRONT_REGION_SLAB_COUNT \
  (HZ5_MIDPAGEFRONT_REGION_BYTES / HZ5_MIDPAGEFRONT_SLAB_BYTES)
#define HZ5_MIDPAGEFRONT_REGION_MAP_BITS 14u
#define HZ5_MIDPAGEFRONT_REGION_MAP_CAP \
  ((size_t)1u << HZ5_MIDPAGEFRONT_REGION_MAP_BITS)
#define HZ5_MIDPAGEFRONT_REGION_CAP 8192u
#define HZ5_MIDPAGEFRONT_TLS_REGION_CACHE_CAP 8u

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
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
  _Atomic uint64_t remote_bits;
#endif
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

typedef struct Hz5MidPageRegion {
  uintptr_t base;
  Hz5MidPage* pages;
  uint32_t class_index;
  uint32_t slab_count;
} Hz5MidPageRegion;

typedef struct Hz5MidPageRegionMapEntry {
  _Atomic uintptr_t region_base;
  _Atomic(Hz5MidPageRegion*) region;
} Hz5MidPageRegionMapEntry;

typedef struct Hz5MidPageRawNode {
  struct Hz5MidPageRawNode* next;
} Hz5MidPageRawNode;

typedef struct Hz5MidPageTls {
  Hz5OwnerToken owner;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT
  void* hot_ptr[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  Hz5MidPage* hot_page[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#endif
  Hz5MidPageNode* free_head[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  Hz5MidPage* owned_pages[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_REGION_CACHE
  uintptr_t region_cache_base[HZ5_MIDPAGEFRONT_TLS_REGION_CACHE_CAP];
  Hz5MidPageRegion*
      region_cache_region[HZ5_MIDPAGEFRONT_TLS_REGION_CACHE_CAP];
  uint32_t region_cache_victim;
#endif
  Hz5OwnerToken remote_batch_owner;
  uint32_t remote_batch_class;
  uint32_t remote_batch_count;
  Hz5MidPageNode* remote_batch_head;
  Hz5MidPageNode* remote_batch_tail;
} Hz5MidPageTls;

static const uint32_t g_hz5_midpagefront_classes
    [HZ5_MIDPAGEFRONT_CLASS_COUNT] = {3072u, 4096u, 8192u, 16384u, 32768u};

static const Hz5OwnerToken k_hz5_midpagefront_no_owner = {0, 0};

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
static Hz5MidPageRegion
    g_hz5_midpagefront_regions[HZ5_MIDPAGEFRONT_REGION_CAP];
static Hz5MidPageRegionMapEntry
    g_hz5_midpagefront_region_map[HZ5_MIDPAGEFRONT_REGION_MAP_CAP];
static Hz5MidPageRawNode*
    g_hz5_midpagefront_source_free[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static size_t g_hz5_midpagefront_region_count;
static pthread_mutex_t g_hz5_midpagefront_region_lock =
    PTHREAD_MUTEX_INITIALIZER;
#else
static Hz5MidPageMapEntry
    g_hz5_midpagefront_map[HZ5_MIDPAGEFRONT_MAP_CAP];
static pthread_mutex_t g_hz5_midpagefront_map_lock =
    PTHREAD_MUTEX_INITIALIZER;
#endif
static _Atomic(void*) g_hz5_midpagefront_owner_inbox[UINT16_MAX + 1u]
                                                  [HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Thread_local Hz5MidPageTls g_hz5_midpagefront_tls;

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
static Hz5MidPageRegion* hz5_midpagefront_lookup_region(uintptr_t region_base);
#endif

static Hz5MidPageTls* hz5_midpagefront_tls(void) {
  Hz5MidPageTls* tls = &g_hz5_midpagefront_tls;
  if (tls->owner.slot == 0) {
    tls->owner = hz5_owner_current();
  }
  return tls;
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_REGION_CACHE
static Hz5MidPageRegion*
hz5_midpagefront_lookup_region_tls_cached(uintptr_t region_base) {
  Hz5MidPageTls* tls = &g_hz5_midpagefront_tls;
  for (uint32_t i = 0; i < HZ5_MIDPAGEFRONT_TLS_REGION_CACHE_CAP; ++i) {
    if (tls->region_cache_base[i] == region_base) {
      return tls->region_cache_region[i];
    }
  }

  Hz5MidPageRegion* region = hz5_midpagefront_lookup_region(region_base);
  if (region) {
    uint32_t slot = tls->region_cache_victim++ %
                    HZ5_MIDPAGEFRONT_TLS_REGION_CACHE_CAP;
    tls->region_cache_region[slot] = region;
    tls->region_cache_base[slot] = region_base;
  }
  return region;
}
#endif

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

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
static size_t hz5_midpagefront_region_hash(uintptr_t region_base) {
  uintptr_t x = region_base >> 26;
  x ^= x >> HZ5_MIDPAGEFRONT_REGION_MAP_BITS;
  x ^= x >> (HZ5_MIDPAGEFRONT_REGION_MAP_BITS * 2u);
  return (size_t)x & (HZ5_MIDPAGEFRONT_REGION_MAP_CAP - 1u);
}

static Hz5MidPageRegion* hz5_midpagefront_lookup_region(uintptr_t region_base) {
  size_t idx = hz5_midpagefront_region_hash(region_base);
  for (size_t probe = 0; probe < HZ5_MIDPAGEFRONT_REGION_MAP_CAP; ++probe) {
    Hz5MidPageRegionMapEntry* entry =
        &g_hz5_midpagefront_region_map
            [(idx + probe) & (HZ5_MIDPAGEFRONT_REGION_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->region_base, memory_order_acquire);
    if (current == region_base) {
      return atomic_load_explicit(&entry->region, memory_order_acquire);
    }
    if (current == 0) {
      return NULL;
    }
  }
  return NULL;
}

static int hz5_midpagefront_region_map_insert(uintptr_t region_base,
                                              Hz5MidPageRegion* region) {
  size_t idx = hz5_midpagefront_region_hash(region_base);
  for (size_t probe = 0; probe < HZ5_MIDPAGEFRONT_REGION_MAP_CAP; ++probe) {
    Hz5MidPageRegionMapEntry* entry =
        &g_hz5_midpagefront_region_map
            [(idx + probe) & (HZ5_MIDPAGEFRONT_REGION_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->region_base, memory_order_acquire);
    if (current == region_base) {
      atomic_store_explicit(&entry->region, region, memory_order_release);
      return 1;
    }
    if (current == 0) {
      atomic_store_explicit(&entry->region, region, memory_order_relaxed);
      atomic_store_explicit(&entry->region_base, region_base,
                            memory_order_release);
      return 1;
    }
  }
  return 0;
}
#else
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
#endif

static Hz5MidPage* hz5_midpagefront_page_for_ptr(void* ptr) {
  if (!ptr) {
    return NULL;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t slab_base =
      p & ~(uintptr_t)(HZ5_MIDPAGEFRONT_SLAB_BYTES - 1u);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
  uintptr_t region_base =
      p & ~(uintptr_t)(HZ5_MIDPAGEFRONT_REGION_BYTES - 1u);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_REGION_CACHE
  Hz5MidPageRegion* region =
      hz5_midpagefront_lookup_region_tls_cached(region_base);
#else
  Hz5MidPageRegion* region =
      hz5_midpagefront_lookup_region(region_base);
#endif
  if (!region || region->base != region_base || !region->pages) {
    return NULL;
  }
  uintptr_t slab_index =
      (slab_base - region_base) / HZ5_MIDPAGEFRONT_SLAB_BYTES;
  if (slab_index >= region->slab_count) {
    return NULL;
  }
  Hz5MidPage* page = &region->pages[slab_index];
#else
  Hz5MidPage* page = hz5_midpagefront_lookup_slab(slab_base);
#endif
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
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_SLOT_SWITCH
  uint32_t slot = 0;
  switch (page->class_index) {
    case 0:
      if (page->class_size != 3072u || offset % 3072u != 0) {
        return 0;
      }
      slot = (uint32_t)(offset / 3072u);
      break;
    case 1:
      if (page->class_size != 4096u || (offset & 4095u) != 0) {
        return 0;
      }
      slot = (uint32_t)(offset >> 12);
      break;
    case 2:
      if (page->class_size != 8192u || (offset & 8191u) != 0) {
        return 0;
      }
      slot = (uint32_t)(offset >> 13);
      break;
    case 3:
      if (page->class_size != 16384u || (offset & 16383u) != 0) {
        return 0;
      }
      slot = (uint32_t)(offset >> 14);
      break;
    case 4:
      if (page->class_size != 32768u || (offset & 32767u) != 0) {
        return 0;
      }
      slot = (uint32_t)(offset >> 15);
      break;
    default:
      return 0;
  }
#else
  if (page->class_size == 0 || offset % page->class_size != 0) {
    return 0;
  }
  uint32_t slot = (uint32_t)(offset / page->class_size);
#endif
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

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT
static int hz5_midpagefront_hot_push(Hz5MidPageTls* tls,
                                     uint32_t class_index,
                                     void* ptr,
                                     Hz5MidPage* page) {
  if (!tls || !ptr || !page ||
      class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT ||
      tls->hot_ptr[class_index]) {
    return 0;
  }
  tls->hot_page[class_index] = page;
  tls->hot_ptr[class_index] = ptr;
  return 1;
}

static void* hz5_midpagefront_hot_pop(Hz5MidPageTls* tls,
                                      uint32_t class_index,
                                      Hz5MidPage** page_out) {
  if (!tls || class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    if (page_out) {
      *page_out = NULL;
    }
    return NULL;
  }
  void* ptr = tls->hot_ptr[class_index];
  if (!ptr) {
    if (page_out) {
      *page_out = NULL;
    }
    return NULL;
  }
  tls->hot_ptr[class_index] = NULL;
  if (page_out) {
    *page_out = tls->hot_page[class_index];
  }
  tls->hot_page[class_index] = NULL;
  return ptr;
}
#endif

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
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
      uint32_t slot = 0;
      if (hz5_midpagefront_slot_index(node->page, node, &slot)) {
        uint64_t mask = hz5_midpagefront_slot_mask(slot);
        uint64_t remote = atomic_load_explicit(&node->page->remote_bits,
                                               memory_order_acquire);
        uint64_t active = atomic_load_explicit(&node->page->active_bits,
                                               memory_order_acquire);
        if ((remote & mask) != 0 && (active & mask) != 0) {
          for (;;) {
            if ((remote & mask) == 0) {
              break;
            }
            uint64_t next_remote = remote & ~mask;
            if (atomic_compare_exchange_weak_explicit(
                    &node->page->remote_bits,
                    &remote,
                    next_remote,
                    memory_order_acq_rel,
                    memory_order_acquire)) {
              break;
            }
          }
          atomic_store_explicit(&node->page->active_bits,
                                active & ~mask,
                                memory_order_release);
          hz5_midpagefront_local_push(tls, class_index, node, node->page);
          ++drained;
        }
      }
#else
      hz5_midpagefront_local_push(tls, class_index, node, node->page);
      ++drained;
#endif
    }
  }
  return drained;
}

static int hz5_midpagefront_mark_active_local(Hz5MidPage* page,
                                              uint32_t slot) {
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_LOCAL_FAST_STATE
  uint64_t bits = atomic_load_explicit(&page->active_bits,
                                       memory_order_acquire);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_LOCAL_ACTIVE_TRUST
  uint64_t remote = atomic_load_explicit(&page->remote_bits,
                                         memory_order_acquire);
  if ((remote & mask) != 0) {
    return 0;
  }
#endif
  if ((bits & mask) != 0) {
    return 0;
  }
  atomic_store_explicit(&page->active_bits, bits | mask,
                        memory_order_release);
  return 1;
#else
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
#endif
}

static int hz5_midpagefront_mark_free_local(Hz5MidPage* page,
                                            uint32_t slot) {
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_LOCAL_FAST_STATE
  uint64_t bits = atomic_load_explicit(&page->active_bits,
                                       memory_order_acquire);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
  uint64_t remote = atomic_load_explicit(&page->remote_bits,
                                         memory_order_acquire);
  if ((remote & mask) != 0) {
    return 0;
  }
#endif
  if ((bits & mask) == 0) {
    return 0;
  }
  atomic_store_explicit(&page->active_bits, bits & ~mask,
                        memory_order_release);
  return 1;
#else
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
#endif
}

static int hz5_midpagefront_mark_free_remote(Hz5MidPage* page,
                                             uint32_t slot) {
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
  uint64_t active = atomic_load_explicit(&page->active_bits,
                                         memory_order_acquire);
  if ((active & mask) == 0) {
    return 0;
  }
  uint64_t old = atomic_load_explicit(&page->remote_bits,
                                      memory_order_acquire);
  for (;;) {
    if ((old & mask) != 0) {
      return 0;
    }
    uint64_t next = old | mask;
    if (atomic_compare_exchange_weak_explicit(&page->remote_bits,
                                              &old,
                                              next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      return 1;
    }
  }
#else
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
#endif
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
static int hz5_midpagefront_source_refill_locked(uint32_t class_index) {
  if (!hz5_midpagefront_class_valid(class_index) ||
      g_hz5_midpagefront_region_count >= HZ5_MIDPAGEFRONT_REGION_CAP) {
    return 0;
  }

  void* block = _aligned_malloc(HZ5_MIDPAGEFRONT_REGION_BYTES,
                                HZ5_MIDPAGEFRONT_REGION_BYTES);
  if (!block) {
    return 0;
  }
  Hz5MidPage* pages = (Hz5MidPage*)_aligned_malloc(
      sizeof(Hz5MidPage) * HZ5_MIDPAGEFRONT_REGION_SLAB_COUNT, 64u);
  if (!pages) {
    _aligned_free(block);
    return 0;
  }
  memset(pages, 0,
         sizeof(Hz5MidPage) * HZ5_MIDPAGEFRONT_REGION_SLAB_COUNT);

  uintptr_t base = (uintptr_t)block;
  if ((base & (uintptr_t)(HZ5_MIDPAGEFRONT_REGION_BYTES - 1u)) != 0) {
    _aligned_free(pages);
    _aligned_free(block);
    return 0;
  }

  Hz5MidPageRegion* region =
      &g_hz5_midpagefront_regions[g_hz5_midpagefront_region_count++];
  region->base = base;
  region->pages = pages;
  region->class_index = class_index;
  region->slab_count = (uint32_t)HZ5_MIDPAGEFRONT_REGION_SLAB_COUNT;
  if (!hz5_midpagefront_region_map_insert(base, region)) {
    --g_hz5_midpagefront_region_count;
    _aligned_free(pages);
    _aligned_free(block);
    return 0;
  }

  for (uint32_t i = 0; i < HZ5_MIDPAGEFRONT_REGION_SLAB_COUNT; ++i) {
    Hz5MidPageRawNode* node =
        (Hz5MidPageRawNode*)(base + (uintptr_t)i *
                                      HZ5_MIDPAGEFRONT_SLAB_BYTES);
    node->next = g_hz5_midpagefront_source_free[class_index];
    g_hz5_midpagefront_source_free[class_index] = node;
  }
  return 1;
}

static void* hz5_midpagefront_source_alloc_slab(uint32_t class_index,
                                                Hz5MidPage** page_out) {
  if (!hz5_midpagefront_class_valid(class_index) || !page_out) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_midpagefront_region_lock);
  if (!g_hz5_midpagefront_source_free[class_index] &&
      !hz5_midpagefront_source_refill_locked(class_index)) {
    pthread_mutex_unlock(&g_hz5_midpagefront_region_lock);
    return NULL;
  }
  Hz5MidPageRawNode* node = g_hz5_midpagefront_source_free[class_index];
  g_hz5_midpagefront_source_free[class_index] = node->next;
  pthread_mutex_unlock(&g_hz5_midpagefront_region_lock);

  uintptr_t slab_base = (uintptr_t)node;
  uintptr_t region_base =
      slab_base & ~(uintptr_t)(HZ5_MIDPAGEFRONT_REGION_BYTES - 1u);
  Hz5MidPageRegion* region =
      hz5_midpagefront_lookup_region(region_base);
  if (!region || !region->pages || region->class_index != class_index) {
    return NULL;
  }
  uintptr_t slab_index =
      (slab_base - region_base) / HZ5_MIDPAGEFRONT_SLAB_BYTES;
  if (slab_index >= region->slab_count) {
    return NULL;
  }
  *page_out = &region->pages[slab_index];
  return node;
}
#endif

static Hz5MidPage* hz5_midpagefront_new_page(Hz5MidPageTls* tls,
                                             uint32_t class_index) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
  Hz5MidPage* page = NULL;
  void* slab = hz5_midpagefront_source_alloc_slab(class_index, &page);
  if (!slab || !page) {
    return NULL;
  }
#else
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
#endif

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
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
  atomic_store_explicit(&page->remote_bits, 0, memory_order_relaxed);
#endif
  page->next_owned = tls->owned_pages[class_index];

#if !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
  if (!hz5_midpagefront_map_insert((uintptr_t)slab, page)) {
    _aligned_free(page);
    _aligned_free(slab);
    return NULL;
  }
#endif

  tls->owned_pages[class_index] = page;
  for (uint32_t slot = slot_count; slot > 0; --slot) {
    void* ptr = (void*)((uintptr_t)slab +
                        (uintptr_t)(slot - 1u) * class_size);
    hz5_midpagefront_local_push(tls, class_index, ptr, page);
  }
  return page;
}

static void* hz5_midpagefront_alloc_class(uint32_t ci) {
  if (!hz5_midpagefront_class_valid(ci)) {
    return NULL;
  }
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  Hz5MidPage* page = NULL;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT
  void* ptr = hz5_midpagefront_hot_pop(tls, ci, &page);
#else
  void* ptr = hz5_midpagefront_local_pop(tls, ci, &page);
#endif
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

Hz5MidPageFrontAllocResult hz5_midpagefront_try_alloc(size_t size,
                                                      size_t align,
                                                      void** ptr_out) {
  if (ptr_out) {
    *ptr_out = NULL;
  }
  if (align > 16u) {
    return HZ5_MIDPAGEFRONT_ALLOC_UNSUPPORTED;
  }
  int class_index = hz5_midpagefront_class_index(size);
  if (class_index < 0) {
    return HZ5_MIDPAGEFRONT_ALLOC_UNSUPPORTED;
  }
  void* ptr = hz5_midpagefront_alloc_class((uint32_t)class_index);
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return ptr ? HZ5_MIDPAGEFRONT_ALLOC_OK : HZ5_MIDPAGEFRONT_ALLOC_OOM;
}

void* hz5_midpagefront_alloc(size_t size, size_t align) {
  void* ptr = NULL;
  (void)hz5_midpagefront_try_alloc(size, align, &ptr);
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
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT
    if (hz5_midpagefront_hot_push(tls, page->class_index, ptr, page)) {
      return HZ5_MIDPAGEFRONT_FREE_OK;
    }
#endif
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

Hz5MidPageFrontAllocResult hz5_midpagefront_try_alloc(size_t size,
                                                      size_t align,
                                                      void** ptr_out) {
  (void)size;
  (void)align;
  if (ptr_out) {
    *ptr_out = NULL;
  }
  return HZ5_MIDPAGEFRONT_ALLOC_UNSUPPORTED;
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
