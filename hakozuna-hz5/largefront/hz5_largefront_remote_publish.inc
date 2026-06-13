static int hz5_largefront_can_publish_to_owner(Hz5OwnerToken owner,
                                               uint32_t class_index,
                                               const Hz5LargeSpan* span) {
  return owner.slot != 0 && hz5_largefront_class_valid(class_index) && span &&
         hz5_owner_is_alive(owner);
}

static int hz5_largefront_remote_publish_one(Hz5OwnerToken owner,
                                             uint32_t class_index,
                                             Hz5LargeSpan* span) {
  if (!hz5_largefront_can_publish_to_owner(owner, class_index, span)) {
    return 0;
  }

  void* old_head = NULL;
  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    span->next = (Hz5LargeSpan*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, span, memory_order_release, memory_order_acquire));
  hz5_ownerhub_mark_pending(owner, HZ5_OWNERHUB_FRONT_LARGE, class_index);
  return 1;
}

static int hz5_largefront_remote_publish_list(Hz5OwnerToken owner,
                                              uint32_t class_index,
                                              Hz5LargeSpan* head,
                                              Hz5LargeSpan* tail) {
  if (!hz5_largefront_can_publish_to_owner(owner, class_index, head) || !tail) {
    return 0;
  }

  void* old_head = NULL;
  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    tail->next = (Hz5LargeSpan*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, head, memory_order_release, memory_order_acquire));
  hz5_ownerhub_mark_pending(owner, HZ5_OWNERHUB_FRONT_LARGE, class_index);
  return 1;
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
static Hz5LargeRemoteChunk* hz5_largefront_tls_chunk_alloc(Hz5LargeTls* tls) {
  if (!tls) {
    return NULL;
  }
  if (!tls->chunk_pool) {
    size_t bytes = sizeof(Hz5LargeRemoteChunk) *
                   (size_t)HZ5_LARGEFRONT_REMOTE_CHUNK_POOL_CAP;
    void* mem = mmap(NULL,
                     bytes,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1,
                     0);
    if (mem == MAP_FAILED) {
      return NULL;
    }
    tls->chunk_pool = (Hz5LargeRemoteChunk*)mem;
    tls->chunk_cap = HZ5_LARGEFRONT_REMOTE_CHUNK_POOL_CAP;
    tls->chunk_next = 0;
  }
  if (tls->chunk_next >= tls->chunk_cap) {
    return NULL;
  }
  return &tls->chunk_pool[tls->chunk_next++];
}

static int hz5_largefront_remote_publish_chunk(Hz5LargeTls* tls,
                                               Hz5OwnerToken owner,
                                               uint32_t class_index,
                                               Hz5LargeSpan* head,
                                               uint32_t count) {
  if (!hz5_largefront_can_publish_to_owner(owner, class_index, head) ||
      count == 0u || count > HZ5_LARGEFRONT_REMOTE_CHUNK_CAP ||
      hz5_largefront_class_bytes(class_index) != 131072u) {
    return 0;
  }
  Hz5LargeRemoteChunk* chunk = hz5_largefront_tls_chunk_alloc(tls);
  if (!chunk) {
    return 0;
  }

  chunk->next = NULL;
  chunk->count = (uint16_t)count;
  chunk->class_index = (uint16_t)class_index;
  chunk->owner = owner;
  Hz5LargeSpan* span = head;
  for (uint32_t i = 0; i < count; ++i) {
    if (!span) {
      chunk->count = (uint16_t)i;
      break;
    }
    Hz5LargeSpan* next = span->next;
    span->next = NULL;
    chunk->spans[i] = span;
    span = next;
  }

  Hz5LargeRemoteChunk* old_head = NULL;
  _Atomic(Hz5LargeRemoteChunk*)* inbox =
      &g_hz5_largefront_owner_chunk_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    chunk->next = old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, chunk, memory_order_release, memory_order_acquire));
  hz5_ownerhub_mark_pending(owner, HZ5_OWNERHUB_FRONT_LARGE, class_index);
  return 1;
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_BATCH
static void hz5_largefront_remote_batch_flush(Hz5LargeTls* tls) {
  if (!tls || tls->remote_batch_count == 0u) {
    return;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  hz5_largefront_policy_note_remote_flush(tls->remote_batch_class,
                                          tls->remote_batch_count,
                                          tls->remote_batch_cap);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L1B
  hz5_largefront_policy_l1b_note_remote_flush(tls,
                                              tls->remote_batch_class,
                                              tls->remote_batch_count);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
  if (hz5_largefront_remote_publish_chunk(tls,
                                          tls->remote_batch_owner,
                                          tls->remote_batch_class,
                                          tls->remote_batch_head,
                                          tls->remote_batch_count)) {
    tls->remote_batch_owner = k_hz5_largefront_no_owner;
    tls->remote_batch_class = 0;
    tls->remote_batch_count = 0;
    tls->remote_batch_head = NULL;
    tls->remote_batch_tail = NULL;
    return;
  }
#endif
  if (!hz5_largefront_remote_publish_list(tls->remote_batch_owner,
                                          tls->remote_batch_class,
                                          tls->remote_batch_head,
                                          tls->remote_batch_tail)) {
    Hz5LargeSpan* span = tls->remote_batch_head;
    while (span) {
      Hz5LargeSpan* next = span->next;
      hz5_largefront_mark_orphan(span);
      span = next;
    }
  }
  tls->remote_batch_owner = k_hz5_largefront_no_owner;
  tls->remote_batch_class = 0;
  tls->remote_batch_count = 0;
  tls->remote_batch_head = NULL;
  tls->remote_batch_tail = NULL;
}

static void hz5_largefront_remote_batch_push(Hz5LargeTls* tls,
                                             Hz5LargeSpan* span) {
  if (!hz5_owner_is_alive(span->owner)) {
    hz5_largefront_mark_orphan(span);
    return;
  }
  uint32_t class_index = span->class_index;
  if (tls->remote_batch_count != 0u &&
      (!hz5_owner_equal(tls->remote_batch_owner, span->owner) ||
       tls->remote_batch_class != class_index)) {
    hz5_largefront_remote_batch_flush(tls);
  }

  span->next = NULL;
  if (tls->remote_batch_count == 0u) {
    tls->remote_batch_owner = span->owner;
    tls->remote_batch_class = class_index;
    tls->remote_batch_head = span;
  } else {
    tls->remote_batch_tail->next = span;
  }
  tls->remote_batch_tail = span;
  ++tls->remote_batch_count;

  uint32_t cap = tls->remote_batch_cap == 0u
                     ? HZ5_LARGEFRONT_REMOTE_BATCH_CAP
                     : tls->remote_batch_cap;
  if (tls->remote_batch_count >= cap) {
    hz5_largefront_remote_batch_flush(tls);
  }
}
#endif
