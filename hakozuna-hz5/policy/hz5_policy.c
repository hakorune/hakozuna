#include "hz5_policy.h"

#include "hz5_hz3_fallback.h"
#include "hz5_lowpage64.h"
#include "hz5_internal.h"
#include "hz5_route.h"
#include "hz5_smallfront.h"
#include "hz5_policy_local2p_common.h"
#include "hz5_trace.h"
#include "hz5_wrapper.h"

#include <malloc.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(__linux__)
#include <pthread.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#endif

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

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P
#define BENCHLAB_HZ5_LINUX_LOCAL2P 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_SMALLFRONT_S1
#define BENCHLAB_HZ5_LINUX_SMALLFRONT_S1 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
#define BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_ROUTE
#define BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_ROUTE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_INIT
#define BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_INIT 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_RAW_BYTES
#define BENCHLAB_HZ5_LINUX_LOCAL2P_RAW_BYTES ((size_t)131072)
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_ALIGN
#define BENCHLAB_HZ5_LINUX_LOCAL2P_ALIGN ((size_t)8192)
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_CAP
#define BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_CAP ((size_t)1)
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_GLOBAL_CAP
#define BENCHLAB_HZ5_LINUX_LOCAL2P_GLOBAL_CAP ((size_t)1024)
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_NO_COOKIE
#define BENCHLAB_HZ5_LINUX_LOCAL2P_NO_COOKIE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_NO_CAS
#define BENCHLAB_HZ5_LINUX_LOCAL2P_NO_CAS 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_OWNER_INBOX
#define BENCHLAB_HZ5_LINUX_LOCAL2P_OWNER_INBOX 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH
#define BENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH_CAP
#define BENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH_CAP 16u
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_OBJECT_NODE
#define BENCHLAB_HZ5_LINUX_LOCAL2P_OBJECT_NODE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_SAME_OWNER_FAST_STATE
#define BENCHLAB_HZ5_LINUX_LOCAL2P_SAME_OWNER_FAST_STATE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_ROUTE_COOKIE
#define BENCHLAB_HZ5_LINUX_LOCAL2P_ROUTE_COOKIE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_REUSE_STATE_ONLY
#define BENCHLAB_HZ5_LINUX_LOCAL2P_REUSE_STATE_ONLY 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_SLIM_CHECK
#define BENCHLAB_HZ5_LINUX_LOCAL2P_SLIM_CHECK 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_FAST_COOKIE
#define BENCHLAB_HZ5_LINUX_LOCAL2P_FAST_COOKIE 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_FREE_FIRST
#define BENCHLAB_HZ5_LINUX_LOCAL2P_FREE_FIRST 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_FAST_RETURN
#define BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_FAST_RETURN 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_SINGLE_SLOT_TLS
#define BENCHLAB_HZ5_LINUX_LOCAL2P_SINGLE_SLOT_TLS 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_LOCAL_OVERFLOW_GLOBAL
#define BENCHLAB_HZ5_LINUX_LOCAL2P_LOCAL_OVERFLOW_GLOBAL 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P_RETAIN_ARRAY
#define BENCHLAB_HZ5_LINUX_LOCAL2P_RETAIN_ARRAY 0
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P
#define BENCHLAB_HZ5_WIN_LOCAL2P 0
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES
#define BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES ((size_t)131072)
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN
#define BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN ((size_t)8192)
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_TLS_CAP
#define BENCHLAB_HZ5_WIN_LOCAL2P_TLS_CAP ((size_t)1)
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_GLOBAL_CAP
#define BENCHLAB_HZ5_WIN_LOCAL2P_GLOBAL_CAP ((size_t)1024)
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_DIRECT_ROUTE
#define BENCHLAB_HZ5_WIN_LOCAL2P_DIRECT_ROUTE 1
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH
#define BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH 0
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_RETAIN_ARRAY
#define BENCHLAB_HZ5_WIN_LOCAL2P_RETAIN_ARRAY 0
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_FAST_COOKIE
#define BENCHLAB_HZ5_WIN_LOCAL2P_FAST_COOKIE 0
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE
#define BENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE 0
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_SAME_OWNER_FAST_STATE
#define BENCHLAB_HZ5_WIN_LOCAL2P_SAME_OWNER_FAST_STATE 0
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_ROUTE_COOKIE
#define BENCHLAB_HZ5_WIN_LOCAL2P_ROUTE_COOKIE 0
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_REUSE_STATE_ONLY
#define BENCHLAB_HZ5_WIN_LOCAL2P_REUSE_STATE_ONLY 0
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_TLS_FAST_RETURN
#define BENCHLAB_HZ5_WIN_LOCAL2P_TLS_FAST_RETURN 0
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_CAP
#define BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_CAP 16u
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS
#define BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS 1u
#endif

