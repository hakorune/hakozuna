#include "hz5_midpagefront.h"

#include "hz5_config.h"
#include "hz5_internal.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
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

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_STATS
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_STATS 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_UNSAFE_LOCAL_NOCHECK
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_UNSAFE_LOCAL_NOCHECK 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_FLAT_MAG_CAP
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_FLAT_MAG_CAP 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_WIDE_32K_CLASS
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_WIDE_32K_CLASS 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_STATS
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_STATS 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY 0
#endif

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

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
#error "MidPageFront M4 overflow array requires M4 magazine"
#endif

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
#define HZ5_MIDPAGEFRONT_NODELESS_PTRCACHE_CAP 64u
#define HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX 64u
#define HZ5_MIDPAGEFRONT_M4_OVERFLOW_CAP_MAX 128u
#ifndef HZ5_MIDPAGEFRONT_M6_RAW_CAP
#define HZ5_MIDPAGEFRONT_M6_RAW_CAP 64u
#endif

#ifndef HZ5_MIDPAGEFRONT_REMOTE_BATCH_CAP
#define HZ5_MIDPAGEFRONT_REMOTE_BATCH_CAP 16u
#endif

typedef enum Hz5MidPageSlotState {
  HZ5_MIDPAGE_SLOT_LIVE = 0,
  HZ5_MIDPAGE_SLOT_CACHE = 1,
  HZ5_MIDPAGE_SLOT_REMOTE = 2,
  HZ5_MIDPAGE_SLOT_DEAD = 3,
} Hz5MidPageSlotState;

