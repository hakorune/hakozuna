#include "hz5_policy.h"

#include "hz5_hz3_fallback.h"
#include "hz5_lowpage64.h"
#include "hz5_route.h"
#include "hz5_trace.h"
#include "hz5_wrapper.h"

#include <malloc.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#if !defined(_WIN32)
void* _aligned_malloc(size_t size, size_t alignment);
void _aligned_free(void* ptr);
#endif

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

#ifndef BENCHLAB_HZ5_P43_UNSAFE_NO_LOOKUP
#define BENCHLAB_HZ5_P43_UNSAFE_NO_LOOKUP 0
#endif

#ifndef BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE
#define BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE 0
#endif

#ifndef BENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP
#define BENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP 0
#endif

#ifndef BENCHLAB_HZ5_P43_WRAPPER_TOKEN
#define BENCHLAB_HZ5_P43_WRAPPER_TOKEN 0
#endif

#ifndef BENCHLAB_HZ5_P43_WRAPPER_TOKEN_BRIDGE
#define BENCHLAB_HZ5_P43_WRAPPER_TOKEN_BRIDGE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
#define BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_CAS
#define BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_CAS 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_COOKIE
#define BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_COOKIE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_READONLY_STATE
#define BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_READONLY_STATE 0
#endif

#ifndef BENCHLAB_HZ5_P43_PREPARED_RELEASE
#define BENCHLAB_HZ5_P43_PREPARED_RELEASE 0
#endif

#ifndef BENCHLAB_HZ5_P43_PREPARED_BRIDGE
#define BENCHLAB_HZ5_P43_PREPARED_BRIDGE 0
#endif

#define HZ5_POLICY_P43_PREPARED_ANY \
  (BENCHLAB_HZ5_P43_PREPARED_RELEASE || \
   BENCHLAB_HZ5_P43_PREPARED_BRIDGE)

#ifndef BENCHLAB_HZ5_NO_HZ3_FALLBACK
#define BENCHLAB_HZ5_NO_HZ3_FALLBACK 0
#endif

#ifndef BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
#define BENCHLAB_HZ5_LAZY_HZ3_FALLBACK 0
#endif

#ifndef BENCHLAB_HZ5_STANDALONE_EXACT_ONLY
#define BENCHLAB_HZ5_STANDALONE_EXACT_ONLY 0
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
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
static _Atomic uint32_t g_hz5_policy_bridge_generation;
static uintptr_t g_hz5_policy_bridge_secret_anchor;
#endif

static void hz5_policy_register_stats_once(void) {
  static atomic_flag registered = ATOMIC_FLAG_INIT;
  if (!atomic_flag_test_and_set_explicit(&registered, memory_order_acq_rel)) {
    atexit(hz5_stats_print_once);
  }
}

static void hz5_policy_register_observers_once(void) {
  hz5_trace_register_once();
}

#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK || HZ5_POLICY_P43_PREPARED_ANY
static int hz5_policy_prepare_p25_lowpage_free(
    void* ptr,
    int* lookup_out,
    Hz5Lowpage64FreeCtx* lowpage_ctx) {
  if (lookup_out) {
    *lookup_out = HZ5_LOWPAGE64_LOOKUP_MISS;
  }
  if (lowpage_ctx) {
    lowpage_ctx->lookup_kind = HZ5_LOWPAGE64_LOOKUP_MISS;
    lowpage_ctx->segment_token = NULL;
    lowpage_ctx->slot_base = NULL;
    lowpage_ctx->slot_size = 0;
    lowpage_ctx->slot_index = 0;
    lowpage_ctx->flags = 0;
  }

#if BENCHLAB_HZ5_P43_UNSAFE_NO_LOOKUP
  if (lookup_out) {
    *lookup_out = HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE;
  }
  if (lowpage_ctx) {
    lowpage_ctx->lookup_kind = HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE;
  }
  (void)ptr;
  return 0;
#else
#if HZ5_POLICY_P43_PREPARED_ANY
  int lowpage_lookup = hz5_lowpage64_prepare_free_user(ptr, lowpage_ctx);
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  int lowpage_lookup = hz5_lowpage64_lookup(ptr);
#else
  int lowpage_lookup = HZ5_LOWPAGE64_LOOKUP_MISS;
  (void)ptr;
#endif
  if (lookup_out) {
    *lookup_out = lowpage_lookup;
  }
#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  uint32_t fb_state = hz5_hz3_fallback_state();
  if ((fb_state == HZ5_HZ3_FALLBACK_READY ||
       fb_state == HZ5_HZ3_FALLBACK_LOADING) &&
      lowpage_lookup == HZ5_LOWPAGE64_LOOKUP_MISS) {
    hz5_hz3_fallback_free(ptr);
    return 1;
  }
#endif
  return 0;
#endif
}
#endif

