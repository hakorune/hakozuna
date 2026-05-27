static uintptr_t hz5_policy_win_local2p_raw_from_node(void* node_ptr) {
#if BENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE
  uintptr_t aligned = (uintptr_t)node_ptr;
  if (aligned < sizeof(Hz5WrapperHdr)) {
    return 0;
  }
  Hz5WrapperHdr* header =
      (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
  return header->raw;
#else
  return (uintptr_t)node_ptr;
#endif
}

static void hz5_policy_win_local2p_free_node(void* node_ptr) {
  uintptr_t raw = hz5_policy_win_local2p_raw_from_node(node_ptr);
  if (raw) {
#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ROUTE
    hz5_policy_win_local2p_sidecar_remove_raw(raw);
#endif
    hz5_trace_add(HZ5_TRACE_LOCAL2P_OS_FREE_BYTES,
                  (uint64_t)BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES);
    _aligned_free((void*)raw);
  }
}

static void*
hz5_policy_win_local2p_node_from_header(Hz5WrapperHdr* header,
                                        uintptr_t aligned) {
#if BENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE
  (void)header;
  return (void*)aligned;
#else
  (void)aligned;
  return (void*)header->raw;
#endif
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

static void* hz5_policy_win_local2p_global_pop(void) {
  if (BENCHLAB_HZ5_WIN_LOCAL2P_GLOBAL_CAP == 0u) {
    return NULL;
  }
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_GLOBAL_POP_LOCK);
  AcquireSRWLockExclusive(&g_hz5_policy_win_local2p_global_lock);
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)
      hz5_policy_local2p_common_pop_node(
          &g_hz5_policy_win_local2p_global_head,
          &g_hz5_policy_win_local2p_global_count);
  hz5_trace_max(HZ5_TRACE_LOCAL2P_GLOBAL_COUNT_MAX,
                (uint64_t)g_hz5_policy_win_local2p_global_count);
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
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_GLOBAL_PUSH_LOCK);
  AcquireSRWLockExclusive(&g_hz5_policy_win_local2p_global_lock);
  accepted = hz5_policy_local2p_common_push_node(
      &g_hz5_policy_win_local2p_global_head,
      &g_hz5_policy_win_local2p_global_count,
      BENCHLAB_HZ5_WIN_LOCAL2P_GLOBAL_CAP, node);
  hz5_trace_max(HZ5_TRACE_LOCAL2P_GLOBAL_COUNT_MAX,
                (uint64_t)g_hz5_policy_win_local2p_global_count);
  ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_global_lock);

  if (accepted) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_GLOBAL_PUSH_ACCEPT);
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_GLOBAL);
    return 1;
  }

  hz5_trace_inc(HZ5_TRACE_LOCAL2P_GLOBAL_PUSH_REJECT);
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
  hz5_trace_max(HZ5_TRACE_LOCAL2P_TLS_COUNT_MAX, (uint64_t)tls->count);
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_TLS);
  return 1;
}
