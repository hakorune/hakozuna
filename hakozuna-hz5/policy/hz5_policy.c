#include "hz5_policy.h"

#include "hz5_hz3_fallback.h"
#include "hz5_lowpage64.h"
#include "hz5_route.h"
#include "hz5_wrapper.h"

#include <malloc.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef HZ5_POLICY_PAGE_SIZE
#define HZ5_POLICY_PAGE_SIZE ((size_t)4096)
#endif

#ifndef HZ5_POLICY_MIN_ALIGN
#define HZ5_POLICY_MIN_ALIGN ((size_t)16)
#endif

#ifndef BENCHLAB_HZ5_P16_WRAPPER_64K_A8192
#define BENCHLAB_HZ5_P16_WRAPPER_64K_A8192 0
#endif

#ifndef BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192
#define BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 0
#endif

#ifndef BENCHLAB_HZ5_P24_RAW64K_A8192
#define BENCHLAB_HZ5_P24_RAW64K_A8192 0
#endif

#ifndef BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192
#define BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 0
#endif

#ifndef BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
#define BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192 0
#endif

#ifndef BENCHLAB_HZ5_NO_HZ3_FALLBACK
#define BENCHLAB_HZ5_NO_HZ3_FALLBACK 0
#endif

#ifndef BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
#define BENCHLAB_HZ5_LAZY_HZ3_FALLBACK 0
#endif

#ifndef BENCHLAB_HZ5_P12_ROUTE_SHADOW
#define BENCHLAB_HZ5_P12_ROUTE_SHADOW 0
#endif

void* hz3_malloc(size_t size);
void hz3_free(void* ptr);
void* hz3_large_aligned_alloc(size_t alignment, size_t size);

void* hz5_p2_alloc_aligned(size_t size, size_t alignment);
void hz5_p2_free(void* ptr);
int hz5_p1_owns(void* ptr);
void hz5_stats_print_once(void);

static _Atomic int g_hz5_policy_seen_allocation;

static void hz5_policy_register_stats_once(void) {
  static atomic_flag registered = ATOMIC_FLAG_INIT;
  if (!atomic_flag_test_and_set_explicit(&registered, memory_order_acq_rel)) {
    atexit(hz5_stats_print_once);
  }
}

