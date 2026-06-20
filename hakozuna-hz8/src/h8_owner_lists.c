#include "h8_internal.h"

#include <sched.h>

void h8_owner_add_owned_span(H8OwnerRecord* owner, H8Span* span) {
  pthread_mutex_lock(&owner->owned_lock);
  span->next_owned = owner->owned_head;
  owner->owned_head = span;
  pthread_mutex_unlock(&owner->owned_lock);
}

void h8_owner_remove_owned_span(H8OwnerRecord* owner, H8Span* span) {
  pthread_mutex_lock(&owner->owned_lock);
  H8Span** cur = &owner->owned_head;
  while (*cur) {
    if (*cur == span) {
      *cur = span->next_owned;
      break;
    }
    cur = &(*cur)->next_owned;
  }
  pthread_mutex_unlock(&owner->owned_lock);
}

void h8_owner_add_orphan_span(H8OwnerRecord* owner, H8Span* span) {
  pthread_mutex_lock(&owner->owned_lock);
  span->next_orphan = owner->orphan_head;
  owner->orphan_head = span;
  pthread_mutex_unlock(&owner->owned_lock);
}

void h8_owner_remove_orphan_span(H8OwnerRecord* owner, H8Span* span) {
  pthread_mutex_lock(&owner->owned_lock);
  H8Span** cur = &owner->orphan_head;
  while (*cur) {
    if (*cur == span) {
      *cur = span->next_orphan;
      break;
    }
    cur = &(*cur)->next_orphan;
  }
  pthread_mutex_unlock(&owner->owned_lock);
}

bool h8_owner_acquire_span_as_orphan(H8OwnerRecord* owner, H8Span* span) {
  if (!owner || !span) {
    atomic_fetch_add_explicit(&h8g.adopt_fail_count, 1, memory_order_relaxed);
    return false;
  }
  if (span->span_state != H8_SPAN_ORPHAN_READY) {
    atomic_fetch_add_explicit(&h8g.adopt_fail_count, 1, memory_order_relaxed);
    return false;
  }
  span->span_state = H8_SPAN_ADOPTING;
  span->owner_slot = owner->slot;
  span->owner_generation = owner->generation;
  atomic_fetch_add_explicit(&span->span_epoch, 1, memory_order_acq_rel);
  span->span_state = H8_SPAN_OWNED_ACTIVE;
  atomic_fetch_add_explicit(&h8g.adopt_success_count, 1, memory_order_relaxed);
  return true;
}

bool h8_owner_wait_publishers_zero(H8OwnerRecord* owner) {
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(atomic_load_explicit(&owner->control,
                                                      memory_order_acquire));
    if (ctl.publish_refs == 0) {
      return true;
    }
    sched_yield();
  }
}
