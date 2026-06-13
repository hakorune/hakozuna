#include "hz5_midpagefront.h"

#include "hz5_config.h"
#include "hz5_internal.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#if defined(__linux__)
#include <sys/mman.h>
#endif

#if !defined(_WIN32)
void* _aligned_malloc(size_t size, size_t alignment);
void _aligned_free(void* ptr);
#endif

#include "hz5_midpagefront_config.inc"

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M2

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
#error "MidPageFront nodeless run requires remote shadow"
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
#error "MidPageFront M4 magazine requires remote shadow"
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
#error "MidPageFront M4 remote packet requires M4 magazine"
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
#error "MidPageFront M7 remote ticket requires M4 remote packet"
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
#error "MidPageFront M8 owner transfer cache requires M4 remote packet"
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M9_BUDGET_DRAIN && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
#error "MidPageFront M9 budget drain requires M4 remote packet"
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
#error "MidPageFront M10 remote slot ring requires M4 remote packet"
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M11_REMOTE_DIRECT_CACHE && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
#error "MidPageFront M11 remote direct-cache requires M4 remote packet"
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
#error "MidPageFront PageRun requires M4 remote packet inbox"
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
#error "MidPageFront M4 overflow array requires M4 magazine"
#endif

#define HZ5_MIDPAGEFRONT_MAGIC UINT64_C(0x485A354D50414732)
#define HZ5_MIDPAGEFRONT_SLAB_BYTES ((size_t)65536)
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
#define HZ5_MIDPAGEFRONT_CLASS_COUNT 6u
#else
#define HZ5_MIDPAGEFRONT_CLASS_COUNT 5u
#endif
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
#define HZ5_MIDPAGEFRONT_NODELESS_PTRCACHE_CAP 64u
#define HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX 64u
#define HZ5_MIDPAGEFRONT_M4_OVERFLOW_CAP_MAX 128u
#ifndef HZ5_MIDPAGEFRONT_M6_RAW_CAP
#define HZ5_MIDPAGEFRONT_M6_RAW_CAP 64u
#endif

#ifndef HZ5_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP
#define HZ5_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP 64u
#endif

#ifndef HZ5_MIDPAGEFRONT_M8_XFER_PAGE_CAP
#define HZ5_MIDPAGEFRONT_M8_XFER_PAGE_CAP 64u
#endif

#ifndef HZ5_MIDPAGEFRONT_REMOTE_BATCH_CAP
#define HZ5_MIDPAGEFRONT_REMOTE_BATCH_CAP 16u
#endif

#include "hz5_midpagefront_state.inc"
#include "hz5_midpagefront_stats.inc"

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
static Hz5MidPageRegion* hz5_midpagefront_lookup_region(uintptr_t region_base);
#endif

static Hz5MidPageTls* hz5_midpagefront_tls(void) {
  Hz5MidPageTls* tls = &g_hz5_midpagefront_tls;
  if (tls->owner.slot == 0) {
    tls->owner = hz5_owner_current();
    hz5_midpagefront_register_tls_destructor();
  }
  return tls;
}

static int hz5_midpagefront_mark_active_local(Hz5MidPage* page,
                                              uint32_t slot);
static uint32_t hz5_midpagefront_drain_remote_class(Hz5MidPageTls* tls,
                                                    uint32_t class_index);

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
static void hz5_midpagefront_owner_mark_pending(Hz5OwnerToken owner,
                                                uint32_t class_index) {
  if (owner.slot == 0 || class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return;
  }
  atomic_fetch_or_explicit(&g_hz5_midpagefront_owner_pending[owner.slot],
                           UINT32_C(1) << class_index,
                           memory_order_release);
}

static uint32_t hz5_midpagefront_owner_pending_load(Hz5OwnerToken owner) {
  if (owner.slot == 0) {
    return 0;
  }
  return atomic_load_explicit(&g_hz5_midpagefront_owner_pending[owner.slot],
                              memory_order_acquire);
}

static void hz5_midpagefront_owner_clear_pending(Hz5OwnerToken owner,
                                                 uint32_t class_index) {
  if (owner.slot == 0 || class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return;
  }
  atomic_fetch_and_explicit(&g_hz5_midpagefront_owner_pending[owner.slot],
                            ~(UINT32_C(1) << class_index),
                            memory_order_acq_rel);
}
#else
static void hz5_midpagefront_owner_mark_pending(Hz5OwnerToken owner,
                                                uint32_t class_index) {
  (void)owner;
  (void)class_index;
}