typedef struct Hz5MidPage {
  uint64_t magic;
  void* slab_base;
  uint32_t class_size;
  uint16_t class_index;
  uint16_t slot_count;
  Hz5OwnerToken owner;
  // active_bits is the canonical live-slot bitmap for the node-list path.
  // nodeless diagnostics use free_bits plus remote_bits instead.
  _Atomic uint64_t active_bits;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  // free_bits is owner-local reusable state. remote_bits claims cross-thread
  // frees until the owner drains them, so the two bitmaps must not overlap.
  _Atomic uint64_t free_bits;
  uint8_t in_partial;
  struct Hz5MidPage* next_partial;
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
  _Atomic uint64_t remote_bits;
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
  _Atomic uint64_t slot_state2;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
  _Atomic uint64_t remote_packet_bits;
  _Atomic uint8_t remote_packet_queued;
  struct Hz5MidPage* remote_packet_next;
#endif
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

typedef struct Hz5MidPagePtrCacheEntry {
  void* ptr;
  Hz5MidPage* page;
  uint64_t mask;
} Hz5MidPagePtrCacheEntry;

typedef struct Hz5MidPageMagEntry {
  Hz5MidPage* page;
  uint16_t slot;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG || \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY || \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  void* ptr;
#endif
} Hz5MidPageMagEntry;

typedef struct Hz5MidPageTls {
  Hz5OwnerToken owner;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT
  void* hot_ptr[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  Hz5MidPage* hot_page[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#endif
  Hz5MidPageNode* free_head[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  Hz5MidPage* current_page[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  uint64_t current_bits[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  Hz5MidPage* partial_pages[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE
  Hz5MidPagePtrCacheEntry ptrcache[HZ5_MIDPAGEFRONT_CLASS_COUNT]
                                  [HZ5_MIDPAGEFRONT_NODELESS_PTRCACHE_CAP];
  uint32_t ptrcache_count[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#endif
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
  Hz5MidPageMagEntry magazine[HZ5_MIDPAGEFRONT_CLASS_COUNT]
                              [HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX];
  uint16_t magazine_count[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  Hz5MidPageMagEntry overflow[HZ5_MIDPAGEFRONT_CLASS_COUNT]
                             [HZ5_MIDPAGEFRONT_M4_OVERFLOW_CAP_MAX];
  uint16_t overflow_count[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
  void* m6_raw_free[HZ5_MIDPAGEFRONT_M6_RAW_CAP];
  uint16_t m6_raw_count;
#endif
#endif
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
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
static _Atomic uint32_t g_hz5_midpagefront_owner_pending[UINT16_MAX + 1u];
#endif
static _Thread_local Hz5MidPageTls g_hz5_midpagefront_tls;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_STATS
static _Atomic uint64_t g_hz5_midpagefront_stat_refill;
static _Atomic uint64_t g_hz5_midpagefront_stat_refill_partial_hit;
static _Atomic uint64_t g_hz5_midpagefront_stat_refill_remote_hit;
static _Atomic uint64_t g_hz5_midpagefront_stat_refill_new_page;
static _Atomic uint64_t g_hz5_midpagefront_stat_partial_push;
static _Atomic uint64_t g_hz5_midpagefront_stat_remote_drained;
static _Atomic uint64_t g_hz5_midpagefront_stat_ptrcache_hit;
static _Atomic uint64_t g_hz5_midpagefront_stat_ptrcache_push;
static _Atomic uint64_t g_hz5_midpagefront_stat_ptrcache_full;

__attribute__((destructor)) static void hz5_midpagefront_stats_dump(void) {
  fprintf(stderr,
          "[HZ5_MIDPAGEFRONT_NODELESS_STATS]"
          " refill=%llu"
          " refill_partial_hit=%llu"
          " refill_remote_hit=%llu"
          " refill_new_page=%llu"
          " partial_push=%llu"
          " remote_drained=%llu"
          " ptrcache_hit=%llu"
          " ptrcache_push=%llu"
          " ptrcache_full=%llu\n",
          (unsigned long long)atomic_load(&g_hz5_midpagefront_stat_refill),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_refill_partial_hit),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_refill_remote_hit),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_refill_new_page),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_partial_push),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_remote_drained),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_ptrcache_hit),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_ptrcache_push),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_ptrcache_full));
}

static void hz5_midpagefront_stat_inc(_Atomic uint64_t* counter) {
  atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
}
#else
#define hz5_midpagefront_stat_inc(counter) ((void)0)
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_STATS
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_alloc_call[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_mag_hit[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_refill_call[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_refill_remote_hit[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_refill_local_pop[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_refill_new_page[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_cache_slot[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_mag_push[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_mag_full[HZ5_MIDPAGEFRONT_CLASS_COUNT];

static void hz5_midpagefront_m4_stat_inc(_Atomic uint64_t* counter) {
  atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
}

__attribute__((destructor)) static void hz5_midpagefront_m4_stats_dump(void) {
  for (uint32_t ci = 0; ci < HZ5_MIDPAGEFRONT_CLASS_COUNT; ++ci) {
    fprintf(stderr,
            "[HZ5_MIDPAGEFRONT_M4_STATS] class=%u bytes=%u"
            " alloc_call=%llu mag_hit=%llu refill_call=%llu"
            " refill_remote_hit=%llu refill_local_pop=%llu"
            " refill_new_page=%llu cache_slot=%llu mag_push=%llu"
            " mag_full=%llu\n",
            ci,
            g_hz5_midpagefront_classes[ci],
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_alloc_call[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_mag_hit[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_refill_call[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_refill_remote_hit[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_refill_local_pop[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_refill_new_page[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_cache_slot[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_mag_push[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_mag_full[ci],
                memory_order_relaxed));
  }
}

#define hz5_midpagefront_m4_stat_inc_class(counter, ci) \
  do {                                                  \
    if ((ci) < HZ5_MIDPAGEFRONT_CLASS_COUNT) {          \
      hz5_midpagefront_m4_stat_inc(&(counter)[(ci)]);   \
    }                                                   \
  } while (0)
#else
#define hz5_midpagefront_m4_stat_inc_class(counter, ci) ((void)0)
#endif

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

static int hz5_midpagefront_class_valid(uint32_t class_index) {
  return class_index < HZ5_MIDPAGEFRONT_CLASS_COUNT;
}

static int hz5_midpagefront_request_bucket(size_t size) {
  if (size <= 2048u || size > 32768u) {
    return -1;
  }
  if (size <= 3072u) {
    return 0;
  }
  if (size <= 4096u) {
    return 1;
  }
  if (size <= 8192u) {
    return 2;
  }
  if (size <= 16384u) {
    return 3;
  }
  return 4;
}

// MidPageFront owns the ordinary malloc gap between SmallFront and MidFront.
// Keep this range narrow: 64K exact/overaligned and LargeFront rows are separate
// routes with different RSS and remote-free behavior.
static int hz5_midpagefront_class_index(size_t size) {
  static const uint8_t strict_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      0u, 1u, 2u, 3u, 4u};
  static const uint8_t band4_16_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      1u, 1u, 3u, 3u, 4u};
  static const uint8_t band8_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      2u, 2u, 2u, 4u, 4u};
  static const uint8_t band16_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      3u, 3u, 3u, 3u, 4u};
  static const uint8_t band4_8_16_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      1u, 1u, 2u, 3u, 4u};
  static const uint8_t band4_8_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      1u, 1u, 2u, 4u, 4u};
  static const uint8_t band8_16_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      2u, 2u, 2u, 3u, 4u};
  static const uint8_t wide32k_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      4u, 4u, 4u, 4u, 4u};

  int bucket = hz5_midpagefront_request_bucket(size);
  if (bucket < 0) {
    return -1;
  }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_WIDE_32K_CLASS
  return (int)wide32k_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 1
  return (int)band4_16_32_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 2
  return (int)band8_32_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 3
  return (int)band16_32_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 4
  return (int)band4_8_16_32_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 5
  return (int)band4_8_32_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 6
  return (int)band8_16_32_map[bucket];
#else
  return (int)strict_map[bucket];
#endif
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

static uint64_t hz5_midpagefront_slot_count_mask(uint32_t slot_count) {
  if (slot_count >= 64u) {
    return UINT64_MAX;
  }
  if (slot_count == 0u) {
    return 0;
  }
  return (UINT64_C(1) << slot_count) - UINT64_C(1);
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
static uint16_t hz5_midpagefront_m4_mag_cap(uint32_t class_index) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_FLAT_MAG_CAP
  (void)class_index;
  return HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX;
#else
  static const uint16_t caps[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      64u, 64u, 32u, 16u, 8u};
  return class_index < HZ5_MIDPAGEFRONT_CLASS_COUNT ? caps[class_index] : 0u;
#endif
}

static uint64_t hz5_midpagefront_m4_state_shift(uint32_t slot) {
  return (uint64_t)slot * UINT64_C(2);
}

static uint64_t hz5_midpagefront_m4_state_mask(uint32_t slot) {
  return UINT64_C(3) << hz5_midpagefront_m4_state_shift(slot);
}

static uint64_t hz5_midpagefront_m4_initial_state(uint32_t slot_count) {
  uint64_t state = 0;
  if (slot_count > 32u) {
    slot_count = 32u;
  }
  for (uint32_t slot = 0; slot < slot_count; ++slot) {
    state |= (uint64_t)HZ5_MIDPAGE_SLOT_CACHE
             << hz5_midpagefront_m4_state_shift(slot);
  }
  return state;
}

static int hz5_midpagefront_m4_transition(Hz5MidPage* page,
                                          uint32_t slot,
                                          Hz5MidPageSlotState expected,
                                          Hz5MidPageSlotState desired) {
  if (!page || slot >= page->slot_count || slot >= 32u) {
    return 0;
  }
  uint64_t shift = hz5_midpagefront_m4_state_shift(slot);
  uint64_t mask = hz5_midpagefront_m4_state_mask(slot);
  uint64_t old = atomic_load_explicit(&page->slot_state2,
                                      memory_order_acquire);
  for (;;) {
    if (((old & mask) >> shift) != (uint64_t)expected) {
      return 0;
    }
    uint64_t next = (old & ~mask) | ((uint64_t)desired << shift);
    if (atomic_compare_exchange_weak_explicit(&page->slot_state2,
                                              &old,
                                              next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      return 1;
    }
  }
}

static int hz5_midpagefront_m4_transition_owner_local(
    Hz5MidPage* page,
    uint32_t slot,
    Hz5MidPageSlotState expected,
    Hz5MidPageSlotState desired) {
  if (!page || slot >= page->slot_count || slot >= 32u) {
    return 0;
  }
  uint64_t shift = hz5_midpagefront_m4_state_shift(slot);
  uint64_t mask = hz5_midpagefront_m4_state_mask(slot);
  uint64_t old = atomic_load_explicit(&page->slot_state2,
                                      memory_order_acquire);
  if (((old & mask) >> shift) != (uint64_t)expected) {
    return 0;
  }
  uint64_t next = (old & ~mask) | ((uint64_t)desired << shift);
  atomic_store_explicit(&page->slot_state2, next, memory_order_release);
  return 1;
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE
static int hz5_midpagefront_m4_slot_state_is(Hz5MidPage* page,
                                             uint32_t slot,
                                             Hz5MidPageSlotState expected) {
  if (!page || slot >= page->slot_count || slot >= 32u) {
    return 0;
  }
  uint64_t shift = hz5_midpagefront_m4_state_shift(slot);
  uint64_t mask = hz5_midpagefront_m4_state_mask(slot);
  uint64_t old = atomic_load_explicit(&page->slot_state2,
                                      memory_order_acquire);
  return ((old & mask) >> shift) == (uint64_t)expected;
}
#endif
#endif

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
static int hz5_midpagefront_m4_magazine_push(Hz5MidPageTls* tls,
                                             uint32_t class_index,
                                             Hz5MidPage* page,
                                             uint32_t slot) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index) ||
      slot >= page->slot_count) {
    return 0;
  }
  uint16_t count = tls->magazine_count[class_index];
  uint16_t cap = hz5_midpagefront_m4_mag_cap(class_index);
  if (count >= cap || count >= HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX) {
    return 0;
  }
  hz5_midpagefront_m4_stat_inc_class(
      g_hz5_midpagefront_m4_stat_mag_push, class_index);
  tls->magazine[class_index][count].page = page;
  tls->magazine[class_index][count].slot = (uint16_t)slot;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG || \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY || \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  tls->magazine[class_index][count].ptr =
      (void*)((uintptr_t)page->slab_base +
              (uintptr_t)slot * (uintptr_t)page->class_size);
#endif
  tls->magazine_count[class_index] = (uint16_t)(count + 1u);
  return 1;
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
static int hz5_midpagefront_m4_overflow_push(Hz5MidPageTls* tls,
                                             uint32_t class_index,
                                             Hz5MidPage* page,
                                             uint32_t slot) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index) ||
      slot >= page->slot_count) {
    return 0;
  }
  uint16_t count = tls->overflow_count[class_index];
  if (count >= HZ5_MIDPAGEFRONT_M4_OVERFLOW_CAP_MAX) {
    return 0;
  }
  tls->overflow[class_index][count].page = page;
  tls->overflow[class_index][count].slot = (uint16_t)slot;
  tls->overflow[class_index][count].ptr =
      (void*)((uintptr_t)page->slab_base +
              (uintptr_t)slot * (uintptr_t)page->class_size);
  tls->overflow_count[class_index] = (uint16_t)(count + 1u);
  return 1;
}

static int hz5_midpagefront_m4_refill_from_overflow(Hz5MidPageTls* tls,
                                                    uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index)) {
    return 0;
  }
  uint16_t cap = hz5_midpagefront_m4_mag_cap(class_index);
  while (tls->magazine_count[class_index] < cap &&
         tls->magazine_count[class_index] <
             HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX &&
         tls->overflow_count[class_index] != 0u) {
    uint16_t next = (uint16_t)(tls->overflow_count[class_index] - 1u);
    Hz5MidPageMagEntry entry = tls->overflow[class_index][next];
    tls->overflow_count[class_index] = next;
    if (!entry.page || entry.slot >= entry.page->slot_count ||
        entry.page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
        entry.page->class_index != class_index) {
      continue;
    }
    if (!hz5_midpagefront_m4_magazine_push(tls,
                                           class_index,
                                           entry.page,
                                           entry.slot)) {
      (void)hz5_midpagefront_m4_overflow_push(tls,
                                              class_index,
                                              entry.page,
                                              entry.slot);
      break;
    }
  }
  return tls->magazine_count[class_index] != 0u;
}
#endif

static void hz5_midpagefront_m4_cache_slot(Hz5MidPageTls* tls,
                                           uint32_t class_index,
                                           Hz5MidPage* page,
                                           uint32_t slot) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index) ||
      slot >= page->slot_count) {
    return;
  }
  hz5_midpagefront_m4_stat_inc_class(
      g_hz5_midpagefront_m4_stat_cache_slot, class_index);
  if (hz5_midpagefront_m4_magazine_push(tls, class_index, page, slot)) {
    return;
  }
  hz5_midpagefront_m4_stat_inc_class(
      g_hz5_midpagefront_m4_stat_mag_full, class_index);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  if (hz5_midpagefront_m4_overflow_push(tls, class_index, page, slot)) {
    return;
  }
#endif
  void* ptr = (void*)((uintptr_t)page->slab_base +
                      (uintptr_t)slot * (uintptr_t)page->class_size);
  hz5_midpagefront_local_push(tls, class_index, ptr, page);
}

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

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
static void hz5_midpagefront_m4_remote_packet_publish(Hz5OwnerToken owner,
                                                      uint32_t class_index,
                                                      Hz5MidPage* page) {
  if (owner.slot == 0 || !hz5_midpagefront_class_valid(class_index) ||
      !page || !hz5_owner_is_alive(owner)) {
    return;
  }
  void* old_head = NULL;
  _Atomic(void*)* inbox =
      &g_hz5_midpagefront_owner_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    page->remote_packet_next = (Hz5MidPage*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, page, memory_order_release, memory_order_acquire));
  hz5_midpagefront_owner_mark_pending(owner, class_index);
}

static void hz5_midpagefront_m4_remote_packet_requeue_if_needed(
    Hz5MidPage* page) {
  if (!page || !hz5_owner_is_alive(page->owner)) {
    return;
  }
  uint64_t bits = atomic_load_explicit(&page->remote_packet_bits,
                                       memory_order_acquire);
  if (bits == 0) {
    return;
  }
  uint8_t expected = 0;
  if (atomic_compare_exchange_strong_explicit(
          &page->remote_packet_queued,
          &expected,
          1,
          memory_order_acq_rel,
          memory_order_acquire)) {
    hz5_midpagefront_m4_remote_packet_publish(page->owner,
                                              page->class_index,
                                              page);
  }
}

static void hz5_midpagefront_m4_remote_packet_push(Hz5MidPage* page,
                                                   uint32_t slot) {
  if (!page || slot >= page->slot_count || slot >= 64u ||
      !hz5_owner_is_alive(page->owner)) {
    return;
  }
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
  atomic_fetch_or_explicit(&page->remote_packet_bits,
                           mask,
                           memory_order_release);
  uint8_t expected = 0;
  if (atomic_compare_exchange_strong_explicit(
          &page->remote_packet_queued,
          &expected,
          1,
          memory_order_acq_rel,
          memory_order_acquire)) {
    hz5_midpagefront_m4_remote_packet_publish(page->owner,
                                              page->class_index,
                                              page);
  }
}

static uint32_t hz5_midpagefront_m4_remote_packet_drain_page(
    Hz5MidPageTls* tls,
    uint32_t class_index,
    Hz5MidPage* page) {
  if (!tls || !page || !hz5_owner_equal(page->owner, tls->owner) ||
      page->class_index != class_index) {
    return 0;
  }
  uint64_t bits = atomic_exchange_explicit(&page->remote_packet_bits,
                                           0,
                                           memory_order_acq_rel);
  bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
  uint32_t drained = 0;
  while (bits != 0) {
    uint64_t mask = bits & (UINT64_C(0) - bits);
    bits &= ~mask;
    uint32_t slot = (uint32_t)__builtin_ctzll(mask);
    if (hz5_midpagefront_m4_transition(page, slot,
                                       HZ5_MIDPAGE_SLOT_REMOTE,
                                       HZ5_MIDPAGE_SLOT_CACHE)) {
      hz5_midpagefront_m4_cache_slot(tls, class_index, page, slot);
      ++drained;
    }
  }
  atomic_store_explicit(&page->remote_packet_queued,
                        0,
                        memory_order_release);
  hz5_midpagefront_m4_remote_packet_requeue_if_needed(page);
  return drained;
}

static void hz5_midpagefront_m4_remote_packet_drain_if_pending(
    Hz5MidPageTls* tls,
    uint32_t class_index) {
  if (!tls || tls->owner.slot == 0 ||
      !hz5_midpagefront_class_valid(class_index)) {
    return;
  }
  void* head = atomic_load_explicit(
      &g_hz5_midpagefront_owner_inbox[tls->owner.slot][class_index],
      memory_order_acquire);
  if (head) {
    (void)hz5_midpagefront_drain_remote_class(tls, class_index);
  }
}
#endif

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
    return (void*)((uintptr_t)page->slab_base +
                   (uintptr_t)slot * (uintptr_t)page->class_size);
#endif
  }
  return NULL;
}
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
static void hz5_midpagefront_nodeless_partial_push(Hz5MidPageTls* tls,
                                                   uint32_t class_index,
                                                   Hz5MidPage* page) {
  if (!tls || !page || class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT ||
      page->in_partial) {
    return;
  }
  if (tls->current_page[class_index] == page) {
    return;
  }
  hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_partial_push);
  page->in_partial = 1;
  page->next_partial = tls->partial_pages[class_index];
  tls->partial_pages[class_index] = page;
}

