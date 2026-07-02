#include "h8_internal.h"

#if defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)

#include "h8_hz9_slab_route_internal.h"

void h9_slab_pending_queue_push(H8OwnerRecord* owner,
                                H9MediumSlabRoutePage* page) {
  H9SlabOwnerState* state = h9_slab_owner_state(owner, false);
  if (!state) {
    return;
  }
  H9MediumSlabRoutePage* head =
      atomic_load_explicit(&state->pending_head, memory_order_relaxed);
  while (true) {
    page->next_pending = head;
    if (atomic_compare_exchange_weak_explicit(&state->pending_head, &head,
                                              page, memory_order_release,
                                              memory_order_relaxed)) {
      atomic_fetch_add_explicit(&state->pending_count, 1,
                                memory_order_relaxed);
      H8_DEBUG_INC(h9_slab_queue_push);
      return;
    }
  }
}

void h9_slab_signal_work(H8OwnerRecord* owner, H9MediumSlabRoutePage* page) {
  uint8_t cur = atomic_load_explicit(&page->qstate, memory_order_acquire);
  for (;;) {
    if (cur == H8_Q_IDLE) {
      uint8_t expected = H8_Q_IDLE;
      if (atomic_compare_exchange_weak_explicit(&page->qstate, &expected,
                                                H8_Q_QUEUED,
                                                memory_order_acq_rel,
                                                memory_order_acquire)) {
        h9_slab_pending_queue_push(owner, page);
        return;
      }
      cur = expected;
      continue;
    }
    if (cur == H8_Q_DRAINING) {
      uint8_t expected = H8_Q_DRAINING;
      if (atomic_compare_exchange_weak_explicit(&page->qstate, &expected,
                                                H8_Q_DRAINING_DIRTY,
                                                memory_order_acq_rel,
                                                memory_order_acquire)) {
        H8_DEBUG_INC(h9_slab_qstate_dirty);
        return;
      }
      cur = expected;
      continue;
    }
    return;
  }
}

static H9MediumSlabRoutePage* h9_slab_pending_pop_one(H8OwnerRecord* owner) {
  h8_platform_mutex_lock(&owner->pending_lock);
  H9SlabOwnerState* state = h9_slab_owner_state(owner, false);
  if (!state) {
    h8_platform_mutex_unlock(&owner->pending_lock);
    return NULL;
  }
  if (!state->pending_carry) {
    state->pending_carry = atomic_exchange_explicit(
        &state->pending_head, NULL, memory_order_acq_rel);
  }
  H9MediumSlabRoutePage* page = state->pending_carry;
  if (page) {
    state->pending_carry = page->next_pending;
    page->next_pending = NULL;
    atomic_fetch_sub_explicit(&state->pending_count, 1,
                              memory_order_relaxed);
    H8_DEBUG_INC(h9_slab_queue_pop);
  }
  h8_platform_mutex_unlock(&owner->pending_lock);
  return page;
}

static bool h9_slab_collect_queued_page(H9MediumSlabRoutePage* page) {
  uint8_t expected = H8_Q_QUEUED;
  if (!atomic_compare_exchange_strong_explicit(&page->qstate, &expected,
                                               H8_Q_DRAINING,
                                               memory_order_acq_rel,
                                               memory_order_acquire)) {
    return false;
  }
  H8_DEBUG_INC(h9_slab_queue_collect_page);
  size_t collected = h9_slab_collect_pending_page(page);
  if (collected != 0) {
    H8_DEBUG_ADD(h9_slab_pending_collect_queue_slot, collected);
  }
  uint8_t finish = H8_Q_DRAINING;
  if (atomic_compare_exchange_strong_explicit(&page->qstate, &finish,
                                              H8_Q_IDLE,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
    return false;
  }
  if (finish == H8_Q_DRAINING_DIRTY) {
    uint8_t dirty = H8_Q_DRAINING_DIRTY;
    bool requeue = atomic_compare_exchange_strong_explicit(
        &page->qstate, &dirty, H8_Q_QUEUED, memory_order_acq_rel,
        memory_order_acquire);
    if (requeue) {
      H8_DEBUG_INC(h9_slab_queue_requeue);
    }
    return requeue;
  }
  return false;
}

size_t h9_slab_collect_owner_pending(H8OwnerRecord* owner, size_t budget) {
  if (!owner || budget == 0) {
    return 0;
  }
  H8_DEBUG_INC(h9_slab_queue_collect_call);
  size_t processed = 0;
  while (processed < budget) {
    H9MediumSlabRoutePage* page = h9_slab_pending_pop_one(owner);
    if (!page) {
      break;
    }
    ++processed;
    if (h9_slab_collect_queued_page(page)) {
      h9_slab_pending_queue_push(owner, page);
    }
  }
  return processed;
}

#else
typedef int h9_slab_queue_translation_unit_silence;
#endif
