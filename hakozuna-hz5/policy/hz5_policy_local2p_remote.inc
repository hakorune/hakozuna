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
  hz5_policy_local2p_common_inbox_push_list(&owner_tls->inbox_head, node,
                                            node);
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
  hz5_policy_local2p_common_inbox_push_list(&owner_tls->inbox_head, head,
                                            tail);

  hz5_policy_local2p_common_remote_batch_reset(
      &tls->remote_batch_owner, &tls->remote_batch_head,
      &tls->remote_batch_tail, &tls->remote_batch_count);
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
  hz5_policy_local2p_common_remote_batch_push(
      owner, node, &tls->remote_batch_owner, &tls->remote_batch_head,
      &tls->remote_batch_tail, &tls->remote_batch_count);
  if (tls->remote_batch_count >= BENCHLAB_HZ5_LINUX_LOCAL2P_REMOTE_BATCH_CAP) {
    return hz5_policy_local2p_remote_batch_flush(tls);
  }
  return 1;
}
#endif
#endif