static uint32_t hz5_midpagefront_owner_pending_load(Hz5OwnerToken owner) {
  (void)owner;
  return 0;
}

static void hz5_midpagefront_owner_clear_pending(Hz5OwnerToken owner,
                                                 uint32_t class_index) {
  (void)owner;
  (void)class_index;
}
#endif

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

#include "hz5_midpagefront_nodeless.inc"

#include "hz5_midpagefront_region.inc"

#include "hz5_midpagefront_m4_state.inc"

static void hz5_midpagefront_local_push(Hz5MidPageTls* tls,
                                        uint32_t class_index,
                                        void* ptr,
                                        Hz5MidPage* page) {
  Hz5MidPageNode* node = (Hz5MidPageNode*)ptr;
  node->page = page;
  node->next = tls->free_head[class_index];
  tls->free_head[class_index] = node;
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
#include "hz5_midpagefront_m4_magazine.inc"

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER
static int hz5_midpagefront_m8_xfer_has_class(uint32_t class_index) {
  if (!hz5_midpagefront_class_valid(class_index)) {
    return 0;
  }
  return g_hz5_midpagefront_m8_xfer_tls.xfer_count[class_index] != 0u;
}

static int hz5_midpagefront_m8_xfer_add_bits(Hz5MidPageTls* tls,
                                             uint32_t class_index,
                                             Hz5MidPage* page,
                                             uint64_t bits) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index) ||
      page->class_index != class_index) {
    return 0;
  }
  bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
  if (bits == 0) {
    return 1;
  }
  Hz5MidPageM8XferTls* xtls = &g_hz5_midpagefront_m8_xfer_tls;
  uint16_t count = xtls->xfer_count[class_index];
  for (uint16_t i = 0; i < count; ++i) {
    if (xtls->xfer[class_index][i].page == page) {
      xtls->xfer[class_index][i].bits |= bits;
      return 1;
    }
  }
  if (count >= HZ5_MIDPAGEFRONT_M8_XFER_PAGE_CAP) {
    return 0;
  }
  xtls->xfer[class_index][count].page = page;
  xtls->xfer[class_index][count].bits = bits;
  xtls->xfer_count[class_index] = (uint16_t)(count + 1u);
  return 1;
}

static int hz5_midpagefront_m8_refill_from_xfer(Hz5MidPageTls* tls,
                                                uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index)) {
    return 0;
  }
  uint16_t cap = hz5_midpagefront_m4_mag_cap(class_index);
  Hz5MidPageM8XferTls* xtls = &g_hz5_midpagefront_m8_xfer_tls;
  while (tls->magazine_count[class_index] < cap &&
         tls->magazine_count[class_index] <
             HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX &&
         xtls->xfer_count[class_index] != 0u) {
    uint16_t idx = (uint16_t)(xtls->xfer_count[class_index] - 1u);
    Hz5MidPageM8XferEntry* entry = &xtls->xfer[class_index][idx];
    Hz5MidPage* page = entry->page;
    uint64_t bits = entry->bits;
    if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
        page->class_index != class_index) {
      xtls->xfer_count[class_index] = idx;
      continue;
    }
    bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
    if (bits == 0) {
      entry->page = NULL;
      entry->bits = 0;
      xtls->xfer_count[class_index] = idx;
      continue;
    }
    uint64_t mask = bits & (UINT64_C(0) - bits);
    uint32_t slot = (uint32_t)__builtin_ctzll(mask);
    if (!hz5_midpagefront_m4_magazine_push(tls, class_index, page, slot)) {
      break;
    }
    entry->bits = bits & ~mask;
    if (entry->bits == 0) {
      entry->page = NULL;
      xtls->xfer_count[class_index] = idx;
    }
  }
  return tls->magazine_count[class_index] != 0u;
}

