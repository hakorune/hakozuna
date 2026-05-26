#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P
typedef Hz5PolicyLocal2PNode Hz5LinuxLocal2PNode;

#if BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
static Hz5PolicyLocal2PTls* hz5_policy_local2p_tls(void) {
  return &g_hz5_policy_local2p_tls;
}
#endif

static uintptr_t hz5_policy_local2p_owner_token(void) {
#if BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
  Hz5PolicyLocal2PTls* tls = hz5_policy_local2p_tls();
#if BENCHLAB_HZ5_LINUX_LOCAL2P_OWNER_INBOX
  tls->owner_token = (uintptr_t)tls;
  return tls->owner_token;
#else
  return hz5_policy_local2p_common_owner_token(&tls->owner_token,
                                               (uintptr_t)&tls->owner_token);
#endif
#else
  return hz5_policy_local2p_common_owner_token(
      &g_hz5_policy_local2p_owner_token,
      (uintptr_t)&g_hz5_policy_local2p_owner_token);
#endif
}

static uint64_t hz5_policy_local2p_cookie(uintptr_t raw,
                                          uintptr_t aligned,
                                          size_t raw_bytes,
                                          uint32_t generation,
                                          uintptr_t owner) {
#if BENCHLAB_HZ5_LINUX_LOCAL2P_FAST_COOKIE
  (void)raw_bytes;
  (void)generation;
  (void)owner;
  return hz5_policy_local2p_common_cookie_fast(
      raw, aligned, (uintptr_t)&g_hz5_policy_local2p_secret_anchor,
      UINT64_C(0x2f125a8192c0ffee), UINT64_C(0x2f12));
#else
  return hz5_policy_local2p_common_cookie_full(
      raw, aligned, raw_bytes, generation, owner,
      (uintptr_t)&g_hz5_policy_local2p_secret_anchor,
      UINT64_C(0x2f125a8192c0ffee), UINT64_C(0x2f12));
#endif
}

static int hz5_policy_local2p_exact(size_t size, size_t align) {
  return hz5_policy_local2p_common_exact(
      size, align, HZ5_POLICY_LOCAL2P_EXACT_SIZE,
      BENCHLAB_HZ5_LINUX_LOCAL2P_ALIGN);
}

#include "hz5_policy_local2p_cache.inc"
#include "hz5_policy_local2p_remote.inc"

static void* hz5_policy_local2p_os_alloc(void) {
  void* raw_ptr = NULL;
  int rc = posix_memalign(&raw_ptr, BENCHLAB_HZ5_LINUX_LOCAL2P_ALIGN,
                          BENCHLAB_HZ5_LINUX_LOCAL2P_RAW_BYTES);
  if (rc != 0) {
    return NULL;
  }
  hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_OS);
  return raw_ptr;
}

static void hz5_policy_local2p_init_header(Hz5WrapperHdr* header,
                                           uintptr_t aligned) {
#if BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
  Hz5PolicyLocal2PTls* tls = hz5_policy_local2p_tls();
  uint32_t generation =
      hz5_policy_local2p_common_next_generation(&tls->generation);
#else
  uint32_t generation = hz5_policy_local2p_common_next_generation(
      &g_hz5_policy_local2p_generation);
#endif
  uintptr_t owner = hz5_policy_local2p_owner_token();
#if BENCHLAB_HZ5_LINUX_LOCAL2P_NO_COOKIE
  uint64_t cookie = UINT64_C(0x2f12);
#else
  uint64_t cookie =
      hz5_policy_local2p_cookie(header->raw, aligned, header->raw_bytes,
                                generation, owner);
#endif
  hz5_policy_local2p_common_activate(header, owner, generation, cookie);
}

static inline int hz5_policy_local2p_validate_free_header(Hz5WrapperHdr* header,
                                                          uintptr_t aligned) {
#if BENCHLAB_HZ5_LINUX_LOCAL2P_SLIM_CHECK
  if (!header) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE);
    return 0;
  }
#else
  if (!hz5_policy_local2p_common_header_matches(
          header, HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P,
          HZ5_POLICY_LOCAL2P_EXACT_SIZE,
          BENCHLAB_HZ5_LINUX_LOCAL2P_RAW_BYTES)) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE);
    return 0;
  }
#endif

#if !BENCHLAB_HZ5_LINUX_LOCAL2P_NO_COOKIE
  uint64_t expected_cookie = hz5_policy_local2p_cookie(
      header->raw, aligned, header->raw_bytes, header->local2p_generation,
      header->local2p_owner);
  if (header->local2p_cookie != expected_cookie) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE);
    return 0;
  }
#endif

  return 1;
}

static inline Hz5FreeResult
hz5_policy_local2p_recycle_local(Hz5WrapperHdr* header, uintptr_t aligned) {
  (void)hz5_policy_local2p_push(
      hz5_policy_local2p_node_from_header(header, aligned));
  return HZ5_FREE_OK_HZ5;
}

static inline Hz5FreeResult
hz5_policy_local2p_recycle_remote(Hz5WrapperHdr* header, uintptr_t aligned) {
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE);
  void* node_ptr = hz5_policy_local2p_node_from_header(header, aligned);
#if BENCHLAB_HZ5_LINUX_LOCAL2P_OWNER_INBOX && \
    BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
#if BENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH
  if (hz5_policy_local2p_remote_batch_push(header->local2p_owner, node_ptr)) {
    return HZ5_FREE_OK_HZ5;
  }
