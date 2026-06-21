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

bool h8_owner_wait_publishers_zero(H8OwnerRecord* owner) {
  if (owner->permanent) {
    return true;
  }
  for (;;) {
    if (atomic_load_explicit(&owner->lifecycle_refs, memory_order_acquire) == 0) {
      return true;
    }
    sched_yield();
  }
}