static void hz5_midpagefront_m8_purge_xfer_page(Hz5MidPageTls* tls,
                                                uint32_t class_index,
                                                Hz5MidPage* page) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index)) {
    return;
  }
  uint16_t out = 0;
  Hz5MidPageM8XferTls* xtls = &g_hz5_midpagefront_m8_xfer_tls;
  uint16_t count = xtls->xfer_count[class_index];
  for (uint16_t i = 0; i < count; ++i) {
    if (xtls->xfer[class_index][i].page != page) {
      if (out != i) {
        xtls->xfer[class_index][out] = xtls->xfer[class_index][i];
      }
      ++out;
    }
  }
  for (uint16_t i = out; i < count; ++i) {
    xtls->xfer[class_index][i].page = NULL;
    xtls->xfer[class_index][i].bits = 0;
  }
  xtls->xfer_count[class_index] = out;
}
#endif

#include "hz5_midpagefront_empty_release.inc"

static void hz5_midpagefront_m4_seed_page(Hz5MidPageTls* tls,
                                          uint32_t class_index,
                                          Hz5MidPage* page) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index)) {
    return;
  }
  for (uint32_t slot = 0; slot < page->slot_count; ++slot) {
    hz5_midpagefront_m4_cache_slot(tls, class_index, page, slot);
  }
}

static int hz5_midpagefront_m4_refill_magazine(Hz5MidPageTls* tls,
                                               uint32_t class_index);

#include "hz5_midpagefront_m4_pagerun.inc"

static void* hz5_midpagefront_m4_alloc_class(uint32_t class_index) {
  if (!hz5_midpagefront_class_valid(class_index)) {
    return NULL;
  }
  hz5_midpagefront_m4_stat_inc_class(
      g_hz5_midpagefront_m4_stat_alloc_call, class_index);
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY
  hz5_midpagefront_m4_remote_packet_drain_if_pending(tls, class_index);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_ON_HIT
  if (tls->magazine_count[class_index] != 0u &&
      hz5_midpagefront_m4_should_drain_on_hit(tls, class_index)) {
    hz5_midpagefront_m4_remote_packet_drain_if_pending(tls, class_index);
  }
#endif
  while (tls->magazine_count[class_index] != 0u ||
         hz5_midpagefront_m4_refill_magazine(tls, class_index)) {
    uint16_t count = tls->magazine_count[class_index];
    if (count == 0u) {
      continue;
    }
    count = (uint16_t)(count - 1u);
    tls->magazine_count[class_index] = count;
    Hz5MidPageMagEntry entry = tls->magazine[class_index][count];
    hz5_midpagefront_m4_stat_inc_class(
        g_hz5_midpagefront_m4_stat_mag_hit, class_index);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG
    if (entry.ptr) {
      hz5_midpagefront_m4_note_alloc(entry.page);
      return entry.ptr;
    }
#endif
    Hz5MidPage* page = entry.page;
    uint32_t slot = entry.slot;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY
    if (!hz5_midpagefront_m4_transition_owner_local(
            page, slot, HZ5_MIDPAGE_SLOT_CACHE, HZ5_MIDPAGE_SLOT_LIVE)) {
      continue;
    }
    hz5_midpagefront_m4_note_alloc(page);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE
    if (page->empty_retained) {
      page->empty_retained = 0;
      if (tls->empty_retained_count[class_index] != 0u) {
        --tls->empty_retained_count[class_index];
      }
    }
#endif
    if (entry.ptr) {
      return entry.ptr;
    }
    return (void*)((uintptr_t)page->slab_base +
                   (uintptr_t)slot * (uintptr_t)page->class_size);
#else
    if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
        page->class_index != class_index || slot >= page->slot_count) {
      continue;
    }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE
    if (!hz5_midpagefront_m4_slot_state_is(page,
                                           slot,
                                           HZ5_MIDPAGE_SLOT_CACHE)) {
      continue;
    }
#else
    if (!hz5_midpagefront_m4_transition_owner_local(
            page, slot, HZ5_MIDPAGE_SLOT_CACHE, HZ5_MIDPAGE_SLOT_LIVE)) {
      continue;
    }
#endif
    hz5_midpagefront_m4_note_alloc(page);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE
    if (page->empty_retained) {
      page->empty_retained = 0;
      if (tls->empty_retained_count[class_index] != 0u) {
        --tls->empty_retained_count[class_index];
      }
    }
#endif
    return (void*)((uintptr_t)page->slab_base +
                   (uintptr_t)slot * (uintptr_t)page->class_size);
#endif
  }
  return NULL;
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
  hz5_midpagefront_owner_mark_pending(owner, class_index);
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
  // mark_free_remote has already claimed the slot. If the owner died before
  // publish, fail closed by not enqueueing into a stale owner inbox.
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

