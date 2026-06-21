#include "h8_internal.h"

#include <sched.h>
#include <stdlib.h>

static H8OwnerWord h8_owner_word_from_span(const H8Span* span) {
  return h8_owner_word_make((uint8_t)span->owner_slot, span->owner_generation,
                            h8_span_state_load(span),
                            atomic_load_explicit(&span->span_epoch, memory_order_acquire));
}

static bool h8_span_handoff_quiescent(const H8Span* span) {
  return atomic_load_explicit(&span->qstate, memory_order_acquire) == H8_Q_IDLE &&
         h8_bitmap_popcount((const _Atomic uint64_t*)span->pending_bits,
                            h8_word_count_for_slots(span->slot_count)) == 0 &&
         atomic_load_explicit(&span->publish_refs, memory_order_acquire) == 0;
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
}

void h8_span_mark_orphan_ready(H8Span* span) {
  h8_span_state_store(span, H8_SPAN_ORPHAN_READY, memory_order_release);
}

bool h8_span_handoff(H8Span* span, H8OwnerWord expected_old_token,
                     H8OwnerRecord* target_owner) {
  if (!span || !target_owner) {
    atomic_fetch_add_explicit(&h8g.handoff_fail_count, 1, memory_order_relaxed);
    return false;
  }
  if (!h8_owner_is_alive_and_open(target_owner)) {
    atomic_fetch_add_explicit(&h8g.handoff_fail_count, 1, memory_order_relaxed);
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
    atomic_fetch_add_explicit(&h8g.handoff_fail_count, 1, memory_order_relaxed);
    return false;
  }

  H8OwnerWord cur = h8_owner_word_from_span(span);
  if (!h8_owner_word_equal(cur, expected_old_token) ||
      ((cur.state != H8_SPAN_OWNED_ACTIVE && cur.state != H8_SPAN_ORPHAN_READY) ||
       !h8_span_handoff_quiescent(span))) {
    atomic_fetch_add_explicit(&h8g.handoff_fail_count, 1, memory_order_relaxed);
    return false;
  }

  h8_span_state_store(span, H8_SPAN_ADOPTING, memory_order_release);
  atomic_fetch_add_explicit(&span->span_epoch, 1, memory_order_acq_rel);
  span->owner_slot = target_owner->slot;
  span->owner_generation = target_owner->generation;
  if (target_is_orphan) {
    h8_owner_add_orphan_span(target_owner, span);
    atomic_fetch_add_explicit(&h8g.orphan_span_count, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&h8g.orphan_handoff_count, 1, memory_order_relaxed);
  } else {
    h8_owner_add_owned_span(target_owner, span);
  }
  atomic_store_explicit(&span->publish_closed, 0, memory_order_release);
  h8_span_state_store(span, H8_SPAN_OWNED_ACTIVE, memory_order_release);
  atomic_fetch_add_explicit(&h8g.handoff_success_count, 1, memory_order_relaxed);
  return true;
}
