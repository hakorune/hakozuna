#include "h8_internal.h"

#include <sched.h>
#include <stdlib.h>
#if defined(H8_ENABLE_DEBUG_STATS)
#include <time.h>
#endif

#if defined(H8_ENABLE_DEBUG_STATS)
static uint64_t h8_owner_exit_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}
#endif

static void h8_owner_close_gate(H8OwnerRecord* owner) {
  uint64_t cur = atomic_load_explicit(&owner->control, memory_order_acquire);
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(cur);
    if (ctl.state != H8_OWNER_ALIVE) {
      return;
    }
    ctl.state = H8_OWNER_DYING;
    ctl.publish_closed = 1;
    uint64_t next = h8_ctl_pack(ctl);
    if (atomic_compare_exchange_weak_explicit(&owner->control, &cur, next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      H8_DEBUG_INC(owner_exit_count);
      return;
    }
  }
}

static void h8_owner_quiesce_span(H8Span* span) {
  H8OwnerRecord* owner = h8_owner_by_slot(span->owner_slot);

  if (h8_span_state_load(span) == H8_SPAN_OWNED_ACTIVE) {
    h8_span_mark_orphan_quiescing(span);
  }

  for (;;) {
    uint8_t qstate = atomic_load_explicit(&span->qstate, memory_order_acquire);
    if (h8_span_pending_quiescent(span)) {
      break;
    }
    if (qstate == H8_Q_IDLE) {
      if (atomic_load_explicit(&span->pending_word_mask, memory_order_acquire) != 0) {
        h8_span_notify(owner, span);
        qstate = H8_Q_QUEUED;
      } else if (h8_span_repair_pending_mask(owner, span)) {
        qstate = H8_Q_QUEUED;
      }
    }
    if (qstate == H8_Q_QUEUED) {
      h8_span_collect_remote(owner, span);
    } else if (qstate == H8_Q_DRAINING_DIRTY) {
      sched_yield();
    } else {
      sched_yield();
    }
  }

  if (h8_span_state_load(span) == H8_SPAN_ORPHAN_QUIESCING &&
      atomic_load_explicit(&span->publish_refs, memory_order_acquire) == 0 &&
      h8_span_pending_quiescent(span)) {
    h8_span_mark_orphan_ready(span);
  }
}

void h8_owner_exit(H8OwnerRecord* owner) {
  if (!owner || owner->permanent) {
    return;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t total_start = h8_owner_exit_now_ns();
#endif
  h8_owner_close_gate(owner);
  h8_owner_wait_publishers_zero(owner);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t collect_start = h8_owner_exit_now_ns();
#endif
  h8_collect_owner_pending(owner);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(owner_exit_collect_ns,
               (size_t)(h8_owner_exit_now_ns() - collect_start));
#endif

  pthread_mutex_lock(&owner->owned_lock);
  H8Span* span = owner->owned_head;
  owner->owned_head = NULL;
  pthread_mutex_unlock(&owner->owned_lock);

#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t walk_start = h8_owner_exit_now_ns();
#endif
  while (span) {
    H8Span* next = span->next_owned;
    h8_slot_shadow_verify_span(span);
    if (atomic_load_explicit(&span->used_count, memory_order_acquire) == 0) {
      h8_span_retire(span);
    } else {
      h8_owner_quiesce_span(span);
      H8OwnerWord expected = h8_span_owner_word_load(span);
      if (owner->active_spans[span->class_id] == span) {
        owner->active_spans[span->class_id] = NULL;
      }
      if (!h8_span_handoff(span, expected, h8_orphan_owner())) {
        H8_DEBUG_INC(invalid_count);
        abort();
      }
    }
    span = next;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(owner_exit_span_walk_ns,
               (size_t)(h8_owner_exit_now_ns() - walk_start));
#endif
  h8_owner_mark_dead(owner);
  h8_owner_free_stack_push(owner);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(owner_exit_total_ns,
               (size_t)(h8_owner_exit_now_ns() - total_start));
#endif
}
