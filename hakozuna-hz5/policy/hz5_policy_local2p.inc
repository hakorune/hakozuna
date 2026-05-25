#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P
typedef struct Hz5LinuxLocal2PNode {
  struct Hz5LinuxLocal2PNode* next;
} Hz5LinuxLocal2PNode;

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
  if (!tls->owner_token) {
    tls->owner_token = (uintptr_t)&tls->owner_token;
  }
  return tls->owner_token;
#endif
#else
  if (!g_hz5_policy_local2p_owner_token) {
    g_hz5_policy_local2p_owner_token =
        (uintptr_t)&g_hz5_policy_local2p_owner_token;
  }
  return g_hz5_policy_local2p_owner_token;
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
  uint64_t mixed =
      (uint64_t)(raw ^ aligned ^
                 (uintptr_t)&g_hz5_policy_local2p_secret_anchor) ^
      UINT64_C(0x2f125a8192c0ffee);
  return mixed ? mixed : UINT64_C(0x2f12);
#else
  uint64_t mixed =
      (uint64_t)(raw ^ aligned ^ owner ^
                 (uintptr_t)&g_hz5_policy_local2p_secret_anchor) ^
      ((uint64_t)raw_bytes << 17) ^ ((uint64_t)generation << 32) ^
      UINT64_C(0x2f125a8192c0ffee);
  return mixed ? mixed : UINT64_C(0x2f12);
#endif
}

static int hz5_policy_local2p_exact(size_t size, size_t align) {
  return size == 65536u && align == BENCHLAB_HZ5_LINUX_LOCAL2P_ALIGN;
}

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

#if BENCHLAB_HZ5_LINUX_LOCAL2P_OWNER_INBOX && \
    BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED
static void* hz5_policy_local2p_inbox_pop(void) {
  Hz5PolicyLocal2PTls* tls = hz5_policy_local2p_tls();
  Hz5LinuxLocal2PNode* node = (Hz5LinuxLocal2PNode*)tls->inbox_cache;
  if (!node) {
    node = (Hz5LinuxLocal2PNode*)atomic_exchange_explicit(
        &tls->inbox_head, NULL, memory_order_acq_rel);
  }
  if (!node) {
    return NULL;
  }
  tls->inbox_cache = node->next;
  hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_INBOX_HIT);
  return (void*)node;
}

static int hz5_policy_local2p_inbox_push(uintptr_t owner, void* node_ptr) {
  if (!owner) {
    return 0;
  }
  Hz5PolicyLocal2PTls* owner_tls = (Hz5PolicyLocal2PTls*)owner;
  Hz5LinuxLocal2PNode* node = (Hz5LinuxLocal2PNode*)node_ptr;
  void* head = atomic_load_explicit(&owner_tls->inbox_head,
                                    memory_order_acquire);
  do {
    node->next = (Hz5LinuxLocal2PNode*)head;
  } while (!atomic_compare_exchange_weak_explicit(
      &owner_tls->inbox_head, &head, node,
      memory_order_acq_rel, memory_order_acquire));
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INBOX);
  return 1;
}

#if BENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH
static int hz5_policy_local2p_remote_batch_flush(Hz5PolicyLocal2PTls* tls) {
  if (!tls || !tls->remote_batch_owner || !tls->remote_batch_head ||
      !tls->remote_batch_tail) {
    return 0;
  }
  Hz5PolicyLocal2PTls* owner_tls =
      (Hz5PolicyLocal2PTls*)tls->remote_batch_owner;
  Hz5LinuxLocal2PNode* head =
      (Hz5LinuxLocal2PNode*)tls->remote_batch_head;
  Hz5LinuxLocal2PNode* tail =
      (Hz5LinuxLocal2PNode*)tls->remote_batch_tail;
  void* inbox_head =
      atomic_load_explicit(&owner_tls->inbox_head, memory_order_acquire);
  do {
    tail->next = (Hz5LinuxLocal2PNode*)inbox_head;
  } while (!atomic_compare_exchange_weak_explicit(
      &owner_tls->inbox_head, &inbox_head, head,
      memory_order_acq_rel, memory_order_acquire));

  tls->remote_batch_owner = 0;
  tls->remote_batch_head = NULL;
  tls->remote_batch_tail = NULL;
  tls->remote_batch_count = 0;
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INBOX);
  return 1;
}

static int hz5_policy_local2p_remote_batch_push(uintptr_t owner,
                                                void* node_ptr) {
  if (!owner || !node_ptr) {
    return 0;
  }
  Hz5PolicyLocal2PTls* tls = hz5_policy_local2p_tls();
  if (tls->remote_batch_owner && tls->remote_batch_owner != owner) {
    (void)hz5_policy_local2p_remote_batch_flush(tls);
  }
  Hz5LinuxLocal2PNode* node = (Hz5LinuxLocal2PNode*)node_ptr;
  node->next = (Hz5LinuxLocal2PNode*)tls->remote_batch_head;
  tls->remote_batch_head = node;
  if (!tls->remote_batch_tail) {
    tls->remote_batch_tail = node;
  }
  tls->remote_batch_owner = owner;
  ++tls->remote_batch_count;
  if (tls->remote_batch_count >= BENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH_CAP) {
    return hz5_policy_local2p_remote_batch_flush(tls);
  }
  return 1;
}
#endif
#endif

