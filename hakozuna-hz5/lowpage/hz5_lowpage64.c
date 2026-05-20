#include "hz5_lowpage64.h"

#include <malloc.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#ifndef HZ5_LOWPAGE64_RAW_PAGE_SIZE
#define HZ5_LOWPAGE64_RAW_PAGE_SIZE ((size_t)65536)
#endif

#ifndef HZ5_LOWPAGE64_BATCH_FLUSH_N
#ifdef BENCHLAB_HZ5_P25_BATCH_FLUSH_N
#define HZ5_LOWPAGE64_BATCH_FLUSH_N BENCHLAB_HZ5_P25_BATCH_FLUSH_N
#else
#define HZ5_LOWPAGE64_BATCH_FLUSH_N 8u
#endif
#endif

#ifndef HZ5_LOWPAGE64_GLOBAL_CAP
#ifdef BENCHLAB_HZ5_P25_GLOBAL_CAP
#define HZ5_LOWPAGE64_GLOBAL_CAP BENCHLAB_HZ5_P25_GLOBAL_CAP
#else
#define HZ5_LOWPAGE64_GLOBAL_CAP 256u
#endif
#endif

#ifndef HZ5_LOWPAGE64_SPAN_CAP
#ifdef BENCHLAB_HZ5_P25_SPAN_CAP
#define HZ5_LOWPAGE64_SPAN_CAP BENCHLAB_HZ5_P25_SPAN_CAP
#else
#define HZ5_LOWPAGE64_SPAN_CAP 32u
#endif
#endif

#ifndef HZ5_LOWPAGE64_STASH_CAP
#ifdef BENCHLAB_HZ5_P25_STASH_CAP
#define HZ5_LOWPAGE64_STASH_CAP BENCHLAB_HZ5_P25_STASH_CAP
#else
#define HZ5_LOWPAGE64_STASH_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP
#ifdef BENCHLAB_HZ5_P25_ADAPTIVE_STASH_CAP
#define HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP \
  BENCHLAB_HZ5_P25_ADAPTIVE_STASH_CAP
#else
#define HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_ADAPTIVE_STASH_TRIGGER
#ifdef BENCHLAB_HZ5_P25_ADAPTIVE_STASH_TRIGGER
#define HZ5_LOWPAGE64_ADAPTIVE_STASH_TRIGGER \
  BENCHLAB_HZ5_P25_ADAPTIVE_STASH_TRIGGER
#else
#define HZ5_LOWPAGE64_ADAPTIVE_STASH_TRIGGER 192u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P37_HOT_CAP
#ifdef BENCHLAB_HZ5_P37_HOT_CAP
#define HZ5_LOWPAGE64_P37_HOT_CAP BENCHLAB_HZ5_P37_HOT_CAP
#else
#define HZ5_LOWPAGE64_P37_HOT_CAP 128u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P37_OVERFLOW_CAP
#ifdef BENCHLAB_HZ5_P37_OVERFLOW_CAP
#define HZ5_LOWPAGE64_P37_OVERFLOW_CAP BENCHLAB_HZ5_P37_OVERFLOW_CAP
#else
#define HZ5_LOWPAGE64_P37_OVERFLOW_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME
#ifdef BENCHLAB_HZ5_P37_OVERFLOW_LIFETIME
#define HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME \
  BENCHLAB_HZ5_P37_OVERFLOW_LIFETIME
#else
#define HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME 1u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P37_HIGH_STREAK
#ifdef BENCHLAB_HZ5_P37_HIGH_STREAK
#define HZ5_LOWPAGE64_P37_HIGH_STREAK BENCHLAB_HZ5_P37_HIGH_STREAK
#else
#define HZ5_LOWPAGE64_P37_HIGH_STREAK 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P37_HIGH_WATER
#ifdef BENCHLAB_HZ5_P37_HIGH_WATER
#define HZ5_LOWPAGE64_P37_HIGH_WATER BENCHLAB_HZ5_P37_HIGH_WATER
#else
#define HZ5_LOWPAGE64_P37_HIGH_WATER HZ5_LOWPAGE64_P37_OVERFLOW_CAP
#endif
#endif

#ifndef HZ5_LOWPAGE64_P39_HOT_CAP
#ifdef BENCHLAB_HZ5_P39_HOT_CAP
#define HZ5_LOWPAGE64_P39_HOT_CAP BENCHLAB_HZ5_P39_HOT_CAP
#else
#define HZ5_LOWPAGE64_P39_HOT_CAP 128u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P39_PROBATION_CAP
#ifdef BENCHLAB_HZ5_P39_PROBATION_CAP
#define HZ5_LOWPAGE64_P39_PROBATION_CAP BENCHLAB_HZ5_P39_PROBATION_CAP
#else
#define HZ5_LOWPAGE64_P39_PROBATION_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P39_PROBATION_LIFETIME
#ifdef BENCHLAB_HZ5_P39_PROBATION_LIFETIME
#define HZ5_LOWPAGE64_P39_PROBATION_LIFETIME \
  BENCHLAB_HZ5_P39_PROBATION_LIFETIME
