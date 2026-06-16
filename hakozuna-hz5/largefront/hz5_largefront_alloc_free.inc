static Hz5LargeSpan* hz5_largefront_new_span(Hz5LargeTls* tls,
                                             uint32_t class_index) {
  uint32_t class_bytes = hz5_largefront_class_bytes(class_index);
  void* raw = hz5_largefront_source_alloc_raw(class_index);
  if (!raw) {
    return NULL;
  }

  Hz5LargeSpan* span = (Hz5LargeSpan*)raw;
  span->magic = 0;
  span->raw = raw;
  span->base = (void*)((uintptr_t)raw + HZ5_LARGEFRONT_PAGE_SIZE);
  span->class_bytes = class_bytes;
  span->page_count = (uint16_t)(class_bytes / HZ5_LARGEFRONT_PAGE_SIZE);
  span->class_index = (uint16_t)class_index;
  span->owner = tls->owner;
  atomic_store_explicit(&span->state,
                        (unsigned char)HZ5_LARGESPAN_ACTIVE,
                        memory_order_release);
  span->generation = 1;
  span->flags = 0;
  span->next = NULL;

  if (!hz5_largefront_map_insert(span)) {
    hz5_largefront_source_free_raw(class_index, raw);
    return NULL;
  }
  span->magic = HZ5_LARGEFRONT_MAGIC;
  return span;
}

void* hz5_largefront_alloc(size_t size, size_t align) {
  if (align > 16u) {
    return NULL;
  }
  int class_index = hz5_largefront_class_index(size);
  if (class_index < 0) {
    return NULL;
  }

  Hz5LargeTls* tls = hz5_largefront_tls();
  uint32_t ci = (uint32_t)class_index;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  hz5_largefront_counter_inc(&g_hz5_largefront_obs_alloc_calls[ci]);
#endif
  Hz5LargeSpan* span = NULL;

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_FIRST_128K && \
    BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
  if (hz5_largefront_class_bytes(ci) == 131072u) {
    hz5_ownerhub_note_alloc_miss(tls->owner, HZ5_OWNERHUB_FRONT_LARGE, ci);
#if HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET > 0u
    span = hz5_largefront_drain_remote_class_budget(
        tls,
        ci,
        HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
        NULL);
#else
    span = hz5_largefront_drain_remote_class(tls, ci);
#endif
    if (span) {
      return span->base;
    }
  }
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_FIRST_GATED_128K && \
    BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
  if (hz5_largefront_class_bytes(ci) == 131072u &&
      tls->owner.slot != 0) {
    _Atomic(void*)* inbox = &g_hz5_largefront_owner_inbox[tls->owner.slot][ci];
    if (atomic_load_explicit(inbox, memory_order_acquire)) {
      hz5_ownerhub_note_alloc_miss(tls->owner, HZ5_OWNERHUB_FRONT_LARGE, ci);
#if HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET > 0u
      span = hz5_largefront_drain_remote_class_budget(
          tls,
          ci,
          HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
          NULL);
#else
      span = hz5_largefront_drain_remote_class(tls, ci);
#endif
      if (span) {
        return span->base;
      }
    }
  }
#endif

  span = hz5_largefront_local_pop(tls, ci);

  if (span) {
    if (hz5_largefront_activate_local_for_owner(tls, span)) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(&g_hz5_largefront_obs_local_pop_hit[ci]);
#endif
      return span->base;
    }
    return NULL;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_TLS_FIRST
  span = hz5_largefront_transfer128_tls_pop_for_owner(tls, ci);
  if (span) {
    return span->base;
  }
#else
  hz5_largefront_transfer128_tls_flush(tls);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_OWNER_SHARD
  span = hz5_largefront_transfer128_owner_shard_pop(tls, ci);
  if (span) {
    return span->base;
  }
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_CONSUMER_SHARD
  span = hz5_largefront_transfer128_consumer_shard_pop(tls, ci);
  if (span) {
    return span->base;
  }
#endif
  span = hz5_largefront_transfer128_pop_for_owner(tls, ci);
  if (span) {
    return span->base;
  }
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_HOLD
  span = hz5_largefront_remote_hold_drain_budget(
      tls,
      ci,
      HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
      NULL);
  if (span) {
    return span->base;
  }
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
  hz5_ownerhub_note_alloc_miss(tls->owner, HZ5_OWNERHUB_FRONT_LARGE, ci);
#if HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET > 0u
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_POP_BUDGET
  span = hz5_largefront_drain_remote_class_pop_budget(
      tls,
      ci,
      HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
      NULL);
#else
  span = hz5_largefront_drain_remote_class_budget(
      tls,
      ci,
      HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
      NULL);
#endif
#else
  span = hz5_largefront_drain_remote_class(tls, ci);
#endif
  if (span) {
    return span->base;
  }
  hz5_ownerhub_drain_cross_fronts(tls->owner, HZ5_OWNERHUB_FRONT_LARGE);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
  hz5_midpagefront_owner_drain_some(2u);
#endif
  span = hz5_largefront_local_pop(tls, ci);
  if (span) {
    if (hz5_largefront_activate_local_for_owner(tls, span)) {
      return span->base;
    }
    return NULL;
  }
#endif

  while ((span = hz5_largefront_global_pop(ci)) != NULL) {
    if (hz5_largefront_activate_global_for_owner(tls, span)) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(&g_hz5_largefront_obs_global_pop_hit[ci]);
#endif
      return span->base;
    }
  }

  span = hz5_largefront_new_span(tls, ci);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  if (span) {
    hz5_largefront_counter_inc(&g_hz5_largefront_obs_new_span[ci]);
  }
