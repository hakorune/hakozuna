#include "hz5_route.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef HZ5_ROUTE_PAGE_SIZE
#define HZ5_ROUTE_PAGE_SIZE ((size_t)4096)
#endif

#ifndef BENCHLAB_HZ5_ROUTE_4K_A8192
#define BENCHLAB_HZ5_ROUTE_4K_A8192 1
#endif

#ifndef BENCHLAB_HZ5_ROUTE_2K_A8192
#define BENCHLAB_HZ5_ROUTE_2K_A8192 0
#endif

#ifndef BENCHLAB_HZ5_ROUTE_8K_A8192
#define BENCHLAB_HZ5_ROUTE_8K_A8192 1
#endif

#ifndef BENCHLAB_HZ5_ROUTE_64K_A8192
#define BENCHLAB_HZ5_ROUTE_64K_A8192 1
#endif

#ifndef BENCHLAB_HZ5_P12_ROUTE_SHADOW
#define BENCHLAB_HZ5_P12_ROUTE_SHADOW 0
#endif

#if BENCHLAB_HZ5_P12_ROUTE_SHADOW
static _Atomic size_t g_hz5_p12_candidate_64k_a8192;
static _Atomic size_t g_hz5_p12_would_route_64k_a8192;
static _Atomic size_t g_hz5_p12_fallback_64k_a8192;
static _Atomic size_t g_hz5_p12_guard_64k_a4096;
static _Atomic size_t g_hz5_p12_reject_65537;
static _Atomic size_t g_hz5_p12_reject_other;

static void hz5_route_p12_print_once(void) {
  static atomic_flag printed = ATOMIC_FLAG_INIT;
  if (atomic_flag_test_and_set_explicit(&printed, memory_order_acq_rel)) {
    return;
  }
  fprintf(stderr,
          "[HZ5_P12_ROUTE_SHADOW] candidate_64k_a8192=%zu "
          "would_route_64k_a8192=%zu fallback_64k_a8192=%zu "
          "guard_64k_a4096=%zu reject_65537=%zu reject_other=%zu\n",
          atomic_load_explicit(&g_hz5_p12_candidate_64k_a8192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p12_would_route_64k_a8192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p12_fallback_64k_a8192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p12_guard_64k_a4096,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p12_reject_65537, memory_order_relaxed),
          atomic_load_explicit(&g_hz5_p12_reject_other, memory_order_relaxed));
}

static void hz5_route_p12_register_once(void) {
  static atomic_flag registered = ATOMIC_FLAG_INIT;
  if (!atomic_flag_test_and_set_explicit(&registered, memory_order_acq_rel)) {
    atexit(hz5_route_p12_print_once);
  }
}
#endif

int hz5_route_exact_a8192(size_t size, size_t align) {
  if (align != (HZ5_ROUTE_PAGE_SIZE * (size_t)2)) {
    return 0;
  }
  if (BENCHLAB_HZ5_ROUTE_2K_A8192 && size == 2048u) {
    return 1;
  }
  if (BENCHLAB_HZ5_ROUTE_4K_A8192 && size == 4096u) {
    return 1;
  }
  if (BENCHLAB_HZ5_ROUTE_8K_A8192 && size == 8192u) {
    return 1;
  }
  if (BENCHLAB_HZ5_ROUTE_64K_A8192 && size == 65536u) {
    return 1;
  }
  return 0;
}

int hz5_route_hz3_medium_can_satisfy(size_t size, size_t align) {
  return size >= HZ5_ROUTE_PAGE_SIZE &&
         size <= (HZ5_ROUTE_PAGE_SIZE * (size_t)16) &&
         align <= HZ5_ROUTE_PAGE_SIZE;
}

void hz5_route_p12_on_alloc(size_t size, size_t align) {
#if BENCHLAB_HZ5_P12_ROUTE_SHADOW
  hz5_route_p12_register_once();
  if (size == 65536u && align == (HZ5_ROUTE_PAGE_SIZE * (size_t)2)) {
    atomic_fetch_add_explicit(&g_hz5_p12_candidate_64k_a8192, 1,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz5_p12_would_route_64k_a8192, 1,
                              memory_order_relaxed);
    if (!BENCHLAB_HZ5_ROUTE_64K_A8192) {
      atomic_fetch_add_explicit(&g_hz5_p12_fallback_64k_a8192, 1,
                                memory_order_relaxed);
    }
  } else if (size == 65536u && align == HZ5_ROUTE_PAGE_SIZE) {
    atomic_fetch_add_explicit(&g_hz5_p12_guard_64k_a4096, 1,
                              memory_order_relaxed);
  } else if (size == 65537u) {
    atomic_fetch_add_explicit(&g_hz5_p12_reject_65537, 1,
                              memory_order_relaxed);
  } else {
    atomic_fetch_add_explicit(&g_hz5_p12_reject_other, 1,
                              memory_order_relaxed);
  }
#else
  (void)size;
  (void)align;
#endif
}