#else
#define HZ5_LOWPAGE64_P39_PROBATION_LIFETIME 1u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P39_USE_ON_MISS
#ifdef BENCHLAB_HZ5_P39_USE_ON_MISS
#define HZ5_LOWPAGE64_P39_USE_ON_MISS BENCHLAB_HZ5_P39_USE_ON_MISS
#else
#define HZ5_LOWPAGE64_P39_USE_ON_MISS 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP
#ifdef BENCHLAB_HZ5_P40_GLOBAL_SOFT_CAP
#define HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP \
  BENCHLAB_HZ5_P40_GLOBAL_SOFT_CAP
#else
#define HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP
#ifdef BENCHLAB_HZ5_P40_GLOBAL_HARD_CAP
#define HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP \
  BENCHLAB_HZ5_P40_GLOBAL_HARD_CAP
#else
#define HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P40_TTL_EPOCHS
#ifdef BENCHLAB_HZ5_P40_TTL_EPOCHS
#define HZ5_LOWPAGE64_P40_TTL_EPOCHS BENCHLAB_HZ5_P40_TTL_EPOCHS
#else
#define HZ5_LOWPAGE64_P40_TTL_EPOCHS 1u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P40_RELEASE_BATCH
#ifdef BENCHLAB_HZ5_P40_RELEASE_BATCH
#define HZ5_LOWPAGE64_P40_RELEASE_BATCH BENCHLAB_HZ5_P40_RELEASE_BATCH
#else
#define HZ5_LOWPAGE64_P40_RELEASE_BATCH 16u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P42_VA_SOURCE
#ifdef BENCHLAB_HZ5_P42_VA_SOURCE
#define HZ5_LOWPAGE64_P42_VA_SOURCE BENCHLAB_HZ5_P42_VA_SOURCE
#else
#define HZ5_LOWPAGE64_P42_VA_SOURCE 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_SPAN_CACHE
#ifdef BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
#define HZ5_LOWPAGE64_SPAN_CACHE BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
#else
#define HZ5_LOWPAGE64_SPAN_CACHE 0
#endif
#endif

#ifndef HZ5_DIAGNOSTIC_STATS
#define HZ5_DIAGNOSTIC_STATS 0
#endif

#ifndef HZ5_LOWPAGE64_STATS
#ifdef BENCHLAB_HZ5_P25_STATS
#define HZ5_LOWPAGE64_STATS BENCHLAB_HZ5_P25_STATS
#else
#define HZ5_LOWPAGE64_STATS HZ5_DIAGNOSTIC_STATS
#endif
#endif

#if HZ5_LOWPAGE64_STATS
#define HZ5_LOWPAGE64_COUNT_ADD(counter, value) \
  atomic_fetch_add_explicit(&(counter), (value), memory_order_relaxed)
