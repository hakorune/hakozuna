#include "hz5_lowpage64.h"

#include <malloc.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

#ifndef HZ5_LOWPAGE64_SPAN_CACHE
#ifdef BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
#define HZ5_LOWPAGE64_SPAN_CACHE BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
#else
#define HZ5_LOWPAGE64_SPAN_CACHE 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_STATS
#ifdef BENCHLAB_HZ5_P25_STATS
#define HZ5_LOWPAGE64_STATS BENCHLAB_HZ5_P25_STATS
#else
#define HZ5_LOWPAGE64_STATS 0
#endif
#endif

#if HZ5_LOWPAGE64_STATS
#define HZ5_LOWPAGE64_COUNT_ADD(counter, value) \
  atomic_fetch_add_explicit(&(counter), (value), memory_order_relaxed)
#else
#define HZ5_LOWPAGE64_COUNT_ADD(counter, value) ((void)0)
#endif

static _Atomic(uintptr_t) g_hz5_lowpage64_global_batch_head;
static _Atomic size_t g_hz5_lowpage64_global_batch_count;
static _Atomic(uintptr_t) g_hz5_lowpage64_raw_min;
static _Atomic(uintptr_t) g_hz5_lowpage64_raw_max;

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
#endif

#if HZ5_LOWPAGE64_SPAN_CACHE
__declspec(thread) static void* g_hz5_lowpage64_span_head;
__declspec(thread) static size_t g_hz5_lowpage64_span_count;
#endif
__declspec(thread) static void* g_hz5_lowpage64_stash_head;
__declspec(thread) static size_t g_hz5_lowpage64_stash_count;
__declspec(thread) static void* g_hz5_lowpage64_relbuf_head;
__declspec(thread) static void* g_hz5_lowpage64_relbuf_tail;
__declspec(thread) static size_t g_hz5_lowpage64_relbuf_count;

static void hz5_lowpage64_link_next(void* raw, void* next) {
  *(void**)raw = next;
}

static void* hz5_lowpage64_link_next_load(void* raw) {
  return *(void**)raw;
}

#if HZ5_LOWPAGE64_STASH_CAP == 0
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

static void hz5_lowpage64_free_list(void* head) {
  while (head) {
    void* next = hz5_lowpage64_link_next_load(head);
    _aligned_free(head);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_global_overflow_frees, 1);
    head = next;
  }
}

static void hz5_lowpage64_set_stash_from_list(void* head) {
#if HZ5_LOWPAGE64_STASH_CAP > 0
  void* keep_head = head;
  void* keep_tail = NULL;
  size_t kept = 0;
  void* node = head;
  while (node && kept < HZ5_LOWPAGE64_STASH_CAP) {
    keep_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    kept++;
  }
  if (keep_tail) {
    hz5_lowpage64_link_next(keep_tail, NULL);
  }
  g_hz5_lowpage64_stash_head = keep_head;
  g_hz5_lowpage64_stash_count = kept;
  hz5_lowpage64_free_list(node);
#else
  g_hz5_lowpage64_stash_head = head;
  g_hz5_lowpage64_stash_count = hz5_lowpage64_list_count(head);
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
  if (global_count + count > HZ5_LOWPAGE64_GLOBAL_CAP) {
    hz5_lowpage64_free_list(head);
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
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_batch_publishes, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_batch_publish_nodes, count);
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
    return raw;
  }

  void* global =
      (void*)atomic_exchange_explicit(&g_hz5_lowpage64_global_batch_head, 0,
                                      memory_order_acq_rel);
  if (global) {
    atomic_store_explicit(&g_hz5_lowpage64_global_batch_count, 0,
                          memory_order_release);
    void* raw = global;
    void* stash = hz5_lowpage64_link_next_load(raw);
    hz5_lowpage64_link_next(raw, NULL);
    hz5_lowpage64_set_stash_from_list(stash);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_global_batch_hits, 1);
    return raw;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_os_allocs, 1);
  return _aligned_malloc(raw_bytes, HZ5_LOWPAGE64_RAW_PAGE_SIZE);
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
          "global_overflow_frees=%zu global_batch_count=%zu\n",
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
