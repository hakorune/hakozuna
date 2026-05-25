#include "hz5_largefront.h"

#include "hz5_config.h"
#include "hz5_internal.h"
#include "hz5_midpagefront.h"
#include "hz5_ownerhub.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
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

#ifndef HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET
#define HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET 0u
#endif

#ifndef BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
#define BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_MAP_BASE_ONLY
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_MAP_BASE_ONLY 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_BASE_FASTMAP
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_BASE_FASTMAP 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_ADAPTIVE128
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_ADAPTIVE128 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
#define BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0 0
#endif

#ifndef HZ5_LARGEFRONT_REMOTE_BATCH_CAP
#define HZ5_LARGEFRONT_REMOTE_BATCH_CAP 16u
#endif

#ifndef HZ5_LARGEFRONT_SCAVENGE_LOCAL_CAP
#define HZ5_LARGEFRONT_SCAVENGE_LOCAL_CAP 4u
#endif

#ifndef HZ5_LARGEFRONT_ADAPTIVE128_MID_BYTES
#define HZ5_LARGEFRONT_ADAPTIVE128_MID_BYTES (320u * 1024u * 1024u)
#endif

#ifndef HZ5_LARGEFRONT_ADAPTIVE128_HIGH_BYTES
#define HZ5_LARGEFRONT_ADAPTIVE128_HIGH_BYTES (512u * 1024u * 1024u)
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LARGEFRONT_L1

#define HZ5_LARGEFRONT_MAGIC UINT64_C(0x485A354C41524731)
#define HZ5_LARGEFRONT_SPAN_PAYLOAD_SCAVENGED 0x0001u
#define HZ5_LARGEFRONT_PAGE_SIZE ((size_t)4096)
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES
#define HZ5_LARGEFRONT_CLASS_COUNT 8u
#else
#define HZ5_LARGEFRONT_CLASS_COUNT 4u
#endif

#ifndef HZ5_LARGEFRONT_MAP_BITS
#define HZ5_LARGEFRONT_MAP_BITS 22u
#endif

#define HZ5_LARGEFRONT_MAP_CAP ((size_t)1u << HZ5_LARGEFRONT_MAP_BITS)

#ifndef HZ5_LARGEFRONT_REGION_BUCKET_BITS
#define HZ5_LARGEFRONT_REGION_BUCKET_BITS 16u
#endif

#define HZ5_LARGEFRONT_REGION_BUCKET_CAP \
  ((size_t)1u << HZ5_LARGEFRONT_REGION_BUCKET_BITS)

#ifndef HZ5_LARGEFRONT_REGION_GRAN_BITS
#define HZ5_LARGEFRONT_REGION_GRAN_BITS 21u
#endif

#ifndef HZ5_LARGEFRONT_REGION_CAP
#define HZ5_LARGEFRONT_REGION_CAP 65536u
#endif

#ifndef HZ5_LARGEFRONT_REGION_LINK_CAP
#define HZ5_LARGEFRONT_REGION_LINK_CAP 262144u
#endif

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

typedef struct Hz5LargeRegion {
  uintptr_t base;
  uintptr_t end;
  uint32_t class_index;
  uint32_t span_count;
  size_t stride;
} Hz5LargeRegion;

typedef struct Hz5LargeRegionLink {
  uintptr_t key;
  const Hz5LargeRegion* region;
  struct Hz5LargeRegionLink* next;
} Hz5LargeRegionLink;

typedef struct Hz5LargeTls {
  Hz5OwnerToken owner;
  Hz5LargeSpan* free_head[HZ5_LARGEFRONT_CLASS_COUNT];
  uint32_t free_count[HZ5_LARGEFRONT_CLASS_COUNT];
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
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES
    8192u, 16384u, 32768u, 65536u,
#endif
    131072u, 262144u, 524288u, 1048576u};

static const Hz5OwnerToken k_hz5_largefront_no_owner = {0, 0};

static Hz5LargeMapEntry g_hz5_largefront_map[HZ5_LARGEFRONT_MAP_CAP];
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
static Hz5LargeRegion g_hz5_largefront_regions[HZ5_LARGEFRONT_REGION_CAP];
static Hz5LargeRegionLink
    g_hz5_largefront_region_links[HZ5_LARGEFRONT_REGION_LINK_CAP];
static _Atomic(Hz5LargeRegionLink*)
    g_hz5_largefront_region_buckets[HZ5_LARGEFRONT_REGION_BUCKET_CAP];
