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
#define HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE ((uintptr_t)UINTPTR_MAX)

enum {
  HZ5_WIN_LOCAL2P_SIDECAR_MISS = 0,
  HZ5_WIN_LOCAL2P_SIDECAR_VALID = 1,
  HZ5_WIN_LOCAL2P_SIDECAR_INVALID = 2
};

static size_t hz5_policy_win_local2p_sidecar_hash(uintptr_t aligned) {
  uint64_t x = (uint64_t)(aligned >> 13);
  x ^= x >> 33;
  x *= UINT64_C(0xff51afd7ed558ccd);
  x ^= x >> 33;
  return (size_t)(x % BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE);
}

static int hz5_policy_win_local2p_sidecar_header_valid(
    const Hz5PolicyWinLocal2PSidecarEntry* entry,
    const Hz5WrapperHdr* header,
    uintptr_t aligned) {
  if (!entry || !header || entry->header != (uintptr_t)header ||
      entry->raw != header->raw ||
      header->magic != HZ5_WRAPPER_HDR_MAGIC ||
      header->layout_version != HZ5_WRAPPER_LAYOUT_VERSION ||
      header->layout_size != sizeof(Hz5WrapperHdr) ||
      header->source != HZ5_WRAPPER_SOURCE_WIN_LOCAL2P ||
      header->requested != HZ5_POLICY_LOCAL2P_EXACT_SIZE ||
      header->raw_bytes != BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES ||
      header->local2p_generation != entry->generation) {
    (void)aligned;
    return 0;
  }
  uint32_t state =
      atomic_load_explicit(&header->local2p_state, memory_order_acquire);
  return state == HZ5_LOCAL2P_STATE_ACTIVE;
}

static void hz5_policy_win_local2p_sidecar_insert(
    uintptr_t aligned,
    Hz5WrapperHdr* header) {
  if (!aligned || !header) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT_FAIL);
    return;
  }

  size_t start = hz5_policy_win_local2p_sidecar_hash(aligned);
  size_t reusable = BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE;
  AcquireSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
  for (size_t probe = 0; probe < BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE;
       ++probe) {
    size_t index =
        (start + probe) % BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE;
    Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[index];
    if (entry->aligned == HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE) {
      if (reusable == BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE) {
        reusable = index;
      }
      continue;
    }
    if (entry->aligned == 0u) {
      if (reusable == BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE) {
        reusable = index;
      }
      break;
    }
    if (entry->aligned == aligned) {
      reusable = index;
      break;
    }
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_COLLISION);
  }
  if (reusable != BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE) {
    Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[reusable];
    entry->aligned = aligned;
    entry->raw = header->raw;
    entry->header = (uintptr_t)header;
    entry->generation = header->local2p_generation;
    entry->state_snapshot =
        atomic_load_explicit(&header->local2p_state, memory_order_relaxed);
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT);
    ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
    return;
  }
  ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT_FAIL);
}

static void hz5_policy_win_local2p_sidecar_remove_raw(uintptr_t raw) {
  if (!raw) {
    return;
  }
  AcquireSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
  for (size_t i = 0; i < BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE; ++i) {
    Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[i];
    if (entry->aligned != 0u &&
        entry->aligned != HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE &&
        entry->raw == raw) {
      entry->aligned = HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE;
      entry->raw = 0u;
      entry->header = 0u;
      entry->generation = 0u;
      entry->state_snapshot = 0u;
      hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_REMOVE);
      ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
      return;
    }
  }
  ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
}

