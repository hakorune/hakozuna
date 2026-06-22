#include "h8_internal.h"

#include <sched.h>
#include <stdlib.h>

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
    size_t pending = atomic_load_explicit(&span->pending_count, memory_order_acquire);
    if (qstate == H8_Q_IDLE && pending == 0) {
      break;
    }
    if (qstate == H8_Q_IDLE && pending != 0) {
      h8_span_notify(owner, span);
      qstate = H8_Q_QUEUED;
    }
    if (qstate == H8_Q_QUEUED) {
      h8_span_collect_remote(owner, span);
    } else {
      sched_yield();
    }
  }

  if (h8_span_state_load(span) == H8_SPAN_ORPHAN_QUIESCING &&
      atomic_load_explicit(&span->qstate, memory_order_acquire) == H8_Q_IDLE &&
      atomic_load_explicit(&span->publish_refs, memory_order_acquire) == 0 &&
      atomic_load_explicit(&span->pending_count, memory_order_acquire) == 0) {
    h8_span_mark_orphan_ready(span);
  }
}

void h8_owner_exit(H8OwnerRecord* owner) {
  if (!owner || owner->permanent) {
    return;
  }
  h8_owner_close_gate(owner);
  h8_owner_wait_publishers_zero(owner);
  h8_collect_owner_pending(owner);

  pthread_mutex_lock(&owner->owned_lock);
  H8Span* span = owner->owned_head;
  owner->owned_head = NULL;
  pthread_mutex_unlock(&owner->owned_lock);

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
  h8_owner_mark_dead(owner);
  h8_owner_free_stack_push(owner);
}