static Hz5MidPage* hz5_midpagefront_nodeless_partial_pop(Hz5MidPageTls* tls,
                                                         uint32_t class_index,
                                                         uint64_t* bits_out) {
  if (bits_out) {
    *bits_out = 0;
  }
  if (!tls || class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return NULL;
  }
  while (tls->partial_pages[class_index]) {
    Hz5MidPage* page = tls->partial_pages[class_index];
    tls->partial_pages[class_index] = page->next_partial;
    page->next_partial = NULL;
    page->in_partial = 0;
    uint64_t bits = atomic_load_explicit(&page->free_bits,
                                         memory_order_acquire);
    bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
    if (bits != 0) {
      if (bits_out) {
        *bits_out = bits;
      }
      return page;
    }
  }
  return NULL;
}

static void hz5_midpagefront_nodeless_publish_free_bit(Hz5MidPageTls* tls,
                                                       Hz5MidPage* page,
                                                       uint64_t mask) {
  if (!tls || !page || mask == 0) {
    return;
  }
  uint32_t class_index = page->class_index;
  if (class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return;
  }
  uint64_t bits = atomic_load_explicit(&page->free_bits,
                                       memory_order_relaxed);
  atomic_store_explicit(&page->free_bits, bits | mask,
                        memory_order_release);
  if (tls->current_page[class_index] == page) {
    tls->current_bits[class_index] |= mask;
    return;
  }
  hz5_midpagefront_nodeless_partial_push(tls, class_index, page);
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE
static int hz5_midpagefront_nodeless_ptrcache_push(Hz5MidPageTls* tls,
                                                   uint32_t class_index,
                                                   void* ptr,
                                                   Hz5MidPage* page,
                                                   uint64_t mask) {
  if (!tls || !ptr || !page || mask == 0 ||
      class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return 0;
  }
  uint32_t count = tls->ptrcache_count[class_index];
  if (count >= HZ5_MIDPAGEFRONT_NODELESS_PTRCACHE_CAP) {
    hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_ptrcache_full);
    return 0;
  }
  Hz5MidPagePtrCacheEntry* entry = &tls->ptrcache[class_index][count];
  entry->ptr = ptr;
  entry->page = page;
  entry->mask = mask;
  tls->ptrcache_count[class_index] = count + 1u;
  hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_ptrcache_push);
  return 1;
}