static void hz5_lowpage64_count_max(_Atomic size_t* counter, size_t value) {
  size_t cur = atomic_load_explicit(counter, memory_order_relaxed);
  while (value > cur &&
         !atomic_compare_exchange_weak_explicit(
             counter, &cur, value, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}
#define HZ5_LOWPAGE64_COUNT_MAX(counter, value) \
  hz5_lowpage64_count_max(&(counter), (value))
#else
#define HZ5_LOWPAGE64_COUNT_ADD(counter, value) ((void)0)
#define HZ5_LOWPAGE64_COUNT_MAX(counter, value) ((void)0)
#endif

static _Atomic(uintptr_t) g_hz5_lowpage64_global_batch_head;
static _Atomic size_t g_hz5_lowpage64_global_batch_count;
static _Atomic(uintptr_t) g_hz5_lowpage64_raw_min;
static _Atomic(uintptr_t) g_hz5_lowpage64_raw_max;
#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static _Atomic uint32_t g_hz5_lowpage64_p40_global_epoch;
#endif

#if HZ5_LOWPAGE64_STATS
static _Atomic size_t g_hz5_lowpage64_alloc_calls;
static _Atomic size_t g_hz5_lowpage64_span_hits;
static _Atomic size_t g_hz5_lowpage64_span_pushes;
static _Atomic size_t g_hz5_lowpage64_span_full;
static _Atomic size_t g_hz5_lowpage64_stash_hits;
static _Atomic size_t g_hz5_lowpage64_global_batch_hits;
static _Atomic size_t g_hz5_lowpage64_os_allocs;
static _Atomic size_t g_hz5_lowpage64_free_calls;
static _Atomic size_t g_hz5_lowpage64_relbuf_pushes;
static _Atomic size_t g_hz5_lowpage64_batch_publishes;
static _Atomic size_t g_hz5_lowpage64_batch_publish_nodes;
static _Atomic size_t g_hz5_lowpage64_global_overflow_frees;
static _Atomic size_t g_hz5_lowpage64_relbuf_count_max;
static _Atomic size_t g_hz5_lowpage64_global_count_before_max;
static _Atomic size_t g_hz5_lowpage64_global_count_after_max;
static _Atomic size_t g_hz5_lowpage64_acquired_count_total;
static _Atomic size_t g_hz5_lowpage64_acquired_count_max;
static _Atomic size_t g_hz5_lowpage64_stash_set_calls;
static _Atomic size_t g_hz5_lowpage64_stash_set_nodes_total;
static _Atomic size_t g_hz5_lowpage64_stash_set_nodes_max;
static _Atomic size_t g_hz5_lowpage64_trim_calls;
static _Atomic size_t g_hz5_lowpage64_trim_freed_nodes;
static _Atomic size_t g_hz5_lowpage64_adaptive_trigger_calls;
static _Atomic size_t g_hz5_lowpage64_acquired_le64;
static _Atomic size_t g_hz5_lowpage64_acquired_le128;
static _Atomic size_t g_hz5_lowpage64_acquired_le192;
static _Atomic size_t g_hz5_lowpage64_acquired_gt192;
static _Atomic size_t g_hz5_lowpage64_stash_nodes_le64;
static _Atomic size_t g_hz5_lowpage64_stash_nodes_le128;
static _Atomic size_t g_hz5_lowpage64_stash_nodes_le192;
static _Atomic size_t g_hz5_lowpage64_stash_nodes_gt192;
static _Atomic size_t g_hz5_lowpage64_p37_overflow_hits;
static _Atomic size_t g_hz5_lowpage64_p37_overflow_set_nodes_total;
static _Atomic size_t g_hz5_lowpage64_p37_overflow_set_nodes_max;
static _Atomic size_t g_hz5_lowpage64_p37_overflow_release_calls;
static _Atomic size_t g_hz5_lowpage64_p37_overflow_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p37_high_event_releases;
static _Atomic size_t g_hz5_lowpage64_p37_lifetime_releases;
static _Atomic size_t g_hz5_lowpage64_p38_hot_hits;
static _Atomic size_t g_hz5_lowpage64_p38_global_first_hits;
static _Atomic size_t g_hz5_lowpage64_p38_set_hot_nodes_total;
static _Atomic size_t g_hz5_lowpage64_p38_set_hot_nodes_max;
static _Atomic size_t g_hz5_lowpage64_p38_global_cap_release_calls;
static _Atomic size_t g_hz5_lowpage64_p38_global_cap_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p38_stash_trim_release_calls;
static _Atomic size_t g_hz5_lowpage64_p38_stash_trim_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p38_p37_release_calls;
static _Atomic size_t g_hz5_lowpage64_p38_p37_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p39_probation_hits;
static _Atomic size_t g_hz5_lowpage64_p39_probation_set_nodes_total;
static _Atomic size_t g_hz5_lowpage64_p39_probation_set_nodes_max;
static _Atomic size_t g_hz5_lowpage64_p39_probation_release_calls;
static _Atomic size_t g_hz5_lowpage64_p39_probation_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p39_probation_lifetime_releases;
static _Atomic size_t g_hz5_lowpage64_p39_probation_replace_releases;
static _Atomic size_t g_hz5_lowpage64_p40_checkpoint_calls;
static _Atomic size_t g_hz5_lowpage64_p40_release_calls;
static _Atomic size_t g_hz5_lowpage64_p40_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p40_release_age_nodes;
static _Atomic size_t g_hz5_lowpage64_p40_release_hard_nodes;
static _Atomic size_t g_hz5_lowpage64_p40_keep_nodes;
static _Atomic size_t g_hz5_lowpage64_p42_va_allocs;
static _Atomic size_t g_hz5_lowpage64_p42_va_alloc_failures;
static _Atomic size_t g_hz5_lowpage64_p42_va_releases;
#endif

#if HZ5_LOWPAGE64_SPAN_CACHE
__declspec(thread) static void* g_hz5_lowpage64_span_head;
__declspec(thread) static size_t g_hz5_lowpage64_span_count;
#endif
__declspec(thread) static void* g_hz5_lowpage64_stash_head;
__declspec(thread) static size_t g_hz5_lowpage64_stash_count;
#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
__declspec(thread) static void* g_hz5_lowpage64_overflow_head;
__declspec(thread) static size_t g_hz5_lowpage64_overflow_count;
__declspec(thread) static uint32_t g_hz5_lowpage64_epoch;
__declspec(thread) static uint32_t g_hz5_lowpage64_overflow_epoch;
__declspec(thread) static uint8_t g_hz5_lowpage64_high_event_streak;
#endif
#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0
__declspec(thread) static void* g_hz5_lowpage64_probation_head;
__declspec(thread) static size_t g_hz5_lowpage64_probation_count;
__declspec(thread) static uint32_t g_hz5_lowpage64_p39_epoch;
__declspec(thread) static uint32_t g_hz5_lowpage64_probation_epoch;
#endif
__declspec(thread) static void* g_hz5_lowpage64_relbuf_head;
__declspec(thread) static void* g_hz5_lowpage64_relbuf_tail;
__declspec(thread) static size_t g_hz5_lowpage64_relbuf_count;

static void hz5_lowpage64_link_next(void* raw, void* next) {
  *(void**)raw = next;
}

static void* hz5_lowpage64_link_next_load(void* raw) {
  return *(void**)raw;
}

static void* hz5_lowpage64_raw_os_alloc(size_t raw_bytes) {
#if HZ5_LOWPAGE64_P42_VA_SOURCE
#if defined(_WIN32)
  void* raw = VirtualAlloc(NULL, raw_bytes, MEM_RESERVE | MEM_COMMIT,
                           PAGE_READWRITE);
  if (!raw) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_va_alloc_failures, 1);
    return NULL;
  }
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_va_allocs, 1);
  return raw;
#else
  return NULL;
#endif
#else
  return _aligned_malloc(raw_bytes, HZ5_LOWPAGE64_RAW_PAGE_SIZE);
#endif
}

