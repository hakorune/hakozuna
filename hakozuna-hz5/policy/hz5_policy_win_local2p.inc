#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
typedef Hz5PolicyLocal2PNode Hz5WinLocal2PNode;

static Hz5PolicyWinLocal2PTls* hz5_policy_win_local2p_tls(void) {
  return &g_hz5_policy_win_local2p_tls;
}

static uintptr_t hz5_policy_win_local2p_owner_token(void) {
  Hz5PolicyWinLocal2PTls* tls = hz5_policy_win_local2p_tls();
  return hz5_policy_local2p_common_owner_token(&tls->owner_token,
                                               (uintptr_t)tls);
}

static uint64_t hz5_policy_win_local2p_cookie(uintptr_t raw,
                                              uintptr_t aligned,
                                              size_t raw_bytes,
                                              uint32_t generation,
                                              uintptr_t owner) {
#if BENCHLAB_HZ5_WIN_LOCAL2P_FAST_COOKIE
  (void)raw_bytes;
  (void)generation;
  (void)owner;
  return hz5_policy_local2p_common_cookie_fast(
      raw, aligned, (uintptr_t)&g_hz5_policy_win_local2p_secret_anchor,
      UINT64_C(0x77125a8192c0ffee), UINT64_C(0x7712));
#else
  return hz5_policy_local2p_common_cookie_full(
      raw, aligned, raw_bytes, generation, owner,
      (uintptr_t)&g_hz5_policy_win_local2p_secret_anchor,
      UINT64_C(0x77125a8192c0ffee), UINT64_C(0x7712));
#endif
}

static int hz5_policy_win_local2p_exact(size_t size, size_t align) {
  return hz5_policy_local2p_common_exact(
      size, align, HZ5_POLICY_LOCAL2P_EXACT_SIZE,
      BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN);
}

#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ROUTE
static size_t hz5_policy_win_local2p_sidecar_hash(uintptr_t aligned) {
  uint64_t x = (uint64_t)(aligned >> 13);
  x ^= x >> 33;
  x *= UINT64_C(0xff51afd7ed558ccd);
  x ^= x >> 33;
  return (size_t)(x % BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE);
}

#if BENCHLAB_HZ5_TRACE_LANE
static void hz5_policy_win_local2p_sidecar_note_probe_steps(size_t steps) {
  hz5_trace_add(HZ5_TRACE_LOCAL2P_SIDECAR_PROBE_STEPS, (uint64_t)steps);
  hz5_trace_max(HZ5_TRACE_LOCAL2P_SIDECAR_PROBE_STEPS_MAX,
                (uint64_t)steps);
}
#else
static void hz5_policy_win_local2p_sidecar_note_probe_steps(size_t steps) {
  (void)steps;
}
#endif

static int hz5_policy_win_local2p_sidecar_header_valid(
    const Hz5WrapperHdr* header,
    uintptr_t raw,
    uint32_t generation) {
  if (!header || raw != header->raw ||
      header->magic != HZ5_WRAPPER_HDR_MAGIC ||
      header->layout_version != HZ5_WRAPPER_LAYOUT_VERSION ||
      header->layout_size != sizeof(Hz5WrapperHdr) ||
      header->source != HZ5_WRAPPER_SOURCE_WIN_LOCAL2P ||
      header->requested != HZ5_POLICY_LOCAL2P_EXACT_SIZE ||
      header->raw_bytes != BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES ||
      header->local2p_generation != generation) {
    return 0;
  }
#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_METADATA_ONLY
  return 1;
#else
  uint32_t state =
      atomic_load_explicit(&header->local2p_state, memory_order_acquire);
  return state == HZ5_LOCAL2P_STATE_ACTIVE;
#endif
}
#endif

#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_TABLE
#include "hz5_policy_win_local2p_sidecar_assoc.inc"
#else
#include "hz5_policy_win_local2p_sidecar.inc"
#endif

static void hz5_policy_win_local2p_note_escape(size_t size, size_t align) {
  hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE);
  if (size != HZ5_POLICY_LOCAL2P_EXACT_SIZE) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE_SIZE);
  }
  if (align != BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE_ALIGN);
  }
  if (size != HZ5_POLICY_LOCAL2P_EXACT_SIZE ||
      align != BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE_GUARD);
  }
}

#include "hz5_policy_win_local2p_remote.inc"
#include "hz5_policy_win_local2p_cache.inc"

static void* hz5_policy_win_local2p_os_alloc(void) {
  void* raw_ptr = _aligned_malloc(BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES,
                                  BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN);
  if (raw_ptr) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_OS);
    hz5_trace_add(HZ5_TRACE_LOCAL2P_OS_ALLOC_BYTES,
                  (uint64_t)BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES);
  }
  return raw_ptr;
}

static void hz5_policy_win_local2p_init_header(Hz5WrapperHdr* header,
                                               uintptr_t aligned) {
  Hz5PolicyWinLocal2PTls* tls = hz5_policy_win_local2p_tls();
  uint32_t generation =
      hz5_policy_local2p_common_next_generation(&tls->generation);
  uintptr_t owner = hz5_policy_win_local2p_owner_token();
  uint64_t cookie =
      hz5_policy_win_local2p_cookie(header->raw, aligned, header->raw_bytes,
                                    generation, owner);
  hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_HEADER_INIT);
  hz5_policy_local2p_common_activate(header, owner, generation, cookie);
}

