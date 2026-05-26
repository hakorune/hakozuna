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
