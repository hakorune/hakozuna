#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
typedef struct Hz5WinLocal2PNode {
  struct Hz5WinLocal2PNode* next;
} Hz5WinLocal2PNode;

static uintptr_t hz5_policy_win_local2p_owner_token(void) {
  if (!g_hz5_policy_win_local2p_owner_token) {
    g_hz5_policy_win_local2p_owner_token =
        (uintptr_t)&g_hz5_policy_win_local2p_owner_token;
  }
  return g_hz5_policy_win_local2p_owner_token;
}

static uint64_t hz5_policy_win_local2p_cookie(uintptr_t raw,
                                              uintptr_t aligned,
                                              size_t raw_bytes,
                                              uint32_t generation,
                                              uintptr_t owner) {
  uint64_t mixed =
      (uint64_t)(raw ^ aligned ^ owner ^
                 (uintptr_t)&g_hz5_policy_win_local2p_secret_anchor) ^
      ((uint64_t)raw_bytes << 17) ^ ((uint64_t)generation << 32) ^
      UINT64_C(0x77125a8192c0ffee);
  return mixed ? mixed : UINT64_C(0x7712);
}

static int hz5_policy_win_local2p_exact(size_t size, size_t align) {
  return size == 65536u && align == BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN;
}

static uintptr_t hz5_policy_win_local2p_raw_from_node(void* node_ptr) {
  return (uintptr_t)node_ptr;
}

static void hz5_policy_win_local2p_free_node(void* node_ptr) {
  uintptr_t raw = hz5_policy_win_local2p_raw_from_node(node_ptr);
  if (raw) {
    _aligned_free((void*)raw);
  }
}

static void*
hz5_policy_win_local2p_node_from_header(Hz5WrapperHdr* header,
                                        uintptr_t aligned) {
  (void)aligned;
  return (void*)header->raw;
}

static void* hz5_policy_win_local2p_pop(void) {
  Hz5WinLocal2PNode* node =
      (Hz5WinLocal2PNode*)g_hz5_policy_win_local2p_head;
  if (!node) {
    return NULL;
  }
  g_hz5_policy_win_local2p_head = node->next;
  --g_hz5_policy_win_local2p_count;
  hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_TLS_HIT);
  return (void*)node;
}

static void* hz5_policy_win_local2p_global_pop(void) {
  if (BENCHLAB_HZ5_WIN_LOCAL2P_GLOBAL_CAP == 0u) {
    return NULL;
  }
  AcquireSRWLockExclusive(&g_hz5_policy_win_local2p_global_lock);
  Hz5WinLocal2PNode* node =
      (Hz5WinLocal2PNode*)g_hz5_policy_win_local2p_global_head;
  if (node) {
    g_hz5_policy_win_local2p_global_head = node->next;
    --g_hz5_policy_win_local2p_global_count;
  }
  ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_global_lock);
  if (node) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_GLOBAL_HIT);
  }
  return (void*)node;
}

static int hz5_policy_win_local2p_global_push(void* node_ptr) {
  if (BENCHLAB_HZ5_WIN_LOCAL2P_GLOBAL_CAP == 0u) {
    hz5_policy_win_local2p_free_node(node_ptr);
    return 0;
  }

  int accepted = 0;
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)node_ptr;
  AcquireSRWLockExclusive(&g_hz5_policy_win_local2p_global_lock);
  if (g_hz5_policy_win_local2p_global_count <
      BENCHLAB_HZ5_WIN_LOCAL2P_GLOBAL_CAP) {
    node->next = (Hz5WinLocal2PNode*)g_hz5_policy_win_local2p_global_head;
    g_hz5_policy_win_local2p_global_head = node;
    ++g_hz5_policy_win_local2p_global_count;
    accepted = 1;
  }
  ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_global_lock);

  if (accepted) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_GLOBAL);
    return 1;
  }

  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_OVERFLOW);
  hz5_policy_win_local2p_free_node(node_ptr);
  return 0;
}

static int hz5_policy_win_local2p_push(void* node_ptr) {
  if (g_hz5_policy_win_local2p_count >= BENCHLAB_HZ5_WIN_LOCAL2P_TLS_CAP) {
    return hz5_policy_win_local2p_global_push(node_ptr);
  }

  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)node_ptr;
  node->next = (Hz5WinLocal2PNode*)g_hz5_policy_win_local2p_head;
  g_hz5_policy_win_local2p_head = node;
  ++g_hz5_policy_win_local2p_count;
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_TLS);
  return 1;
}