static int
hz5_policy_win_local2p_validate_free_header(Hz5WrapperHdr* header,
                                            uintptr_t aligned) {
  if (!hz5_policy_local2p_common_header_matches(
          header, HZ5_WRAPPER_SOURCE_WIN_LOCAL2P,
          HZ5_POLICY_LOCAL2P_EXACT_SIZE,
          BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES)) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE);
    return 0;
  }
  uint64_t expected_cookie = hz5_policy_win_local2p_cookie(
      header->raw, aligned, header->raw_bytes, header->local2p_generation,
      header->local2p_owner);
  if (header->local2p_cookie != expected_cookie) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE);
    return 0;
  }
  return 1;
}

static void* hz5_policy_win_local2p_alloc(size_t size, size_t align) {
  if (!hz5_policy_win_local2p_exact(size, align)) {
    hz5_policy_win_local2p_note_escape(size, align);
    return NULL;
  }

  void* node_ptr = hz5_policy_win_local2p_pop();
  int tls_reuse = node_ptr != NULL;
#if BENCHLAB_HZ5_WIN_LOCAL2P_INBOX_FAST_ACTIVATE
  int inbox_reuse = 0;
#endif
  (void)tls_reuse;
#if BENCHLAB_HZ5_WIN_LOCAL2P_TLS_FAST_RETURN && \
    BENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE && \
    BENCHLAB_HZ5_WIN_LOCAL2P_REUSE_STATE_ONLY
  if (tls_reuse) {
    uintptr_t aligned = (uintptr_t)node_ptr;
    Hz5WrapperHdr* header =
        (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
    atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_ACTIVE,
                          memory_order_release);
#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ROUTE
    hz5_policy_win_local2p_sidecar_insert(aligned, header);
#endif
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_TLS_FAST_RETURN);
    return (void*)aligned;
  }
#endif
  if (!node_ptr) {
    node_ptr = hz5_policy_win_local2p_inbox_pop();
#if BENCHLAB_HZ5_WIN_LOCAL2P_INBOX_FAST_ACTIVATE
    inbox_reuse = node_ptr != NULL;
#endif
  }
  if (!node_ptr) {
    node_ptr = hz5_policy_win_local2p_global_pop();
  }
  void* raw_ptr = node_ptr ? (void*)hz5_policy_win_local2p_raw_from_node(
                                  node_ptr)
                           : hz5_policy_win_local2p_os_alloc();
  if (!raw_ptr) {
    return NULL;
  }

  uintptr_t raw = (uintptr_t)raw_ptr;
#if BENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE
  uintptr_t aligned = node_ptr
                          ? (uintptr_t)node_ptr
                          : hz5_policy_local2p_common_align_user(
                                raw, BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN);
#else
  uintptr_t aligned = hz5_policy_local2p_common_align_user(
      raw, BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN);
#endif
  if (!hz5_policy_local2p_common_layout_fits(
          raw, aligned, size, BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES)) {
    hz5_policy_win_local2p_free_node(raw_ptr);
    return NULL;
  }

  Hz5WrapperHdr* header = (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
#if BENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE
  if (!node_ptr) {
#endif
  hz5_wrapper_init(header, raw, aligned, size,
                   BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES,
                   HZ5_WRAPPER_SOURCE_WIN_LOCAL2P);
#if BENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE
  }
#endif
#if BENCHLAB_HZ5_WIN_LOCAL2P_REUSE_STATE_ONLY
  if (tls_reuse) {
    atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_ACTIVE,
                          memory_order_release);
#if BENCHLAB_HZ5_WIN_LOCAL2P_INBOX_FAST_ACTIVATE
  } else if (inbox_reuse &&
             header->local2p_owner == hz5_policy_win_local2p_owner_token()) {
    atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_ACTIVE,
                          memory_order_release);
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_INBOX_FAST_ACTIVATE);
#endif
  } else {
    hz5_policy_win_local2p_init_header(header, aligned);
  }
#else
  hz5_policy_win_local2p_init_header(header, aligned);
#endif
#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ROUTE
  hz5_policy_win_local2p_sidecar_insert(aligned, header);
#endif
  atomic_store_explicit(&g_hz5_policy_seen_allocation, 1,
                        memory_order_relaxed);
  return (void*)aligned;
}

static Hz5FreeResult hz5_policy_win_local2p_free(Hz5WrapperHdr* header,
                                                 uintptr_t aligned) {
  if (!hz5_policy_win_local2p_validate_free_header(header, aligned)) {
    return HZ5_FREE_INVALID;
  }

  uintptr_t current_owner = hz5_policy_win_local2p_owner_token();
  void* node_ptr = hz5_policy_win_local2p_node_from_header(header, aligned);

#if BENCHLAB_HZ5_WIN_LOCAL2P_SAME_OWNER_FAST_STATE
  if (header->local2p_owner == current_owner) {
    uint32_t state =
        atomic_load_explicit(&header->local2p_state, memory_order_acquire);
    if (state != HZ5_LOCAL2P_STATE_ACTIVE) {
      hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_STATE);
      return HZ5_FREE_INVALID;
    }
    atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_FREED,
                          memory_order_release);
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_SAME_OWNER_FAST);
    (void)hz5_policy_win_local2p_push(node_ptr);
    return HZ5_FREE_OK_HZ5;
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
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE);
#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH
    if (hz5_policy_win_local2p_remote_batch_push(header->local2p_owner,
                                                 node_ptr)) {
      return HZ5_FREE_OK_HZ5;
    }
#else
    if (hz5_policy_win_local2p_inbox_push(header->local2p_owner, node_ptr)) {
      return HZ5_FREE_OK_HZ5;
    }
#endif
    (void)hz5_policy_win_local2p_global_push(node_ptr);
    return HZ5_FREE_OK_HZ5;
  }

  (void)hz5_policy_win_local2p_push(node_ptr);
  return HZ5_FREE_OK_HZ5;
}

#include "hz5_policy_win_local2p_route.inc"
#endif