static void hz5_lowpage64_raw_os_release(void* raw) {
#if HZ5_LOWPAGE64_P42_VA_SOURCE
#if defined(_WIN32)
  if (raw) {
    VirtualFree(raw, 0, MEM_RELEASE);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_va_releases, 1);
  }
#else
  (void)raw;
#endif
#else
  _aligned_free(raw);
#endif
}

#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static void hz5_lowpage64_node_epoch_store(void* raw, uint32_t epoch) {
  *(uint32_t*)((char*)raw + sizeof(void*)) = epoch;
}

static uint32_t hz5_lowpage64_node_epoch_load(void* raw) {
  return *(uint32_t*)((char*)raw + sizeof(void*));
}

static void hz5_lowpage64_mark_list_epoch(void* head, uint32_t epoch) {
  while (head) {
    hz5_lowpage64_node_epoch_store(head, epoch);
    head = hz5_lowpage64_link_next_load(head);
  }
}
#endif

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP == 0 && \
    HZ5_LOWPAGE64_P39_PROBATION_CAP == 0
static size_t hz5_lowpage64_list_count(void* head) {
  size_t count = 0;
  while (head) {
    count++;
    head = hz5_lowpage64_link_next_load(head);
  }
  return count;
}
#endif

size_t hz5_lowpage64_round_raw_bytes(size_t size,
                                     size_t alignment,
                                     size_t header_size) {
  size_t raw_request = size + alignment + header_size;
  return (raw_request + HZ5_LOWPAGE64_RAW_PAGE_SIZE - 1u) &
         ~(HZ5_LOWPAGE64_RAW_PAGE_SIZE - 1u);
}

void hz5_lowpage64_note_raw_range(void* raw_ptr, size_t raw_bytes) {
  uintptr_t begin = (uintptr_t)raw_ptr;
  uintptr_t end = begin + raw_bytes;

  uintptr_t min_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_min, memory_order_relaxed);
  while ((min_cur == 0 || begin < min_cur) &&
         !atomic_compare_exchange_weak_explicit(
             &g_hz5_lowpage64_raw_min, &min_cur, begin, memory_order_relaxed,
             memory_order_relaxed)) {
  }

  uintptr_t max_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_max, memory_order_relaxed);
  while (end > max_cur &&
         !atomic_compare_exchange_weak_explicit(
             &g_hz5_lowpage64_raw_max, &max_cur, end, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}

int hz5_lowpage64_may_own(void* ptr) {
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t min_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_min, memory_order_acquire);
  uintptr_t max_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_max, memory_order_acquire);
  return min_cur != 0 && p >= min_cur && p < max_cur;
}

static size_t hz5_lowpage64_free_list(void* head) {
  size_t freed = 0;
  while (head) {
    void* next = hz5_lowpage64_link_next_load(head);
    hz5_lowpage64_raw_os_release(head);
    freed++;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_global_overflow_frees, 1);
    head = next;
  }
  return freed;
}

static size_t hz5_lowpage64_free_list_global_cap(void* head) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_global_cap_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_global_cap_release_nodes,
                            freed);
  }
  return freed;
}

static size_t hz5_lowpage64_free_list_stash_trim(void* head) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_stash_trim_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_stash_trim_release_nodes,
                            freed);
  }
  return freed;
}

static void hz5_lowpage64_count_trim(size_t freed) {
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_freed_nodes, freed);
  }
}

#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static void hz5_lowpage64_global_push_list(void* head,
                                           void* tail,
                                           size_t count) {
  if (!head || !tail || count == 0) {
    return;
  }

  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_global_batch_head, memory_order_acquire);
  do {
    hz5_lowpage64_link_next(tail, (void*)old_head);
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_lowpage64_global_batch_head, &old_head, (uintptr_t)head,
      memory_order_acq_rel, memory_order_acquire));

  atomic_fetch_add_explicit(&g_hz5_lowpage64_global_batch_count, count,
                            memory_order_relaxed);
}

static size_t hz5_lowpage64_free_list_p40_global(void* head,
                                                 size_t age_nodes,
                                                 size_t hard_nodes) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_nodes, freed);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_age_nodes,
                            age_nodes);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_hard_nodes,
                            hard_nodes);
  }
  return freed;
}

