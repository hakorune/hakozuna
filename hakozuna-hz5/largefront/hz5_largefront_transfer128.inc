#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128
static int hz5_largefront_transfer128_class(uint32_t class_index) {
  return hz5_largefront_class_valid(class_index) &&
         hz5_largefront_class_bytes(class_index) == 131072u;
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_OWNER_SHARD
static uint32_t hz5_largefront_transfer128_owner_shard(Hz5OwnerToken owner) {
  if (HZ5_LARGEFRONT_TRANSFER128_SHARD_COUNT == 0u) {
    return 0u;
  }
  return (uint32_t)owner.slot % HZ5_LARGEFRONT_TRANSFER128_SHARD_COUNT;
}

static void hz5_largefront_transfer128_shard_lock(uint32_t shard) {
  while (atomic_exchange_explicit(
             &g_hz5_largefront_transfer128_shard_lock[shard],
             (unsigned char)1,
             memory_order_acquire) != 0u) {
  }
}

static void hz5_largefront_transfer128_shard_unlock(uint32_t shard) {
  atomic_store_explicit(&g_hz5_largefront_transfer128_shard_lock[shard],
                        (unsigned char)0,
                        memory_order_release);
}

static void hz5_largefront_transfer128_overflow_global(Hz5LargeSpan* span) {
  if (!span) {
    return;
  }
  span->next = NULL;
  if (hz5_largefront_state_cas(span,
                               (unsigned char)HZ5_LARGESPAN_TRANSFER_FREE,
                               (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
    hz5_largefront_global_push(span->class_index, span);
  } else {
    hz5_largefront_mark_orphan(span);
  }
}

static void hz5_largefront_transfer128_owner_shard_push_one(
    Hz5LargeSpan* span) {
  if (!span) {
    return;
  }
  if (!hz5_owner_is_alive(span->owner)) {
    hz5_largefront_transfer128_overflow_global(span);
    return;
  }

  uint32_t shard = hz5_largefront_transfer128_owner_shard(span->owner);
  hz5_largefront_transfer128_shard_lock(shard);
  if (g_hz5_largefront_transfer128_shard_count[shard] >=
      HZ5_LARGEFRONT_TRANSFER128_SHARD_CAP) {
    hz5_largefront_transfer128_shard_unlock(shard);
    hz5_largefront_transfer128_overflow_global(span);
    return;
  }

  span->next = g_hz5_largefront_transfer128_shard_head[shard];
  g_hz5_largefront_transfer128_shard_head[shard] = span;
  ++g_hz5_largefront_transfer128_shard_count[shard];
  hz5_largefront_transfer128_shard_unlock(shard);
}

static Hz5LargeSpan* hz5_largefront_transfer128_shard_pop_one(
    Hz5LargeTls* tls,
    uint32_t class_index,
    uint32_t shard) {
  if (!tls || !hz5_largefront_transfer128_class(class_index)) {
    return NULL;
  }

  hz5_largefront_transfer128_shard_lock(shard);
  Hz5LargeSpan* span = g_hz5_largefront_transfer128_shard_head[shard];
  if (span) {
    g_hz5_largefront_transfer128_shard_head[shard] = span->next;
    if (g_hz5_largefront_transfer128_shard_count[shard] != 0u) {
      --g_hz5_largefront_transfer128_shard_count[shard];
    }
  }
  hz5_largefront_transfer128_shard_unlock(shard);

  if (!span) {
    return NULL;
  }
  span->next = NULL;
  if (!hz5_largefront_state_cas(span,
                                (unsigned char)HZ5_LARGESPAN_TRANSFER_FREE,
                                (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
    hz5_largefront_mark_orphan(span);
    return NULL;
  }
  span->owner = tls->owner;
  hz5_largefront_payload_reactivate(span);
  return span;
}

static Hz5LargeSpan* hz5_largefront_transfer128_owner_shard_pop(
    Hz5LargeTls* tls,
    uint32_t class_index) {
  if (!tls || tls->owner.slot == 0 ||
      !hz5_largefront_transfer128_class(class_index)) {
    return NULL;
  }

  uint32_t shard = hz5_largefront_transfer128_owner_shard(tls->owner);
  Hz5LargeSpan* span =
      hz5_largefront_transfer128_shard_pop_one(tls, class_index, shard);
  if (span) {
    return span;
  }

  uint32_t neighbor = shard + 1u;
  if (neighbor >= HZ5_LARGEFRONT_TRANSFER128_SHARD_COUNT) {
    neighbor = 0u;
  }
  if (neighbor == shard) {
    return NULL;
  }
  return hz5_largefront_transfer128_shard_pop_one(tls, class_index, neighbor);
}
#endif

static void hz5_largefront_transfer128_global_push_list(Hz5LargeSpan* head,
                                                        Hz5LargeSpan* tail,
                                                        uint32_t count) {
  if (!head || !tail || count == 0u) {
    return;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_OWNER_SHARD
  (void)tail;
  (void)count;
  Hz5LargeSpan* span = head;
  while (span) {
    Hz5LargeSpan* next = span->next;
    span->next = NULL;
    hz5_largefront_transfer128_owner_shard_push_one(span);
    span = next;
  }
  return;
#endif

  pthread_mutex_lock(&g_hz5_largefront_transfer128_lock);
  if (g_hz5_largefront_transfer128_count + count >
      HZ5_LARGEFRONT_TRANSFER128_CAP) {
    pthread_mutex_unlock(&g_hz5_largefront_transfer128_lock);
    Hz5LargeSpan* span = head;
    while (span) {
      Hz5LargeSpan* next = span->next;
      span->next = NULL;
      if (hz5_largefront_state_cas(
              span,
              (unsigned char)HZ5_LARGESPAN_TRANSFER_FREE,
              (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
        hz5_largefront_global_push(span->class_index, span);
      } else {
        hz5_largefront_mark_orphan(span);
      }
      span = next;
    }
    return;
  }

  tail->next = g_hz5_largefront_transfer128_head;
  g_hz5_largefront_transfer128_head = head;
  g_hz5_largefront_transfer128_count += count;
  pthread_mutex_unlock(&g_hz5_largefront_transfer128_lock);
}

static void hz5_largefront_transfer128_tls_flush(Hz5LargeTls* tls) {
  if (!tls || tls->transfer128_count == 0u) {
    return;
  }
  hz5_largefront_transfer128_global_push_list(tls->transfer128_head,
                                              tls->transfer128_tail,
                                              tls->transfer128_count);
  tls->transfer128_head = NULL;
  tls->transfer128_tail = NULL;
  tls->transfer128_count = 0;
}

static Hz5LargeSpan* hz5_largefront_transfer128_tls_pop_for_owner(
    Hz5LargeTls* tls,
    uint32_t class_index) {
  if (!tls || !hz5_largefront_transfer128_class(class_index)) {
    return NULL;
  }

  while (tls->transfer128_head) {
    Hz5LargeSpan* span = tls->transfer128_head;
    tls->transfer128_head = span->next;
    if (!tls->transfer128_head) {
      tls->transfer128_tail = NULL;
    }
    if (tls->transfer128_count != 0u) {
      --tls->transfer128_count;
    }

    span->next = NULL;
    if (!hz5_largefront_state_cas(span,
                                  (unsigned char)HZ5_LARGESPAN_TRANSFER_FREE,
                                  (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
      hz5_largefront_mark_orphan(span);
      continue;
    }
    span->owner = tls->owner;
    hz5_largefront_payload_reactivate(span);
    return span;
  }

  return NULL;
}

static int hz5_largefront_transfer128_push(Hz5LargeTls* tls,
                                           Hz5LargeSpan* span) {
  if (!tls || !span || !hz5_largefront_transfer128_class(span->class_index)) {
    return 0;
  }

  span->next = NULL;
  if (tls->transfer128_count == 0u) {
    tls->transfer128_head = span;
  } else {
    tls->transfer128_tail->next = span;
  }
  tls->transfer128_tail = span;
  ++tls->transfer128_count;

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_TLS_FIRST
  if (tls->transfer128_count > HZ5_LARGEFRONT_TRANSFER128_CHUNK_CAP) {
    Hz5LargeSpan* flush_head = tls->transfer128_head;
    Hz5LargeSpan* flush_tail = tls->transfer128_head;
    uint32_t flush_count = 1u;

    tls->transfer128_head = flush_head->next;
    if (!tls->transfer128_head) {
      tls->transfer128_tail = NULL;
    }
    flush_head->next = NULL;
    if (tls->transfer128_count != 0u) {
      --tls->transfer128_count;
    }
    hz5_largefront_transfer128_global_push_list(flush_head,
                                                flush_tail,
                                                flush_count);
  }
#else
  if (tls->transfer128_count >= HZ5_LARGEFRONT_TRANSFER128_CHUNK_CAP) {
    hz5_largefront_transfer128_tls_flush(tls);
  }
#endif
  return 1;
}

static Hz5LargeSpan* hz5_largefront_transfer128_pop_for_owner(
    Hz5LargeTls* tls,
    uint32_t class_index) {
  if (!tls || !hz5_largefront_transfer128_class(class_index)) {
    return NULL;
  }

  pthread_mutex_lock(&g_hz5_largefront_transfer128_lock);
  Hz5LargeSpan* span = g_hz5_largefront_transfer128_head;
  if (span) {
    g_hz5_largefront_transfer128_head = span->next;
    if (g_hz5_largefront_transfer128_count != 0u) {
      --g_hz5_largefront_transfer128_count;
    }
  }
  pthread_mutex_unlock(&g_hz5_largefront_transfer128_lock);

  if (!span) {
    return NULL;
  }
  span->next = NULL;
  if (!hz5_largefront_state_cas(span,
                                (unsigned char)HZ5_LARGESPAN_TRANSFER_FREE,
                                (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
    hz5_largefront_mark_orphan(span);
    return NULL;
  }
  span->owner = tls->owner;
  hz5_largefront_payload_reactivate(span);
  return span;
}
#endif