#ifndef BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SWITCH_FLUSH_MIN
#define BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SWITCH_FLUSH_MIN 0u
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

static _Atomic int g_hz5_policy_seen_allocation;
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
static _Atomic uint32_t g_hz5_policy_bridge_generation;
static uintptr_t g_hz5_policy_bridge_secret_anchor;
#endif
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P
static uintptr_t g_hz5_policy_local2p_secret_anchor;
static pthread_mutex_t g_hz5_policy_local2p_global_lock =
    PTHREAD_MUTEX_INITIALIZER;
static void* g_hz5_policy_local2p_global_head;
static size_t g_hz5_policy_local2p_global_count;
#if BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
typedef struct Hz5PolicyLocal2PTls {
  void* head;
  size_t count;
#if BENCHLAB_HZ5_LINUX_LOCAL2P_RETAIN_ARRAY
  void* retained[BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_CAP];
#endif
  uintptr_t owner_token;
  uint32_t generation;
#if BENCHLAB_HZ5_LINUX_LOCAL2P_OWNER_INBOX
  _Atomic(void*) inbox_head;
  void* inbox_cache;
#if BENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH
  uintptr_t remote_batch_owner;
  void* remote_batch_head;
  void* remote_batch_tail;
  size_t remote_batch_count;
#endif
#endif
} Hz5PolicyLocal2PTls;
static _Thread_local Hz5PolicyLocal2PTls g_hz5_policy_local2p_tls;
#else
static _Thread_local void* g_hz5_policy_local2p_head;
static _Thread_local size_t g_hz5_policy_local2p_count;
static _Thread_local uintptr_t g_hz5_policy_local2p_owner_token;
static _Thread_local uint32_t g_hz5_policy_local2p_generation;
#endif
#endif

#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
static uintptr_t g_hz5_policy_win_local2p_secret_anchor;
static SRWLOCK g_hz5_policy_win_local2p_global_lock = SRWLOCK_INIT;
static void* g_hz5_policy_win_local2p_global_head;
static size_t g_hz5_policy_win_local2p_global_count;
typedef struct Hz5PolicyWinLocal2PTls {
  void* head;
  size_t count;
#if BENCHLAB_HZ5_WIN_LOCAL2P_RETAIN_ARRAY
  void* retained[BENCHLAB_HZ5_WIN_LOCAL2P_TLS_CAP];
#endif
  uintptr_t owner_token;
  uint32_t generation;
  _Atomic(void*) inbox_head;
  void* inbox_cache;
#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH && \
    BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS > 1u
  uintptr_t remote_batch_owner[BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS];
  void* remote_batch_head[BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS];
  void* remote_batch_tail[BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS];
  size_t remote_batch_count[BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS];
  size_t remote_batch_next_victim;
  uintptr_t remote_batch_last_owner;
#else
  uintptr_t remote_batch_owner;
  void* remote_batch_head;
  void* remote_batch_tail;
  size_t remote_batch_count;
#endif
} Hz5PolicyWinLocal2PTls;
static _Thread_local Hz5PolicyWinLocal2PTls g_hz5_policy_win_local2p_tls;
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