#if HZ5_POLICY_P43_PREPARED_ANY
static int hz5_policy_p25_raw_matches_lowpage_ctx(
    const Hz5Lowpage64FreeCtx* lowpage_ctx,
    uintptr_t raw) {
  if (!lowpage_ctx ||
      lowpage_ctx->lookup_kind != HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE ||
      !lowpage_ctx->slot_base) {
    return 1;
  }
  return (uintptr_t)lowpage_ctx->slot_base == raw;
}
#endif

static void hz5_policy_free_p25_lowpage_raw(
    const Hz5Lowpage64FreeCtx* lowpage_ctx,
    uintptr_t raw) {
#if HZ5_POLICY_P43_PREPARED_ANY
  if (hz5_lowpage64_release_prepared(lowpage_ctx, (void*)raw)) {
    hz5_trace_inc(HZ5_TRACE_FREE_P43_LOOKUP_PREPARED);
    return;
  }
  hz5_lowpage64_p43g_note_old_path();
#else
  (void)lowpage_ctx;
#endif
  hz5_trace_inc(HZ5_TRACE_FREE_P25_BRIDGE);
  hz5_lowpage64_release((void*)raw);
}
#endif

#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
static uint64_t hz5_policy_bridge_attr_cookie(uintptr_t raw,
                                              uintptr_t aligned,
                                              size_t raw_bytes,
                                              uint32_t source,
                                              uint32_t generation) {
  uint64_t mixed =
      (uint64_t)raw ^ ((uint64_t)aligned << 7) ^ ((uint64_t)aligned >> 3) ^
      ((uint64_t)raw_bytes * UINT64_C(0x9e3779b97f4a7c15)) ^
      ((uint64_t)source << 32) ^ (uint64_t)generation ^
      (uint64_t)(uintptr_t)&g_hz5_policy_bridge_secret_anchor ^
      UINT64_C(0x25a8192b6d3f51c7);
  mixed ^= mixed >> 30;
  mixed *= UINT64_C(0xbf58476d1ce4e5b9);
  mixed ^= mixed >> 27;
  mixed *= UINT64_C(0x94d049bb133111eb);
  mixed ^= mixed >> 31;
  return mixed ? mixed : UINT64_C(0x25);
}

static void hz5_policy_bridge_attr_init(Hz5WrapperHdr* header,
                                        uintptr_t aligned) {
  uint32_t generation = atomic_fetch_add_explicit(
                            &g_hz5_policy_bridge_generation, 1u,
                            memory_order_relaxed) +
                        1u;
  if (generation == 0u) {
    generation = atomic_fetch_add_explicit(
                     &g_hz5_policy_bridge_generation, 1u,
                     memory_order_relaxed) +
                 1u;
  }
  header->bridge_generation = generation;
  header->bridge_cookie = hz5_policy_bridge_attr_cookie(
      header->raw, aligned, header->raw_bytes, header->source, generation);
  atomic_store_explicit(&header->bridge_state,
                        HZ5_BRIDGE_ATTR_STATE_ACTIVE, memory_order_release);
}