#if BENCHLAB_HZ5_P16_WRAPPER_64K_A8192
static void* hz5_policy_wrapper_alloc_with_hz3(size_t size, size_t align) {
  if (align > SIZE_MAX - sizeof(Hz5WrapperHdr)) {
    return NULL;
  }
  size_t overhead = sizeof(Hz5WrapperHdr) + align;
  if (size > SIZE_MAX - overhead) {
    return NULL;
  }

  void* raw_ptr = hz3_malloc(size + overhead);
  if (!raw_ptr) {
    return NULL;
  }

  uintptr_t raw = (uintptr_t)raw_ptr;
  uintptr_t aligned =
      (raw + sizeof(Hz5WrapperHdr) + align - 1u) & ~(uintptr_t)(align - 1u);
  Hz5WrapperHdr* header = (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
  hz5_wrapper_init(header, raw, aligned, size, size + overhead,
                   HZ5_WRAPPER_SOURCE_HZ3);
  return (void*)aligned;
}
#endif

#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
static void* hz5_policy_p25_wrapper_alloc(size_t size, size_t align) {
  hz5_lowpage64_register_stats_once();

  size_t raw_bytes =
      hz5_lowpage64_round_raw_bytes(size, align, sizeof(Hz5WrapperHdr));
  void* raw_ptr = hz5_lowpage64_acquire(raw_bytes);
  if (!raw_ptr) {
    return NULL;
  }
  hz5_lowpage64_note_raw_range(raw_ptr, raw_bytes);

  uintptr_t raw = (uintptr_t)raw_ptr;
  uintptr_t aligned =
      (raw + sizeof(Hz5WrapperHdr) + align - 1u) & ~(uintptr_t)(align - 1u);
  if ((aligned & (align - 1u)) != 0 || aligned + size > raw + raw_bytes) {
    hz5_lowpage64_release(raw_ptr);
    return NULL;
  }

  Hz5WrapperHdr* header = (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
  hz5_wrapper_init(header, raw, aligned, size, raw_bytes,
                   HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE);
  return (void*)aligned;
}
#endif

#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
static void* hz5_policy_control_fallback_alloc(size_t size, size_t align) {
  if (size == 0) {
    size = 1;
  }
  if (align < sizeof(void*)) {
    align = sizeof(void*);
  }
  return _aligned_malloc(size, align);
}

static void hz5_policy_control_fallback_free(void* ptr) {
  _aligned_free(ptr);
}
#endif

#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
static void* hz5_policy_p20_crt_bypass_alloc(size_t size, size_t align) {
  if (align > SIZE_MAX - sizeof(Hz5WrapperHdr)) {
    return NULL;
  }
  size_t overhead = sizeof(Hz5WrapperHdr) + align;
  if (size > SIZE_MAX - overhead) {
    return NULL;
  }

  size_t raw_bytes = size + overhead;
  void* raw_ptr = _aligned_malloc(raw_bytes, HZ5_POLICY_MIN_ALIGN);
  if (!raw_ptr) {
    return NULL;
  }

  uintptr_t raw = (uintptr_t)raw_ptr;
  uintptr_t aligned =
      (raw + sizeof(Hz5WrapperHdr) + align - 1u) & ~(uintptr_t)(align - 1u);
  if ((aligned & (align - 1u)) != 0 || aligned + size > raw + raw_bytes) {
    _aligned_free(raw_ptr);
    return NULL;
  }

  Hz5WrapperHdr* header = (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
  hz5_wrapper_init(header, raw, aligned, size, raw_bytes,
                   HZ5_WRAPPER_SOURCE_P20_CRT_BYPASS);
  hz5_hz3_fallback_note_crt_bypass_alloc();
  return (void*)aligned;
}
#endif

void* hz5_policy_alloc_aligned(size_t size, size_t align,
                               const Hz5PolicyHooks* hooks) {
#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  hz5_hz3_fallback_register_once();
#endif

#if BENCHLAB_HZ5_P12_ROUTE_SHADOW
  hz5_route_p12_on_alloc(size, align);
#endif

  if (hz5_route_exact_a8192(size, align)) {
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
    if (size == 65536u && align == (HZ5_POLICY_PAGE_SIZE * (size_t)2)) {
      void* wrapped = hz5_policy_p25_wrapper_alloc(size, align);
      if (wrapped) {
        return wrapped;
      }
    }
#endif
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || BENCHLAB_HZ5_P24_RAW64K_A8192
    if (size == 65536u && align == (HZ5_POLICY_PAGE_SIZE * (size_t)2) &&
        hooks && hooks->p17_wrapper_alloc) {
      void* wrapped = hooks->p17_wrapper_alloc(size, align);
      if (wrapped) {
        return wrapped;
      }
    }
#endif
#if BENCHLAB_HZ5_P16_WRAPPER_64K_A8192
    if (size == 65536u && align == (HZ5_POLICY_PAGE_SIZE * (size_t)2)) {
      void* wrapped = hz5_policy_wrapper_alloc_with_hz3(size, align);
      if (wrapped) {
        return wrapped;
      }
    }
#endif
    void* ptr = hz5_p2_alloc_aligned(size, align);
    if (ptr) {
      hz5_policy_register_stats_once();
      atomic_store_explicit(&g_hz5_policy_seen_allocation, 1,
                            memory_order_relaxed);
      return ptr;
    }
  }

  if (align <= HZ5_POLICY_MIN_ALIGN ||
      hz5_route_hz3_medium_can_satisfy(size, align)) {
#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
    return hz5_policy_control_fallback_alloc(size, align);
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
    if (size <= 16u && align <= HZ5_POLICY_MIN_ALIGN) {
      void* bypass = hz5_policy_p20_crt_bypass_alloc(size, align);
      if (bypass) {
        return bypass;
      }
    }
    return hz5_hz3_fallback_alloc(size, align);
#else
    return hz3_malloc(size);
#endif
  }

#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
  return hz5_policy_control_fallback_alloc(size, align);
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  return hz5_hz3_fallback_alloc(size, align);
#else
  return hz3_large_aligned_alloc(align, size);
#endif
}

void hz5_policy_free(void* ptr, const Hz5PolicyHooks* hooks) {
  if (atomic_load_explicit(&g_hz5_policy_seen_allocation,
                           memory_order_relaxed) &&
      hz5_p1_owns(ptr)) {
    hz5_p2_free(ptr);
    return;
  }

#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK && !BENCHLAB_HZ5_P16_WRAPPER_64K_A8192 && \
    !BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 && !BENCHLAB_HZ5_P24_RAW64K_A8192 && \
    !BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 && \
    !BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
  uint32_t p20_fb_state = hz5_hz3_fallback_state();
  if (p20_fb_state == HZ5_HZ3_FALLBACK_READY ||
      p20_fb_state == HZ5_HZ3_FALLBACK_LOADING) {
    hz5_hz3_fallback_free(ptr);
    return;
  }
#endif

#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK && \
    (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192)
  uint32_t p25_fb_state = hz5_hz3_fallback_state();
  int p25_lowpage_lookup = hz5_lowpage64_lookup(ptr);
  if ((p25_fb_state == HZ5_HZ3_FALLBACK_READY ||
       p25_fb_state == HZ5_HZ3_FALLBACK_LOADING) &&
      p25_lowpage_lookup == HZ5_LOWPAGE64_LOOKUP_MISS) {
    hz5_hz3_fallback_free(ptr);
    return;
  }
#else
  int p25_lowpage_lookup = HZ5_LOWPAGE64_LOOKUP_MISS;
#endif

#if BENCHLAB_HZ5_P16_WRAPPER_64K_A8192 || BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || \
    BENCHLAB_HZ5_P24_RAW64K_A8192 || BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192 || \
    BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
  if (p25_lowpage_lookup == HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE) {
    return;
  }
#endif
  Hz5WrapperHdr* wrapped = NULL;
  if (hz5_wrapper_decode(ptr, &wrapped)) {
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
    if (wrapped->source == HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE) {
      hz5_lowpage64_release((void*)wrapped->raw);
      return;
    }
#endif
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || BENCHLAB_HZ5_P24_RAW64K_A8192
    if ((wrapped->source == HZ5_WRAPPER_SOURCE_P17_PAGEBIN ||
         wrapped->source == HZ5_WRAPPER_SOURCE_P24_RAW64K) &&
        hooks && hooks->p17_raw_release) {
      hooks->p17_raw_release((void*)wrapped->raw);
      return;
    }
#endif
#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
    if (wrapped->source == HZ5_WRAPPER_SOURCE_P20_CRT_BYPASS) {
      hz5_hz3_fallback_note_crt_bypass_free();
      _aligned_free((void*)wrapped->raw);
      return;
    }
#endif
#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
    hz5_policy_control_fallback_free((void*)wrapped->raw);
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
    hz5_hz3_fallback_free((void*)wrapped->raw);
#else
    hz3_free((void*)wrapped->raw);
#endif
    return;
  }
#if (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192) && BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  if (p25_lowpage_lookup != HZ5_LOWPAGE64_LOOKUP_MISS) {
    return;
  }
#endif
#endif

#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
  hz5_policy_control_fallback_free(ptr);
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  hz5_hz3_fallback_free(ptr);
#else
  hz3_free(ptr);
#endif
}