static int hz5_policy_win_local2p_sidecar_probe(uintptr_t aligned,
                                                Hz5WrapperHdr** header_out) {
  if (header_out) {
    *header_out = NULL;
  }
  if (!aligned) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_MISS);
    return HZ5_WIN_LOCAL2P_SIDECAR_MISS;
  }

  int found = 0;
  int valid = 0;
  const Hz5WrapperHdr* found_header = NULL;
  size_t start = hz5_policy_win_local2p_sidecar_hash(aligned);
  AcquireSRWLockShared(&g_hz5_policy_win_local2p_sidecar_lock);
  for (size_t probe = 0; probe < BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE;
       ++probe) {
    size_t index =
        (start + probe) % BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE;
    const Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[index];
    if (entry->aligned == 0u) {
      break;
    }
    if (entry->aligned == HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE) {
      continue;
    }
    if (entry->aligned == aligned) {
      const Hz5WrapperHdr* header = (const Hz5WrapperHdr*)entry->header;
      found = 1;
      found_header = header;
      valid =
          hz5_policy_win_local2p_sidecar_header_valid(entry, header, aligned);
      break;
    }
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_COLLISION);
  }
  ReleaseSRWLockShared(&g_hz5_policy_win_local2p_sidecar_lock);

  if (!found) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_MISS);
    return HZ5_WIN_LOCAL2P_SIDECAR_MISS;
  }
  if (valid) {
    if (header_out) {
      *header_out = (Hz5WrapperHdr*)found_header;
    }
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_HIT_VALID);
    return HZ5_WIN_LOCAL2P_SIDECAR_VALID;
  }
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_HIT_INVALID);
  return HZ5_WIN_LOCAL2P_SIDECAR_INVALID;
}
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

static void* hz5_policy_win_local2p_inbox_pop(void) {
  Hz5PolicyWinLocal2PTls* tls = hz5_policy_win_local2p_tls();
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)tls->inbox_cache;
  if (!node) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_INBOX_EXCHANGE);
    node = (Hz5WinLocal2PNode*)atomic_exchange_explicit(
        &tls->inbox_head, NULL, memory_order_acq_rel);
  } else {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_INBOX_CACHE_HIT);
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
typedef enum Hz5WinLocal2PRemoteFlushReason {
  HZ5_WIN_LOCAL2P_REMOTE_FLUSH_EXPLICIT = 0,
  HZ5_WIN_LOCAL2P_REMOTE_FLUSH_OWNER = 1,
  HZ5_WIN_LOCAL2P_REMOTE_FLUSH_CAP = 2,
  HZ5_WIN_LOCAL2P_REMOTE_FLUSH_SWITCH = 3,
} Hz5WinLocal2PRemoteFlushReason;

#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS > 1u
static void hz5_policy_win_local2p_note_remote_flush(
    size_t count,
    Hz5WinLocal2PRemoteFlushReason reason) {
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH);
  hz5_trace_add(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_NODES,
                (uint64_t)count);
  if (reason == HZ5_WIN_LOCAL2P_REMOTE_FLUSH_OWNER) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_OWNER);
  } else if (reason == HZ5_WIN_LOCAL2P_REMOTE_FLUSH_CAP) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_CAP);
  } else if (reason == HZ5_WIN_LOCAL2P_REMOTE_FLUSH_SWITCH) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_SWITCH);
  }
}

static int hz5_policy_win_local2p_remote_batch_flush_slot(
    Hz5PolicyWinLocal2PTls* tls,
    size_t slot,
    Hz5WinLocal2PRemoteFlushReason reason) {
  if (!tls || slot >= BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS ||
      !tls->remote_batch_owner[slot] || !tls->remote_batch_head[slot] ||
      !tls->remote_batch_tail[slot]) {
    return 0;
  }
  Hz5PolicyWinLocal2PTls* owner_tls =
      (Hz5PolicyWinLocal2PTls*)tls->remote_batch_owner[slot];
  Hz5WinLocal2PNode* head =
      (Hz5WinLocal2PNode*)tls->remote_batch_head[slot];
  Hz5WinLocal2PNode* tail =
      (Hz5WinLocal2PNode*)tls->remote_batch_tail[slot];
  size_t count = tls->remote_batch_count[slot];
  hz5_policy_local2p_common_inbox_push_list(&owner_tls->inbox_head, head,
                                            tail);

  tls->remote_batch_owner[slot] = 0;
  tls->remote_batch_head[slot] = NULL;
  tls->remote_batch_tail[slot] = NULL;
  tls->remote_batch_count[slot] = 0;
  tls->remote_batch_age[slot] = 0;
  hz5_policy_win_local2p_note_remote_flush(count, reason);
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INBOX);
  return 1;
}

