#include "h8_internal.h"

static H8OwnerWord h8_owner_word_from_span(const H8Span* span) {
  return h8_owner_word_make((uint8_t)span->owner_slot, span->owner_generation,
                            (H8SpanState)span->span_state,
                            atomic_load_explicit(&span->span_epoch, memory_order_acquire));
}

static bool h8_span_handoff_quiescent(const H8Span* span) {
  return atomic_load_explicit(&span->qstate, memory_order_acquire) == H8_Q_IDLE &&
         h8_bitmap_popcount((const _Atomic uint64_t*)span->pending_bits,
                            h8_word_count_for_slots(span->slot_count)) == 0;
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
      cur.state != H8_SPAN_OWNED_ACTIVE || !h8_span_handoff_quiescent(span)) {
    atomic_fetch_add_explicit(&h8g.handoff_fail_count, 1, memory_order_relaxed);
    return false;
  }

  span->span_state = H8_SPAN_ADOPTING;
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
  span->span_state = H8_SPAN_OWNED_ACTIVE;
  atomic_fetch_add_explicit(&h8g.handoff_success_count, 1, memory_order_relaxed);
  return true;
}