static void* hz5_policy_win_local2p_os_alloc(void) {
  void* raw_ptr = _aligned_malloc(BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES,
                                  BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN);
  if (raw_ptr) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_OS);
  }
  return raw_ptr;
}

static void hz5_policy_win_local2p_init_header(Hz5WrapperHdr* header,
                                               uintptr_t aligned) {
  uint32_t generation = ++g_hz5_policy_win_local2p_generation;
  if (!generation) {
    generation = ++g_hz5_policy_win_local2p_generation;
  }
  uintptr_t owner = hz5_policy_win_local2p_owner_token();
  header->local2p_owner = owner;
  header->local2p_generation = generation;
  header->local2p_cookie =
      hz5_policy_win_local2p_cookie(header->raw, aligned, header->raw_bytes,
                                    generation, owner);
  atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_ACTIVE,
                        memory_order_release);
}

static int
hz5_policy_win_local2p_validate_free_header(Hz5WrapperHdr* header,
                                            uintptr_t aligned) {
  if (!header || header->source != HZ5_WRAPPER_SOURCE_WIN_LOCAL2P ||
      header->requested != 65536u ||
      header->raw_bytes != BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES) {
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
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE);
    return NULL;
  }

  void* node_ptr = hz5_policy_win_local2p_pop();
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
  uintptr_t header_min = raw + sizeof(Hz5WrapperHdr);
  uintptr_t aligned =
      (header_min + (BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN - 1u)) &
      ~(uintptr_t)(BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN - 1u);
  if (aligned < header_min ||
      aligned + size > raw + BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES) {
    hz5_policy_win_local2p_free_node(raw_ptr);
    return NULL;
  }

  Hz5WrapperHdr* header = (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
  hz5_wrapper_init(header, raw, aligned, size,
                   BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES,
                   HZ5_WRAPPER_SOURCE_WIN_LOCAL2P);
  hz5_policy_win_local2p_init_header(header, aligned);
  atomic_store_explicit(&g_hz5_policy_seen_allocation, 1,
                        memory_order_relaxed);
  return (void*)aligned;
}

static Hz5FreeResult hz5_policy_win_local2p_free(Hz5WrapperHdr* header,
                                                 uintptr_t aligned) {
  if (!hz5_policy_win_local2p_validate_free_header(header, aligned)) {
    return HZ5_FREE_INVALID;
  }

  uint32_t old_state = atomic_exchange_explicit(
      &header->local2p_state, HZ5_LOCAL2P_STATE_FREED,
      memory_order_acq_rel);
  if (old_state != HZ5_LOCAL2P_STATE_ACTIVE) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_STATE);
    return HZ5_FREE_INVALID;
  }

  uintptr_t current_owner = hz5_policy_win_local2p_owner_token();
  void* node_ptr = hz5_policy_win_local2p_node_from_header(header, aligned);
  if (header->local2p_owner != current_owner) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE);
    (void)hz5_policy_win_local2p_global_push(node_ptr);
    return HZ5_FREE_OK_HZ5;
  }

  (void)hz5_policy_win_local2p_push(node_ptr);
  return HZ5_FREE_OK_HZ5;
}

static int hz5_policy_win_local2p_direct_decode(void* ptr,
                                                Hz5WrapperHdr** header_out) {
  Hz5WrapperHdr* header = NULL;
  if (!hz5_wrapper_decode(ptr, &header)) {
    return 0;
  }
  if (header->source != HZ5_WRAPPER_SOURCE_WIN_LOCAL2P) {
    return 0;
  }
  if (header_out) {
    *header_out = header;
  }
  return 1;
}

static int
hz5_policy_win_local2p_try_free_direct(void* ptr,
                                       Hz5FreeResult* result_out) {
  Hz5WrapperHdr* wrapped = NULL;
  if (!hz5_policy_win_local2p_direct_decode(ptr, &wrapped)) {
    return 0;
  }
  if (result_out) {
    *result_out = hz5_policy_win_local2p_free(wrapped, (uintptr_t)ptr);
  }
  return 1;
}

static int hz5_policy_win_local2p_metadata_present(
    const Hz5WrapperHdr* header) {
  return header->local2p_cookie != 0 ||
         header->local2p_generation != 0 ||
         header->local2p_owner != 0 ||
         atomic_load_explicit(&header->local2p_state,
                              memory_order_acquire) !=
             HZ5_LOCAL2P_STATE_INVALID;
}
#endif
