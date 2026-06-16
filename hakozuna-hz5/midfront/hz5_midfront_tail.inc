// MidFront public API tail helpers.

void* hz5_midfront_alloc(size_t size, size_t align) {
  if (align > 16u) {
    return NULL;
  }
  int class_index = hz5_midfront_class_index(size);
  if (class_index < 0) {
    return NULL;
  }

  Hz5MidTls* tls = hz5_midfront_tls();
  uint32_t ci = (uint32_t)class_index;
  Hz5MidSpan* span = hz5_midfront_local_pop(tls, ci);
  if (!span) {
#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_RBUF
    hz5_midfront_remote_rbuf_flush(tls);
#endif
#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_OUTBOX && \
    BENCHLAB_HZ5_LINUX_MIDFRONT_OUTBOX_FLUSH_ON_MISS
    hz5_midfront_remote_outbox_flush_class(tls, ci);
#endif
    hz5_ownerhub_note_alloc_miss(tls->owner, HZ5_OWNERHUB_FRONT_MID, ci);
    span = hz5_midfront_drain_remote_on_miss(tls, ci);
    if (span) {
      return span->base;
    }
    hz5_ownerhub_drain_cross_fronts(tls->owner, HZ5_OWNERHUB_FRONT_MID);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
    hz5_midpagefront_owner_drain_some(2u);
#endif
    span = hz5_midfront_local_pop(tls, ci);
  }
  if (span) {
    if (!hz5_midfront_activate_local_for_owner(tls, span)) {
      return NULL;
    }
    return span->base;
  }

#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE
  while ((span = hz5_midfront_global_pop(ci)) != NULL) {
    if (hz5_midfront_activate_global_for_owner(tls, span)) {
      return span->base;
    }
  }
#endif

  span = hz5_midfront_new_span(tls, ci);
  return span ? span->base : NULL;
}

Hz5MidFrontFreeResult hz5_midfront_free(void* ptr) {
  Hz5MidSpan* span = hz5_midfront_span_for_ptr(ptr);
  if (!span) {
    return HZ5_MIDFRONT_FREE_NOT_OWNED;
  }
  if (ptr != span->base) {
    return HZ5_MIDFRONT_FREE_INVALID;
  }

  Hz5MidTls* tls = hz5_midfront_tls();
  if (hz5_owner_equal(span->owner, tls->owner)) {
    if (!hz5_midfront_owner_local_state_transition(
            span,
            (unsigned char)HZ5_MIDSPAN_ACTIVE,
            (unsigned char)HZ5_MIDSPAN_LOCAL_FREE)) {
      return HZ5_MIDFRONT_FREE_INVALID;
    }
    hz5_midfront_local_push(tls, span->class_index, span);
  } else {
#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_GLOBAL_RECYCLE
    if (!hz5_midfront_state_cas(span,
                                (unsigned char)HZ5_MIDSPAN_ACTIVE,
                                (unsigned char)HZ5_MIDSPAN_LOCAL_FREE)) {
      return HZ5_MIDFRONT_FREE_INVALID;
    }
    hz5_midfront_global_push(span->class_index, span);
#else
#if BENCHLAB_HZ5_LINUX_MIDFRONT_REMOTE_DIRECT_FREE_STATE
    if (!hz5_midfront_state_cas(span,
                                (unsigned char)HZ5_MIDSPAN_ACTIVE,
                                (unsigned char)HZ5_MIDSPAN_LOCAL_FREE)) {
      return HZ5_MIDFRONT_FREE_INVALID;
    }
#else
    if (!hz5_midfront_state_cas(span,
                                (unsigned char)HZ5_MIDSPAN_ACTIVE,
                                (unsigned char)HZ5_MIDSPAN_REMOTE_PENDING)) {
      return HZ5_MIDFRONT_FREE_INVALID;
    }
#endif
    hz5_midfront_remote_batch_push(tls, span);
#endif
  }
  return HZ5_MIDFRONT_FREE_OK;
}

int hz5_midfront_can_handle(size_t size, size_t align) {
  return align <= 16u && hz5_midfront_class_index(size) >= 0;
}

int hz5_midfront_owns(void* ptr) {
  return hz5_midfront_span_for_ptr(ptr) != NULL;
}

size_t hz5_midfront_usable_size(void* ptr) {
  Hz5MidSpan* span = hz5_midfront_span_for_ptr(ptr);
  if (!span || ptr != span->base) {
    return 0;
  }
  return span->class_bytes;
}