static void hz5_lowpage64_p40_checkpoint(void) {
  size_t observed = atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                                         memory_order_acquire);
  if (observed <= HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP) {
    return;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_checkpoint_calls, 1);
  uint32_t epoch = atomic_fetch_add_explicit(
                       &g_hz5_lowpage64_p40_global_epoch, 1,
                       memory_order_relaxed) +
                   1u;

  void* head =
      (void*)atomic_exchange_explicit(&g_hz5_lowpage64_global_batch_head, 0,
                                      memory_order_acq_rel);
  size_t expected_count = atomic_exchange_explicit(
      &g_hz5_lowpage64_global_batch_count, 0, memory_order_acq_rel);
  (void)expected_count;
  if (!head) {
    return;
  }

  void* keep_head = NULL;
  void* keep_tail = NULL;
  size_t keep_count = 0;
  void* release_head = NULL;
  void* release_tail = NULL;
  size_t release_count = 0;
  size_t age_nodes = 0;
  size_t hard_nodes = 0;

  void* node = head;
  while (node) {
    void* next = hz5_lowpage64_link_next_load(node);
    uint32_t node_epoch = hz5_lowpage64_node_epoch_load(node);
    uint32_t age = epoch - node_epoch;
    int over_hard =
#if HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP > 0
        observed > HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP;
#else
        0;
#endif
    int old_enough = age >= HZ5_LOWPAGE64_P40_TTL_EPOCHS;
    int should_release =
        release_count < HZ5_LOWPAGE64_P40_RELEASE_BATCH &&
        observed - release_count > HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP &&
        (old_enough || over_hard);

    if (should_release) {
      hz5_lowpage64_link_next(node, NULL);
      if (!release_head) {
        release_head = node;
      } else {
        hz5_lowpage64_link_next(release_tail, node);
      }
      release_tail = node;
      release_count++;
      if (over_hard && !old_enough) {
        hard_nodes++;
      } else {
        age_nodes++;
      }
    } else {
      hz5_lowpage64_link_next(node, NULL);
      if (!keep_head) {
        keep_head = node;
      } else {
        hz5_lowpage64_link_next(keep_tail, node);
      }
      keep_tail = node;
      keep_count++;
    }

    node = next;
  }

  if (keep_head) {
    hz5_lowpage64_global_push_list(keep_head, keep_tail, keep_count);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_keep_nodes, keep_count);
  }
  hz5_lowpage64_count_trim(
      hz5_lowpage64_free_list_p40_global(release_head, age_nodes,
                                         hard_nodes));
}
#else
#define hz5_lowpage64_p40_checkpoint() ((void)0)
#define hz5_lowpage64_mark_list_epoch(head, epoch) ((void)0)
#endif

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
static size_t hz5_lowpage64_free_list_p37_overflow(void* head) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_p37_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_p37_release_nodes, freed);
  }
  return freed;
}

static void hz5_lowpage64_p37_release_overflow(int high_event) {
  void* head = g_hz5_lowpage64_overflow_head;
  size_t count = g_hz5_lowpage64_overflow_count;
  g_hz5_lowpage64_overflow_head = NULL;
  g_hz5_lowpage64_overflow_count = 0;
  g_hz5_lowpage64_high_event_streak = 0;
  if (!head) {
    return;
  }

  size_t freed = hz5_lowpage64_free_list_p37_overflow(head);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_overflow_release_calls, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_overflow_release_nodes,
                          freed);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_calls, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_freed_nodes, freed);
  if (high_event) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_high_event_releases, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_lifetime_releases, 1);
  }
  (void)freed;
  (void)count;
}

static void hz5_lowpage64_p37_checkpoint(void) {
  g_hz5_lowpage64_epoch++;
  if (g_hz5_lowpage64_overflow_count == 0) {
    g_hz5_lowpage64_high_event_streak = 0;
    return;
  }

#if HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME > 0
  if (g_hz5_lowpage64_epoch >
      g_hz5_lowpage64_overflow_epoch +
          (uint32_t)HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME) {
    hz5_lowpage64_p37_release_overflow(0);
    return;
  }
#endif

#if HZ5_LOWPAGE64_P37_HIGH_STREAK > 0
  if (g_hz5_lowpage64_stash_count >= HZ5_LOWPAGE64_P37_HOT_CAP &&
      g_hz5_lowpage64_overflow_count >= HZ5_LOWPAGE64_P37_HIGH_WATER) {
    if (g_hz5_lowpage64_high_event_streak < UINT8_MAX) {
      g_hz5_lowpage64_high_event_streak++;
    }
    if (g_hz5_lowpage64_high_event_streak >=
        HZ5_LOWPAGE64_P37_HIGH_STREAK) {
      hz5_lowpage64_p37_release_overflow(1);
      return;
    }
  } else {
    g_hz5_lowpage64_high_event_streak = 0;
  }
#endif
}
#else
#define hz5_lowpage64_p37_checkpoint() ((void)0)
#endif

#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0
static size_t hz5_lowpage64_free_list_p39_probation(void* head,
                                                    int lifetime_release) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p39_probation_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p39_probation_release_nodes,
                            freed);
    if (lifetime_release) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p39_probation_lifetime_releases, 1);
    } else {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p39_probation_replace_releases, 1);
    }
  }
  return freed;
}

static void hz5_lowpage64_p39_release_probation(int lifetime_release) {
  void* head = g_hz5_lowpage64_probation_head;
  g_hz5_lowpage64_probation_head = NULL;
  g_hz5_lowpage64_probation_count = 0;
  hz5_lowpage64_count_trim(
      hz5_lowpage64_free_list_p39_probation(head, lifetime_release));
}

static void hz5_lowpage64_p39_checkpoint(void) {
  g_hz5_lowpage64_p39_epoch++;
  if (!g_hz5_lowpage64_probation_head) {
    return;
  }

#if HZ5_LOWPAGE64_P39_PROBATION_LIFETIME > 0
  if (g_hz5_lowpage64_p39_epoch >
      g_hz5_lowpage64_probation_epoch +
          (uint32_t)HZ5_LOWPAGE64_P39_PROBATION_LIFETIME) {
    hz5_lowpage64_p39_release_probation(1);
  }
#endif
}