static Hz5FreeResult hz5_policy_free_p25_lowpage_raw(
    const Hz5Lowpage64FreeCtx* lowpage_ctx,
    uintptr_t raw) {
#if HZ5_POLICY_P43_PREPARED_ANY
  if (hz5_lowpage64_release_prepared(lowpage_ctx, (void*)raw)) {
    hz5_trace_inc(HZ5_TRACE_FREE_P43_LOOKUP_PREPARED);
    return HZ5_FREE_OK_HZ5;
  }
  hz5_lowpage64_p43g_note_old_path();
#else
  (void)lowpage_ctx;
#endif
  hz5_trace_inc(HZ5_TRACE_FREE_P25_BRIDGE);
  hz5_lowpage64_release((void*)raw);
  return HZ5_FREE_OK_HZ5;
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

#include "hz5_policy_local2p.inc"
#include "hz5_policy_win_local2p.inc"

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

static Hz5FreeResult hz5_policy_p43_release_wrapper_token(Hz5WrapperHdr* header,
                                                          uintptr_t aligned) {
  if (!header->p43_segment_token ||
      header->p43_token_cookie != hz5_policy_p43_token_cookie(
                                      header->raw, aligned,
                                      header->p43_segment_token,
                                      header->p43_slot_index)) {
    hz5_trace_inc(HZ5_TRACE_WRAPPER_TOKEN_INVALID);
    return HZ5_FREE_INVALID;
  }
  hz5_trace_inc(HZ5_TRACE_WRAPPER_TOKEN_VALID);
  if (!BENCHLAB_HZ5_P43_WRAPPER_TOKEN_BRIDGE) {
    hz5_trace_inc(HZ5_TRACE_FREE_P43_TOKEN_DIRECT);
    if (!hz5_lowpage64_release_p43_token((void*)header->p43_segment_token,
                                         header->p43_slot_index,
                                         (void*)header->raw)) {
      return HZ5_FREE_INVALID;
    }
    return HZ5_FREE_OK_HZ5;
  }
  hz5_trace_inc(HZ5_TRACE_FREE_P43_TOKEN_BRIDGE);
  hz5_lowpage64_release((void*)header->raw);
  return HZ5_FREE_OK_HZ5;
}
#endif

#if BENCHLAB_HZ5_P16_WRAPPER_64K_A8192 || BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || \
    BENCHLAB_HZ5_P24_RAW64K_A8192 || BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192 || \
    (defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P) || \
    BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
static Hz5FreeResult hz5_policy_free_decoded_fallback_raw(void* raw) {
#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
  _aligned_free(raw);
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  hz5_hz3_fallback_free(raw);
#else
  hz3_free(raw);
#endif
  return HZ5_FREE_OK_FALLBACK;
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
  hz5_trace_inc(HZ5_TRACE_P20_CRT_BYPASS_ALLOC);
  hz5_trace_add(HZ5_TRACE_P20_CRT_BYPASS_ALLOC_BYTES, (uint64_t)raw_bytes);
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

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_SMALLFRONT_S1
  if (hz5_smallfront_can_handle(size, align)) {
    void* small = hz5_smallfront_alloc(size, align);
    if (small) {
      atomic_store_explicit(&g_hz5_policy_seen_allocation, 1,
                            memory_order_relaxed);
    }
    return small;
  }
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P && \
    BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_ROUTE
  if (hz5_policy_local2p_exact(size, align)) {
    void* wrapped = hz5_policy_local2p_alloc(size, align);
    if (wrapped) {
      return wrapped;
    }
  }
#endif
#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P && \
    BENCHLAB_HZ5_WIN_LOCAL2P_DIRECT_ROUTE
  if (hz5_policy_win_local2p_exact(size, align)) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_EXACT_HIT);
    void* wrapped = hz5_policy_win_local2p_alloc(size, align);
    if (wrapped) {
      return wrapped;
    }
  } else {
    hz5_policy_win_local2p_note_escape(size, align);
  }