static void* hz5_midpagefront_nodeless_ptrcache_pop(Hz5MidPageTls* tls,
                                                    uint32_t class_index) {
  if (!tls || class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return NULL;
  }
  while (tls->ptrcache_count[class_index] != 0u) {
    uint32_t count = tls->ptrcache_count[class_index] - 1u;
    tls->ptrcache_count[class_index] = count;
    Hz5MidPagePtrCacheEntry entry = tls->ptrcache[class_index][count];
    if (!entry.ptr || !entry.page ||
        entry.page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
        entry.page->class_index != class_index ||
        entry.mask == 0 ||
        (entry.mask & (entry.mask - UINT64_C(1))) != 0) {
      continue;
    }
    uint32_t slot = (uint32_t)__builtin_ctzll(entry.mask);
    if (slot >= entry.page->slot_count ||
        !hz5_midpagefront_mark_active_local(entry.page, slot)) {
      continue;
    }
    hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_ptrcache_hit);
    return entry.ptr;
  }
  return NULL;
}
#endif
#endif

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

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
typedef struct Hz5MidPageM6Batch {
  Hz5MidPage* page;
  uint64_t bits;
} Hz5MidPageM6Batch;

static int hz5_midpagefront_m6_batch_add(Hz5MidPageM6Batch* batch,
                                         uint32_t* count,
                                         Hz5MidPage* page,
                                         uint64_t mask) {
  if (!batch || !count || !page || mask == 0) {
    return 0;
  }
  for (uint32_t i = 0; i < *count; ++i) {
    if (batch[i].page == page) {
      batch[i].bits |= mask;
      return 1;
    }
  }
  if (*count >= HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    return 0;
  }
  batch[*count].page = page;
  batch[*count].bits = mask;
  *count += 1u;
  return 1;
}

