#include "hz5_legacy_p17.h"

#include "hz5_wrapper.h"

#include <malloc.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
#define BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 0
#endif

#ifndef BENCHLAB_HZ5_P24_RAW64K_A8192
#define BENCHLAB_HZ5_P24_RAW64K_A8192 0
#endif

#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || BENCHLAB_HZ5_P24_RAW64K_A8192
#define HZ5_LEGACY_P17_RAW_PAGE_SIZE ((size_t)65536)

#ifndef BENCHLAB_HZ5_P17_TLS_CAP
#define BENCHLAB_HZ5_P17_TLS_CAP 32u
#endif

#ifndef BENCHLAB_HZ5_P17_GLOBAL_CAP
#define BENCHLAB_HZ5_P17_GLOBAL_CAP 256u
#endif

static _Atomic(uintptr_t) g_hz5_p17_global_head;
static _Atomic size_t g_hz5_p17_global_count;
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
static _Atomic size_t g_hz5_p17_alloc_calls;
static _Atomic size_t g_hz5_p17_tls_hits;
static _Atomic size_t g_hz5_p17_global_hits;
static _Atomic size_t g_hz5_p17_os_allocs;
static _Atomic size_t g_hz5_p17_free_calls;
static _Atomic size_t g_hz5_p17_tls_pushes;
static _Atomic size_t g_hz5_p17_global_flushes;
static _Atomic size_t g_hz5_p17_global_flush_nodes;
static _Atomic size_t g_hz5_p17_global_overflow_frees;
static _Atomic size_t g_hz5_p17_bad_alignment;
#endif

__declspec(thread) static void* g_hz5_p17_tls_head;
__declspec(thread) static size_t g_hz5_p17_tls_count;

static size_t hz5_legacy_p17_round_raw_bytes(size_t size, size_t align) {
  size_t raw_request = size + align + sizeof(Hz5WrapperHdr);
  return (raw_request + HZ5_LEGACY_P17_RAW_PAGE_SIZE - 1u) &
         ~(HZ5_LEGACY_P17_RAW_PAGE_SIZE - 1u);
}

#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
static void hz5_legacy_p17_print_once(void) {
  static atomic_flag printed = ATOMIC_FLAG_INIT;
  if (atomic_flag_test_and_set_explicit(&printed, memory_order_acq_rel)) {
    return;
  }

  fprintf(stderr,
          "[HZ5_P17_PAGEBIN64K] alloc_calls=%zu tls_hits=%zu global_hits=%zu "
          "os_allocs=%zu free_calls=%zu tls_pushes=%zu global_flushes=%zu "
          "global_flush_nodes=%zu global_overflow_frees=%zu global_count=%zu "
          "bad_alignment=%zu\n",
          atomic_load_explicit(&g_hz5_p17_alloc_calls, memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p17_tls_hits, memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p17_global_hits, memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p17_os_allocs, memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p17_free_calls, memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p17_tls_pushes, memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p17_global_flushes, memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p17_global_flush_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p17_global_overflow_frees,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p17_global_count, memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p17_bad_alignment, memory_order_relaxed));
}
#endif

static void hz5_legacy_p17_register_once(void) {
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
  static atomic_flag registered = ATOMIC_FLAG_INIT;
  if (!atomic_flag_test_and_set_explicit(&registered, memory_order_acq_rel)) {
    atexit(hz5_legacy_p17_print_once);
  }
#endif
}

static void hz5_legacy_p17_link_next(void* raw, void* next) {
  *(void**)raw = next;
}

static void* hz5_legacy_p17_link_next_load(void* raw) {
  return *(void**)raw;
}

static size_t hz5_legacy_p17_list_count(void* head) {
  size_t count = 0;
  while (head) {
    count++;
    head = hz5_legacy_p17_link_next_load(head);
  }
  return count;
}

static void hz5_legacy_p17_global_push_list(void* head, void* tail,
                                            size_t count) {
  if (!head || count == 0) {
    return;
  }

  size_t global_count =
      atomic_load_explicit(&g_hz5_p17_global_count, memory_order_relaxed);
  if (global_count + count > BENCHLAB_HZ5_P17_GLOBAL_CAP) {
    void* node = head;
    while (node) {
      void* next = hz5_legacy_p17_link_next_load(node);
      _aligned_free(node);
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
      atomic_fetch_add_explicit(&g_hz5_p17_global_overflow_frees, 1,
                                memory_order_relaxed);
#endif
      node = next;
    }
    return;
  }

  uintptr_t old_head = atomic_load_explicit(&g_hz5_p17_global_head,
                                            memory_order_acquire);
  do {
    hz5_legacy_p17_link_next(tail, (void*)old_head);
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_p17_global_head, &old_head, (uintptr_t)head,
      memory_order_acq_rel, memory_order_acquire));

  atomic_fetch_add_explicit(&g_hz5_p17_global_count, count,
                            memory_order_relaxed);
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
  atomic_fetch_add_explicit(&g_hz5_p17_global_flushes, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&g_hz5_p17_global_flush_nodes, count,
                            memory_order_relaxed);
