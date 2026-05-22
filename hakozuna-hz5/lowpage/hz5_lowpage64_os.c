#include "hz5_lowpage64.h"
#include "hz5_lowpage64_p43_segment.h"

#include <malloc.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#else
void* _aligned_malloc(size_t size, size_t alignment);
void _aligned_free(void* ptr);
#endif

#ifndef HZ5_LOWPAGE64_RAW_PAGE_SIZE
#define HZ5_LOWPAGE64_RAW_PAGE_SIZE ((size_t)65536)
#endif

#if HZ5_LOWPAGE64_P42_VA_SOURCE && HZ5_LOWPAGE64_P42_DECOMMIT_COLD
typedef struct Hz5Lowpage64ColdNode {
  void* raw;
  size_t bytes;
  struct Hz5Lowpage64ColdNode* next;
} Hz5Lowpage64ColdNode;

extern _Atomic(uintptr_t) g_hz5_lowpage64_p42_cold_head;
extern _Atomic size_t g_hz5_lowpage64_p42_cold_count;

#if HZ5_LOWPAGE64_STATS
extern _Atomic size_t g_hz5_lowpage64_p42_recommit_calls;
extern _Atomic size_t g_hz5_lowpage64_p42_recommit_failures;
extern _Atomic size_t g_hz5_lowpage64_p42_cold_hits;
extern _Atomic size_t g_hz5_lowpage64_p42_cold_count_max;
#endif

static size_t hz5_lowpage64_p42_raw_bytes(void) {
  return HZ5_LOWPAGE64_RAW_PAGE_SIZE * (size_t)2u;
}

static void hz5_lowpage64_p42_cold_push(Hz5Lowpage64ColdNode* node) {
  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_p42_cold_head, memory_order_acquire);
  do {
    node->next = (Hz5Lowpage64ColdNode*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_lowpage64_p42_cold_head, &old_head, (uintptr_t)node,
      memory_order_acq_rel, memory_order_acquire));
  size_t count = atomic_fetch_add_explicit(
                     &g_hz5_lowpage64_p42_cold_count, 1,
                     memory_order_relaxed) +
                 1u;
#if HZ5_LOWPAGE64_STATS
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p42_cold_count_max, count);
#endif
  (void)count;
}

static Hz5Lowpage64ColdNode* hz5_lowpage64_p42_cold_pop(void) {
  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_p42_cold_head, memory_order_acquire);
  while (old_head) {
    Hz5Lowpage64ColdNode* node = (Hz5Lowpage64ColdNode*)old_head;
    uintptr_t next = (uintptr_t)node->next;
    if (atomic_compare_exchange_weak_explicit(
            &g_hz5_lowpage64_p42_cold_head, &old_head, next,
            memory_order_acq_rel, memory_order_acquire)) {
      atomic_fetch_sub_explicit(&g_hz5_lowpage64_p42_cold_count, 1,
                                memory_order_relaxed);
      node->next = NULL;
      return node;
    }
  }
  return NULL;
}

void* hz5_lowpage64_p42_try_recommit(size_t raw_bytes) {
#if defined(_WIN32)
  for (size_t i = 0; i < HZ5_LOWPAGE64_P42_RECOMMIT_BATCH; i++) {
    Hz5Lowpage64ColdNode* node = hz5_lowpage64_p42_cold_pop();
    if (!node) {
      return NULL;
    }
    void* raw = VirtualAlloc(node->raw, node->bytes, MEM_COMMIT,
                             PAGE_READWRITE);
    if (raw) {
#if HZ5_LOWPAGE64_STATS
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_recommit_calls, 1);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_cold_hits, 1);
#endif
      free(node);
      (void)raw_bytes;
      return raw;
    }
#if HZ5_LOWPAGE64_STATS
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_recommit_failures, 1);
#endif
    VirtualFree(node->raw, 0, MEM_RELEASE);
    free(node);
  }
#else
  (void)raw_bytes;
#endif
  return NULL;
}
#endif

void* hz5_lowpage64_raw_os_alloc(size_t raw_bytes) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  // P43 is the lower raw source for P43i/P44. The hot P25 bridge still lives
  // above this function in acquire/release_common.
  return hz5_lowpage64_p43_alloc_slot(raw_bytes);
#elif HZ5_LOWPAGE64_P42_VA_SOURCE
#if defined(_WIN32)
  void* raw = VirtualAlloc(NULL, raw_bytes, MEM_RESERVE | MEM_COMMIT,
                           PAGE_READWRITE);
  if (!raw) {
#if HZ5_LOWPAGE64_STATS
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_va_alloc_failures, 1);
#endif
    return NULL;
  }
#if HZ5_LOWPAGE64_STATS
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_va_allocs, 1);
#endif
  return raw;
#else
  return NULL;
#endif
#else
  return _aligned_malloc(raw_bytes, HZ5_LOWPAGE64_RAW_PAGE_SIZE);
#endif
}

void hz5_lowpage64_raw_os_release(void* raw) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  // Only direct descriptor-release controls should commonly reach this P43
  // release path. P43i/P44 balanced lanes normally preserve release_common().
  (void)hz5_lowpage64_p43_release_slot(raw);
  return;
#elif HZ5_LOWPAGE64_P42_VA_SOURCE
#if defined(_WIN32)
  if (raw) {
#if HZ5_LOWPAGE64_P42_DECOMMIT_COLD
    Hz5Lowpage64ColdNode* node =
        (Hz5Lowpage64ColdNode*)malloc(sizeof(Hz5Lowpage64ColdNode));
    if (node && VirtualFree(raw, hz5_lowpage64_p42_raw_bytes(),
                            MEM_DECOMMIT)) {
      node->raw = raw;
      node->bytes = hz5_lowpage64_p42_raw_bytes();
      node->next = NULL;
#if HZ5_LOWPAGE64_STATS
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_decommit_calls, 1);
#endif
      hz5_lowpage64_p42_cold_push(node);
      return;
    }
#if HZ5_LOWPAGE64_STATS
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_decommit_failures, 1);
#endif
    free(node);
#endif
    VirtualFree(raw, 0, MEM_RELEASE);
#if HZ5_LOWPAGE64_STATS
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_va_releases, 1);
#endif
  }
#else
  (void)raw;
#endif
#else
  _aligned_free(raw);
#endif
}