#if HZ5_LOWPAGE64_P39_USE_ON_MISS
static void* hz5_lowpage64_p39_pop_probation(void) {
  if (!g_hz5_lowpage64_probation_head) {
    return NULL;
  }
  void* raw = g_hz5_lowpage64_probation_head;
  g_hz5_lowpage64_probation_head = hz5_lowpage64_link_next_load(raw);
  g_hz5_lowpage64_probation_count--;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_hits, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p39_probation_hits, 1);
  return raw;
}
#endif
#else
#define hz5_lowpage64_p39_checkpoint() ((void)0)
#endif

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP == 0 && \
    HZ5_LOWPAGE64_P39_PROBATION_CAP == 0
static size_t hz5_lowpage64_stash_cap_for_batch(size_t acquired_count) {
#if HZ5_LOWPAGE64_STASH_CAP > 0
  (void)acquired_count;
  return HZ5_LOWPAGE64_STASH_CAP;
#elif HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP > 0
  if (acquired_count > HZ5_LOWPAGE64_ADAPTIVE_STASH_TRIGGER) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_adaptive_trigger_calls, 1);
    return HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP;
  }
  return 0;
#else
  (void)acquired_count;
  return 0;
#endif
}
#endif

#if HZ5_LOWPAGE64_STATS
static void hz5_lowpage64_count_acquired_bucket(size_t count) {
  if (count <= 64u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_le64, 1);
  } else if (count <= 128u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_le128, 1);
  } else if (count <= 192u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_le192, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_gt192, 1);
  }
}

static void hz5_lowpage64_count_stash_bucket(size_t count) {
  if (count <= 64u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_le64, 1);
  } else if (count <= 128u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_le128, 1);
  } else if (count <= 192u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_le192, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_gt192, 1);
  }
}
#else
#define hz5_lowpage64_count_acquired_bucket(count) ((void)0)
#define hz5_lowpage64_count_stash_bucket(count) ((void)0)
#endif

static void hz5_lowpage64_set_stash_from_list(void* head,
                                              size_t acquired_count) {
#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0
  (void)acquired_count;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_calls, 1);

  if (g_hz5_lowpage64_probation_head) {
    hz5_lowpage64_p39_release_probation(0);
  }

  void* hot_head = head;
  void* hot_tail = NULL;
  size_t hot_kept = 0;
  void* node = head;
  while (node && hot_kept < HZ5_LOWPAGE64_P39_HOT_CAP) {
    hot_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    hot_kept++;
  }
  if (hot_tail) {
    hz5_lowpage64_link_next(hot_tail, NULL);
  }

  void* probation_head = node;
  void* probation_tail = NULL;
  size_t probation_kept = 0;
  while (node && probation_kept < HZ5_LOWPAGE64_P39_PROBATION_CAP) {
    probation_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    probation_kept++;
  }
  if (probation_tail) {
    hz5_lowpage64_link_next(probation_tail, NULL);
  }

  g_hz5_lowpage64_stash_head = hot_head;
  g_hz5_lowpage64_stash_count = hot_kept;
  g_hz5_lowpage64_probation_head = probation_head;
  g_hz5_lowpage64_probation_count = probation_kept;
  if (probation_kept > 0) {
    g_hz5_lowpage64_probation_epoch = g_hz5_lowpage64_p39_epoch;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max, hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max, hot_kept);
  hz5_lowpage64_count_stash_bucket(hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p39_probation_set_nodes_total, probation_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p39_probation_set_nodes_max,
                          probation_kept);

  hz5_lowpage64_count_trim(hz5_lowpage64_free_list_stash_trim(node));
  return;
#elif HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
  (void)acquired_count;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_calls, 1);

  void* hot_head = head;
  void* hot_tail = NULL;
  size_t hot_kept = 0;
  void* node = head;
  while (node && hot_kept < HZ5_LOWPAGE64_P37_HOT_CAP) {
    hot_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    hot_kept++;
  }
  if (hot_tail) {
    hz5_lowpage64_link_next(hot_tail, NULL);
  }

  void* overflow_head = node;
  void* overflow_tail = NULL;
  size_t overflow_kept = 0;
  while (node && overflow_kept < HZ5_LOWPAGE64_P37_OVERFLOW_CAP) {
    overflow_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    overflow_kept++;
  }
  if (overflow_tail) {
    hz5_lowpage64_link_next(overflow_tail, NULL);
  }

  g_hz5_lowpage64_stash_head = hot_head;
  g_hz5_lowpage64_stash_count = hot_kept;
  g_hz5_lowpage64_overflow_head = overflow_head;
  g_hz5_lowpage64_overflow_count = overflow_kept;
  if (overflow_kept > 0) {
    g_hz5_lowpage64_overflow_epoch = g_hz5_lowpage64_epoch;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max, hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max, hot_kept);
  hz5_lowpage64_count_stash_bucket(hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p37_overflow_set_nodes_total, overflow_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p37_overflow_set_nodes_max,
                          overflow_kept);

  hz5_lowpage64_count_trim(hz5_lowpage64_free_list_stash_trim(node));
  return;
