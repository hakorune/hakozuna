static void hz5_largefront_local_push(Hz5LargeTls* tls,
                                      uint32_t class_index,
                                      Hz5LargeSpan* span) {
  span->next = tls->free_head[class_index];
  tls->free_head[class_index] = span;
  ++tls->free_count[class_index];
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  hz5_largefront_obs_max(&g_hz5_largefront_obs_local_free_highwater[class_index],
                         tls->free_count[class_index]);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
  if (hz5_largefront_payload_scavenge_class(class_index) &&
      tls->free_count[class_index] > HZ5_LARGEFRONT_SCAVENGE_LOCAL_CAP) {
    hz5_largefront_payload_scavenge(span);
  }
#endif
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_BULK_LOCAL
static void hz5_largefront_local_push_list(Hz5LargeTls* tls,
                                           uint32_t class_index,
                                           Hz5LargeSpan* head,
                                           Hz5LargeSpan* tail,
                                           uint32_t count) {
  if (!head || count == 0u) {
    return;
  }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
  Hz5LargeSpan* span = head;
  while (span) {
    Hz5LargeSpan* next = span->next;
    hz5_largefront_local_push(tls, class_index, span);
    span = next;
  }
#else
  if (!tail) {
    tail = head;
    while (tail->next) {
      tail = tail->next;
    }
  }
  tail->next = tls->free_head[class_index];
  tls->free_head[class_index] = head;
  tls->free_count[class_index] += count;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  hz5_largefront_obs_max(&g_hz5_largefront_obs_local_free_highwater[class_index],
                         tls->free_count[class_index]);
#endif
#endif
}
#endif

static Hz5LargeSpan* hz5_largefront_local_pop(Hz5LargeTls* tls,
                                              uint32_t class_index) {
  Hz5LargeSpan* span = tls->free_head[class_index];
  if (!span) {
    return NULL;
  }
  tls->free_head[class_index] = span->next;
  if (tls->free_count[class_index] != 0u) {
    --tls->free_count[class_index];
  }
  span->next = NULL;
  return span;
}

static void hz5_largefront_global_push(uint32_t class_index,
                                       Hz5LargeSpan* span) {
  if (!hz5_largefront_class_valid(class_index) || !span) {
    return;
  }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
  if (hz5_largefront_payload_scavenge_class(class_index)) {
    hz5_largefront_payload_scavenge(span);
  }
#endif
  pthread_mutex_lock(&g_hz5_largefront_global_lock);
  span->next = g_hz5_largefront_global_free[class_index];
  g_hz5_largefront_global_free[class_index] = span;
  pthread_mutex_unlock(&g_hz5_largefront_global_lock);
}

static Hz5LargeSpan* hz5_largefront_global_pop(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_largefront_global_lock);
  Hz5LargeSpan* span = g_hz5_largefront_global_free[class_index];
  if (span) {
    g_hz5_largefront_global_free[class_index] = span->next;
    span->next = NULL;
  }
  pthread_mutex_unlock(&g_hz5_largefront_global_lock);
  return span;
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_HOLD
static int hz5_largefront_remote_hold_push(Hz5LargeTls* tls,
                                           uint32_t class_index,
                                           Hz5LargeSpan* span) {
  if (!tls || !span || class_index >= HZ5_LARGEFRONT_CLASS_COUNT ||
      HZ5_LARGEFRONT_REMOTE_HOLD_CAP == 0u ||
      tls->remote_hold_count[class_index] >=
          HZ5_LARGEFRONT_REMOTE_HOLD_CAP) {
    return 0;
  }
  span->next = NULL;
  if (tls->remote_hold_tail[class_index]) {
    tls->remote_hold_tail[class_index]->next = span;
  } else {
    tls->remote_hold_head[class_index] = span;
  }
  tls->remote_hold_tail[class_index] = span;
  ++tls->remote_hold_count[class_index];
  return 1;
}

static Hz5LargeSpan* hz5_largefront_remote_hold_drain_budget(
    Hz5LargeTls* tls,
    uint32_t class_index,
    uint32_t budget,
    uint32_t* drained_out) {
  uint32_t drained = 0;
  Hz5LargeSpan* taken = NULL;
  if (!tls || class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }
  Hz5LargeSpan* span = tls->remote_hold_head[class_index];
  while (span && (!taken || (budget != 0u && drained < budget))) {
    tls->remote_hold_head[class_index] = span->next;
    if (!tls->remote_hold_head[class_index]) {
      tls->remote_hold_tail[class_index] = NULL;
    }
    if (tls->remote_hold_count[class_index] != 0u) {
      --tls->remote_hold_count[class_index];
    }
    span->next = NULL;

    if (!hz5_owner_equal(span->owner, tls->owner)) {
      hz5_largefront_mark_orphan(span);
      span = tls->remote_hold_head[class_index];
      continue;
    }
    if (!taken &&
        hz5_largefront_state_cas(span,
                                 (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                 (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
      hz5_largefront_payload_reactivate(span);
      taken = span;
      span = tls->remote_hold_head[class_index];
      continue;
    }
    if (budget != 0u && drained < budget &&
        hz5_largefront_remote_pending_to_local(span)) {
      hz5_largefront_local_push(tls, class_index, span);
      ++drained;
      span = tls->remote_hold_head[class_index];
      continue;
    }
    hz5_largefront_mark_orphan(span);
    span = tls->remote_hold_head[class_index];
  }
  if (drained_out) {
    *drained_out = drained;
  }
  return taken;
}
#endif