#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_ASSOC_LRU
static void hz5_policy_win_local2p_remote_batch_touch(
    Hz5PolicyWinLocal2PTls* tls,
    size_t slot) {
  uint32_t next_age = tls->remote_batch_clock + 1u;
  if (next_age == 0u) {
    next_age = 1u;
  }
  tls->remote_batch_clock = next_age;
  tls->remote_batch_age[slot] = next_age;
}

static size_t hz5_policy_win_local2p_remote_batch_oldest_slot(
    Hz5PolicyWinLocal2PTls* tls) {
  size_t victim = 0;
  uint32_t oldest = tls->remote_batch_age[0];
  for (size_t i = 1; i < BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS; ++i) {
    uint32_t age = tls->remote_batch_age[i];
    if (age < oldest) {
      oldest = age;
      victim = i;
    }
  }
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_REMOTE_ASSOC_LRU_VICTIM);
  return victim;
}
#endif

static size_t hz5_policy_win_local2p_remote_batch_slot(
    Hz5PolicyWinLocal2PTls* tls,
    uintptr_t owner) {
  size_t empty = BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS;
#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_DIRECT_INDEX
  uintptr_t h = owner >> 4u;
  h ^= owner >> 12u;
  h ^= owner >> 20u;
  size_t preferred =
      (size_t)(h % BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS);
  if (tls->remote_batch_owner[preferred] == owner &&
      tls->remote_batch_count[preferred] != 0u) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_REMOTE_DIRECT_INDEX_HIT);
    return preferred;
  }
  if (tls->remote_batch_count[preferred] == 0u) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_REMOTE_DIRECT_INDEX_HIT);
    return preferred;
  }
#endif
  for (size_t i = 0; i < BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS; ++i) {
    if (tls->remote_batch_owner[i] == owner &&
        tls->remote_batch_count[i] != 0u) {
#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_ASSOC_LRU
      hz5_policy_win_local2p_remote_batch_touch(tls, i);
#endif
      return i;
    }
    if (empty == BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS &&
        tls->remote_batch_count[i] == 0u) {
      empty = i;
    }
  }
  if (empty != BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS) {
#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_ASSOC_LRU
    hz5_policy_win_local2p_remote_batch_touch(tls, empty);
#endif
    return empty;
  }

#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_DIRECT_INDEX
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_REMOTE_DIRECT_INDEX_COLLISION);
  (void)hz5_policy_win_local2p_remote_batch_flush_slot(
      tls, preferred, HZ5_WIN_LOCAL2P_REMOTE_FLUSH_OWNER);
  return preferred;
#endif

#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_ASSOC_LRU
  size_t victim = hz5_policy_win_local2p_remote_batch_oldest_slot(tls);
  (void)hz5_policy_win_local2p_remote_batch_flush_slot(
      tls, victim, HZ5_WIN_LOCAL2P_REMOTE_FLUSH_OWNER);
  hz5_policy_win_local2p_remote_batch_touch(tls, victim);
  return victim;
#else
  size_t victim = tls->remote_batch_next_victim %
                  BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS;
  tls->remote_batch_next_victim = victim + 1u;
  (void)hz5_policy_win_local2p_remote_batch_flush_slot(
      tls, victim, HZ5_WIN_LOCAL2P_REMOTE_FLUSH_OWNER);
  return victim;
#endif
}

static size_t hz5_policy_win_local2p_remote_batch_active_slots(
    Hz5PolicyWinLocal2PTls* tls) {
  size_t active = 0;
  for (size_t i = 0; i < BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS; ++i) {
    if (tls->remote_batch_count[i] != 0u) {
      ++active;
    }
  }
  return active;
}

#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SWITCH_FLUSH_MIN > 0u
static void hz5_policy_win_local2p_remote_batch_flush_previous_owner(
    Hz5PolicyWinLocal2PTls* tls,
    uintptr_t owner) {
  uintptr_t previous = tls ? tls->remote_batch_last_owner : 0;
  if (!tls || !previous || previous == owner) {
    return;
  }
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_REMOTE_OWNER_SWITCH);
#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_OWNER_GATE
  if (hz5_policy_win_local2p_remote_batch_active_slots(tls) <
      BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_REMOTE_OWNER_GATE_SKIP);
    return;
  }