static int hz5_policy_bridge_attr_verify_and_mark_free(
    Hz5WrapperHdr* header,
    uintptr_t aligned) {
  if (!header ||
      header->source != HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE ||
      header->raw_bytes < header->requested) {
    hz5_trace_inc(HZ5_TRACE_BRIDGE_ATTR_INVALID);
    return 0;
  }

#if !BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_COOKIE
  if (header->bridge_cookie != hz5_policy_bridge_attr_cookie(
                                   header->raw, aligned, header->raw_bytes,
                                   header->source,
                                   header->bridge_generation)) {
    hz5_trace_inc(HZ5_TRACE_BRIDGE_ATTR_INVALID);
    return 0;
  }
#endif

#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_READONLY_STATE
  uint32_t state =
      atomic_load_explicit(&header->bridge_state, memory_order_acquire);
  if (state != HZ5_BRIDGE_ATTR_STATE_ACTIVE) {
    hz5_trace_inc(HZ5_TRACE_BRIDGE_ATTR_CAS_FAIL);
    return 0;
  }
#elif BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_CAS
  uint32_t state =
      atomic_load_explicit(&header->bridge_state, memory_order_acquire);
  if (state != HZ5_BRIDGE_ATTR_STATE_ACTIVE) {
    hz5_trace_inc(HZ5_TRACE_BRIDGE_ATTR_CAS_FAIL);
    return 0;
  }
  atomic_store_explicit(&header->bridge_state, HZ5_BRIDGE_ATTR_STATE_FREED,
                        memory_order_release);
#else
  uint32_t expected = HZ5_BRIDGE_ATTR_STATE_ACTIVE;
  if (!atomic_compare_exchange_strong_explicit(
          &header->bridge_state, &expected, HZ5_BRIDGE_ATTR_STATE_FREED,
          memory_order_acq_rel, memory_order_acquire)) {
    hz5_trace_inc(HZ5_TRACE_BRIDGE_ATTR_CAS_FAIL);
    return 0;
  }
#endif

  hz5_trace_inc(HZ5_TRACE_BRIDGE_ATTR_VALID);
  return 1;
}
#endif

#if BENCHLAB_HZ5_P43_WRAPPER_TOKEN
static uint32_t hz5_policy_p43_token_cookie(uintptr_t raw,
                                            uintptr_t aligned,
                                            uintptr_t segment_token,
                                            uint32_t slot_index) {
  uintptr_t mixed = raw ^ aligned ^ segment_token ^
                    ((uintptr_t)slot_index * UINT64_C(0x9e3779b97f4a7c15)) ^
                    UINT64_C(0x43a8192d5);
  mixed ^= mixed >> 33;
  mixed *= UINT64_C(0xff51afd7ed558ccd);
  mixed ^= mixed >> 33;
  return (uint32_t)(mixed ^ (mixed >> 32));
}

static int hz5_policy_p43_store_wrapper_token(Hz5WrapperHdr* header,
                                              uintptr_t aligned,
                                              const Hz5Lowpage64FreeCtx* ctx) {
  Hz5Lowpage64FreeCtx fallback_ctx = {0};
  const Hz5Lowpage64FreeCtx* token_ctx = ctx;
  if (!token_ctx ||
      token_ctx->lookup_kind != HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE) {
    int lookup =
        hz5_lowpage64_prepare_free_raw((void*)header->raw, &fallback_ctx);
    if (lookup != HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE) {
      return 0;
    }
    token_ctx = &fallback_ctx;
  }
  if (token_ctx->slot_base != (void*)header->raw ||
      !token_ctx->segment_token) {
    return 0;
  }
  header->p43_segment_token = (uintptr_t)token_ctx->segment_token;
  header->p43_slot_index = token_ctx->slot_index;
  header->p43_token_cookie = hz5_policy_p43_token_cookie(
      header->raw, aligned, header->p43_segment_token,
      header->p43_slot_index);
  return 1;
}

