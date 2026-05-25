#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_RBUF
static void hz5_midfront_remote_rbuf_flush(Hz5MidTls* tls) {
  if (!tls || tls->remote_rbuf_count == 0u) {
    return;
  }

  uint32_t count = tls->remote_rbuf_count;
  if (count > HZ5_MIDFRONT_REMOTE_RBUF_CAP) {
    count = HZ5_MIDFRONT_REMOTE_RBUF_CAP;
  }

  for (uint32_t i = 0; i < count; ++i) {
    Hz5MidSpan* first = tls->remote_rbuf[i].span;
    if (!first) {
      continue;
    }
    Hz5OwnerToken owner = tls->remote_rbuf[i].owner;
    uint32_t class_index = tls->remote_rbuf[i].class_index;
    Hz5MidSpan* head = NULL;
    Hz5MidSpan* tail = NULL;

    for (uint32_t j = i; j < count; ++j) {
      Hz5MidRemoteRbufEntry* ent = &tls->remote_rbuf[j];
      Hz5MidSpan* span = ent->span;
      if (!span || ent->class_index != class_index ||
          !hz5_owner_equal(ent->owner, owner)) {
        continue;
      }
      ent->span = NULL;
      span->next = NULL;
      if (!head) {
        head = span;
      } else {
        tail->next = span;
      }
      tail = span;
    }

    if (!hz5_midfront_can_publish_to_owner(owner, class_index, head, tail)) {
      Hz5MidSpan* cur = head;
      while (cur) {
        Hz5MidSpan* next = cur->next;
        hz5_midfront_mark_orphan(cur);
        cur = next;
      }
      continue;
    }
    hz5_midfront_remote_publish_list(owner, class_index, head, tail);
  }

  for (uint32_t i = 0; i < count; ++i) {
    tls->remote_rbuf[i].owner = k_hz5_midfront_no_owner;
    tls->remote_rbuf[i].class_index = 0;
    tls->remote_rbuf[i].span = NULL;
  }
  tls->remote_rbuf_count = 0;
}

static void hz5_midfront_remote_batch_push(Hz5MidTls* tls, Hz5MidSpan* span) {
  if (!tls || !span || !hz5_owner_is_alive(span->owner)) {
    if (span) {
      hz5_midfront_mark_orphan(span);
    }
    return;
  }

  if (tls->remote_rbuf_count >= HZ5_MIDFRONT_REMOTE_RBUF_CAP) {
    hz5_midfront_remote_rbuf_flush(tls);
  }
  if (tls->remote_rbuf_count >= HZ5_MIDFRONT_REMOTE_RBUF_CAP) {
    hz5_midfront_remote_publish_list(
        span->owner, span->class_index, span, span);
    return;
  }

  Hz5MidRemoteRbufEntry* ent =
      &tls->remote_rbuf[tls->remote_rbuf_count++];
  ent->owner = span->owner;
  ent->class_index = span->class_index;
  span->next = NULL;
  ent->span = span;

  if (tls->remote_rbuf_count >= HZ5_MIDFRONT_REMOTE_RBUF_FLUSH_THRESHOLD) {
    hz5_midfront_remote_rbuf_flush(tls);
  }
}
#elif BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_OUTBOX
static void hz5_midfront_remote_outbox_flush_slot(
    Hz5MidRemoteOutboxSlot* slot) {
  if (!slot || slot->count == 0u) {
    return;
  }

  hz5_midfront_remote_publish_list(
      slot->owner, slot->class_index, slot->head, slot->tail);
  slot->owner = k_hz5_midfront_no_owner;
  slot->class_index = 0;
  slot->count = 0;
  slot->head = NULL;
  slot->tail = NULL;
}

static void hz5_midfront_remote_outbox_flush_class(Hz5MidTls* tls,
                                                   uint32_t class_index) {
  if (!tls || !hz5_midfront_class_valid(class_index)) {
    return;
  }
  for (uint32_t i = 0; i < HZ5_MIDFRONT_REMOTE_OUTBOX_SLOTS; ++i) {
    Hz5MidRemoteOutboxSlot* slot = &tls->remote_outbox[i];
    if (slot->count != 0u && slot->class_index == class_index) {
      hz5_midfront_remote_outbox_flush_slot(slot);
    }
  }
}

static Hz5MidRemoteOutboxSlot* hz5_midfront_remote_outbox_slot(
    Hz5MidTls* tls,
    Hz5OwnerToken owner,
    uint32_t class_index) {
  Hz5MidRemoteOutboxSlot* empty = NULL;
  for (uint32_t i = 0; i < HZ5_MIDFRONT_REMOTE_OUTBOX_SLOTS; ++i) {
    Hz5MidRemoteOutboxSlot* slot = &tls->remote_outbox[i];
    if (slot->count != 0u && slot->class_index == class_index &&
        hz5_owner_equal(slot->owner, owner)) {
      return slot;
    }
    if (slot->count == 0u && !empty) {
      empty = slot;
    }
  }
  if (empty) {
    return empty;
  }

  uint32_t victim =
      tls->remote_outbox_victim++ % HZ5_MIDFRONT_REMOTE_OUTBOX_SLOTS;
  Hz5MidRemoteOutboxSlot* slot = &tls->remote_outbox[victim];
  hz5_midfront_remote_outbox_flush_slot(slot);
  return slot;
}

static void hz5_midfront_remote_batch_push(Hz5MidTls* tls, Hz5MidSpan* span) {
  if (!tls || !span || !hz5_owner_is_alive(span->owner)) {
    if (span) {
      hz5_midfront_mark_orphan(span);
    }
    return;
  }

  uint32_t class_index = span->class_index;
  Hz5MidRemoteOutboxSlot* slot =
      hz5_midfront_remote_outbox_slot(tls, span->owner, class_index);
  span->next = NULL;

  if (slot->count == 0u) {
    slot->owner = span->owner;
    slot->class_index = class_index;
    slot->head = span;
  } else {
    slot->tail->next = span;
  }
  slot->tail = span;
  ++slot->count;

  if (slot->count >= HZ5_MIDFRONT_REMOTE_BATCH_CAP) {
    hz5_midfront_remote_outbox_flush_slot(slot);
  }
}
#else
static void hz5_midfront_remote_batch_flush(Hz5MidTls* tls) {
  if (!tls || tls->remote_batch_count == 0u) {
    return;
  }

  hz5_midfront_remote_publish_list(tls->remote_batch_owner,
                                   tls->remote_batch_class,
                                   tls->remote_batch_head,
                                   tls->remote_batch_tail);
  tls->remote_batch_owner = k_hz5_midfront_no_owner;
  tls->remote_batch_class = 0;
  tls->remote_batch_count = 0;
  tls->remote_batch_head = NULL;
  tls->remote_batch_tail = NULL;
}

static void hz5_midfront_remote_batch_push(Hz5MidTls* tls, Hz5MidSpan* span) {
  if (!hz5_owner_is_alive(span->owner)) {
    hz5_midfront_mark_orphan(span);
    return;
  }
  uint32_t class_index = span->class_index;
  if (tls->remote_batch_count != 0u &&
      (!hz5_owner_equal(tls->remote_batch_owner, span->owner) ||
       tls->remote_batch_class != class_index)) {
    hz5_midfront_remote_batch_flush(tls);
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

  if (tls->remote_batch_count >= HZ5_MIDFRONT_REMOTE_BATCH_CAP) {
    hz5_midfront_remote_batch_flush(tls);
  }
}
#endif
