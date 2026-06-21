#include "h8_internal.h"

#include <sched.h>

void h8_owner_add_owned_span(H8OwnerRecord* owner, H8Span* span) {
  pthread_mutex_lock(&owner->owned_lock);
  span->next_owned = owner->owned_head;
  owner->owned_head = span;
  span->next_owned_class = owner->owned_by_class[span->class_id];
  owner->owned_by_class[span->class_id] = span;
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
  cur = &owner->owned_by_class[span->class_id];
  while (*cur) {
    if (*cur == span) {
      *cur = span->next_owned_class;
      break;
    }
    cur = &(*cur)->next_owned_class;
  }
  span->next_owned = NULL;
  span->next_owned_class = NULL;
  if (owner->active_spans[span->class_id] == span) {
    owner->active_spans[span->class_id] = NULL;
  }
  pthread_mutex_unlock(&owner->owned_lock);
}

void h8_owner_add_orphan_span(H8OwnerRecord* owner, H8Span* span) {
  pthread_mutex_lock(&owner->owned_lock);
  span->next_orphan = owner->orphan_head;
  owner->orphan_head = span;
  span->next_orphan_class = owner->orphan_by_class[span->class_id];
  owner->orphan_by_class[span->class_id] = span;
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
  cur = &owner->orphan_by_class[span->class_id];
  while (*cur) {
    if (*cur == span) {
      *cur = span->next_orphan_class;
      break;
    }
    cur = &(*cur)->next_orphan_class;
  }
  span->next_orphan = NULL;
  span->next_orphan_class = NULL;
  if (owner->active_spans[span->class_id] == span) {
    owner->active_spans[span->class_id] = NULL;
  }
  pthread_mutex_unlock(&owner->owned_lock);
}

bool h8_owner_wait_publishers_zero(H8OwnerRecord* owner) {
  if (owner->permanent) {
    return true;
  }
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(atomic_load_explicit(&owner->control,
                                                        memory_order_acquire));
    if (ctl.publish_refs == 0) {
      return true;
    }
    sched_yield();
  }
}
