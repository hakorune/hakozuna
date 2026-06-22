#include "h8_internal.h"

#include <sched.h>
#include <stdlib.h>

static H8OwnerWord h8_owner_word_from_span(const H8Span* span) {
  return h8_span_owner_word_load(span);
}

static bool h8_span_handoff_quiescent(const H8Span* span) {
  return atomic_load_explicit(&span->publish_refs, memory_order_acquire) == 0 &&
         h8_span_pending_quiescent((H8Span*)span);
}

bool h8_span_publish_enter(H8Span* span) {
  uint16_t cur = atomic_load_explicit(&span->publish_refs, memory_order_acquire);
  for (;;) {
    if (atomic_load_explicit(&span->publish_closed, memory_order_acquire) ||
        h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
      return false;
    }
    if (cur == UINT16_MAX) {
      abort();
    }
    uint16_t next = (uint16_t)(cur + 1u);
    if (atomic_compare_exchange_weak_explicit(&span->publish_refs, &cur, next,
                                              memory_order_acquire,
                                              memory_order_relaxed)) {
      H8_DEBUG_INC(span_publish_enter_count);
      if (atomic_load_explicit(&span->publish_closed, memory_order_acquire) ||
          h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
        h8_span_publish_exit(span);
        return false;
      }
      return true;
    }
  }
}

void h8_span_publish_exit(H8Span* span) {
  atomic_fetch_sub_explicit(&span->publish_refs, 1, memory_order_acq_rel);
  H8_DEBUG_INC(span_publish_exit_count);
}

void h8_span_close_publish(H8Span* span) {
  atomic_store_explicit(&span->publish_closed, 1, memory_order_release);
}

void h8_span_wait_publishers_zero(H8Span* span) {
  for (;;) {
    if (atomic_load_explicit(&span->publish_refs, memory_order_acquire) == 0) {
      return;
    }
    sched_yield();
  }
}

void h8_span_mark_orphan_quiescing(H8Span* span) {
  h8_span_close_publish(span);
  h8_span_state_store(span, H8_SPAN_ORPHAN_QUIESCING, memory_order_release);
  H8_DEBUG_INC(orphan_quiesce_count);
}

void h8_span_mark_orphan_ready(H8Span* span) {
  h8_span_state_store(span, H8_SPAN_ORPHAN_READY, memory_order_release);
  H8_DEBUG_INC(orphan_ready_count);
}

bool h8_span_handoff(H8Span* span, H8OwnerWord expected_old_token,
                     H8OwnerRecord* target_owner) {
  if (!span || !target_owner) {
    H8_DEBUG_INC(handoff_fail_count);
    return false;
  }
  if (!h8_owner_is_alive_and_open(target_owner)) {
    H8_DEBUG_INC(handoff_fail_count);
    return false;
  }

  bool target_is_orphan = false;
  switch (target_owner->placement) {
  case H8_OWNER_PLACEMENT_ORPHAN:
    target_is_orphan = true;
    break;
  case H8_OWNER_PLACEMENT_OWNED:
    break;
  default:
    H8_DEBUG_INC(handoff_fail_count);
    return false;
  }

  H8OwnerWord cur = h8_owner_word_from_span(span);
  if (!h8_owner_word_equal(cur, expected_old_token) ||
      ((cur.state != H8_SPAN_OWNED_ACTIVE && cur.state != H8_SPAN_ORPHAN_READY) ||
       !h8_span_handoff_quiescent(span))) {
    H8_DEBUG_INC(handoff_fail_count);
    return false;
  }

  H8OwnerWord next = cur;
  next.state = H8_SPAN_ADOPTING;
  next.span_epoch = (uint32_t)(next.span_epoch + 1u);
  next.slot = (uint8_t)target_owner->slot;
  next.generation = (uint16_t)target_owner->generation;
  h8_span_owner_word_store(span, next, memory_order_release);
  if (target_is_orphan) {
    h8_owner_add_orphan_span(target_owner, span);
    atomic_fetch_add_explicit(&h8g.orphan_span_count, 1, memory_order_relaxed);
    H8_DEBUG_INC(orphan_handoff_count);
  } else {
    h8_owner_add_owned_span(target_owner, span);
  }
  next.state = H8_SPAN_OWNED_ACTIVE;
  h8_span_owner_word_store(span, next, memory_order_release);
  atomic_store_explicit(&span->publish_closed, 0, memory_order_release);
  H8_DEBUG_INC(handoff_success_count);
  return true;
}