#include "hz5_midpagefront_remote_experiments.inc"

static uint32_t hz5_midpagefront_drain_remote_class(Hz5MidPageTls* tls,
                                                    uint32_t class_index) {
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return 0;
  }
  hz5_midpagefront_owner_clear_pending(tls->owner, class_index);
  void* head = atomic_exchange_explicit(
      &g_hz5_midpagefront_owner_inbox[tls->owner.slot][class_index],
      NULL,
      memory_order_acq_rel);
  uint32_t drained = 0;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
  while (head) {
    Hz5MidPage* page = (Hz5MidPage*)head;
    head = page->remote_packet_next;
    page->remote_packet_next = NULL;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
    drained += hz5_midpagefront_pagerun_remote_drain_page(
        tls, class_index, page);
#else
    drained += hz5_midpagefront_m4_remote_packet_drain_page(
        tls, class_index, page);
#endif
  }
  return drained;
#else
  while (head) {
    Hz5MidPageNode* node = (Hz5MidPageNode*)head;
    head = node->next;
    if (node->page && hz5_owner_equal(node->page->owner, tls->owner)) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
      uint32_t slot = 0;
      if (hz5_midpagefront_slot_index(node->page, node, &slot) &&
          hz5_midpagefront_m4_transition(node->page, slot,
                                         HZ5_MIDPAGE_SLOT_REMOTE,
                                         HZ5_MIDPAGE_SLOT_CACHE)) {
        hz5_midpagefront_m4_cache_slot(tls, class_index, node->page, slot);
        ++drained;
      }
#else
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
      uint32_t slot = 0;
      if (hz5_midpagefront_slot_index(node->page, node, &slot)) {
        uint64_t mask = hz5_midpagefront_slot_mask(slot);
        uint64_t remote = atomic_load_explicit(&node->page->remote_bits,
                                               memory_order_acquire);
        if ((remote & mask) != 0) {
          for (;;) {
            uint64_t next_remote = remote & ~mask;
            if (atomic_compare_exchange_weak_explicit(
                    &node->page->remote_bits,
                    &remote,
                    next_remote,
                    memory_order_acq_rel,
                    memory_order_acquire)) {
              hz5_midpagefront_nodeless_publish_free_bit(tls, node->page,
                                                         mask);
              hz5_midpagefront_stat_inc(
                  &g_hz5_midpagefront_stat_remote_drained);
              ++drained;
              break;
            }
            if ((remote & mask) == 0) {
              break;
            }
          }
        }
      }
#else
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
#endif
#endif
    }
  }
  return drained;
#endif
}

static int hz5_midpagefront_mark_active_local(Hz5MidPage* page,
                                              uint32_t slot) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_UNSAFE_LOCAL_NOCHECK
  (void)page;
  (void)slot;
  return 1;
#else
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  uint64_t bits = atomic_load_explicit(&page->free_bits,
                                       memory_order_relaxed);
  uint64_t remote = atomic_load_explicit(&page->remote_bits,
                                         memory_order_acquire);
  if ((bits & mask) == 0 || (remote & mask) != 0) {
    return 0;
  }
  atomic_store_explicit(&page->free_bits, bits & ~mask,
                        memory_order_release);
  return 1;
#else
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
#endif
#endif
}

static int hz5_midpagefront_mark_free_local(Hz5MidPage* page,
                                            uint32_t slot) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_UNSAFE_LOCAL_NOCHECK
  (void)page;
  (void)slot;
  return 1;
#else
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  uint64_t bits = atomic_load_explicit(&page->free_bits,
                                       memory_order_relaxed);
  uint64_t remote = atomic_load_explicit(&page->remote_bits,
                                         memory_order_acquire);
  if (((bits | remote) & mask) != 0) {
    return 0;
  }
  atomic_store_explicit(&page->free_bits, bits | mask,
                        memory_order_release);
  return 1;
#else
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
#endif
#endif
}