#endif
  if (hz5_policy_local2p_inbox_push(header->local2p_owner, node_ptr)) {
    return HZ5_FREE_OK_HZ5;
  }
#endif
  (void)hz5_policy_local2p_global_push(node_ptr);
  return HZ5_FREE_OK_HZ5;
}

static void* hz5_policy_local2p_alloc(size_t size, size_t align) {
  if (!hz5_policy_local2p_exact(size, align)) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE);
    return NULL;
  }

  void* node_ptr = hz5_policy_local2p_pop();
  int tls_reuse = node_ptr != NULL;
#if BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_FAST_RETURN && \
    BENCHLAB_HZ5_LINUX_LOCAL2P_OBJECT_NODE && \
    BENCHLAB_HZ5_LINUX_LOCAL2P_REUSE_STATE_ONLY
  if (tls_reuse) {
    uintptr_t aligned = (uintptr_t)node_ptr;
    Hz5WrapperHdr* header =
        (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
    atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_ACTIVE,
                          memory_order_release);
    return (void*)aligned;
  }
#endif
#if BENCHLAB_HZ5_LINUX_LOCAL2P_OWNER_INBOX && \
    BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
  if (!node_ptr) {
    node_ptr = hz5_policy_local2p_inbox_pop();
  }
#endif
  if (!node_ptr) {
    node_ptr = hz5_policy_local2p_global_pop();
  }
  void* raw_ptr = node_ptr ? (void*)hz5_policy_local2p_raw_from_node(node_ptr)
                           : NULL;
  if (!raw_ptr) {
    raw_ptr = hz5_policy_local2p_os_alloc();
  }
  if (!raw_ptr) {
    return NULL;
  }
  int cached_node = node_ptr != NULL;

  uintptr_t raw = (uintptr_t)raw_ptr;
#if BENCHLAB_HZ5_LINUX_LOCAL2P_OBJECT_NODE
  uintptr_t aligned = node_ptr
                          ? (uintptr_t)node_ptr
                          : hz5_policy_local2p_common_align_user(raw, align);
#else
  uintptr_t aligned = hz5_policy_local2p_common_align_user(raw, align);
#endif
  if ((aligned & (align - 1u)) != 0 ||
      !hz5_policy_local2p_common_layout_fits(
          raw, aligned, size, BENCHLAB_HZ5_LINUX_LOCAL2P_RAW_BYTES)) {
    free(raw_ptr);
    return NULL;
  }

  Hz5WrapperHdr* header = (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
#if BENCHLAB_HZ5_LINUX_LOCAL2P_OBJECT_NODE
  if (!cached_node) {
#endif
#if BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_INIT
    hz5_wrapper_init_prefix(header, raw, aligned, size,
                            BENCHLAB_HZ5_LINUX_LOCAL2P_RAW_BYTES,
                            HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P);
#else
    hz5_wrapper_init(header, raw, aligned, size,
                     BENCHLAB_HZ5_LINUX_LOCAL2P_RAW_BYTES,
                     HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P);
#endif
#if BENCHLAB_HZ5_LINUX_LOCAL2P_OBJECT_NODE
  }
#endif
#if BENCHLAB_HZ5_LINUX_LOCAL2P_REUSE_STATE_ONLY
  if (tls_reuse) {
    atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_ACTIVE,
                          memory_order_release);
  } else {
    hz5_policy_local2p_init_header(header, aligned);
  }
#else
  hz5_policy_local2p_init_header(header, aligned);
#endif
  return (void*)aligned;
}

static Hz5FreeResult hz5_policy_local2p_free(Hz5WrapperHdr* header,
                                             uintptr_t aligned) {
  if (!hz5_policy_local2p_validate_free_header(header, aligned)) {
    return HZ5_FREE_INVALID;
  }

  uintptr_t current_owner = hz5_policy_local2p_owner_token();

#if BENCHLAB_HZ5_LINUX_LOCAL2P_NO_CAS
  uint32_t state =
      atomic_load_explicit(&header->local2p_state, memory_order_acquire);
  if (state != HZ5_LOCAL2P_STATE_ACTIVE) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_STATE);
    return HZ5_FREE_INVALID;
  }
  atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_FREED,
                        memory_order_release);
#elif BENCHLAB_HZ5_LINUX_LOCAL2P_SAME_OWNER_FAST_STATE
  if (header->local2p_owner == current_owner) {
    uint32_t state =
        atomic_load_explicit(&header->local2p_state, memory_order_acquire);
    if (state != HZ5_LOCAL2P_STATE_ACTIVE) {
      hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_STATE);
      return HZ5_FREE_INVALID;
    }
    atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_FREED,
                          memory_order_release);
    return hz5_policy_local2p_recycle_local(header, aligned);
  }

  uint32_t old_state = atomic_exchange_explicit(
      &header->local2p_state, HZ5_LOCAL2P_STATE_FREED,
      memory_order_acq_rel);
  if (old_state != HZ5_LOCAL2P_STATE_ACTIVE) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_STATE);
    return HZ5_FREE_INVALID;
  }
#else
  if (!hz5_policy_local2p_common_mark_freed(header)) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_STATE);
    return HZ5_FREE_INVALID;
  }
#endif

  if (header->local2p_owner != current_owner) {
    return hz5_policy_local2p_recycle_remote(header, aligned);
  }

  return hz5_policy_local2p_recycle_local(header, aligned);
}

#include "hz5_policy_local2p_route.inc"
#endif