static uint64_t hz5_midpagefront_m6_state_bits(uint64_t slots,
                                               Hz5MidPageSlotState state) {
  uint64_t out = 0;
  while (slots != 0) {
    uint64_t mask = slots & (UINT64_C(0) - slots);
    slots &= ~mask;
    uint32_t slot = (uint32_t)__builtin_ctzll(mask);
    out |= (uint64_t)state << hz5_midpagefront_m4_state_shift(slot);
  }
  return out;
}

static int hz5_midpagefront_m6_transition_owner_local_batch(
    Hz5MidPage* page,
    uint64_t slots,
    Hz5MidPageSlotState expected,
    Hz5MidPageSlotState desired) {
  if (!page) {
    return 0;
  }
  slots &= hz5_midpagefront_slot_count_mask(page->slot_count);
  if (slots == 0) {
    return 0;
  }
  uint64_t state_mask = hz5_midpagefront_m6_state_bits(
      slots, HZ5_MIDPAGE_SLOT_DEAD);
  uint64_t expected_bits = hz5_midpagefront_m6_state_bits(slots, expected);
  uint64_t desired_bits = hz5_midpagefront_m6_state_bits(slots, desired);
  uint64_t old = atomic_load_explicit(&page->slot_state2,
                                      memory_order_acquire);
  if ((old & state_mask) != expected_bits) {
    return 0;
  }
  uint64_t next = (old & ~state_mask) | desired_bits;
  atomic_store_explicit(&page->slot_state2, next, memory_order_release);
  return 1;
}