static int hz5_midpagefront_mark_free_remote(Hz5MidPage* page,
                                             uint32_t slot) {
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  uint64_t free_bits = atomic_load_explicit(&page->free_bits,
                                            memory_order_acquire);
  if ((free_bits & mask) != 0) {
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
    Hz5MidPage* page = &pages[i];
    page->slab_base =
        (void*)(base + (uintptr_t)i * HZ5_MIDPAGEFRONT_SLAB_BYTES);
    page->class_index = (uint16_t)class_index;
    page->source_free = 1;
    page->source_next = g_hz5_midpagefront_source_free[class_index];
    g_hz5_midpagefront_source_free[class_index] = page;
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
  Hz5MidPage* page = g_hz5_midpagefront_source_free[class_index];
  g_hz5_midpagefront_source_free[class_index] = page->source_next;
  page->source_next = NULL;
  page->source_free = 0;
  page->empty_retained = 0;
  page->retired_empty = 0;
  page->retired_next = NULL;
  pthread_mutex_unlock(&g_hz5_midpagefront_region_lock);

  uintptr_t slab_base = (uintptr_t)page->slab_base;
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
  if (page != &region->pages[slab_index]) {
    return NULL;
  }
  *page_out = page;
  return page->slab_base;
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
  page->empty_retained = 0;
  page->retired_empty = 0;
  page->retired_next = NULL;
  atomic_store_explicit(&page->active_bits, 0, memory_order_relaxed);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  atomic_store_explicit(
      &page->free_bits,
      hz5_midpagefront_slot_count_mask(slot_count),
      memory_order_relaxed);
  page->in_partial = 0;
  page->next_partial = NULL;
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
  atomic_store_explicit(&page->remote_bits, 0, memory_order_relaxed);
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
  atomic_store_explicit(&page->slot_state2,
                        hz5_midpagefront_m4_initial_state(slot_count),
                        memory_order_relaxed);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
  atomic_store_explicit(&page->remote_packet_bits, 0, memory_order_relaxed);
  atomic_store_explicit(&page->remote_packet_queued, 0, memory_order_relaxed);
  page->remote_packet_next = NULL;
#endif
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
  page->pagerun_owner_free_bits =
      hz5_midpagefront_slot_count_mask(slot_count);
  page->pagerun_in_partial = 0;
  page->pagerun_next_partial = NULL;
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
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
  (void)class_size;
#if !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
  hz5_midpagefront_m4_seed_page(tls, class_index, page);
#endif
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  (void)class_size;
#else
  for (uint32_t slot = slot_count; slot > 0; --slot) {
    void* ptr = (void*)((uintptr_t)slab +
                        (uintptr_t)(slot - 1u) * class_size);
    hz5_midpagefront_local_push(tls, class_index, ptr, page);
  }
#endif
  return page;
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
static Hz5MidPage* hz5_midpagefront_pagerun_refill_current(
    Hz5MidPageTls* tls,
    uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index)) {
    return NULL;
  }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_RELEASE_ON_REFILL
  (void)hz5_midpagefront_m4_release_retired_one(tls, class_index);
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_FLUSH_ON_REFILL
  hz5_midpagefront_m6_remote_flush_raw(tls);
#endif

  (void)hz5_midpagefront_drain_remote_class(tls, class_index);
  Hz5MidPage* current = tls->pagerun_current[class_index];
  if (current && current->magic == HZ5_MIDPAGEFRONT_MAGIC &&
      hz5_owner_equal(current->owner, tls->owner) &&
      current->pagerun_owner_free_bits != 0) {
    return current;
  }
  Hz5MidPage* page =
      hz5_midpagefront_pagerun_partial_pop(tls, class_index);
  if (!page) {
    page = hz5_midpagefront_new_page(tls, class_index);
  }
  if (!page || page->pagerun_owner_free_bits == 0) {
    return NULL;
  }
  tls->pagerun_current[class_index] = page;
  return page;
}