#endif
  return span ? span->base : NULL;
}

Hz5LargeFrontFreeResult hz5_largefront_free(void* ptr) {
  Hz5LargeSpan* span = hz5_largefront_span_for_ptr(ptr);
  if (!span) {
    return HZ5_LARGEFRONT_FREE_NOT_OWNED;
  }
  if (ptr != span->base) {
    return HZ5_LARGEFRONT_FREE_INVALID;
  }

  Hz5LargeTls* tls = hz5_largefront_tls();
  if (hz5_owner_equal(span->owner, tls->owner)) {
    if (!hz5_largefront_owner_local_state_transition(
            span,
            (unsigned char)HZ5_LARGESPAN_ACTIVE,
            (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
      return HZ5_LARGEFRONT_FREE_INVALID;
    }
    hz5_largefront_local_push(tls, span->class_index, span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
    hz5_largefront_counter_inc(
        &g_hz5_largefront_obs_free_local[span->class_index]);
#endif
  } else {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128
    if (hz5_largefront_transfer128_class(span->class_index)) {
      if (!hz5_largefront_state_cas(
              span,
              (unsigned char)HZ5_LARGESPAN_ACTIVE,
              (unsigned char)HZ5_LARGESPAN_TRANSFER_FREE)) {
        return HZ5_LARGEFRONT_FREE_INVALID;
      }
      if (!hz5_largefront_transfer128_push(tls, span)) {
        hz5_largefront_mark_orphan(span);
      }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_free_remote[span->class_index]);
#endif
      return HZ5_LARGEFRONT_FREE_OK;
    }
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_GLOBAL_128K
    if (hz5_largefront_class_bytes(span->class_index) == 131072u) {
      if (!hz5_largefront_state_cas(
              span,
              (unsigned char)HZ5_LARGESPAN_ACTIVE,
              (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
        return HZ5_LARGEFRONT_FREE_INVALID;
      }
      hz5_largefront_global_push(span->class_index, span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_free_global[span->class_index]);
#endif
      return HZ5_LARGEFRONT_FREE_OK;
    }
#endif
    if (hz5_owner_is_alive(span->owner)) {
      if (!hz5_largefront_state_cas(
              span,
              (unsigned char)HZ5_LARGESPAN_ACTIVE,
              (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING)) {
        return HZ5_LARGEFRONT_FREE_INVALID;
      }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_BATCH
      hz5_largefront_remote_batch_push(tls, span);
#else
      if (!hz5_largefront_remote_publish_one(span->owner,
                                             span->class_index,
                                             span)) {
        hz5_largefront_mark_orphan(span);
      }
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_free_remote[span->class_index]);
#endif
      return HZ5_LARGEFRONT_FREE_OK;
    }
#endif
    if (!hz5_largefront_state_cas(span,
                                  (unsigned char)HZ5_LARGESPAN_ACTIVE,
                                  (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
      return HZ5_LARGEFRONT_FREE_INVALID;
    }
    hz5_largefront_global_push(span->class_index, span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
    hz5_largefront_counter_inc(
        &g_hz5_largefront_obs_free_global[span->class_index]);
#endif
  }
  return HZ5_LARGEFRONT_FREE_OK;
}

int hz5_largefront_can_handle(size_t size, size_t align) {
  return align <= 16u && hz5_largefront_class_index(size) >= 0;
}

int hz5_largefront_owns(void* ptr) {
  return hz5_largefront_span_for_ptr(ptr) != NULL;
}

size_t hz5_largefront_usable_size(void* ptr) {
  Hz5LargeSpan* span = hz5_largefront_span_for_ptr(ptr);
  if (!span || ptr != span->base) {
    return 0;
  }
  return span->class_bytes;
}

#else

void hz5_largefront_ownerhub_drain_some(uint32_t budget) {
  (void)budget;
}

void* hz5_largefront_alloc(size_t size, size_t align) {
  (void)size;
  (void)align;
  return NULL;
}

Hz5LargeFrontFreeResult hz5_largefront_free(void* ptr) {
  (void)ptr;
  return HZ5_LARGEFRONT_FREE_NOT_OWNED;
}

int hz5_largefront_can_handle(size_t size, size_t align) {
  (void)size;
  (void)align;
  return 0;
}

int hz5_largefront_owns(void* ptr) {
  (void)ptr;
  return 0;
}

size_t hz5_largefront_usable_size(void* ptr) {
  (void)ptr;
  return 0;
}