static void hz5_midpagefront_m6_promote_owner_batch(
    Hz5MidPageTls* tls,
    Hz5MidPageM6Batch* batch,
    uint32_t count) {
  if (!tls || !batch) {
    return;
  }
  for (uint32_t i = 0; i < count; ++i) {
    Hz5MidPage* page = batch[i].page;
    uint64_t bits = batch[i].bits;
    if (!page || bits == 0 ||
        !hz5_owner_equal(page->owner, tls->owner) ||
        !hz5_midpagefront_m6_transition_owner_local_batch(
            page, bits, HZ5_MIDPAGE_SLOT_LIVE,
            HZ5_MIDPAGE_SLOT_CACHE)) {
      continue;
    }
    while (bits != 0) {
      uint64_t mask = bits & (UINT64_C(0) - bits);
      bits &= ~mask;
      uint32_t slot = (uint32_t)__builtin_ctzll(mask);
      hz5_midpagefront_m4_cache_slot(tls, page->class_index, page, slot);
    }
  }
}

static void hz5_midpagefront_m6_flush_raw(Hz5MidPageTls* tls) {
  if (!tls || tls->m6_raw_count == 0u) {
    return;
  }

  uint16_t count = tls->m6_raw_count;
  tls->m6_raw_count = 0;
  Hz5MidPageM6Batch owner_batch[HZ5_MIDPAGEFRONT_M6_RAW_CAP];
  uint32_t owner_batch_count = 0;
  memset(owner_batch, 0, sizeof(owner_batch));
  for (uint16_t i = 0; i < count; ++i) {
    void* ptr = tls->m6_raw_free[i];
    tls->m6_raw_free[i] = NULL;
    Hz5MidPage* page = hz5_midpagefront_page_for_ptr(ptr);
    if (!page) {
      continue;
    }
    uint32_t slot = 0;
    if (!hz5_midpagefront_slot_index(page, ptr, &slot)) {
      continue;
    }
    if (hz5_owner_equal(page->owner, tls->owner)) {
      (void)hz5_midpagefront_m6_batch_add(
          owner_batch,
          &owner_batch_count,
          page,
          hz5_midpagefront_slot_mask(slot));
      continue;
    }

    if (!hz5_midpagefront_m4_transition(page,
                                        slot,
                                        HZ5_MIDPAGE_SLOT_LIVE,
                                        HZ5_MIDPAGE_SLOT_REMOTE)) {
      continue;
    }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
    hz5_midpagefront_m4_remote_packet_push(page, slot);
#else
    hz5_midpagefront_remote_batch_push(tls, page, ptr);
#endif
  }
  hz5_midpagefront_m6_promote_owner_batch(
      tls, owner_batch, owner_batch_count);
}