static void* hz5_policy_local2p_global_pop(void) {
  if (BENCHLAB_HZ5_LINUX_LOCAL2P_GLOBAL_CAP == 0u) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_policy_local2p_global_lock);
  Hz5LinuxLocal2PNode* node =
      (Hz5LinuxLocal2PNode*)g_hz5_policy_local2p_global_head;
  if (node) {
    g_hz5_policy_local2p_global_head = node->next;
    --g_hz5_policy_local2p_global_count;
  }
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
  if (g_hz5_policy_local2p_global_count <
      BENCHLAB_HZ5_LINUX_LOCAL2P_GLOBAL_CAP) {
    Hz5LinuxLocal2PNode* node = (Hz5LinuxLocal2PNode*)node_ptr;
    node->next = (Hz5LinuxLocal2PNode*)g_hz5_policy_local2p_global_head;
    g_hz5_policy_local2p_global_head = node;
    ++g_hz5_policy_local2p_global_count;
    pushed = 1;
  }
  pthread_mutex_unlock(&g_hz5_policy_local2p_global_lock);
  if (pushed) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_GLOBAL);
    return 1;
  }
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_OVERFLOW);
  hz5_policy_local2p_free_node(node_ptr);
  return 0;
}

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
  uint32_t generation = ++tls->generation;
  if (generation == 0u) {
    generation = ++tls->generation;
  }
#else
  uint32_t generation = ++g_hz5_policy_local2p_generation;
  if (generation == 0u) {
    generation = ++g_hz5_policy_local2p_generation;
  }
#endif
  uintptr_t owner = hz5_policy_local2p_owner_token();
  header->local2p_owner = owner;
  header->local2p_generation = generation;
#if BENCHLAB_HZ5_LINUX_LOCAL2P_NO_COOKIE
  header->local2p_cookie = UINT64_C(0x2f12);
#else
  header->local2p_cookie =
      hz5_policy_local2p_cookie(header->raw, aligned, header->raw_bytes,
                                generation, owner);
#endif
  atomic_store_explicit(&header->local2p_state, HZ5_LOCAL2P_STATE_ACTIVE,
                        memory_order_release);
}

static inline int hz5_policy_local2p_validate_free_header(Hz5WrapperHdr* header,
                                                          uintptr_t aligned) {
#if BENCHLAB_HZ5_LINUX_LOCAL2P_SLIM_CHECK
  if (!header) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE);
    return 0;
  }
#else
  if (!header ||
      header->source != HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P ||
      header->requested != 65536u ||
      header->raw_bytes != BENCHLAB_HZ5_LINUX_LOCAL2P_RAW_BYTES) {
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
                          : ((raw + sizeof(Hz5WrapperHdr) + align - 1u) &
                             ~(uintptr_t)(align - 1u));
#else
  uintptr_t aligned =
      (raw + sizeof(Hz5WrapperHdr) + align - 1u) & ~(uintptr_t)(align - 1u);
#endif
  if ((aligned & (align - 1u)) != 0 ||
      aligned + size > raw + BENCHLAB_HZ5_LINUX_LOCAL2P_RAW_BYTES) {
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
  uint32_t old_state = atomic_exchange_explicit(
      &header->local2p_state, HZ5_LOCAL2P_STATE_FREED,
      memory_order_acq_rel);
  if (old_state != HZ5_LOCAL2P_STATE_ACTIVE) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_STATE);
    return HZ5_FREE_INVALID;
  }
#endif

  if (header->local2p_owner != current_owner) {
    return hz5_policy_local2p_recycle_remote(header, aligned);
  }

  return hz5_policy_local2p_recycle_local(header, aligned);
}

static int hz5_policy_local2p_direct_decode(void* ptr,
                                            Hz5WrapperHdr** header_out) {
  if (!ptr) {
    return 0;
  }
  uintptr_t aligned = (uintptr_t)ptr;
  if (aligned < sizeof(Hz5WrapperHdr)) {
    return 0;
  }

  Hz5WrapperHdr* header =
      (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
  if (header->magic != HZ5_WRAPPER_HDR_MAGIC ||
      header->layout_version != HZ5_WRAPPER_LAYOUT_VERSION ||
      header->layout_size != sizeof(Hz5WrapperHdr) ||
      header->source != HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P) {
    return 0;
  }
  uintptr_t raw = header->raw;
#if BENCHLAB_HZ5_LINUX_LOCAL2P_ROUTE_COOKIE
  if (raw == 0) {
    return 0;
#else
  if (raw == 0 || header->cookie != hz5_wrapper_cookie(raw, aligned)) {
    return 0;
  }
#endif
  if (header_out) {
    *header_out = header;
  }
  return 1;
}

static inline int
hz5_policy_local2p_try_free_direct(void* ptr, Hz5FreeResult* result_out) {
  Hz5WrapperHdr* local2p_wrapped = NULL;
  if (!hz5_policy_local2p_direct_decode(ptr, &local2p_wrapped)) {
    return 0;
  }
  hz5_trace_inc(HZ5_TRACE_WRAPPER_DECODE_OK);
  if (result_out) {
    *result_out = hz5_policy_local2p_free(local2p_wrapped, (uintptr_t)ptr);
  }
  return 1;
}

static int hz5_policy_local2p_metadata_present(const Hz5WrapperHdr* header) {
  if (!header) {
    return 0;
  }
  return header->local2p_cookie != 0 ||
         header->local2p_generation != 0 ||
         header->local2p_owner != 0 ||
         atomic_load_explicit(&header->local2p_state,
                              memory_order_acquire) !=
             HZ5_LOCAL2P_STATE_INVALID;
}
#endif
