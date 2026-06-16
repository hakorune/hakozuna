#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
#include "hz5_largefront_remote_publish.inc"

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_POP_BUDGET
static Hz5LargeSpan* hz5_largefront_owner_inbox_pop_one(Hz5OwnerToken owner,
                                                        uint32_t class_index) {
  if (owner.slot == 0 || class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    return NULL;
  }
  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[owner.slot][class_index];
  void* head = atomic_load_explicit(inbox, memory_order_acquire);
  while (head) {
    Hz5LargeSpan* span = (Hz5LargeSpan*)head;
    void* next = span->next;
    if (atomic_compare_exchange_weak_explicit(inbox,
                                              &head,
                                              next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      span->next = NULL;
      return span;
    }
  }
  hz5_ownerhub_clear_pending(owner, HZ5_OWNERHUB_FRONT_LARGE, class_index);
  return NULL;
}

static Hz5LargeSpan* hz5_largefront_drain_remote_class_pop_budget(
    Hz5LargeTls* tls,
    uint32_t class_index,
    uint32_t budget,
    uint32_t* drained_out) {
  uint32_t drained = 0;
  Hz5LargeSpan* taken = NULL;
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }

  while (!taken || (budget != 0u && drained < budget)) {
    Hz5LargeSpan* span =
        hz5_largefront_owner_inbox_pop_one(tls->owner, class_index);
    if (!span) {
      break;
    }
    if (!hz5_owner_equal(span->owner, tls->owner)) {
      hz5_largefront_mark_orphan(span);
      continue;
    }
    if (!taken &&
        hz5_largefront_state_cas(span,
                                 (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                 (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
      hz5_largefront_payload_reactivate(span);
      taken = span;
      continue;
    }
    if (budget != 0u && drained < budget &&
        hz5_largefront_remote_pending_to_local(span)) {
      hz5_largefront_local_push(tls, class_index, span);
      ++drained;
      continue;
    }
    hz5_largefront_mark_orphan(span);
  }

  if (drained_out) {
    *drained_out = drained;
  }
  return taken;
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
static Hz5LargeSpan* hz5_largefront_drain_chunk_inbox(
    Hz5LargeTls* tls,
    uint32_t class_index,
    uint32_t* drained_out) {
  uint32_t drained = 0;
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }

  _Atomic(Hz5LargeRemoteChunk*)* inbox =
      &g_hz5_largefront_owner_chunk_inbox[tls->owner.slot][class_index];
  hz5_ownerhub_clear_pending(tls->owner,
                             HZ5_OWNERHUB_FRONT_LARGE,
                             class_index);
  Hz5LargeRemoteChunk* chunk =
      atomic_exchange_explicit(inbox, NULL, memory_order_acq_rel);
  Hz5LargeSpan* taken = NULL;
  while (chunk) {
    Hz5LargeRemoteChunk* next_chunk = chunk->next;
    for (uint32_t i = 0; i < chunk->count; ++i) {
      Hz5LargeSpan* span = chunk->spans[i];
      if (!span) {
        continue;
      }
      if (!hz5_owner_equal(span->owner, tls->owner)) {
        hz5_largefront_mark_orphan(span);
        continue;
      }
      if (!taken &&
          hz5_largefront_state_cas(span,
                                   (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                   (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
        hz5_largefront_payload_reactivate(span);
        taken = span;
        continue;
      }
      if (hz5_largefront_remote_pending_to_local(span)) {
        hz5_largefront_local_push(tls, class_index, span);
        ++drained;
      } else {
        hz5_largefront_mark_orphan(span);
      }
    }
    chunk = next_chunk;
  }
  if (drained_out) {
    *drained_out = drained;
  }
  return taken;
}
#endif

static Hz5LargeSpan* hz5_largefront_drain_remote_class_budget(
    Hz5LargeTls* tls,
    uint32_t class_index,
    uint32_t budget,
    uint32_t* drained_out) {
  uint32_t drained = 0;
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
  Hz5LargeSpan* chunk_taken =
      hz5_largefront_drain_chunk_inbox(tls, class_index, &drained);
  if (chunk_taken) {
    if (drained_out) {
      *drained_out = drained;
    }
    return chunk_taken;
  }
#endif

  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[tls->owner.slot][class_index];
  hz5_ownerhub_clear_pending(tls->owner,
                             HZ5_OWNERHUB_FRONT_LARGE,
                             class_index);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_EMPTY_GATED
  if (!atomic_load_explicit(inbox, memory_order_acquire)) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }
#endif
  void* head = atomic_exchange_explicit(inbox, NULL, memory_order_acq_rel);
  Hz5LargeSpan* taken = NULL;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  uint32_t policy_take_first = 0;
  uint32_t policy_to_local = 0;
  uint32_t policy_republished = 0;
  uint32_t policy_held = 0;
  uint32_t policy_orphaned = 0;
  uint32_t policy_state_fail = 0;
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_BULK_LOCAL
  Hz5LargeSpan* bulk_local_head = NULL;
  Hz5LargeSpan* bulk_local_tail = NULL;
  uint32_t bulk_local_count = 0;
#define HZ5_LARGEFRONT_FLUSH_BULK_LOCAL()                                      \
  do {                                                                         \
    hz5_largefront_local_push_list(tls,                                        \
                                   class_index,                                \
                                   bulk_local_head,                            \
                                   bulk_local_tail,                            \
                                   bulk_local_count);                          \
    bulk_local_head = NULL;                                                    \
    bulk_local_tail = NULL;                                                    \
    bulk_local_count = 0;                                                      \
  } while (0)
#define HZ5_LARGEFRONT_STAGE_LOCAL_SPAN(span_)                                 \
  do {                                                                         \
    (span_)->next = bulk_local_head;                                           \
    if (!bulk_local_head) {                                                    \
      bulk_local_tail = (span_);                                               \
    }                                                                          \
    bulk_local_head = (span_);                                                 \
    ++bulk_local_count;                                                        \
  } while (0)
#else
#define HZ5_LARGEFRONT_FLUSH_BULK_LOCAL() ((void)0)
#define HZ5_LARGEFRONT_STAGE_LOCAL_SPAN(span_)                                 \
  hz5_largefront_local_push(tls, class_index, (span_))
#endif
  while (head) {
    Hz5LargeSpan* span = (Hz5LargeSpan*)head;
    head = span->next;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TAKE_ONLY
    if (taken) {
      HZ5_LARGEFRONT_FLUSH_BULK_LOCAL();
      Hz5LargeSpan* tail = span;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      uint32_t republished = 1;
#endif
      while (tail->next) {
        tail = tail->next;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
        ++republished;
#endif
      }
      (void)hz5_largefront_remote_publish_list(tls->owner,
                                               class_index,
                                               span,
                                               tail);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      policy_republished += republished;
      hz5_largefront_policy_note_owner_drain(class_index,
                                             policy_take_first +
                                                 policy_republished,
                                             policy_take_first,
                                             policy_to_local,
                                             policy_republished,
                                             policy_held,
                                             policy_orphaned,
                                             policy_state_fail);
#endif
      if (drained_out) {
        *drained_out = drained;
      }
      return taken;
    }
#endif
    if (budget != 0u && drained >= budget) {
      HZ5_LARGEFRONT_FLUSH_BULK_LOCAL();
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      uint32_t republished = 0;
#endif
      Hz5LargeSpan* tail = span;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L7
      if (hz5_largefront_class_bytes(class_index) == 131072u) {
        uint32_t remainder_count = 0;
        for (Hz5LargeSpan* it = span; it; it = it->next) {
          ++remainder_count;
        }
        if (remainder_count >=
            HZ5_LARGEFRONT_POLICY_L7_REMAINDER_LOCAL_THRESHOLD) {
          Hz5LargeSpan* remainder = span;
          while (remainder) {
            Hz5LargeSpan* next = remainder->next;
            remainder->next = NULL;
            if (!hz5_owner_equal(remainder->owner, tls->owner)) {
              hz5_largefront_mark_orphan(remainder);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
              ++policy_orphaned;
#endif
            } else if (hz5_largefront_remote_pending_to_local(remainder)) {
              HZ5_LARGEFRONT_STAGE_LOCAL_SPAN(remainder);
              ++drained;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
              ++policy_to_local;
#endif
            } else {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
              ++policy_state_fail;
#endif
              hz5_largefront_mark_orphan(remainder);
            }
            remainder = next;
          }
          HZ5_LARGEFRONT_FLUSH_BULK_LOCAL();
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
          hz5_largefront_policy_note_owner_drain(class_index,
                                                 drained + policy_take_first +
                                                     policy_held +
                                                     policy_orphaned +
                                                     policy_state_fail,
                                                 policy_take_first,
                                                 policy_to_local,
                                                 policy_republished,
                                                 policy_held,
                                                 policy_orphaned,
                                                 policy_state_fail);
#endif
          if (drained_out) {
            *drained_out = drained;
          }
          return taken;
        }
      }
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_HOLD
      Hz5LargeSpan* remainder = span;
      while (remainder) {
        Hz5LargeSpan* next = remainder->next;
        if (!hz5_owner_equal(remainder->owner, tls->owner)) {
          remainder->next = NULL;
          hz5_largefront_mark_orphan(remainder);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
          ++policy_orphaned;
#endif
        } else if (!hz5_largefront_remote_hold_push(tls,
                                                    class_index,
                                                    remainder)) {
          remainder->next = next;
          break;
        }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
        ++policy_held;
#endif
        remainder = next;
      }
      if (!remainder) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
        hz5_largefront_policy_note_owner_drain(class_index,
                                               drained + policy_take_first +
                                                   policy_held +
                                                   policy_orphaned,
                                               policy_take_first,
                                               policy_to_local,
                                               policy_republished,
                                               policy_held,
                                               policy_orphaned,
                                               policy_state_fail);
#endif
        if (drained_out) {
          *drained_out = drained;
        }
        return taken;
      }
      span = remainder;
      tail = span;
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      republished = 1;
#endif
      while (tail->next) {
        tail = tail->next;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
        ++republished;
#endif
      }
      (void)hz5_largefront_remote_publish_list(tls->owner,
                                               class_index,
                                               span,
                                               tail);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      policy_republished += republished;
      hz5_largefront_policy_note_owner_drain(class_index,
                                             drained + policy_take_first +
                                                 policy_republished +
                                                 policy_held +
                                                 policy_orphaned,
                                             policy_take_first,
                                             policy_to_local,
                                             policy_republished,
                                             policy_held,
                                             policy_orphaned,
                                             policy_state_fail);
#endif
      if (drained_out) {
        *drained_out = drained;
      }
      return taken;
    }
    if (!hz5_owner_equal(span->owner, tls->owner)) {
      hz5_largefront_mark_orphan(span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      ++policy_orphaned;
#endif
      continue;
    }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TAKE_FIRST
    if (!taken &&
        hz5_largefront_state_cas(span,
                                 (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                 (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
      span->next = NULL;
      hz5_largefront_payload_reactivate(span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_remote_take_first[class_index]);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      ++policy_take_first;
#endif
      taken = span;
      continue;
    }
#endif
    if (hz5_largefront_remote_pending_to_local(span)) {
      HZ5_LARGEFRONT_STAGE_LOCAL_SPAN(span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_remote_to_local[class_index]);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      ++policy_to_local;
#endif
    } else {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      ++policy_state_fail;
#endif
    }
    ++drained;
  }
  HZ5_LARGEFRONT_FLUSH_BULK_LOCAL();
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  hz5_largefront_policy_note_owner_drain(class_index,
                                         drained + policy_take_first +
                                             policy_orphaned +
                                             policy_state_fail,
                                         policy_take_first,
                                         policy_to_local,
                                         policy_republished,
                                         policy_held,
                                         policy_orphaned,
                                         policy_state_fail);
#endif
  if (drained_out) {
    *drained_out = drained;
  }
#undef HZ5_LARGEFRONT_STAGE_LOCAL_SPAN
#undef HZ5_LARGEFRONT_FLUSH_BULK_LOCAL
  return taken;
}

static Hz5LargeSpan* hz5_largefront_drain_remote_class(Hz5LargeTls* tls,
                                                       uint32_t class_index) {
  return hz5_largefront_drain_remote_class_budget(tls,
                                                  class_index,
                                                  0u,
                                                  NULL);
}

void hz5_largefront_ownerhub_drain_some(uint32_t budget) {
  Hz5LargeTls* tls = hz5_largefront_tls();
  uint32_t remaining = budget;
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_CLASS_COUNT; ++i) {
    if (remaining == 0u) {
      break;
    }
    uint32_t drained = 0;
    (void)hz5_largefront_drain_remote_class_budget(tls,
                                                   i,
                                                   remaining,
                                                   &drained);
    remaining -= drained > remaining ? remaining : drained;
  }
}
#endif
