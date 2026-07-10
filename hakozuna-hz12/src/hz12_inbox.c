#include "hz12_inbox.h"

#include "hz12.h"
#include "hz12_span_accounting.h"

#ifndef HZ12_INBOX_ACCOUNTING
#define HZ12_INBOX_ACCOUNTING 1
#endif

#ifndef HZ12_INBOX_DIAG_COUNTERS
#define HZ12_INBOX_DIAG_COUNTERS 1
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdatomic.h>
#include <string.h>

typedef struct H12OwnerInbox {
  CRITICAL_SECTION lock;
  void* head;
  uint32_t count;
  uint8_t adopted;
} H12OwnerInbox;

typedef struct H12InboxCounters {
  _Atomic uint64_t deferred_objects;
  _Atomic uint64_t route_batches;
  _Atomic uint64_t route_objects;
  _Atomic uint64_t fallback_unknown;
  _Atomic uint64_t fallback_overflow;
  _Atomic uint64_t fallback_adopted;
  _Atomic uint64_t owner_drain_batches;
  _Atomic uint64_t owner_drain_objects;
  _Atomic uint64_t inbox_current_max;
  _Atomic uint64_t retired_owner_marked;
  _Atomic uint64_t retired_owner_with_pending;
  _Atomic uint64_t retired_owner_pending_objects;
  _Atomic uint64_t adoption_shadow_scans;
  _Atomic uint64_t adoption_shadow_pending_owners;
  _Atomic uint64_t adoption_shadow_pending_objects;
  _Atomic uint64_t adoption_reject_active;
  _Atomic uint64_t adoption_reject_duplicate;
  _Atomic uint64_t adoption_batches;
  _Atomic uint64_t adoption_objects;
} H12InboxCounters;

static H12OwnerInbox h12_inboxes[HZ12_SHADOW_MAX_OWNERS];
static H12InboxCounters h12_inbox_counters;
static INIT_ONCE h12_inbox_once = INIT_ONCE_STATIC_INIT;
static uint32_t h12_inbox_owner_count;
static _Atomic uint8_t h12_inbox_owner_retired[HZ12_SHADOW_MAX_OWNERS];

static BOOL CALLBACK h12_inbox_init_once(PINIT_ONCE once, PVOID parameter,
                                         PVOID* context) {
  uint32_t i;
  (void)once;
  (void)parameter;
  (void)context;
  for (i = 0u; i < HZ12_SHADOW_MAX_OWNERS; ++i) {
    InitializeCriticalSection(&h12_inboxes[i].lock);
  }
  return TRUE;
}