#else
  size_t cap = hz5_lowpage64_stash_cap_for_batch(acquired_count);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_calls, 1);
  if (cap > 0) {
    void* keep_head = head;
    void* keep_tail = NULL;
    size_t kept = 0;
    void* node = head;
    while (node && kept < cap) {
      keep_tail = node;
      node = hz5_lowpage64_link_next_load(node);
      kept++;
    }
    if (keep_tail) {
      hz5_lowpage64_link_next(keep_tail, NULL);
    }
    g_hz5_lowpage64_stash_head = keep_head;
    g_hz5_lowpage64_stash_count = kept;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total, kept);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max, kept);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total, kept);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max, kept);
    hz5_lowpage64_count_stash_bucket(kept);
    hz5_lowpage64_count_trim(hz5_lowpage64_free_list_stash_trim(node));
    return;
  }

  g_hz5_lowpage64_stash_head = head;
  g_hz5_lowpage64_stash_count = hz5_lowpage64_list_count(head);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total,
                          g_hz5_lowpage64_stash_count);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max,
                          g_hz5_lowpage64_stash_count);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total,
                          g_hz5_lowpage64_stash_count);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max,
                          g_hz5_lowpage64_stash_count);
  hz5_lowpage64_count_stash_bucket(g_hz5_lowpage64_stash_count);
#endif
}

static void hz5_lowpage64_publish_relbuf(void) {
  if (!g_hz5_lowpage64_relbuf_head || !g_hz5_lowpage64_relbuf_tail ||
      g_hz5_lowpage64_relbuf_count == 0) {
    return;
  }

  void* head = g_hz5_lowpage64_relbuf_head;
  void* tail = g_hz5_lowpage64_relbuf_tail;
  size_t count = g_hz5_lowpage64_relbuf_count;
  g_hz5_lowpage64_relbuf_head = NULL;
  g_hz5_lowpage64_relbuf_tail = NULL;
  g_hz5_lowpage64_relbuf_count = 0;

  size_t global_count = atomic_load_explicit(
      &g_hz5_lowpage64_global_batch_count, memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_relbuf_count_max, count);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_global_count_before_max,
                          global_count);
  if (global_count + count > HZ5_LOWPAGE64_GLOBAL_CAP) {
    (void)hz5_lowpage64_free_list_global_cap(head);
    return;
  }

  uint32_t publish_epoch = 0;
#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
  publish_epoch = atomic_load_explicit(&g_hz5_lowpage64_p40_global_epoch,
                                       memory_order_relaxed);
  hz5_lowpage64_mark_list_epoch(head, publish_epoch);
#endif

  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_global_batch_head, memory_order_acquire);
  do {
    hz5_lowpage64_link_next(tail, (void*)old_head);
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_lowpage64_global_batch_head, &old_head, (uintptr_t)head,
      memory_order_acq_rel, memory_order_acquire));

  atomic_fetch_add_explicit(&g_hz5_lowpage64_global_batch_count, count,
                            memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_global_count_after_max,
                          global_count + count);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_batch_publishes, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_batch_publish_nodes, count);
  (void)publish_epoch;
  hz5_lowpage64_p40_checkpoint();
}

void* hz5_lowpage64_acquire(size_t raw_bytes) {
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_alloc_calls, 1);

#if HZ5_LOWPAGE64_SPAN_CACHE
  if (g_hz5_lowpage64_span_head) {
    void* raw = g_hz5_lowpage64_span_head;
    g_hz5_lowpage64_span_head = hz5_lowpage64_link_next_load(raw);
    g_hz5_lowpage64_span_count--;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_span_hits, 1);
    return raw;
  }
#endif

  if (g_hz5_lowpage64_stash_head) {
    void* raw = g_hz5_lowpage64_stash_head;
    g_hz5_lowpage64_stash_head = hz5_lowpage64_link_next_load(raw);
    g_hz5_lowpage64_stash_count--;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_hits, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_hot_hits, 1);
    return raw;
  }

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
  if (g_hz5_lowpage64_overflow_head) {
    void* raw = g_hz5_lowpage64_overflow_head;
    g_hz5_lowpage64_overflow_head = hz5_lowpage64_link_next_load(raw);
    g_hz5_lowpage64_overflow_count--;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_hits, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_overflow_hits, 1);
    return raw;
  }
#endif

  hz5_lowpage64_p37_checkpoint();
  hz5_lowpage64_p39_checkpoint();

  void* global =
      (void*)atomic_exchange_explicit(&g_hz5_lowpage64_global_batch_head, 0,
                                      memory_order_acq_rel);
  if (global) {
    size_t acquired_count = atomic_exchange_explicit(
        &g_hz5_lowpage64_global_batch_count, 0, memory_order_acq_rel);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_count_total,
                            acquired_count);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_acquired_count_max,
                            acquired_count);
    hz5_lowpage64_count_acquired_bucket(acquired_count);
    void* raw = global;
    void* stash = hz5_lowpage64_link_next_load(raw);
    hz5_lowpage64_link_next(raw, NULL);
    hz5_lowpage64_set_stash_from_list(stash, acquired_count);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_global_batch_hits, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_global_first_hits, 1);
    return raw;
  }

#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0 && HZ5_LOWPAGE64_P39_USE_ON_MISS
  void* probation = hz5_lowpage64_p39_pop_probation();
  if (probation) {
    return probation;
  }
#endif

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_os_allocs, 1);
  return hz5_lowpage64_raw_os_alloc(raw_bytes);
}

void hz5_lowpage64_release(void* raw) {
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_free_calls, 1);
  if (!raw) {
    return;
  }