static void* hz5_midpagefront_pagerun_alloc_class(uint32_t class_index) {
  if (!hz5_midpagefront_class_valid(class_index)) {
    return NULL;
  }
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  Hz5MidPage* page = tls->pagerun_current[class_index];
  if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
      !hz5_owner_equal(page->owner, tls->owner) ||
      page->pagerun_owner_free_bits == 0) {
    page = hz5_midpagefront_pagerun_refill_current(tls, class_index);
    if (!page) {
      return NULL;
    }
  }

  uint64_t bits = page->pagerun_owner_free_bits;
  uint64_t mask = bits & (UINT64_C(0) - bits);
  uint32_t slot = (uint32_t)__builtin_ctzll(mask);
  page->pagerun_owner_free_bits = bits & ~mask;
  if (!hz5_midpagefront_pagerun_cache_to_live(page, mask)) {
    return NULL;
  }
  return (void*)((uintptr_t)page->slab_base +
                 (uintptr_t)slot * (uintptr_t)page->class_size);
}
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
static int hz5_midpagefront_m4_refill_magazine(Hz5MidPageTls* tls,
                                               uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index)) {
    return 0;
  }
  hz5_midpagefront_m4_stat_inc_class(
      g_hz5_midpagefront_m4_stat_refill_call, class_index);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_RELEASE_ON_REFILL
  (void)hz5_midpagefront_m4_release_retired_one(tls, class_index);
#endif
#endif
  if (tls->magazine_count[class_index] != 0u) {
    return 1;
  }

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER
  if (hz5_midpagefront_m8_xfer_has_class(class_index) &&
      hz5_midpagefront_m8_refill_from_xfer(tls, class_index)) {
    return 1;
  }
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
  hz5_midpagefront_m6_flush_raw(tls);
  if (tls->magazine_count[class_index] != 0u) {
    return 1;
  }
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING
  hz5_midpagefront_m10_remote_flush_slots(tls);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_FLUSH_ON_REFILL
  hz5_midpagefront_m6_remote_flush_raw(tls);
#endif

  (void)hz5_midpagefront_drain_remote_class(tls, class_index);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER
  if (hz5_midpagefront_m8_xfer_has_class(class_index) &&
      hz5_midpagefront_m8_refill_from_xfer(tls, class_index)) {
    hz5_midpagefront_m4_stat_inc_class(
        g_hz5_midpagefront_m4_stat_refill_remote_hit, class_index);
    return 1;
  }
#endif
  if (tls->magazine_count[class_index] != 0u) {
    hz5_midpagefront_m4_stat_inc_class(
        g_hz5_midpagefront_m4_stat_refill_remote_hit, class_index);
    return 1;
  }

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  if (hz5_midpagefront_m4_refill_from_overflow(tls, class_index)) {
    return 1;
  }
#endif

  while (tls->magazine_count[class_index] <
         hz5_midpagefront_m4_mag_cap(class_index)) {
    Hz5MidPage* page = NULL;
    void* ptr = hz5_midpagefront_local_pop(tls, class_index, &page);
    if (!ptr) {
      break;
    }
    hz5_midpagefront_m4_stat_inc_class(
        g_hz5_midpagefront_m4_stat_refill_local_pop, class_index);
    uint32_t slot = 0;
    if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
        page->class_index != class_index ||
        !hz5_midpagefront_slot_index(page, ptr, &slot)) {
      continue;
    }
    hz5_midpagefront_m4_magazine_push(tls, class_index, page, slot);
  }
  if (tls->magazine_count[class_index] != 0u) {
    return 1;
  }

  Hz5MidPage* page = hz5_midpagefront_new_page(tls, class_index);
  if (page) {
    hz5_midpagefront_m4_stat_inc_class(
        g_hz5_midpagefront_m4_stat_refill_new_page, class_index);
  }
  return page && tls->magazine_count[class_index] != 0u;
}
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
static int hz5_midpagefront_nodeless_refill_current(Hz5MidPageTls* tls,
                                                    uint32_t ci) {
  if (!tls || ci >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return 0;
  }

  hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_refill);
  uint64_t bits = 0;
  Hz5MidPage* page = hz5_midpagefront_nodeless_partial_pop(tls, ci, &bits);
  if (page) {
    hz5_midpagefront_stat_inc(
        &g_hz5_midpagefront_stat_refill_partial_hit);
  }
  if (!page) {
    (void)hz5_midpagefront_drain_remote_class(tls, ci);
    page = hz5_midpagefront_nodeless_partial_pop(tls, ci, &bits);
    if (page) {
      hz5_midpagefront_stat_inc(
          &g_hz5_midpagefront_stat_refill_remote_hit);
    }
  }
  if (!page) {
    page = hz5_midpagefront_new_page(tls, ci);
    if (!page) {
      return 0;
    }
    hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_refill_new_page);
    bits = atomic_load_explicit(&page->free_bits, memory_order_acquire);
    bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
  }
  if (bits == 0) {
    return 0;
  }
  tls->current_page[ci] = page;
  tls->current_bits[ci] = bits;
  return 1;
}