#if HZ12_INBOX_DIAG_COUNTERS
static void h12_atomic_max(_Atomic uint64_t* destination, uint64_t value) {
  uint64_t current = atomic_load_explicit(destination, memory_order_relaxed);
  while (current < value &&
         !atomic_compare_exchange_weak_explicit(destination, &current, value,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
}
#endif

#if HZ12_INBOX_DIAG_COUNTERS
#define H12_INBOX_COUNTER_ADD(field, value)                               \
  atomic_fetch_add_explicit(&h12_inbox_counters.field, (value), memory_order_relaxed)
#define H12_INBOX_COUNTER_MAX(field, value) \
  h12_atomic_max(&h12_inbox_counters.field, (value))
#else
#define H12_INBOX_COUNTER_ADD(field, value) ((void)0)
#define H12_INBOX_COUNTER_MAX(field, value) ((void)0)
#endif

int h12_inbox_init(uint32_t owner_count) {
  if (owner_count == 0u || owner_count > HZ12_SHADOW_MAX_OWNERS) {
    return 0;
  }
  if (!InitOnceExecuteOnce(&h12_inbox_once, h12_inbox_init_once, NULL, NULL)) {
    return 0;
  }
  h12_inbox_owner_count = owner_count;
  atomic_store_explicit(&h12_inbox_counters.deferred_objects, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.route_batches, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.route_objects, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.fallback_unknown, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.fallback_overflow, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.fallback_adopted, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.owner_drain_batches, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.owner_drain_objects, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.inbox_current_max, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.retired_owner_marked, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.retired_owner_with_pending, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.retired_owner_pending_objects, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.adoption_shadow_scans, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.adoption_shadow_pending_owners, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.adoption_shadow_pending_objects, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.adoption_reject_active, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.adoption_reject_duplicate, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.adoption_batches, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_inbox_counters.adoption_objects, 0u,
                        memory_order_relaxed);
  for (uint32_t i = 0u; i < HZ12_SHADOW_MAX_OWNERS; ++i) {
    atomic_store_explicit(&h12_inbox_owner_retired[i], 0u,
                          memory_order_relaxed);
    h12_inboxes[i].head = NULL;
    h12_inboxes[i].count = 0u;
    h12_inboxes[i].adopted = 0u;
  }
  return 1;
}

void h12_inbox_deferred_init(H12InboxDeferred* deferred) {
  if (deferred) deferred->count = 0u;
}

static void h12_inbox_free_chain(void* head) {
  while (head) {
    void* next = *(void**)head;
#if HZ12_INBOX_ACCOUNTING
    h12_span_accounting_on_release(head);
#endif
    hz12_free(head);
    head = next;
  }
}

static void h12_inbox_publish(uint32_t owner_id, void* head, void* tail,
                               uint32_t count) {
  H12OwnerInbox* inbox;
  if (!head || count == 0u) return;
  if (owner_id >= h12_inbox_owner_count) {
    H12_INBOX_COUNTER_ADD(fallback_unknown, count);
    h12_inbox_free_chain(head);
    return;
  }
  inbox = &h12_inboxes[owner_id];
  EnterCriticalSection(&inbox->lock);
  if (inbox->adopted) {
    LeaveCriticalSection(&inbox->lock);
    H12_INBOX_COUNTER_ADD(fallback_adopted, count);
    h12_inbox_free_chain(head);
    return;
  }
  if (inbox->count + count > HZ12_INBOX_CAP) {
    LeaveCriticalSection(&inbox->lock);
    H12_INBOX_COUNTER_ADD(fallback_overflow, count);
    h12_inbox_free_chain(head);
    return;
  }
  *(void**)tail = inbox->head;
  inbox->head = head;
  inbox->count += count;
  H12_INBOX_COUNTER_MAX(inbox_current_max, inbox->count);
  LeaveCriticalSection(&inbox->lock);
  H12_INBOX_COUNTER_ADD(route_batches, 1u);
  H12_INBOX_COUNTER_ADD(route_objects, count);
}

void h12_inbox_flush(H12InboxDeferred* deferred) {
  void* heads[HZ12_SHADOW_MAX_OWNERS];
  void* tails[HZ12_SHADOW_MAX_OWNERS];
  uint32_t counts[HZ12_SHADOW_MAX_OWNERS];
  uint32_t i;
  if (!deferred || deferred->count == 0u) return;
  memset(heads, 0, sizeof(heads));
  memset(tails, 0, sizeof(tails));
  memset(counts, 0, sizeof(counts));
  for (i = 0u; i < deferred->count; ++i) {
    uint32_t owner_id;
    void* ptr = deferred->items[i];
    if (!h12_shadow_owner_for_ptr(ptr, &owner_id)) {
      H12_INBOX_COUNTER_ADD(fallback_unknown, 1u);
      hz12_free(ptr);
      continue;
    }
    *(void**)ptr = heads[owner_id];
    if (!tails[owner_id]) tails[owner_id] = ptr;
    heads[owner_id] = ptr;
    counts[owner_id] += 1u;
  }
  for (i = 0u; i < h12_inbox_owner_count; ++i) {
    h12_inbox_publish(i, heads[i], tails[i], counts[i]);
  }
  deferred->count = 0u;
}

void h12_inbox_defer_free(H12InboxDeferred* deferred, void* ptr) {
  if (!deferred || !ptr) return;
  deferred->items[deferred->count++] = ptr;
  H12_INBOX_COUNTER_ADD(deferred_objects, 1u);
  if (deferred->count == HZ12_SHADOW_FLUSH_CAP) {
    h12_inbox_flush(deferred);
  }
}

uint32_t h12_inbox_drain_owner(uint32_t owner_id) {
  H12OwnerInbox* inbox;
  void* head;
  uint32_t count;
  if (owner_id >= h12_inbox_owner_count) return 0u;
  inbox = &h12_inboxes[owner_id];
  EnterCriticalSection(&inbox->lock);
  head = inbox->head;
  count = inbox->count;
  inbox->head = NULL;
  inbox->count = 0u;
  LeaveCriticalSection(&inbox->lock);
  if (head) {
    h12_inbox_free_chain(head);
    H12_INBOX_COUNTER_ADD(owner_drain_batches, 1u);
    H12_INBOX_COUNTER_ADD(owner_drain_objects, count);
  }
  return count;
}

void h12_inbox_mark_owner_retired(uint32_t owner_id) {
  H12OwnerInbox* inbox;
  uint32_t pending;
  if (owner_id >= h12_inbox_owner_count) return;
  inbox = &h12_inboxes[owner_id];
  atomic_store_explicit(&h12_inbox_owner_retired[owner_id], 1u,
                        memory_order_relaxed);
  H12_INBOX_COUNTER_ADD(retired_owner_marked, 1u);
  EnterCriticalSection(&inbox->lock);
  pending = inbox->count;
  LeaveCriticalSection(&inbox->lock);
  if (pending != 0u) {
    H12_INBOX_COUNTER_ADD(retired_owner_with_pending, 1u);
    H12_INBOX_COUNTER_ADD(retired_owner_pending_objects, pending);
  }
}

H12AdoptionShadow h12_inbox_adoption_shadow_scan(void) {
  H12AdoptionShadow result = {0u, 0u, 0u};
  uint32_t i;
  for (i = 0u; i < h12_inbox_owner_count; ++i) {
    H12OwnerInbox* inbox;
    uint32_t pending;
    if (atomic_load_explicit(&h12_inbox_owner_retired[i],
                             memory_order_relaxed) == 0u) {
      continue;
    }
    result.retired_owners += 1u;
    inbox = &h12_inboxes[i];
    EnterCriticalSection(&inbox->lock);
    pending = inbox->count;
    LeaveCriticalSection(&inbox->lock);
    if (pending != 0u) {
      result.pending_owners += 1u;
      result.pending_objects += pending;
    }
  }
  H12_INBOX_COUNTER_ADD(adoption_shadow_scans, 1u);
  H12_INBOX_COUNTER_ADD(adoption_shadow_pending_owners, result.pending_owners);
  H12_INBOX_COUNTER_ADD(adoption_shadow_pending_objects, result.pending_objects);
  return result;
}

uint32_t h12_inbox_adopt_retired_owner(uint32_t owner_id) {
  H12OwnerInbox* inbox;
  void* head;
  uint32_t count;
  if (owner_id >= h12_inbox_owner_count) return 0u;
  if (atomic_load_explicit(&h12_inbox_owner_retired[owner_id],
                           memory_order_relaxed) == 0u) {
    H12_INBOX_COUNTER_ADD(adoption_reject_active, 1u);
    return 0u;
  }
  inbox = &h12_inboxes[owner_id];
  EnterCriticalSection(&inbox->lock);
  if (inbox->adopted) {
    LeaveCriticalSection(&inbox->lock);
    H12_INBOX_COUNTER_ADD(adoption_reject_duplicate, 1u);
    return 0u;
  }
  inbox->adopted = 1u;
  head = inbox->head;
  count = inbox->count;
  inbox->head = NULL;
  inbox->count = 0u;
  LeaveCriticalSection(&inbox->lock);
  H12_INBOX_COUNTER_ADD(adoption_batches, 1u);
  H12_INBOX_COUNTER_ADD(adoption_objects, count);
  h12_inbox_free_chain(head);
  return count;
}

void h12_inbox_dump(FILE* out) {
  if (!out) return;
  fprintf(out,
          "[HZ12_OWNER_INBOX] deferred_objects=%llu route_batches=%llu "
          "route_objects=%llu fallback_unknown=%llu fallback_overflow=%llu "
          "fallback_adopted=%llu "
          "owner_drain_batches=%llu owner_drain_objects=%llu "
          "inbox_current_max=%llu retired_owner_marked=%llu "
          "retired_owner_with_pending=%llu retired_owner_pending_objects=%llu "
          "adoption_shadow_scans=%llu adoption_shadow_pending_owners=%llu "
          "adoption_shadow_pending_objects=%llu adoption_reject_active=%llu "
          "adoption_reject_duplicate=%llu adoption_batches=%llu "
          "adoption_objects=%llu\n",
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.deferred_objects,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.route_batches,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.route_objects,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.fallback_unknown,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.fallback_overflow,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.fallback_adopted,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.owner_drain_batches,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.owner_drain_objects,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.inbox_current_max,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.retired_owner_marked,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.retired_owner_with_pending,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.retired_owner_pending_objects,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.adoption_shadow_scans,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.adoption_shadow_pending_owners,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.adoption_shadow_pending_objects,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.adoption_reject_active,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.adoption_reject_duplicate,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.adoption_batches,
                                                    memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&h12_inbox_counters.adoption_objects,
                                                    memory_order_relaxed));
}