static size_t g_hz5_largefront_region_count;
static size_t g_hz5_largefront_region_link_count;
static pthread_mutex_t g_hz5_largefront_region_lock =
    PTHREAD_MUTEX_INITIALIZER;
#endif
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
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_ADAPTIVE128
// Diagnostic-only L3 policy state. This is intentionally updated only while
// holding the source lock; do not move it into malloc/free hot paths.
static size_t g_hz5_largefront_mapped_spans[HZ5_LARGEFRONT_CLASS_COUNT];
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
// Heavy diagnostic lane: these counters are touched from malloc/free paths.
// Keep this separate from Policy-L0 so speed lanes never inherit hot counters.
static _Atomic uint64_t
    g_hz5_largefront_obs_alloc_calls[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_local_pop_hit[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_remote_take_first[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_remote_to_local[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_global_pop_hit[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_new_span[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_free_local[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_free_remote[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_free_global[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_refill[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_spans[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_batch4[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_batch8[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_batch16[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_batch_other[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_local_free_highwater[HZ5_LARGEFRONT_CLASS_COUNT];
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
// Control-plane observation lane: only source refill, remote flush, and owner
// drain slow paths update these counters. This is the safe base for future
// policy selection; it is not a speed lane.
static _Atomic uint64_t
    g_hz5_largefront_policy_source_empty[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_refill[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_spans[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_batch4[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_batch8[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_batch16[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_batch_other[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_mapped_spans[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_remote_flush[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_remote_flush_spans[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_remote_flush_cap_hit[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_drain[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_drain_spans[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_take_first[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_to_local[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_republish[HZ5_LARGEFRONT_CLASS_COUNT];
#endif

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
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES
  if (size <= 4096u || size > 1048576u) {
    return -1;
  }
#else
  if (size <= 65536u || size > 1048576u) {
    return -1;
  }
#endif
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

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE || \
    BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
static void hz5_largefront_counter_inc(_Atomic uint64_t* counter) {
  atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
}

static void hz5_largefront_counter_add(_Atomic uint64_t* counter,
                                       uint64_t value) {
  atomic_fetch_add_explicit(counter, value, memory_order_relaxed);
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
static void hz5_largefront_obs_max(_Atomic uint64_t* counter, uint64_t value) {
  uint64_t old = atomic_load_explicit(counter, memory_order_relaxed);
  while (old < value &&
         !atomic_compare_exchange_weak_explicit(counter,
                                                &old,
                                                value,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
}

static void hz5_largefront_obs_note_source_refill(uint32_t class_index,
                                                  uint32_t batch_count) {
  if (!hz5_largefront_class_valid(class_index)) {
    return;
  }
  hz5_largefront_counter_inc(
      &g_hz5_largefront_obs_source_refill[class_index]);
  hz5_largefront_counter_add(&g_hz5_largefront_obs_source_spans[class_index],
                             batch_count);
  if (batch_count == 4u) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_obs_source_batch4[class_index]);
  } else if (batch_count == 8u) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_obs_source_batch8[class_index]);
  } else if (batch_count == 16u) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_obs_source_batch16[class_index]);
  } else {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_obs_source_batch_other[class_index]);
  }
}

__attribute__((destructor)) static void hz5_largefront_obs_dump(void) {
  if (!getenv("HZ5_LARGEFRONT_OBSERVE")) {
    return;
  }
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_CLASS_COUNT; ++i) {
    fprintf(stderr,
            "[HZ5_LARGEFRONT_OBSERVE] class=%u bytes=%u"
            " alloc_calls=%llu local_pop_hit=%llu remote_take_first=%llu"
            " remote_to_local=%llu global_pop_hit=%llu new_span=%llu"
            " free_local=%llu free_remote=%llu free_global=%llu"
            " source_refill=%llu source_spans=%llu"
            " source_batch4=%llu source_batch8=%llu"
            " source_batch16=%llu source_batch_other=%llu"
            " local_free_highwater=%llu\n",
            i,
            hz5_largefront_class_bytes(i),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_alloc_calls[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_local_pop_hit[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_remote_take_first[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_remote_to_local[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_global_pop_hit[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_new_span[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_free_local[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_free_remote[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_free_global[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_refill[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_spans[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_batch4[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_batch8[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_batch16[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_batch_other[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_local_free_highwater[i],
                memory_order_relaxed));
  }
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
static void hz5_largefront_policy_note_source_empty(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return;
  }
  hz5_largefront_counter_inc(
      &g_hz5_largefront_policy_source_empty[class_index]);
}

static void hz5_largefront_policy_note_source_refill(uint32_t class_index,
                                                     uint32_t batch_count) {
  if (!hz5_largefront_class_valid(class_index)) {
    return;
  }
  hz5_largefront_counter_inc(
      &g_hz5_largefront_policy_source_refill[class_index]);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_source_spans[class_index], batch_count);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_mapped_spans[class_index], batch_count);
  if (batch_count == 4u) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_source_batch4[class_index]);
  } else if (batch_count == 8u) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_source_batch8[class_index]);
  } else if (batch_count == 16u) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_source_batch16[class_index]);
  } else {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_source_batch_other[class_index]);
  }
}

static void hz5_largefront_policy_note_remote_flush(uint32_t class_index,
                                                    uint32_t count) {
  if (!hz5_largefront_class_valid(class_index) || count == 0u) {
    return;
  }
  hz5_largefront_counter_inc(
      &g_hz5_largefront_policy_remote_flush[class_index]);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_remote_flush_spans[class_index], count);
  if (count >= HZ5_LARGEFRONT_REMOTE_BATCH_CAP) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_remote_flush_cap_hit[class_index]);
  }
}

static void hz5_largefront_policy_note_owner_drain(uint32_t class_index,
                                                   uint32_t spans,
                                                   uint32_t take_first,
                                                   uint32_t to_local,
                                                   uint32_t republished) {
  if (!hz5_largefront_class_valid(class_index) || spans == 0u) {
    return;
  }
  hz5_largefront_counter_inc(
      &g_hz5_largefront_policy_owner_drain[class_index]);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_drain_spans[class_index], spans);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_take_first[class_index], take_first);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_to_local[class_index], to_local);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_republish[class_index], republished);
}

__attribute__((destructor)) static void hz5_largefront_policy_l0_dump(void) {
  if (!getenv("HZ5_LARGEFRONT_POLICY_L0")) {
    return;
  }
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_CLASS_COUNT; ++i) {
    fprintf(stderr,
            "[HZ5_LARGEFRONT_POLICY_L0] class=%u bytes=%u"
            " source_empty=%llu source_refill=%llu source_spans=%llu"
            " source_batch4=%llu source_batch8=%llu"
            " source_batch16=%llu source_batch_other=%llu"
            " mapped_spans_proxy=%llu"
            " remote_flush=%llu remote_flush_spans=%llu"
            " remote_flush_cap_hit=%llu"
            " owner_drain=%llu owner_drain_spans=%llu"
            " owner_take_first=%llu owner_to_local=%llu"
            " owner_republish=%llu\n",
            i,
            hz5_largefront_class_bytes(i),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_empty[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_refill[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_spans[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_batch4[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_batch8[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_batch16[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_batch_other[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_mapped_spans[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_remote_flush[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_remote_flush_spans[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_remote_flush_cap_hit[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_drain[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_drain_spans[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_take_first[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_to_local[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_republish[i],
                memory_order_relaxed));
  }
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
static int hz5_largefront_payload_scavenge_class(uint32_t class_index) {
  return hz5_largefront_class_valid(class_index) &&
         hz5_largefront_class_bytes(class_index) == 131072u;
}

static void hz5_largefront_payload_scavenge(Hz5LargeSpan* span) {
  if (!span || !span->base || span->class_bytes == 0u ||
      (span->flags & HZ5_LARGEFRONT_SPAN_PAYLOAD_SCAVENGED)) {
    return;
  }
  (void)madvise(span->base, (size_t)span->class_bytes, MADV_DONTNEED);
  span->flags |= HZ5_LARGEFRONT_SPAN_PAYLOAD_SCAVENGED;
}

static void hz5_largefront_payload_reactivate(Hz5LargeSpan* span) {
  if (!span) {
    return;
  }
  span->flags &= (uint16_t)~HZ5_LARGEFRONT_SPAN_PAYLOAD_SCAVENGED;
}
#else
static void hz5_largefront_payload_reactivate(Hz5LargeSpan* span) {
  (void)span;
}
#endif

static size_t hz5_largefront_hash(uintptr_t page_base) {
  uintptr_t x = page_base >> 12;
  x ^= x >> HZ5_LARGEFRONT_MAP_BITS;
  x ^= x >> (HZ5_LARGEFRONT_MAP_BITS * 2u);
  return (size_t)x & (HZ5_LARGEFRONT_MAP_CAP - 1u);
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
static size_t hz5_largefront_region_hash(uintptr_t key) {
  uintptr_t x = key;
  x ^= x >> HZ5_LARGEFRONT_REGION_BUCKET_BITS;
  x ^= x >> (HZ5_LARGEFRONT_REGION_BUCKET_BITS * 2u);
  return (size_t)x & (HZ5_LARGEFRONT_REGION_BUCKET_CAP - 1u);
}

static int hz5_largefront_region_register(void* block,
                                          size_t bytes,
                                          uint32_t class_index,
                                          uint32_t span_count) {
  if (!block || bytes == 0 || !hz5_largefront_class_valid(class_index)) {
    return 0;
  }

  uintptr_t base = (uintptr_t)block;
  uintptr_t end = base + (uintptr_t)bytes;
  if (end <= base) {
    return 0;
  }

  uintptr_t first_key = base >> HZ5_LARGEFRONT_REGION_GRAN_BITS;
  uintptr_t last_key = (end - 1u) >> HZ5_LARGEFRONT_REGION_GRAN_BITS;
  uintptr_t link_count = last_key - first_key + 1u;

  pthread_mutex_lock(&g_hz5_largefront_region_lock);
  if (g_hz5_largefront_region_count >= HZ5_LARGEFRONT_REGION_CAP ||
      link_count >
          (uintptr_t)(HZ5_LARGEFRONT_REGION_LINK_CAP -
                      g_hz5_largefront_region_link_count)) {
    pthread_mutex_unlock(&g_hz5_largefront_region_lock);
    return 0;
  }

  Hz5LargeRegion* region =
      &g_hz5_largefront_regions[g_hz5_largefront_region_count++];
  region->base = base;
  region->end = end;
  region->class_index = class_index;
  region->span_count = span_count;
  region->stride = hz5_largefront_span_stride(class_index);

  for (uintptr_t key = first_key; key <= last_key; ++key) {
    Hz5LargeRegionLink* link =
        &g_hz5_largefront_region_links[g_hz5_largefront_region_link_count++];
    link->key = key;
    link->region = region;
    size_t bucket = hz5_largefront_region_hash(key);
    Hz5LargeRegionLink* old_head = atomic_load_explicit(
        &g_hz5_largefront_region_buckets[bucket], memory_order_acquire);
    link->next = old_head;
    atomic_store_explicit(&g_hz5_largefront_region_buckets[bucket],
                          link,
                          memory_order_release);
  }
  pthread_mutex_unlock(&g_hz5_largefront_region_lock);
  return 1;
}

static Hz5LargeSpan* hz5_largefront_lookup_region_ptr(uintptr_t p) {
  uintptr_t key = p >> HZ5_LARGEFRONT_REGION_GRAN_BITS;
  size_t bucket = hz5_largefront_region_hash(key);
  Hz5LargeRegionLink* link = atomic_load_explicit(
      &g_hz5_largefront_region_buckets[bucket], memory_order_acquire);

  while (link) {
    const Hz5LargeRegion* region = link->region;
    if (link->key == key && region && p >= region->base && p < region->end) {
      uintptr_t offset = p - region->base;
      size_t stride = region->stride;
      if (stride == 0) {
        return NULL;
      }
      uintptr_t span_index = offset / (uintptr_t)stride;
      uintptr_t span_offset = offset % (uintptr_t)stride;
      if (span_index >= region->span_count ||
          span_offset < HZ5_LARGEFRONT_PAGE_SIZE) {
        return NULL;
      }
      Hz5LargeSpan* span =
          (Hz5LargeSpan*)(region->base + span_index * (uintptr_t)stride);
      if (span->magic != HZ5_LARGEFRONT_MAGIC ||
          span->class_index != region->class_index) {
        return NULL;
      }
      return span;
    }
    link = link->next;
  }
  return NULL;
}
#endif

static uint32_t hz5_largefront_source_refill_count_locked(
    uint32_t class_index) {
  uint32_t batch_count = HZ5_LARGEFRONT_SOURCE_BATCH_COUNT;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_ADAPTIVE128
  if (hz5_largefront_class_valid(class_index) &&
      hz5_largefront_class_bytes(class_index) == 131072u) {
    size_t mapped_bytes = g_hz5_largefront_mapped_spans[class_index] *
                          hz5_largefront_span_stride(class_index);
    if (mapped_bytes >= HZ5_LARGEFRONT_ADAPTIVE128_HIGH_BYTES) {
      batch_count = 4u;
    } else if (mapped_bytes >= HZ5_LARGEFRONT_ADAPTIVE128_MID_BYTES) {
      batch_count = 8u;
    }
  }
#else
  (void)class_index;
#endif
  return batch_count == 0u ? 1u : batch_count;
}

static int hz5_largefront_source_refill_locked(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return 0;
  }
  size_t stride = hz5_largefront_span_stride(class_index);
  uint32_t batch_count = hz5_largefront_source_refill_count_locked(class_index);
  if (stride == 0 ||
      batch_count > SIZE_MAX / stride) {
    return 0;
  }
  size_t bytes = stride * (size_t)batch_count;
  void* block = mmap(NULL,
                     bytes,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1,
                     0);
  if (block == MAP_FAILED) {
    return 0;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
  if (!hz5_largefront_region_register(block, bytes, class_index, batch_count)) {
    (void)munmap(block, bytes);
    return 0;
  }
#endif

  uintptr_t base = (uintptr_t)block;
  for (uint32_t i = 0; i < batch_count; ++i) {
    Hz5LargeRawNode* node = (Hz5LargeRawNode*)(base + (uintptr_t)i * stride);
    node->next = g_hz5_largefront_source_free[class_index];
    g_hz5_largefront_source_free[class_index] = node;
  }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  hz5_largefront_obs_note_source_refill(class_index, batch_count);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  hz5_largefront_policy_note_source_refill(class_index, batch_count);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_ADAPTIVE128
  g_hz5_largefront_mapped_spans[class_index] += (size_t)batch_count;
#endif
  return 1;
}

static void* hz5_largefront_source_alloc_raw(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_largefront_source_lock);
  if (!g_hz5_largefront_source_free[class_index]) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
    hz5_largefront_policy_note_source_empty(class_index);
#endif
    if (!hz5_largefront_source_refill_locked(class_index)) {
      pthread_mutex_unlock(&g_hz5_largefront_source_lock);
      return NULL;
    }
  }
  if (!g_hz5_largefront_source_free[class_index]) {
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
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
  // Region lookup remains the ownership fallback. With the base fast map also
  // enabled, keep a best-effort exact-base map so normal free(ptr) avoids the
  // variable stride division in the region path.
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_BASE_FASTMAP
  if (span && span->base) {
    uintptr_t base = (uintptr_t)span->base;
    pthread_mutex_lock(&g_hz5_largefront_map_lock);
    (void)hz5_largefront_map_insert_one(base, span);
    pthread_mutex_unlock(&g_hz5_largefront_map_lock);
  }
#else
  (void)span;
#endif
  return 1;
#else
  uintptr_t base = (uintptr_t)span->base;
  pthread_mutex_lock(&g_hz5_largefront_map_lock);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_MAP_BASE_ONLY
  if (!hz5_largefront_map_insert_one(base, span)) {
    pthread_mutex_unlock(&g_hz5_largefront_map_lock);
    return 0;
  }
#else
  for (uint32_t i = 0; i < span->page_count; ++i) {
    uintptr_t page_base = base + ((uintptr_t)i * HZ5_LARGEFRONT_PAGE_SIZE);
    if (!hz5_largefront_map_insert_one(page_base, span)) {
      pthread_mutex_unlock(&g_hz5_largefront_map_lock);
      return 0;
    }
  }
#endif
  pthread_mutex_unlock(&g_hz5_largefront_map_lock);
  return 1;
#endif
}

static Hz5LargeSpan* hz5_largefront_span_for_ptr(void* ptr) {
  if (!ptr) {
    return NULL;
  }
  uintptr_t p = (uintptr_t)ptr;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_BASE_FASTMAP
  uintptr_t page_base = p & ~(uintptr_t)(HZ5_LARGEFRONT_PAGE_SIZE - 1u);
  Hz5LargeSpan* base_span = hz5_largefront_lookup_page(page_base);
  if (base_span && base_span->magic == HZ5_LARGEFRONT_MAGIC &&
      ptr == base_span->base) {
    return base_span;
  }
#endif
  Hz5LargeSpan* span = hz5_largefront_lookup_region_ptr(p);
#else
  uintptr_t page_base = p & ~(uintptr_t)(HZ5_LARGEFRONT_PAGE_SIZE - 1u);
  Hz5LargeSpan* span = hz5_largefront_lookup_page(page_base);
#endif
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
  ++tls->free_count[class_index];
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  hz5_largefront_obs_max(&g_hz5_largefront_obs_local_free_highwater[class_index],
                         tls->free_count[class_index]);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
  if (hz5_largefront_payload_scavenge_class(class_index) &&
      tls->free_count[class_index] > HZ5_LARGEFRONT_SCAVENGE_LOCAL_CAP) {
    hz5_largefront_payload_scavenge(span);
  }
#endif
}

static Hz5LargeSpan* hz5_largefront_local_pop(Hz5LargeTls* tls,
                                              uint32_t class_index) {
  Hz5LargeSpan* span = tls->free_head[class_index];
  if (!span) {
    return NULL;
  }
  tls->free_head[class_index] = span->next;
  if (tls->free_count[class_index] != 0u) {
    --tls->free_count[class_index];
  }
  span->next = NULL;
  return span;
}

static void hz5_largefront_global_push(uint32_t class_index,
                                       Hz5LargeSpan* span) {
  if (!hz5_largefront_class_valid(class_index) || !span) {
    return;
  }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
  if (hz5_largefront_payload_scavenge_class(class_index)) {
    hz5_largefront_payload_scavenge(span);
  }
#endif
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
  hz5_largefront_payload_reactivate(span);
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
  hz5_largefront_payload_reactivate(span);
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

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  hz5_largefront_policy_note_remote_flush(tls->remote_batch_class,
                                          tls->remote_batch_count);
#endif
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
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  uint32_t policy_take_first = 0;
  uint32_t policy_to_local = 0;
  uint32_t policy_republished = 0;
#endif
  while (head) {
    Hz5LargeSpan* span = (Hz5LargeSpan*)head;
    head = span->next;
    if (budget != 0u && drained >= budget) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      uint32_t republished = 1;
#endif
      Hz5LargeSpan* tail = span;
      while (tail->next) {
        tail = tail->next;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
        ++republished;
#endif
      }
      (void)hz5_largefront_remote_publish_list(tls->owner,
                                               class_index,
                                               span,
                                               tail);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      policy_republished += republished;
      hz5_largefront_policy_note_owner_drain(class_index,
                                             drained + policy_take_first +
                                                 policy_republished,
                                             policy_take_first,
                                             policy_to_local,
                                             policy_republished);
#endif
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
      hz5_largefront_payload_reactivate(span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_remote_take_first[class_index]);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      ++policy_take_first;
#endif
      taken = span;
      continue;
    }
#endif
    if (hz5_largefront_state_cas(span,
                                 (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                 (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
      hz5_largefront_local_push(tls, class_index, span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_remote_to_local[class_index]);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      ++policy_to_local;
#endif
    }
    ++drained;
  }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  hz5_largefront_policy_note_owner_drain(class_index,
                                         drained + policy_take_first,
                                         policy_take_first,
                                         policy_to_local,
                                         policy_republished);
#endif
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
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  hz5_largefront_counter_inc(&g_hz5_largefront_obs_alloc_calls[ci]);
#endif
  Hz5LargeSpan* span = hz5_largefront_local_pop(tls, ci);
  if (span) {
    if (hz5_largefront_activate_local_for_owner(tls, span)) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(&g_hz5_largefront_obs_local_pop_hit[ci]);
#endif
      return span->base;
    }
    return NULL;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
  hz5_ownerhub_note_alloc_miss(tls->owner, HZ5_OWNERHUB_FRONT_LARGE, ci);
#if HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET > 0u
  span = hz5_largefront_drain_remote_class_budget(
      tls,
      ci,
      HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
      NULL);
#else
  span = hz5_largefront_drain_remote_class(tls, ci);
#endif
  if (span) {
    return span->base;
  }
  hz5_ownerhub_drain_cross_fronts(tls->owner, HZ5_OWNERHUB_FRONT_LARGE);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
  hz5_midpagefront_owner_drain_some(2u);
#endif
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
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(&g_hz5_largefront_obs_global_pop_hit[ci]);
#endif
      return span->base;
    }
  }

  span = hz5_largefront_new_span(tls, ci);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  if (span) {
    hz5_largefront_counter_inc(&g_hz5_largefront_obs_new_span[ci]);
  }
#endif
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
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
    hz5_largefront_counter_inc(
        &g_hz5_largefront_obs_free_local[span->class_index]);
#endif
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
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_free_remote[span->class_index]);
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
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
    hz5_largefront_counter_inc(
        &g_hz5_largefront_obs_free_global[span->class_index]);
#endif
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