#if HZ5_LOWPAGE64_SPAN_CACHE
  if (g_hz5_lowpage64_span_count < HZ5_LOWPAGE64_SPAN_CAP) {
    hz5_lowpage64_link_next(raw, g_hz5_lowpage64_span_head);
    g_hz5_lowpage64_span_head = raw;
    g_hz5_lowpage64_span_count++;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_span_pushes, 1);
    return;
  }
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_span_full, 1);
#endif

  hz5_lowpage64_link_next(raw, NULL);
  if (!g_hz5_lowpage64_relbuf_head) {
    g_hz5_lowpage64_relbuf_head = raw;
    g_hz5_lowpage64_relbuf_tail = raw;
  } else {
    hz5_lowpage64_link_next(g_hz5_lowpage64_relbuf_tail, raw);
    g_hz5_lowpage64_relbuf_tail = raw;
  }
  g_hz5_lowpage64_relbuf_count++;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_relbuf_pushes, 1);

  if (g_hz5_lowpage64_relbuf_count >= HZ5_LOWPAGE64_BATCH_FLUSH_N) {
    hz5_lowpage64_publish_relbuf();
  }
}

#if HZ5_LOWPAGE64_STATS
static void hz5_lowpage64_print_once(void) {
  static atomic_flag printed = ATOMIC_FLAG_INIT;
  if (atomic_flag_test_and_set_explicit(&printed, memory_order_acq_rel)) {
    return;
  }

  fprintf(stderr,
          "[HZ5_LOWPAGE64] alloc_calls=%zu span_hits=%zu "
          "span_pushes=%zu span_full=%zu stash_hits=%zu "
          "global_batch_hits=%zu os_allocs=%zu free_calls=%zu "
          "relbuf_pushes=%zu batch_publishes=%zu batch_publish_nodes=%zu "
          "global_overflow_frees=%zu global_batch_count=%zu "
          "relbuf_count_max=%zu global_count_before_max=%zu "
          "global_count_after_max=%zu acquired_count_total=%zu "
          "acquired_count_max=%zu stash_set_calls=%zu "
          "stash_set_nodes_total=%zu stash_set_nodes_max=%zu "
          "trim_calls=%zu trim_freed_nodes=%zu "
          "adaptive_trigger_calls=%zu acquired_le64=%zu "
          "acquired_le128=%zu acquired_le192=%zu acquired_gt192=%zu "
          "stash_nodes_le64=%zu stash_nodes_le128=%zu "
          "stash_nodes_le192=%zu stash_nodes_gt192=%zu "
          "p37_overflow_hits=%zu p37_overflow_set_nodes_total=%zu "
          "p37_overflow_set_nodes_max=%zu "
          "p37_overflow_release_calls=%zu "
          "p37_overflow_release_nodes=%zu "
          "p37_high_event_releases=%zu p37_lifetime_releases=%zu "
          "p38_hot_hits=%zu p38_global_first_hits=%zu "
          "p38_set_hot_nodes_total=%zu p38_set_hot_nodes_max=%zu "
          "p38_global_cap_release_calls=%zu "
          "p38_global_cap_release_nodes=%zu "
          "p38_stash_trim_release_calls=%zu "
          "p38_stash_trim_release_nodes=%zu "
          "p38_p37_release_calls=%zu p38_p37_release_nodes=%zu "
          "p39_probation_hits=%zu "
          "p39_probation_set_nodes_total=%zu "
          "p39_probation_set_nodes_max=%zu "
          "p39_probation_release_calls=%zu "
          "p39_probation_release_nodes=%zu "
          "p39_probation_lifetime_releases=%zu "
          "p39_probation_replace_releases=%zu "
          "p40_checkpoint_calls=%zu p40_release_calls=%zu "
          "p40_release_nodes=%zu p40_release_age_nodes=%zu "
          "p40_release_hard_nodes=%zu p40_keep_nodes=%zu "
          "p42_va_allocs=%zu p42_va_alloc_failures=%zu "
          "p42_va_releases=%zu\n",
          atomic_load_explicit(&g_hz5_lowpage64_alloc_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_span_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_span_pushes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_span_full,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_batch_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_os_allocs,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_free_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_relbuf_pushes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_batch_publishes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_batch_publish_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_overflow_frees,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_relbuf_count_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_count_before_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_count_after_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_count_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_count_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_nodes_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_trim_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_trim_freed_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_adaptive_trigger_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_le64,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_le128,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_le192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_gt192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_le64,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_le128,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_le192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_gt192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_hits,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p37_overflow_set_nodes_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_set_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_high_event_releases,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_lifetime_releases,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_hot_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_global_first_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_set_hot_nodes_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_set_hot_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_global_cap_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_global_cap_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_stash_trim_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_stash_trim_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_p37_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_p37_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_hits,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p39_probation_set_nodes_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_set_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p39_probation_lifetime_releases,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p39_probation_replace_releases,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_checkpoint_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_age_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_hard_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_keep_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_va_allocs,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_va_alloc_failures,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_va_releases,
                               memory_order_relaxed));
}
#endif

void hz5_lowpage64_register_stats_once(void) {
#if HZ5_LOWPAGE64_STATS
  static atomic_flag registered = ATOMIC_FLAG_INIT;
  if (!atomic_flag_test_and_set_explicit(&registered, memory_order_acq_rel)) {
    atexit(hz5_lowpage64_print_once);
  }
#endif
}
