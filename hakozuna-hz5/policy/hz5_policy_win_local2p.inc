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
  return hz5_policy_local2p_common_cookie_full(
      raw, aligned, raw_bytes, generation, owner,
      (uintptr_t)&g_hz5_policy_win_local2p_secret_anchor,
      UINT64_C(0x77125a8192c0ffee), UINT64_C(0x7712));
}

static int hz5_policy_win_local2p_exact(size_t size, size_t align) {
  return hz5_policy_local2p_common_exact(
      size, align, HZ5_POLICY_LOCAL2P_EXACT_SIZE,
      BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN);
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
  Hz5PolicyWinLocal2PTls* tls = hz5_policy_win_local2p_tls();
#if BENCHLAB_HZ5_WIN_LOCAL2P_RETAIN_ARRAY
  if (tls->count == 0u) {
    return NULL;
  }
  Hz5WinLocal2PNode* node =
      (Hz5WinLocal2PNode*)tls->retained[--tls->count];
#else
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)
      hz5_policy_local2p_common_pop_node(&tls->head, &tls->count);
#endif
  if (!node) {
    return NULL;
  }
  hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_TLS_HIT);
  return (void*)node;
}

static void* hz5_policy_win_local2p_inbox_pop(void) {
  Hz5PolicyWinLocal2PTls* tls = hz5_policy_win_local2p_tls();
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)tls->inbox_cache;
  if (!node) {
    node = (Hz5WinLocal2PNode*)atomic_exchange_explicit(
        &tls->inbox_head, NULL, memory_order_acq_rel);
  }
  if (!node) {
    return NULL;
  }
  tls->inbox_cache = node->next;
  hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_INBOX_HIT);
  return (void*)node;
}

#if !BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH
static int hz5_policy_win_local2p_inbox_push(uintptr_t owner,
                                             void* node_ptr) {
  if (!owner || !node_ptr) {
    return 0;
  }
  Hz5PolicyWinLocal2PTls* owner_tls = (Hz5PolicyWinLocal2PTls*)owner;
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)node_ptr;
  hz5_policy_local2p_common_inbox_push_list(&owner_tls->inbox_head, node,
                                            node);
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INBOX);
  return 1;
}
#endif

#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH
static int
hz5_policy_win_local2p_remote_batch_flush(Hz5PolicyWinLocal2PTls* tls) {
  if (!tls || !tls->remote_batch_owner || !tls->remote_batch_head ||
      !tls->remote_batch_tail) {
    return 0;
  }
  Hz5PolicyWinLocal2PTls* owner_tls =
      (Hz5PolicyWinLocal2PTls*)tls->remote_batch_owner;
  Hz5WinLocal2PNode* head =
      (Hz5WinLocal2PNode*)tls->remote_batch_head;
  Hz5WinLocal2PNode* tail =
      (Hz5WinLocal2PNode*)tls->remote_batch_tail;
  hz5_policy_local2p_common_inbox_push_list(&owner_tls->inbox_head, head,
                                            tail);

  hz5_policy_local2p_common_remote_batch_reset(
      &tls->remote_batch_owner, &tls->remote_batch_head,
      &tls->remote_batch_tail, &tls->remote_batch_count);
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INBOX);
  return 1;
}

static int hz5_policy_win_local2p_remote_batch_push(uintptr_t owner,
                                                    void* node_ptr) {
  if (!owner || !node_ptr) {
    return 0;
  }
  Hz5PolicyWinLocal2PTls* tls = hz5_policy_win_local2p_tls();
  if (tls->remote_batch_owner && tls->remote_batch_owner != owner) {
    (void)hz5_policy_win_local2p_remote_batch_flush(tls);
  }
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)node_ptr;
  hz5_policy_local2p_common_remote_batch_push(
      owner, node, &tls->remote_batch_owner, &tls->remote_batch_head,
      &tls->remote_batch_tail, &tls->remote_batch_count);
  if (tls->remote_batch_count >= BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_CAP) {
    return hz5_policy_win_local2p_remote_batch_flush(tls);
  }
  return 1;
}
#endif

static void* hz5_policy_win_local2p_global_pop(void) {
  if (BENCHLAB_HZ5_WIN_LOCAL2P_GLOBAL_CAP == 0u) {
    return NULL;
  }
  AcquireSRWLockExclusive(&g_hz5_policy_win_local2p_global_lock);
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)
      hz5_policy_local2p_common_pop_node(
          &g_hz5_policy_win_local2p_global_head,
          &g_hz5_policy_win_local2p_global_count);
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
  accepted = hz5_policy_local2p_common_push_node(
      &g_hz5_policy_win_local2p_global_head,
      &g_hz5_policy_win_local2p_global_count,
      BENCHLAB_HZ5_WIN_LOCAL2P_GLOBAL_CAP, node);
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
  Hz5PolicyWinLocal2PTls* tls = hz5_policy_win_local2p_tls();
  if (tls->count >= BENCHLAB_HZ5_WIN_LOCAL2P_TLS_CAP) {
    return hz5_policy_win_local2p_global_push(node_ptr);
  }

#if BENCHLAB_HZ5_WIN_LOCAL2P_RETAIN_ARRAY
  tls->retained[tls->count++] = node_ptr;
#else
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)node_ptr;
  (void)hz5_policy_local2p_common_push_node(
      &tls->head, &tls->count, BENCHLAB_HZ5_WIN_LOCAL2P_TLS_CAP, node);
#endif
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
  Hz5PolicyWinLocal2PTls* tls = hz5_policy_win_local2p_tls();
  uint32_t generation =
      hz5_policy_local2p_common_next_generation(&tls->generation);
  uintptr_t owner = hz5_policy_win_local2p_owner_token();
  uint64_t cookie =
      hz5_policy_win_local2p_cookie(header->raw, aligned, header->raw_bytes,
                                    generation, owner);
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
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE);
    return NULL;
  }

  void* node_ptr = hz5_policy_win_local2p_pop();
  if (!node_ptr) {
    node_ptr = hz5_policy_win_local2p_inbox_pop();
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
  uintptr_t aligned = hz5_policy_local2p_common_align_user(
      raw, BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN);
  if (!hz5_policy_local2p_common_layout_fits(
          raw, aligned, size, BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES)) {
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

  if (!hz5_policy_local2p_common_mark_freed(header)) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_STATE);
    return HZ5_FREE_INVALID;
  }

  uintptr_t current_owner = hz5_policy_win_local2p_owner_token();
  void* node_ptr = hz5_policy_win_local2p_node_from_header(header, aligned);
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
  return hz5_policy_local2p_common_metadata_present(header);
}
#endif