#endif

  if (hz5_route_exact_a8192(size, align)) {
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P && \
    !BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_ROUTE
    if (hz5_policy_local2p_exact(size, align)) {
      void* wrapped = hz5_policy_local2p_alloc(size, align);
      if (wrapped) {
        return wrapped;
      }
    }
#endif
#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P && \
    !BENCHLAB_HZ5_WIN_LOCAL2P_DIRECT_ROUTE
    if (hz5_policy_win_local2p_exact(size, align)) {
      void* wrapped = hz5_policy_win_local2p_alloc(size, align);
      if (wrapped) {
        return wrapped;
      }
    }
#endif
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

void* hz5_policy_alloc_local2p_64k_a8192(void) {
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P
  hz5_policy_register_observers_once();
  return hz5_policy_local2p_alloc(65536u,
                                  BENCHLAB_HZ5_LINUX_LOCAL2P_ALIGN);
#elif defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
  hz5_policy_register_observers_once();
  return hz5_policy_win_local2p_alloc(65536u,
                                      BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN);
#else
  return NULL;
#endif
}

Hz5FreeResult hz5_policy_free_local2p_64k_a8192(void* ptr) {
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P
  if (!ptr) {
    return HZ5_FREE_OK_HZ5;
  }
  Hz5FreeResult result = HZ5_FREE_INVALID;
  if (hz5_policy_local2p_try_free_direct(ptr, &result)) {
    return result;
  }
  return HZ5_FREE_INVALID;
#elif defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
  if (!ptr) {
    return HZ5_FREE_OK_HZ5;
  }
  Hz5FreeResult result = HZ5_FREE_INVALID;
  if (hz5_policy_win_local2p_try_free_direct(ptr, &result)) {
    return result;
  }
  return HZ5_FREE_INVALID;
#else
  (void)ptr;
  return HZ5_FREE_INVALID;
#endif
}

Hz5FreeResult hz5_policy_free(void* ptr, const Hz5PolicyHooks* hooks) {
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_SMALLFRONT_S1
  Hz5SmallFrontFreeResult small_result = hz5_smallfront_free(ptr);
  if (small_result == HZ5_SMALLFRONT_FREE_OK) {
    return HZ5_FREE_OK_HZ5;
  }
  if (small_result == HZ5_SMALLFRONT_FREE_INVALID) {
    return HZ5_FREE_INVALID;
  }
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P && \
    BENCHLAB_HZ5_LINUX_LOCAL2P_FREE_FIRST
  Hz5FreeResult local2p_result = HZ5_FREE_INVALID;
  if (hz5_policy_local2p_try_free_direct(ptr, &local2p_result)) {
    return local2p_result;
  }
#endif
#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
  Hz5FreeResult win_local2p_result = HZ5_FREE_INVALID;
  if (hz5_policy_win_local2p_try_free_direct(ptr, &win_local2p_result)) {
    return win_local2p_result;
  }
#endif

  if (atomic_load_explicit(&g_hz5_policy_seen_allocation,
                           memory_order_relaxed) &&
      hz5_p1_owns(ptr)) {
    hz5_p2_free(ptr);
    return HZ5_FREE_OK_HZ5;
  }

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P && \
    !BENCHLAB_HZ5_LINUX_LOCAL2P_FREE_FIRST
  Hz5FreeResult local2p_result = HZ5_FREE_INVALID;
  if (hz5_policy_local2p_try_free_direct(ptr, &local2p_result)) {
    return local2p_result;
  }
#endif

#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK && !BENCHLAB_HZ5_P16_WRAPPER_64K_A8192 && \
    !BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 && !BENCHLAB_HZ5_P24_RAW64K_A8192 && \
    !BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 && \
    !BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
  uint32_t p20_fb_state = hz5_hz3_fallback_state();
  if (p20_fb_state == HZ5_HZ3_FALLBACK_READY ||
      p20_fb_state == HZ5_HZ3_FALLBACK_LOADING) {
    hz5_hz3_fallback_free(ptr);
    return HZ5_FREE_OK_FALLBACK;
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
    return HZ5_FREE_OK_FALLBACK;
  }
#endif
#else
  int p25_lowpage_lookup = HZ5_LOWPAGE64_LOOKUP_MISS;
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
  Hz5Lowpage64FreeCtx p25_lowpage_ctx = {0};
#else
  (void)p25_lowpage_lookup;
#endif
#endif

#if BENCHLAB_HZ5_P16_WRAPPER_64K_A8192 || BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || \
    BENCHLAB_HZ5_P24_RAW64K_A8192 || BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192 || \
    (defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P) || \
    BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
  if (p25_lowpage_lookup == HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE) {
    /* HZ5 owns this range, but it is not an active allocation. Do not let a
       stale/nonactive HZ5 pointer fall through to HZ3 fallback free. */
    return HZ5_FREE_INVALID;
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
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P
    if (wrapped->source == HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P) {
      return hz5_policy_local2p_free(wrapped, (uintptr_t)ptr);
    }
    if (hz5_policy_local2p_metadata_present(wrapped)) {
      hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE);
      return HZ5_FREE_INVALID;
    }
#endif
#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
    if (wrapped->source == HZ5_WRAPPER_SOURCE_WIN_LOCAL2P) {
      return hz5_policy_win_local2p_free(wrapped, (uintptr_t)ptr);
    }
    if (hz5_policy_win_local2p_metadata_present(wrapped)) {
      hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE);
      return HZ5_FREE_INVALID;
    }
#endif
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
    if (wrapped->source == HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE) {
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
      if (hz5_policy_bridge_attr_verify_and_mark_free(wrapped,
                                                      (uintptr_t)ptr)) {
        hz5_trace_inc(HZ5_TRACE_FREE_P25_BRIDGE_ATTR);
        hz5_lowpage64_release((void*)wrapped->raw);
        return HZ5_FREE_OK_HZ5;
      } else {
        hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
        return HZ5_FREE_INVALID;
      }
#elif BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE
      hz5_trace_inc(HZ5_TRACE_FREE_TRUSTWRAP);
      hz5_lowpage64_release((void*)wrapped->raw);
      return HZ5_FREE_OK_HZ5;
#elif BENCHLAB_HZ5_P43_WRAPPER_TOKEN
      if (hz5_policy_p43_release_wrapper_token(wrapped, (uintptr_t)ptr) !=
          HZ5_FREE_OK_HZ5) {
        hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
        return HZ5_FREE_INVALID;
      }
      return HZ5_FREE_OK_HZ5;
#elif BENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP
      hz5_trace_inc(HZ5_TRACE_FREE_RAWLOOKUP);
      if (p25_lowpage_lookup == HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE) {
        return hz5_policy_free_p25_lowpage_raw(&p25_lowpage_ctx, wrapped->raw);
      }
      hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
      return HZ5_FREE_INVALID;
#else
      return hz5_policy_free_p25_lowpage_raw(&p25_lowpage_ctx, wrapped->raw);
#endif
    }
#endif
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || BENCHLAB_HZ5_P24_RAW64K_A8192
    if ((wrapped->source == HZ5_WRAPPER_SOURCE_P17_PAGEBIN ||
         wrapped->source == HZ5_WRAPPER_SOURCE_P24_RAW64K) &&
        hooks && hooks->p17_raw_release) {
      hooks->p17_raw_release((void*)wrapped->raw);
      return HZ5_FREE_OK_HZ5;
    }
#endif
    if (wrapped->source == HZ5_WRAPPER_SOURCE_P20_CRT_BYPASS) {
#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
      hz5_hz3_fallback_note_crt_bypass_free();
#endif
      hz5_trace_inc(HZ5_TRACE_P20_CRT_BYPASS_FREE);
      hz5_trace_add(HZ5_TRACE_P20_CRT_BYPASS_FREE_BYTES,
                    (uint64_t)wrapped->raw_bytes);
      _aligned_free((void*)wrapped->raw);
      return HZ5_FREE_OK_FALLBACK;
    }
    return hz5_policy_free_decoded_fallback_raw((void*)wrapped->raw);
  }
  hz5_trace_inc(HZ5_TRACE_WRAPPER_DECODE_MISS);
#if (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192) && BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  if (p25_lowpage_lookup != HZ5_LOWPAGE64_LOOKUP_MISS) {
    return HZ5_FREE_INVALID;
  }
#endif
#endif

#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
  /* No-HZ3 control fallback allocations are wrapper-tagged above. Reaching
     here means the pointer was not produced by this allocator. */
  (void)ptr;
  hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
  return HZ5_FREE_INVALID;
#elif BENCHLAB_HZ5_STANDALONE_EXACT_ONLY
  (void)ptr;
  return HZ5_FREE_INVALID;
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  hz5_hz3_fallback_free(ptr);
  return HZ5_FREE_OK_FALLBACK;
#else
  hz3_free(ptr);
  return HZ5_FREE_OK_FALLBACK;
#endif
}