static void hz5_midpagefront_m6_defer_free(Hz5MidPageTls* tls, void* ptr) {
  if (!tls || !ptr) {
    return;
  }
  uint16_t count = tls->m6_raw_count;
  if (count >= HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    hz5_midpagefront_m6_flush_raw(tls);
    count = tls->m6_raw_count;
  }
  if (count < HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    tls->m6_raw_free[count] = ptr;
    tls->m6_raw_count = (uint16_t)(count + 1u);
  }
  if (tls->m6_raw_count >= HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    hz5_midpagefront_m6_flush_raw(tls);
  }
}
#endif

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
    drained += hz5_midpagefront_m4_remote_packet_drain_page(
        tls, class_index, page);
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
  hz5_midpagefront_m4_seed_page(tls, class_index, page);
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

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
static int hz5_midpagefront_m4_refill_magazine(Hz5MidPageTls* tls,
                                               uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index)) {
    return 0;
  }
  hz5_midpagefront_m4_stat_inc_class(
      g_hz5_midpagefront_m4_stat_refill_call, class_index);
  if (tls->magazine_count[class_index] != 0u) {
    return 1;
  }

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
  hz5_midpagefront_m6_flush_raw(tls);
  if (tls->magazine_count[class_index] != 0u) {
    return 1;
  }
#endif

  (void)hz5_midpagefront_drain_remote_class(tls, class_index);
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
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
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
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE
    hz5_midpagefront_m4_cache_slot(tls, page->class_index, page, slot);
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
    hz5_midpagefront_m4_cache_slot(tls, page->class_index, page, slot);
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

#endif