static void* hz5_midpagefront_nodeless_alloc_class(uint32_t ci) {
  if (!hz5_midpagefront_class_valid(ci)) {
    return NULL;
  }
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE
  void* cached = hz5_midpagefront_nodeless_ptrcache_pop(tls, ci);
  if (cached) {
    return cached;
  }
#endif
  if (tls->current_bits[ci] == 0 &&
      !hz5_midpagefront_nodeless_refill_current(tls, ci)) {
    return NULL;
  }

  Hz5MidPage* page = tls->current_page[ci];
  uint64_t bits = tls->current_bits[ci];
  if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC || bits == 0) {
    tls->current_bits[ci] = 0;
    return NULL;
  }

  uint64_t mask = bits & (UINT64_C(0) - bits);
  uint32_t slot = (uint32_t)__builtin_ctzll(mask);
  tls->current_bits[ci] = bits & ~mask;
  if (!hz5_midpagefront_mark_active_local(page, slot)) {
    return NULL;
  }
  return (void*)((uintptr_t)page->slab_base +
                 (uintptr_t)slot * (uintptr_t)page->class_size);
}
#endif

static void* hz5_midpagefront_alloc_class(uint32_t ci) {
  if (!hz5_midpagefront_class_valid(ci)) {
    return NULL;
  }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
  return hz5_midpagefront_pagerun_alloc_class(ci);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
  return hz5_midpagefront_m4_alloc_class(ci);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  return hz5_midpagefront_nodeless_alloc_class(ci);
#else
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
#endif
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

static Hz5MidPageFrontFreeResult hz5_midpagefront_free_page(
    void* ptr,
    Hz5MidPage* page) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  hz5_midpagefront_m6_defer_free(tls, ptr);
  return HZ5_MIDPAGEFRONT_FREE_OK;
#else
  if (!page) {
    return HZ5_MIDPAGEFRONT_FREE_NOT_OWNED;
  }
  uint32_t slot = 0;
  if (!hz5_midpagefront_slot_index(page, ptr, &slot)) {
    return HZ5_MIDPAGEFRONT_FREE_INVALID;
  }

  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  if (hz5_owner_equal(page->owner, tls->owner)) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
    return hz5_midpagefront_pagerun_free_local(tls, page, slot);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE
    if (!hz5_midpagefront_m4_note_free(page)) {
      return HZ5_MIDPAGEFRONT_FREE_INVALID;
    }
    hz5_midpagefront_m4_cache_slot(tls, page->class_index, page, slot);
    hz5_midpagefront_m4_try_release_empty_page(tls, page);
#else
    if (!hz5_midpagefront_m4_transition_owner_local(
            page, slot, HZ5_MIDPAGE_SLOT_LIVE, HZ5_MIDPAGE_SLOT_CACHE)) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE
      if (!hz5_midpagefront_m4_slot_state_is(page,
                                             slot,
                                             HZ5_MIDPAGE_SLOT_CACHE)) {
        return HZ5_MIDPAGEFRONT_FREE_INVALID;
      }
#else
      return HZ5_MIDPAGEFRONT_FREE_INVALID;
#endif
    }
    if (!hz5_midpagefront_m4_note_free(page)) {
      return HZ5_MIDPAGEFRONT_FREE_INVALID;
    }
    hz5_midpagefront_m4_cache_slot(tls, page->class_index, page, slot);
    hz5_midpagefront_m4_try_release_empty_page(tls, page);
#endif
#else
    if (!hz5_midpagefront_mark_free_local(page, slot)) {
      return HZ5_MIDPAGEFRONT_FREE_INVALID;
    }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
    uint64_t mask = hz5_midpagefront_slot_mask(slot);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE
    if (hz5_midpagefront_nodeless_ptrcache_push(
            tls, page->class_index, ptr, page, mask)) {
      return HZ5_MIDPAGEFRONT_FREE_OK;
    }
