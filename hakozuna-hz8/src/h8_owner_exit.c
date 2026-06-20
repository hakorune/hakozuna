#include "h8_internal.h"

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
      atomic_fetch_add_explicit(&h8g.owner_exit_count, 1, memory_order_relaxed);
      return;
    }
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
    if (atomic_load_explicit(&span->used_count, memory_order_acquire) == 0) {
      h8_span_retire(span);
    } else {
      span->span_state = H8_SPAN_ORPHAN_READY;
      atomic_fetch_add_explicit(&span->span_epoch, 1, memory_order_acq_rel);
      span->owner_slot = h8_orphan_owner()->slot;
      span->owner_generation = h8_orphan_owner()->generation;
      h8_owner_add_orphan_span(h8_orphan_owner(), span);
      atomic_fetch_add_explicit(&h8g.orphan_span_count, 1, memory_order_relaxed);
      atomic_fetch_add_explicit(&h8g.orphan_handoff_count, 1, memory_order_relaxed);
    }
    span = next;
  }
  h8_owner_mark_dead(owner);
  h8_owner_free_stack_push(owner);
}
