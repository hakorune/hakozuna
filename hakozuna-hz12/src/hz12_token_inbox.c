#include "hz12_token_inbox.h"

#include "hz12.h"
#include "hz12_port.h"

#include <stdatomic.h>
#include <string.h>

typedef struct H12TokenInbox {
  HZ12_MUTEX lock;
  void* head;
  uint32_t count;
  uint32_t generation;
} H12TokenInbox;

static H12TokenInbox h12_token_inboxes[HZ12_OWNER_REGISTRY_CAP];
typedef struct H12TokenInboxCounters {
  atomic_uint_fast64_t publish_attempt;
  atomic_uint_fast64_t publish_accept;
  atomic_uint_fast64_t publish_registry_reject;
  atomic_uint_fast64_t publish_generation_reject;
  atomic_uint_fast64_t publish_overflow;
  atomic_uint_fast64_t fallback_objects;
  atomic_uint_fast64_t drain_batches;
  atomic_uint_fast64_t drain_objects;
  atomic_uint_fast64_t drain_generation_reject;
  atomic_uint_fast32_t inbox_current_max;
} H12TokenInboxCounters;

static H12TokenInboxCounters h12_token_counters;
static int h12_token_inbox_initialized;

static void h12_token_counters_reset(void) {
  atomic_store_explicit(&h12_token_counters.publish_attempt, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_token_counters.publish_accept, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_token_counters.publish_registry_reject, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_token_counters.publish_generation_reject, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_token_counters.publish_overflow, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_token_counters.fallback_objects, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_token_counters.drain_batches, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_token_counters.drain_objects, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_token_counters.drain_generation_reject, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_token_counters.inbox_current_max, 0u,
                        memory_order_relaxed);
}

