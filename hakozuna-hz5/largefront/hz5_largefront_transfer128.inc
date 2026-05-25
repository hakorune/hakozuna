#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128
static int hz5_largefront_transfer128_class(uint32_t class_index) {
  return hz5_largefront_class_valid(class_index) &&
         hz5_largefront_class_bytes(class_index) == 131072u;
}

static void hz5_largefront_transfer128_global_push_list(Hz5LargeSpan* head,
                                                        Hz5LargeSpan* tail,
                                                        uint32_t count) {
  if (!head || !tail || count == 0u) {
    return;
  }

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

  if (tls->transfer128_count >= HZ5_LARGEFRONT_TRANSFER128_CHUNK_CAP) {
    hz5_largefront_transfer128_tls_flush(tls);
  }
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