static int hz5_policy_p43_release_wrapper_token(Hz5WrapperHdr* header,
                                                uintptr_t aligned) {
  if (!header->p43_segment_token ||
      header->p43_token_cookie != hz5_policy_p43_token_cookie(
                                      header->raw, aligned,
                                      header->p43_segment_token,
                                      header->p43_slot_index)) {
    hz5_trace_inc(HZ5_TRACE_WRAPPER_TOKEN_INVALID);
    return 0;
  }
  hz5_trace_inc(HZ5_TRACE_WRAPPER_TOKEN_VALID);
  if (!BENCHLAB_HZ5_P43_WRAPPER_TOKEN_BRIDGE) {
    hz5_trace_inc(HZ5_TRACE_FREE_P43_TOKEN_DIRECT);
    return hz5_lowpage64_release_p43_token(
        (void*)header->p43_segment_token, header->p43_slot_index,
        (void*)header->raw);
  }
  hz5_trace_inc(HZ5_TRACE_FREE_P43_TOKEN_BRIDGE);
  hz5_lowpage64_release((void*)header->raw);
  return 1;
}
#endif

#if BENCHLAB_HZ5_P16_WRAPPER_64K_A8192 || BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || \
    BENCHLAB_HZ5_P24_RAW64K_A8192 || BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192 || \
    BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
static void hz5_policy_free_decoded_fallback_raw(void* raw) {
#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
  _aligned_free(raw);
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  hz5_hz3_fallback_free(raw);
#else
  hz3_free(raw);
#endif
}
#endif

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
  hz5_policy_register_observers_once();

  size_t raw_bytes =
      hz5_lowpage64_round_raw_bytes(size, align, sizeof(Hz5WrapperHdr));
#if BENCHLAB_HZ5_P43_WRAPPER_TOKEN
  Hz5Lowpage64FreeCtx token_ctx = {0};
  void* raw_ptr = hz5_lowpage64_acquire_p43_token(raw_bytes, &token_ctx);
  if (raw_ptr) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_P43_TOKEN);
  }
#else
  void* raw_ptr = hz5_lowpage64_acquire(raw_bytes);
#endif
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
#if BENCHLAB_HZ5_P43_WRAPPER_TOKEN
  if (!hz5_policy_p43_store_wrapper_token(header, aligned, &token_ctx)) {
    hz5_lowpage64_release(raw_ptr);
    return NULL;
  }
#elif BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
  hz5_policy_bridge_attr_init(header, aligned);
#endif
  hz5_trace_inc(HZ5_TRACE_ALLOC_P25_BRIDGE);
  return (void*)aligned;
}
#endif

#if BENCHLAB_HZ5_NO_HZ3_FALLBACK || BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
static void* hz5_policy_crt_wrapped_alloc(size_t size, size_t align) {
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
#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  hz5_hz3_fallback_note_crt_bypass_alloc();
#endif
  hz5_trace_inc(HZ5_TRACE_ALLOC_P25_BRIDGE);
  return (void*)aligned;
}
#endif

