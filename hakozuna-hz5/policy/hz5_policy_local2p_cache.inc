static uintptr_t hz5_policy_local2p_raw_from_node(void* node_ptr) {
#if BENCHLAB_HZ5_LINUX_LOCAL2P_OBJECT_NODE
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

static void hz5_policy_local2p_free_node(void* node_ptr) {
  uintptr_t raw = hz5_policy_local2p_raw_from_node(node_ptr);
  if (raw) {
    free((void*)raw);
  }
}

static void* hz5_policy_local2p_node_from_header(Hz5WrapperHdr* header,
                                                 uintptr_t aligned) {
#if BENCHLAB_HZ5_LINUX_LOCAL2P_OBJECT_NODE
  (void)header;
  return (void*)aligned;
#else
  return (void*)header->raw;
#endif
}

static int hz5_policy_local2p_global_push(void* node_ptr);

static int hz5_policy_local2p_push_overflow(void* node_ptr) {
#if BENCHLAB_HZ5_LINUX_LOCAL2P_LOCAL_OVERFLOW_GLOBAL
  return hz5_policy_local2p_global_push(node_ptr);
#else
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_OVERFLOW);
  hz5_policy_local2p_free_node(node_ptr);
  return 0;
#endif
}

static void* hz5_policy_local2p_pop(void) {
#if BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
  Hz5PolicyLocal2PTls* tls = hz5_policy_local2p_tls();
#if BENCHLAB_HZ5_LINUX_LOCAL2P_RETAIN_ARRAY
  if (tls->count == 0u) {
    return NULL;
  }
  Hz5LinuxLocal2PNode* node =
      (Hz5LinuxLocal2PNode*)tls->retained[--tls->count];
#else
  Hz5LinuxLocal2PNode* node = (Hz5LinuxLocal2PNode*)tls->head;
#endif
#else
  Hz5LinuxLocal2PNode* node =
      (Hz5LinuxLocal2PNode*)g_hz5_policy_local2p_head;
#endif
  if (!node) {
    return NULL;
  }
#if BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
#if BENCHLAB_HZ5_LINUX_LOCAL2P_RETAIN_ARRAY
  /* Array mode keeps freelist pointers in TLS, avoiding user-payload writes. */
#elif BENCHLAB_HZ5_LINUX_LOCAL2P_SINGLE_SLOT_TLS
  tls->head = NULL;
#else
  tls->head = node->next;
  --tls->count;
#endif
#else
#if BENCHLAB_HZ5_LINUX_LOCAL2P_SINGLE_SLOT_TLS
  g_hz5_policy_local2p_head = NULL;
#else
  g_hz5_policy_local2p_head = node->next;
  --g_hz5_policy_local2p_count;
#endif
#endif
  hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_TLS_HIT);
  return (void*)node;
}

static void* hz5_policy_local2p_global_pop(void) {
  if (BENCHLAB_HZ5_LINUX_LOCAL2P_GLOBAL_CAP == 0u) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_policy_local2p_global_lock);
  Hz5LinuxLocal2PNode* node = (Hz5LinuxLocal2PNode*)
      hz5_policy_local2p_common_pop_node(&g_hz5_policy_local2p_global_head,
                                         &g_hz5_policy_local2p_global_count);
  pthread_mutex_unlock(&g_hz5_policy_local2p_global_lock);
  if (node) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_GLOBAL_HIT);
  }
  return (void*)node;
}

static int hz5_policy_local2p_push(void* node_ptr) {
#if BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED && \
    BENCHLAB_HZ5_LINUX_LOCAL2P_RETAIN_ARRAY
  Hz5PolicyLocal2PTls* tls = hz5_policy_local2p_tls();
  if (tls->count >= BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_CAP) {
    return hz5_policy_local2p_push_overflow(node_ptr);
  }
  tls->retained[tls->count++] = node_ptr;
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_TLS);
  return 1;
#else
#if BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
  Hz5PolicyLocal2PTls* tls = hz5_policy_local2p_tls();
#if BENCHLAB_HZ5_LINUX_LOCAL2P_SINGLE_SLOT_TLS
  if (tls->head != NULL) {
#else
  if (tls->count >= BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_CAP) {
#endif
#else
#if BENCHLAB_HZ5_LINUX_LOCAL2P_SINGLE_SLOT_TLS
  if (g_hz5_policy_local2p_head != NULL) {
#else
  if (g_hz5_policy_local2p_count >= BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_CAP) {
#endif
#endif
    return hz5_policy_local2p_push_overflow(node_ptr);
  }
  Hz5LinuxLocal2PNode* node = (Hz5LinuxLocal2PNode*)node_ptr;
#if BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
#if BENCHLAB_HZ5_LINUX_LOCAL2P_SINGLE_SLOT_TLS
  tls->head = node;
#else
  node->next = (Hz5LinuxLocal2PNode*)tls->head;
  tls->head = node;
  ++tls->count;
#endif
#else
#if BENCHLAB_HZ5_LINUX_LOCAL2P_SINGLE_SLOT_TLS
  g_hz5_policy_local2p_head = node;
#else
  node->next = (Hz5LinuxLocal2PNode*)g_hz5_policy_local2p_head;
  g_hz5_policy_local2p_head = node;
  ++g_hz5_policy_local2p_count;
#endif
#endif
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_TLS);
  return 1;
#endif
}

static int hz5_policy_local2p_global_push(void* node_ptr) {
  if (BENCHLAB_HZ5_LINUX_LOCAL2P_GLOBAL_CAP == 0u) {
    hz5_policy_local2p_free_node(node_ptr);
    return 0;
  }
  int pushed = 0;
  pthread_mutex_lock(&g_hz5_policy_local2p_global_lock);
  Hz5LinuxLocal2PNode* node = (Hz5LinuxLocal2PNode*)node_ptr;
  pushed = hz5_policy_local2p_common_push_node(
      &g_hz5_policy_local2p_global_head, &g_hz5_policy_local2p_global_count,
      BENCHLAB_HZ5_LINUX_LOCAL2P_GLOBAL_CAP, node);
  pthread_mutex_unlock(&g_hz5_policy_local2p_global_lock);
  if (pushed) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_GLOBAL);
    return 1;
  }
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_OVERFLOW);
  hz5_policy_local2p_free_node(node_ptr);
  return 0;
}