#endif
  for (size_t i = 0; i < BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SLOTS; ++i) {
    if (tls->remote_batch_owner[i] == previous &&
        tls->remote_batch_count[i] >=
            BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SWITCH_FLUSH_MIN) {
      (void)hz5_policy_win_local2p_remote_batch_flush_slot(
          tls, i, HZ5_WIN_LOCAL2P_REMOTE_FLUSH_SWITCH);
      return;
    }
  }
}
#endif

static int hz5_policy_win_local2p_remote_batch_push(uintptr_t owner,
                                                    void* node_ptr) {
  if (!owner || !node_ptr) {
    return 0;
  }
  Hz5PolicyWinLocal2PTls* tls = hz5_policy_win_local2p_tls();
#if BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_SWITCH_FLUSH_MIN > 0u
  hz5_policy_win_local2p_remote_batch_flush_previous_owner(tls, owner);
#endif
  size_t slot = hz5_policy_win_local2p_remote_batch_slot(tls, owner);
  tls->remote_batch_last_owner = owner;
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)node_ptr;
  node->next = NULL;
  if (tls->remote_batch_count[slot] == 0u) {
    tls->remote_batch_owner[slot] = owner;
    tls->remote_batch_head[slot] = node;
  } else {
    ((Hz5WinLocal2PNode*)tls->remote_batch_tail[slot])->next = node;
  }
  tls->remote_batch_tail[slot] = node;
  ++tls->remote_batch_count[slot];
  hz5_trace_max(HZ5_TRACE_LOCAL2P_REMOTE_BATCH_DEPTH_MAX,
                (uint64_t)tls->remote_batch_count[slot]);
  hz5_trace_max(
      HZ5_TRACE_LOCAL2P_REMOTE_ACTIVE_SLOTS_MAX,
      (uint64_t)hz5_policy_win_local2p_remote_batch_active_slots(tls));
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_ENQUEUE);
  if (tls->remote_batch_count[slot] >=
      BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_CAP) {
    return hz5_policy_win_local2p_remote_batch_flush_slot(
        tls, slot, HZ5_WIN_LOCAL2P_REMOTE_FLUSH_CAP);
  }
  return 1;
}
#else
static int
hz5_policy_win_local2p_remote_batch_flush(
    Hz5PolicyWinLocal2PTls* tls,
    Hz5WinLocal2PRemoteFlushReason reason) {
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
  size_t count = tls->remote_batch_count;
  hz5_policy_local2p_common_inbox_push_list(&owner_tls->inbox_head, head,
                                            tail);

  hz5_policy_local2p_common_remote_batch_reset(
      &tls->remote_batch_owner, &tls->remote_batch_head,
      &tls->remote_batch_tail, &tls->remote_batch_count);
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH);
  hz5_trace_add(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_NODES,
                (uint64_t)count);
  if (reason == HZ5_WIN_LOCAL2P_REMOTE_FLUSH_OWNER) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_OWNER);
  } else if (reason == HZ5_WIN_LOCAL2P_REMOTE_FLUSH_CAP) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_CAP);
  } else if (reason == HZ5_WIN_LOCAL2P_REMOTE_FLUSH_SWITCH) {
    hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_SWITCH);
  }
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
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_REMOTE_OWNER_SWITCH);
    (void)hz5_policy_win_local2p_remote_batch_flush(
        tls, HZ5_WIN_LOCAL2P_REMOTE_FLUSH_OWNER);
  }
  Hz5WinLocal2PNode* node = (Hz5WinLocal2PNode*)node_ptr;
  hz5_policy_local2p_common_remote_batch_push(
      owner, node, &tls->remote_batch_owner, &tls->remote_batch_head,
      &tls->remote_batch_tail, &tls->remote_batch_count);
  hz5_trace_max(HZ5_TRACE_LOCAL2P_REMOTE_BATCH_DEPTH_MAX,
                (uint64_t)tls->remote_batch_count);
  hz5_trace_max(HZ5_TRACE_LOCAL2P_REMOTE_ACTIVE_SLOTS_MAX,
                tls->remote_batch_count ? 1u : 0u);
  hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_ENQUEUE);
  if (tls->remote_batch_count >= BENCHLAB_HZ5_WIN_LOCAL2P_REMOTE_BATCH_CAP) {
    return hz5_policy_win_local2p_remote_batch_flush(
        tls, HZ5_WIN_LOCAL2P_REMOTE_FLUSH_CAP);
  }
  return 1;
}
#endif
#endif

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