void* hz5_policy_alloc_aligned(size_t size, size_t align,
                               const Hz5PolicyHooks* hooks) {
  hz5_policy_register_observers_once();
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

#if BENCHLAB_HZ5_STANDALONE_EXACT_ONLY
  return NULL;
#endif

  if (align <= HZ5_POLICY_MIN_ALIGN ||
      hz5_route_hz3_medium_can_satisfy(size, align)) {
#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
    return hz5_policy_crt_wrapped_alloc(size, align);
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
    if (size <= 16u && align <= HZ5_POLICY_MIN_ALIGN) {
      void* bypass = hz5_policy_crt_wrapped_alloc(size, align);
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
  return hz5_policy_crt_wrapped_alloc(size, align);
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

#if (BENCHLAB_HZ5_LAZY_HZ3_FALLBACK || HZ5_POLICY_P43_PREPARED_ANY) && \
    (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192)
  int p25_lowpage_lookup = HZ5_LOWPAGE64_LOOKUP_MISS;
  Hz5Lowpage64FreeCtx p25_lowpage_ctx = {0};
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR || \
    BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE || \
    BENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP || \
    BENCHLAB_HZ5_P43_WRAPPER_TOKEN
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
  (void)p25_lowpage_lookup;
  (void)p25_lowpage_ctx;
#elif BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE
  (void)p25_lowpage_lookup;
  (void)p25_lowpage_ctx;
#elif BENCHLAB_HZ5_P43_WRAPPER_TOKEN
  (void)p25_lowpage_lookup;
  (void)p25_lowpage_ctx;
#endif
#else
  if (hz5_policy_prepare_p25_lowpage_free(ptr, &p25_lowpage_lookup,
                                          &p25_lowpage_ctx)) {
    return;
  }
#endif
#else
  int p25_lowpage_lookup = HZ5_LOWPAGE64_LOOKUP_MISS;
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
  Hz5Lowpage64FreeCtx p25_lowpage_ctx = {0};
#endif
#endif

#if BENCHLAB_HZ5_P16_WRAPPER_64K_A8192 || BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || \
    BENCHLAB_HZ5_P24_RAW64K_A8192 || BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192 || \
    BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
  if (p25_lowpage_lookup == HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE) {
    /* HZ5 owns this range, but it is not an active allocation. Do not let a
       stale/nonactive HZ5 pointer fall through to HZ3 fallback free. */
    return;
  }
#endif
  Hz5WrapperHdr* wrapped = NULL;
  if (hz5_wrapper_decode(ptr, &wrapped)) {
    hz5_trace_inc(HZ5_TRACE_WRAPPER_DECODE_OK);
#if HZ5_POLICY_P43_PREPARED_ANY && \
    BENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP && \
    (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192)
    if (wrapped->source == HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE) {
      p25_lowpage_lookup =
          hz5_lowpage64_prepare_free_raw((void*)wrapped->raw,
                                         &p25_lowpage_ctx);
    }
#endif
#if HZ5_POLICY_P43_PREPARED_ANY && \
    (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192)
#if BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE
    hz5_lowpage64_p43g_note_wrapper(
        wrapped->source == HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE, 1);
#else
    hz5_lowpage64_p43g_note_wrapper(
        wrapped->source == HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE,
        hz5_policy_p25_raw_matches_lowpage_ctx(&p25_lowpage_ctx,
                                               wrapped->raw));
#endif
#endif
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
    if (wrapped->source == HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE) {
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
      if (hz5_policy_bridge_attr_verify_and_mark_free(wrapped,
                                                      (uintptr_t)ptr)) {
        hz5_trace_inc(HZ5_TRACE_FREE_P25_BRIDGE_ATTR);
        hz5_lowpage64_release((void*)wrapped->raw);
      } else {
        hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
      }
#elif BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE
      hz5_trace_inc(HZ5_TRACE_FREE_TRUSTWRAP);
      hz5_lowpage64_release((void*)wrapped->raw);
#elif BENCHLAB_HZ5_P43_WRAPPER_TOKEN
      if (!hz5_policy_p43_release_wrapper_token(wrapped, (uintptr_t)ptr)) {
        hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
      }
#elif BENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP
      hz5_trace_inc(HZ5_TRACE_FREE_RAWLOOKUP);
      if (p25_lowpage_lookup == HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE) {
        hz5_policy_free_p25_lowpage_raw(&p25_lowpage_ctx, wrapped->raw);
      }
#else
      hz5_policy_free_p25_lowpage_raw(&p25_lowpage_ctx, wrapped->raw);
#endif
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
    if (wrapped->source == HZ5_WRAPPER_SOURCE_P20_CRT_BYPASS) {
#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
      hz5_hz3_fallback_note_crt_bypass_free();
#endif
      _aligned_free((void*)wrapped->raw);
      return;
    }
    hz5_policy_free_decoded_fallback_raw((void*)wrapped->raw);
    return;
  }
  hz5_trace_inc(HZ5_TRACE_WRAPPER_DECODE_MISS);
#if (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192) && BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  if (p25_lowpage_lookup != HZ5_LOWPAGE64_LOOKUP_MISS) {
    return;
  }
#endif
#endif

#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
  /* No-HZ3 control fallback allocations are wrapper-tagged above. Reaching
     here means the pointer was not produced by this allocator. */
  (void)ptr;
  hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
#elif BENCHLAB_HZ5_STANDALONE_EXACT_ONLY
  (void)ptr;
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  hz5_hz3_fallback_free(ptr);
#else
  hz3_free(ptr);
#endif
}