#endif
}

static void hz5_legacy_p17_flush_tls(void) {
  if (!g_hz5_p17_tls_head || g_hz5_p17_tls_count == 0) {
    return;
  }

  void* head = g_hz5_p17_tls_head;
  void* tail = head;
  while (hz5_legacy_p17_link_next_load(tail)) {
    tail = hz5_legacy_p17_link_next_load(tail);
  }
  size_t count = g_hz5_p17_tls_count;
  g_hz5_p17_tls_head = NULL;
  g_hz5_p17_tls_count = 0;
  hz5_legacy_p17_global_push_list(head, tail, count);
}

static void* hz5_legacy_p17_raw_acquire(size_t raw_bytes) {
  (void)raw_bytes;
  if (g_hz5_p17_tls_head) {
    void* raw = g_hz5_p17_tls_head;
    g_hz5_p17_tls_head = hz5_legacy_p17_link_next_load(raw);
    g_hz5_p17_tls_count--;
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
    atomic_fetch_add_explicit(&g_hz5_p17_tls_hits, 1, memory_order_relaxed);
#endif
    return raw;
  }

  void* global = (void*)atomic_exchange_explicit(&g_hz5_p17_global_head, 0,
                                                memory_order_acq_rel);
  if (global) {
    atomic_store_explicit(&g_hz5_p17_global_count, 0, memory_order_release);
    void* raw = global;
    g_hz5_p17_tls_head = hz5_legacy_p17_link_next_load(raw);
    g_hz5_p17_tls_count = hz5_legacy_p17_list_count(g_hz5_p17_tls_head);
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
    atomic_fetch_add_explicit(&g_hz5_p17_global_hits, 1, memory_order_relaxed);
#endif
    return raw;
  }

#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
  atomic_fetch_add_explicit(&g_hz5_p17_os_allocs, 1, memory_order_relaxed);
#endif
  return _aligned_malloc(raw_bytes, HZ5_LEGACY_P17_RAW_PAGE_SIZE);
}
#endif

void hz5_legacy_p17_raw_release(void* raw) {
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || BENCHLAB_HZ5_P24_RAW64K_A8192
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
  atomic_fetch_add_explicit(&g_hz5_p17_free_calls, 1, memory_order_relaxed);
#endif
  if (!raw) {
    return;
  }

  if (g_hz5_p17_tls_count >= BENCHLAB_HZ5_P17_TLS_CAP) {
    hz5_legacy_p17_flush_tls();
  }

  hz5_legacy_p17_link_next(raw, g_hz5_p17_tls_head);
  g_hz5_p17_tls_head = raw;
  g_hz5_p17_tls_count++;
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
  atomic_fetch_add_explicit(&g_hz5_p17_tls_pushes, 1, memory_order_relaxed);
#endif
#else
  (void)raw;
#endif
}

void* hz5_legacy_p17_wrapper_alloc(size_t size, size_t align) {
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || BENCHLAB_HZ5_P24_RAW64K_A8192
  hz5_legacy_p17_register_once();
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
  atomic_fetch_add_explicit(&g_hz5_p17_alloc_calls, 1, memory_order_relaxed);
#endif

  size_t raw_bytes = hz5_legacy_p17_round_raw_bytes(size, align);
  void* raw_ptr = hz5_legacy_p17_raw_acquire(raw_bytes);
  if (!raw_ptr) {
    return NULL;
  }

  uintptr_t raw = (uintptr_t)raw_ptr;
  uintptr_t aligned =
      (raw + sizeof(Hz5WrapperHdr) + align - 1u) & ~(uintptr_t)(align - 1u);
  if ((aligned & (align - 1u)) != 0 || aligned + size > raw + raw_bytes) {
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
    atomic_fetch_add_explicit(&g_hz5_p17_bad_alignment, 1,
                              memory_order_relaxed);
#endif
    hz5_legacy_p17_raw_release(raw_ptr);
    return NULL;
  }

  Hz5WrapperHdr* header = (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
#if BENCHLAB_HZ5_P24_RAW64K_A8192
  hz5_wrapper_init(header, raw, aligned, size, raw_bytes,
                   HZ5_WRAPPER_SOURCE_P24_RAW64K);
#else
  hz5_wrapper_init(header, raw, aligned, size, raw_bytes,
                   HZ5_WRAPPER_SOURCE_P17_PAGEBIN);
#endif
  return (void*)aligned;
#else
  (void)size;
  (void)align;
  return NULL;
#endif
}