#endif
    if (tls->current_page[page->class_index] == page) {
      tls->current_bits[page->class_index] |= mask;
    } else {
      hz5_midpagefront_nodeless_partial_push(tls, page->class_index, page);
    }
#else
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT
    if (hz5_midpagefront_hot_push(tls, page->class_index, ptr, page)) {
      return HZ5_MIDPAGEFRONT_FREE_OK;
    }
#endif
    hz5_midpagefront_local_push(tls, page->class_index, ptr, page);
#endif
#endif
  } else {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE
    hz5_midpagefront_m6_remote_defer_free(tls, ptr);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
    uint64_t mask = hz5_midpagefront_slot_mask(slot);
    if (!hz5_midpagefront_pagerun_live_to_cache(page, mask)) {
      return HZ5_MIDPAGEFRONT_FREE_INVALID;
    }
    atomic_fetch_or_explicit(&page->remote_bits,
                             mask,
                             memory_order_release);
    hz5_midpagefront_pagerun_remote_publish(page);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING
    return hz5_midpagefront_m10_remote_slot_defer(tls, page, slot);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET
    return hz5_midpagefront_m7_remote_ticket_defer(tls, page, slot);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE
    hz5_midpagefront_m6_remote_defer_free(tls, ptr);
#else
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
    if (!hz5_midpagefront_m4_transition(page, slot,
                                        HZ5_MIDPAGE_SLOT_LIVE,
                                        HZ5_MIDPAGE_SLOT_REMOTE)) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE
      if (!hz5_midpagefront_m4_transition(page,
                                          slot,
                                          HZ5_MIDPAGE_SLOT_CACHE,
                                          HZ5_MIDPAGE_SLOT_REMOTE)) {
        return HZ5_MIDPAGEFRONT_FREE_INVALID;
      }
#else
      return HZ5_MIDPAGEFRONT_FREE_INVALID;
#endif
    }
    if (!hz5_midpagefront_m4_note_free(page)) {
      return HZ5_MIDPAGEFRONT_FREE_INVALID;
    }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
    hz5_midpagefront_m4_remote_packet_push(page, slot);
#else
    hz5_midpagefront_remote_batch_push(tls, page, ptr);
#endif
#else
    if (!hz5_midpagefront_mark_free_remote(page, slot)) {
      return HZ5_MIDPAGEFRONT_FREE_INVALID;
    }
    hz5_midpagefront_remote_batch_push(tls, page, ptr);
#endif
#endif
  }
  return HZ5_MIDPAGEFRONT_FREE_OK;
#endif
}

Hz5MidPageFrontFreeResult hz5_midpagefront_free(void* ptr) {
  return hz5_midpagefront_free_page(ptr, hz5_midpagefront_page_for_ptr(ptr));
}

Hz5MidPageFrontTag hz5_midpagefront_tag(void* ptr) {
  Hz5MidPageFrontTag tag;
  tag.page = hz5_midpagefront_page_for_ptr(ptr);
  return tag;
}

Hz5MidPageFrontFreeResult hz5_midpagefront_free_tagged(
    void* ptr,
    Hz5MidPageFrontTag tag) {
  return hz5_midpagefront_free_page(ptr, (Hz5MidPage*)tag.page);
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

void hz5_midpagefront_owner_drain_some(unsigned budget) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
  if (budget == 0u) {
    return;
  }
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  uint32_t pending = hz5_midpagefront_owner_pending_load(tls->owner);
  if (pending == 0) {
    return;
  }
  unsigned drained_classes = 0;
  for (uint32_t ci = 0; ci < HZ5_MIDPAGEFRONT_CLASS_COUNT; ++ci) {
    if ((pending & (UINT32_C(1) << ci)) == 0) {
      continue;
    }
    (void)hz5_midpagefront_drain_remote_class(tls, ci);
    if (++drained_classes >= budget) {
      break;
    }
  }
#else
  (void)budget;
#endif
}

size_t hz5_midpagefront_release_retired(void) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
  return hz5_midpagefront_m4_release_checkpoint(hz5_midpagefront_tls());
#else
  return 0;
#endif
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

void hz5_midpagefront_owner_drain_some(unsigned budget) {
  (void)budget;
}

size_t hz5_midpagefront_release_retired(void) {
  return 0;
}

#endif