#if BENCHLAB_HZ5_WIN_LOCAL2P_ROUTE_HEADER_GUARD
static int hz5_policy_win_local2p_route_header_readable(uintptr_t aligned) {
  if (aligned < sizeof(Hz5WrapperHdr)) {
    return 0;
  }

  /*
   * HZ4's Windows wrapper guard only probes page-boundary cases so the hot
   * exact route does not pay a VirtualQuery tax on every free. This optional
   * guard gives Local2P the same protection when ptr-header would cross into
   * the previous page before route-cookie direct decode touches the header.
   */
  if ((aligned & ((uintptr_t)HZ5_POLICY_PAGE_SIZE - 1u)) >=
      sizeof(Hz5WrapperHdr)) {
    return 1;
  }

  uintptr_t hdr_addr = aligned - sizeof(Hz5WrapperHdr);
  MEMORY_BASIC_INFORMATION mbi;
  if (VirtualQuery((const void*)hdr_addr, &mbi, sizeof(mbi)) == 0) {
    return 0;
  }
  if (mbi.State != MEM_COMMIT) {
    return 0;
  }
  if ((mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
    return 0;
  }

  uintptr_t region_start = (uintptr_t)mbi.BaseAddress;
  uintptr_t region_end = region_start + (uintptr_t)mbi.RegionSize;
  uintptr_t hdr_end = hdr_addr + sizeof(Hz5WrapperHdr);
  if (region_end < region_start || hdr_end < hdr_addr) {
    return 0;
  }
  return hdr_addr >= region_start && hdr_end <= region_end;
}
#endif

static int hz5_policy_win_local2p_direct_decode(void* ptr,
                                                Hz5WrapperHdr** header_out) {
#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ROUTE
#if !BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_FIRST
  (void)hz5_policy_win_local2p_sidecar_probe((uintptr_t)ptr, NULL);
#endif
#endif
#if BENCHLAB_HZ5_WIN_LOCAL2P_ROUTE_COOKIE
  if (!ptr) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_MISS);
    return 0;
  }
  uintptr_t aligned = (uintptr_t)ptr;
  if (aligned < sizeof(Hz5WrapperHdr)) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_MISS);
    return 0;
  }
#if BENCHLAB_HZ5_WIN_LOCAL2P_ROUTE_HEADER_GUARD
  if (!hz5_policy_win_local2p_route_header_readable(aligned)) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_MISS);
    return 0;
  }
#endif
  Hz5WrapperHdr* header =
      (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
  if (header->magic != HZ5_WRAPPER_HDR_MAGIC ||
      header->layout_version != HZ5_WRAPPER_LAYOUT_VERSION ||
      header->layout_size != sizeof(Hz5WrapperHdr) ||
      header->source != HZ5_WRAPPER_SOURCE_WIN_LOCAL2P ||
      header->raw == 0) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_MISS);
    return 0;
  }
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_OK);
  if (header_out) {
    *header_out = header;
  }
  return 1;
#else
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
#endif
}

static int
hz5_policy_win_local2p_try_free_direct(void* ptr,
                                       Hz5FreeResult* result_out) {
  Hz5WrapperHdr* wrapped = NULL;
#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ROUTE && \
    BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_FIRST
  int sidecar_kind =
      hz5_policy_win_local2p_sidecar_probe((uintptr_t)ptr, &wrapped);
  if (sidecar_kind == HZ5_WIN_LOCAL2P_SIDECAR_VALID) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_FIRST_OK);
    if (result_out) {
      *result_out = hz5_policy_win_local2p_free(wrapped, (uintptr_t)ptr);
    }
    return 1;
  }
  if (sidecar_kind == HZ5_WIN_LOCAL2P_SIDECAR_INVALID) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_FIRST_INVALID);
    if (result_out) {
      *result_out = HZ5_FREE_INVALID;
    }
    return 1;
  }
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_FIRST_MISS);
#endif
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