static void h12_token_counter_max(atomic_uint_fast32_t* counter,
                                  uint32_t candidate) {
  uint_fast32_t current = atomic_load_explicit(counter, memory_order_relaxed);
  while (candidate > current &&
         !atomic_compare_exchange_weak_explicit(
             counter, &current, candidate, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}

static void h12_token_free_chain(void* head) {
  while (head) {
    void* next = *(void**)head;
    hz12_free(head);
    head = next;
  }
}

static uint32_t h12_token_link_batch(void** objects, uint32_t count,
                                     void** out_head, void** out_tail) {
  uint32_t i;
  uint32_t linked = 0u;
  void* head = NULL;
  void* tail = NULL;
  for (i = 0u; i < count; ++i) {
    void* ptr = objects[i];
    if (!ptr) continue;
    *(void**)ptr = head;
    if (!tail) tail = ptr;
    head = ptr;
    linked += 1u;
  }
  *out_head = head;
  *out_tail = tail;
  return linked;
}

void h12_token_inbox_reset(void) {
  uint32_t i;
  if (!h12_token_inbox_initialized) {
    for (i = 0u; i < HZ12_OWNER_REGISTRY_CAP; ++i) {
      hz12_mutex_init(&h12_token_inboxes[i].lock);
    }
    h12_token_inbox_initialized = 1;
  }
  for (i = 0u; i < HZ12_OWNER_REGISTRY_CAP; ++i) {
    hz12_mutex_lock(&h12_token_inboxes[i].lock);
    h12_token_inboxes[i].head = NULL;
    h12_token_inboxes[i].count = 0u;
    h12_token_inboxes[i].generation = 0u;
    hz12_mutex_unlock(&h12_token_inboxes[i].lock);
  }
  h12_token_counters_reset();
}

int h12_token_inbox_publish(H12OwnerToken token, void** objects,
                            uint32_t count) {
  H12TokenInbox* inbox;
  void* head;
  void* tail;
  uint32_t linked;
  if (!h12_token_inbox_initialized || !objects || count == 0u ||
      token.slot >= HZ12_OWNER_REGISTRY_CAP) {
    return 0;
  }
  atomic_fetch_add_explicit(&h12_token_counters.publish_attempt, 1u,
                            memory_order_relaxed);
  linked = h12_token_link_batch(objects, count, &head, &tail);
  if (linked == 0u) return 1;
  if (!h12_owner_publishable(token)) {
    atomic_fetch_add_explicit(&h12_token_counters.publish_registry_reject, 1u,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&h12_token_counters.fallback_objects, linked,
                              memory_order_relaxed);
    h12_token_free_chain(head);
    return 0;
  }
  inbox = &h12_token_inboxes[token.slot];
  hz12_mutex_lock(&inbox->lock);
  if (inbox->generation != token.generation) {
    if (inbox->count != 0u) {
      hz12_mutex_unlock(&inbox->lock);
      atomic_fetch_add_explicit(
          &h12_token_counters.publish_generation_reject, 1u,
          memory_order_relaxed);
      atomic_fetch_add_explicit(&h12_token_counters.fallback_objects, linked,
                                memory_order_relaxed);
      h12_token_free_chain(head);
      return 0;
    }
    inbox->generation = token.generation;
  }
  if (inbox->count + linked > HZ12_TOKEN_INBOX_CAP) {
    hz12_mutex_unlock(&inbox->lock);
    atomic_fetch_add_explicit(&h12_token_counters.publish_overflow, 1u,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&h12_token_counters.fallback_objects, linked,
                              memory_order_relaxed);
    h12_token_free_chain(head);
    return 0;
  }
  *(void**)tail = inbox->head;
  inbox->head = head;
  inbox->count += linked;
  h12_token_counter_max(&h12_token_counters.inbox_current_max, inbox->count);
  hz12_mutex_unlock(&inbox->lock);
  atomic_fetch_add_explicit(&h12_token_counters.publish_accept, 1u,
                            memory_order_relaxed);
  return 1;
}

uint32_t h12_token_inbox_drain(H12OwnerToken token) {
  H12TokenInbox* inbox;
  void* head;
  uint32_t count;
  if (!h12_token_inbox_initialized || token.slot >= HZ12_OWNER_REGISTRY_CAP) {
    return 0u;
  }
  inbox = &h12_token_inboxes[token.slot];
  hz12_mutex_lock(&inbox->lock);
  if (inbox->generation != token.generation && inbox->count != 0u) {
    hz12_mutex_unlock(&inbox->lock);
    atomic_fetch_add_explicit(
        &h12_token_counters.drain_generation_reject, 1u,
        memory_order_relaxed);
    return 0u;
  }
  if (inbox->generation != token.generation) {
    hz12_mutex_unlock(&inbox->lock);
    return 0u;
  }
  head = inbox->head;
  count = inbox->count;
  inbox->head = NULL;
  inbox->count = 0u;
  hz12_mutex_unlock(&inbox->lock);
  if (head) {
    h12_token_free_chain(head);
    atomic_fetch_add_explicit(&h12_token_counters.drain_batches, 1u,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&h12_token_counters.drain_objects, count,
                              memory_order_relaxed);
  }
  return count;
}

uint32_t h12_token_inbox_pending(H12OwnerToken token) {
  H12TokenInbox* inbox;
  uint32_t count = 0u;
  if (!h12_token_inbox_initialized || token.slot >= HZ12_OWNER_REGISTRY_CAP) {
    return 0u;
  }
  inbox = &h12_token_inboxes[token.slot];
  hz12_mutex_lock(&inbox->lock);
  if (inbox->generation == token.generation) count = inbox->count;
  hz12_mutex_unlock(&inbox->lock);
  return count;
}

void h12_token_inbox_stats(H12TokenInboxStats* out) {
  if (!out) return;
  out->publish_attempt = atomic_load_explicit(
      &h12_token_counters.publish_attempt, memory_order_relaxed);
  out->publish_accept = atomic_load_explicit(&h12_token_counters.publish_accept,
                                             memory_order_relaxed);
  out->publish_registry_reject = atomic_load_explicit(
      &h12_token_counters.publish_registry_reject, memory_order_relaxed);
  out->publish_generation_reject = atomic_load_explicit(
      &h12_token_counters.publish_generation_reject, memory_order_relaxed);
  out->publish_overflow = atomic_load_explicit(
      &h12_token_counters.publish_overflow, memory_order_relaxed);
  out->fallback_objects = atomic_load_explicit(
      &h12_token_counters.fallback_objects, memory_order_relaxed);
  out->drain_batches = atomic_load_explicit(&h12_token_counters.drain_batches,
                                            memory_order_relaxed);
  out->drain_objects = atomic_load_explicit(&h12_token_counters.drain_objects,
                                            memory_order_relaxed);
  out->drain_generation_reject = atomic_load_explicit(
      &h12_token_counters.drain_generation_reject, memory_order_relaxed);
  out->inbox_current_max = (uint32_t)atomic_load_explicit(
      &h12_token_counters.inbox_current_max, memory_order_relaxed);
}

void h12_token_inbox_dump(FILE* out) {
  H12TokenInboxStats stats;
  if (!out) return;
  h12_token_inbox_stats(&stats);
  fprintf(out,
          "[HZ12_TOKEN_INBOX] publish_attempt=%llu publish_accept=%llu "
          "registry_reject=%llu generation_reject=%llu overflow=%llu "
          "fallback_objects=%llu drain_batches=%llu drain_objects=%llu "
          "drain_generation_reject=%llu inbox_max=%u\n",
          (unsigned long long)stats.publish_attempt,
          (unsigned long long)stats.publish_accept,
          (unsigned long long)stats.publish_registry_reject,
          (unsigned long long)stats.publish_generation_reject,
          (unsigned long long)stats.publish_overflow,
          (unsigned long long)stats.fallback_objects,
          (unsigned long long)stats.drain_batches,
          (unsigned long long)stats.drain_objects,
          (unsigned long long)stats.drain_generation_reject,
          stats.inbox_current_max);
}
