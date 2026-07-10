#include "hz12_flush_owner_route.h"

#include "hz12_shadow.h"
#include "hz12_span.h"
#include "hz12_thread_cache.h"
#if HZ12_OWNER_BATCH_LEDGER_DIAG
#include "hz12_owner_batch_ledger.h"
#include "hz12_span_owner_shadow.h"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdatomic.h>
#include <string.h>

#ifndef HZ12_FLUSH_OWNER_INBOX_CAP
#define HZ12_FLUSH_OWNER_INBOX_CAP 1024u
#endif

typedef struct H12FlushOwnerInbox {
  CRITICAL_SECTION lock;
  void* heads[HZ12_CLASS_COUNT];
  void* tails[HZ12_CLASS_COUNT];
  uint32_t counts[HZ12_CLASS_COUNT];
  uint32_t total_count;
  _Atomic uint32_t generation;
  _Atomic uint8_t active;
  _Atomic uint8_t pending;
} H12FlushOwnerInbox;

typedef struct H12FlushOwnerRouteAtomicStats {
  _Atomic uint64_t attach_success;
  _Atomic uint64_t attach_reuse;
  _Atomic uint64_t attach_full;
  _Atomic uint64_t detach_success;
  _Atomic uint64_t stale_fallback;
} H12FlushOwnerRouteAtomicStats;

static pthread_once_t hz12_flush_owner_once = PTHREAD_ONCE_INIT;
#if !HZ12_FLUSH_OWNER_COLD_SPAN
static _Atomic uint32_t hz12_flush_owner_next;
#endif
static H12FlushOwnerInbox
    hz12_flush_owner_inboxes[HZ12_SHADOW_MAX_OWNERS];
static H12FlushOwnerRouteAtomicStats hz12_flush_owner_stats;

#if HZ12_OWNER_BATCH_LEDGER_DIAG
static H12OwnerToken hz12_flush_owner_token(const H12ThreadCache* tc) {
  H12OwnerToken owner = {0u, 0u};
  if (tc && tc->flush_owner_valid) {
    owner.slot = tc->flush_owner_id;
    owner.generation = tc->flush_owner_generation;
  }
  return owner;
}

static void hz12_flush_owner_ledger_return_local(H12ThreadCache* tc,
                                                 void* const* items,
                                                 uint32_t count) {
  H12OwnerToken owner = hz12_flush_owner_token(tc);
  void* local[HZ12_CACHE_CAP];
  uint32_t local_count = 0u;
  if (owner.generation == 0u) return;
  for (uint32_t i = 0u; items && i < count; ++i) {
    uint32_t slot;
    uint32_t generation;
    if (h12_shadow_owner_token_for_ptr(items[i], &slot, &generation) &&
        slot == owner.slot && generation == owner.generation) {
      local[local_count++] = items[i];
    }
  }
  if (local_count != 0u) {
    (void)h12_owner_batch_ledger_return_range(owner, local, local_count);
  }
}

static void hz12_flush_owner_ledger_drain_chain(H12ThreadCache* tc,
                                                void* head) {
  H12OwnerToken owner = hz12_flush_owner_token(tc);
  void* items[HZ12_CACHE_CAP];
  if (owner.generation == 0u) return;
  while (head) {
    uint32_t count = 0u;
    while (head && count < HZ12_CACHE_CAP) {
      items[count++] = head;
      head = *(void**)head;
    }
    (void)h12_owner_batch_ledger_owner_drain_range(owner, items, count);
  }
}
#else
#define hz12_flush_owner_ledger_return_local(tc, items, count) ((void)0)
#define hz12_flush_owner_ledger_drain_chain(tc, head) ((void)0)
#endif

static void hz12_flush_owner_init_once(void) {
  (void)h12_shadow_init(HZ12_SHADOW_MAX_OWNERS);
  for (uint32_t i = 0u; i < HZ12_SHADOW_MAX_OWNERS; ++i) {
    InitializeCriticalSection(&hz12_flush_owner_inboxes[i].lock);
  }
}

static void hz12_flush_owner_return_chain(uint8_t class_id, void* head) {
  void* items[HZ12_CACHE_CAP];
  while (head) {
    uint32_t count = 0u;
    while (head && count < HZ12_CACHE_CAP) {
      void* next = *(void**)head;
      items[count++] = head;
      head = next;
    }
    hz12_returned_push_range(class_id, items, count);
  }
}

static void hz12_flush_owner_publish(uint32_t owner_id, uint8_t class_id,
                                     uint32_t generation, void* head,
                                     void* tail, uint32_t count) {
  H12FlushOwnerInbox* inbox;
  if (owner_id >= HZ12_SHADOW_MAX_OWNERS || class_id >= HZ12_CLASS_COUNT ||
      !head || !tail || count == 0u) {
    hz12_flush_owner_return_chain(class_id, head);
    return;
  }
  inbox = &hz12_flush_owner_inboxes[owner_id];
  EnterCriticalSection(&inbox->lock);
  if (!atomic_load_explicit(&inbox->active, memory_order_relaxed) ||
      atomic_load_explicit(&inbox->generation, memory_order_relaxed) !=
          generation) {
    LeaveCriticalSection(&inbox->lock);
    atomic_fetch_add_explicit(&hz12_flush_owner_stats.stale_fallback, count,
                              memory_order_relaxed);
    hz12_flush_owner_return_chain(class_id, head);
    return;
  }
  if (inbox->total_count + count > HZ12_FLUSH_OWNER_INBOX_CAP) {
    LeaveCriticalSection(&inbox->lock);
    hz12_flush_owner_return_chain(class_id, head);
    return;
  }
  *(void**)tail = inbox->heads[class_id];
  inbox->heads[class_id] = head;
  if (!inbox->tails[class_id]) inbox->tails[class_id] = tail;
  inbox->counts[class_id] += count;
  inbox->total_count += count;
  atomic_store_explicit(&inbox->pending, 1u, memory_order_release);
  LeaveCriticalSection(&inbox->lock);
}

void hz12_flush_owner_route_attach(H12ThreadCache* tc) {
  uint32_t owner_id;
  if (!tc || tc->flush_owner_valid) return;
  (void)pthread_once(&hz12_flush_owner_once, hz12_flush_owner_init_once);
#if HZ12_FLUSH_OWNER_COLD_SPAN
  for (owner_id = 0u; owner_id < HZ12_SHADOW_MAX_OWNERS; ++owner_id) {
    H12FlushOwnerInbox* inbox = &hz12_flush_owner_inboxes[owner_id];
    EnterCriticalSection(&inbox->lock);
    if (!atomic_load_explicit(&inbox->active, memory_order_relaxed) &&
        inbox->total_count == 0u) {
      uint32_t generation = atomic_load_explicit(&inbox->generation,
                                                  memory_order_relaxed) + 1u;
      if (generation == 0u) generation = 1u;
      atomic_store_explicit(&inbox->generation, generation,
                            memory_order_relaxed);
      atomic_store_explicit(&inbox->active, 1u, memory_order_release);
      tc->flush_owner_id = owner_id;
      tc->flush_owner_generation = generation;
      tc->flush_owner_valid = 1u;
      atomic_fetch_add_explicit(&hz12_flush_owner_stats.attach_success, 1u,
                                memory_order_relaxed);
      if (generation > 1u) {
        atomic_fetch_add_explicit(&hz12_flush_owner_stats.attach_reuse, 1u,
                                  memory_order_relaxed);
      }
      LeaveCriticalSection(&inbox->lock);
      return;
    }
    LeaveCriticalSection(&inbox->lock);
  }
  atomic_fetch_add_explicit(&hz12_flush_owner_stats.attach_full, 1u,
                            memory_order_relaxed);
  return;
#else
  owner_id = atomic_fetch_add_explicit(&hz12_flush_owner_next, 1u,
                                       memory_order_relaxed);
  if (owner_id >= HZ12_SHADOW_MAX_OWNERS) return;
  tc->flush_owner_id = owner_id;
  tc->flush_owner_generation = 1u;
  tc->flush_owner_valid = 1u;
#endif
}

void hz12_flush_owner_route_assign_span(H12ThreadCache* tc, void* span_base) {
  if (!tc || !tc->flush_owner_valid || !span_base) return;
  h12_shadow_on_alloc_token(span_base, tc->flush_owner_id,
                            tc->flush_owner_generation);
#if HZ12_OWNER_BATCH_LEDGER_DIAG
  {
    H12OwnerToken owner = hz12_flush_owner_token(tc);
    (void)h12_span_owner_shadow_assign(span_base, owner);
  }
#endif
}

void hz12_flush_owner_route_detach(H12ThreadCache* tc) {
#if HZ12_FLUSH_OWNER_COLD_SPAN
  H12FlushOwnerInbox* inbox;
  void* heads[HZ12_CLASS_COUNT];
  if (!tc || !tc->flush_owner_valid ||
      tc->flush_owner_id >= HZ12_SHADOW_MAX_OWNERS) return;
  inbox = &hz12_flush_owner_inboxes[tc->flush_owner_id];
  EnterCriticalSection(&inbox->lock);
  if (!atomic_load_explicit(&inbox->active, memory_order_relaxed) ||
      atomic_load_explicit(&inbox->generation, memory_order_relaxed) !=
          tc->flush_owner_generation) {
    LeaveCriticalSection(&inbox->lock);
    tc->flush_owner_valid = 0u;
    return;
  }
  memcpy(heads, inbox->heads, sizeof(heads));
  memset(inbox->heads, 0, sizeof(inbox->heads));
  memset(inbox->tails, 0, sizeof(inbox->tails));
  memset(inbox->counts, 0, sizeof(inbox->counts));
  inbox->total_count = 0u;
  atomic_store_explicit(&inbox->pending, 0u, memory_order_relaxed);
  atomic_store_explicit(&inbox->active, 0u, memory_order_release);
  LeaveCriticalSection(&inbox->lock);
  for (uint32_t class_id = 0u; class_id < HZ12_CLASS_COUNT; ++class_id) {
    hz12_flush_owner_return_chain((uint8_t)class_id, heads[class_id]);
  }
  tc->flush_owner_valid = 0u;
  atomic_fetch_add_explicit(&hz12_flush_owner_stats.detach_success, 1u,
                            memory_order_relaxed);
#else
  (void)tc;
#endif
}

void hz12_flush_owner_route_batch(H12ThreadCache* tc, uint8_t class_id,
                                  void** items, uint32_t count) {
  void* heads[HZ12_SHADOW_MAX_OWNERS];
  void* tails[HZ12_SHADOW_MAX_OWNERS];
  uint32_t counts[HZ12_SHADOW_MAX_OWNERS];
  uint32_t generations[HZ12_SHADOW_MAX_OWNERS];
  void* local[HZ12_CACHE_CAP];
  uint32_t local_count = 0u;

  if (!tc || !items || count == 0u || class_id >= HZ12_CLASS_COUNT) return;

  if (tc->flush_owner_valid &&
      h12_shadow_batch_all_owner(items, count, tc->flush_owner_id)) {
    hz12_flush_owner_ledger_return_local(tc, items, count);
    hz12_returned_push_range(class_id, items, count);
    return;
  }

  memset(heads, 0, sizeof(heads));
  memset(tails, 0, sizeof(tails));
  memset(counts, 0, sizeof(counts));
  memset(generations, 0, sizeof(generations));

  for (uint32_t i = 0u; i < count; ++i) {
    void* ptr = items[i];
    uint32_t owner_id;
    uint32_t generation;
    if (!ptr) continue;
    if (!hz12_arena_contains(ptr) ||
        !h12_shadow_owner_token_for_ptr(ptr, &owner_id, &generation) ||
        !tc->flush_owner_valid ||
        (owner_id == tc->flush_owner_id &&
         generation == tc->flush_owner_generation) ||
        !atomic_load_explicit(&hz12_flush_owner_inboxes[owner_id].active,
                              memory_order_acquire) ||
        atomic_load_explicit(&hz12_flush_owner_inboxes[owner_id].generation,
                             memory_order_relaxed) != generation ||
        (generations[owner_id] != 0u &&
         generations[owner_id] != generation)) {
      local[local_count++] = ptr;
      continue;
    }
    generations[owner_id] = generation;
    *(void**)ptr = heads[owner_id];
    if (!tails[owner_id]) tails[owner_id] = ptr;
    heads[owner_id] = ptr;
    counts[owner_id] += 1u;
  }

  if (local_count != 0u) {
    hz12_flush_owner_ledger_return_local(tc, local, local_count);
    hz12_returned_push_range(class_id, local, local_count);
  }
  for (uint32_t owner_id = 0u; owner_id < HZ12_SHADOW_MAX_OWNERS;
       ++owner_id) {
    if (counts[owner_id] != 0u) {
      hz12_flush_owner_publish(owner_id, class_id, generations[owner_id],
                               heads[owner_id], tails[owner_id],
                               counts[owner_id]);
    }
  }
}

void hz12_flush_owner_route_drain(H12ThreadCache* tc) {
  H12FlushOwnerInbox* inbox;
  void* heads[HZ12_CLASS_COUNT];
  if (!tc || !tc->flush_owner_valid) return;
  inbox = &hz12_flush_owner_inboxes[tc->flush_owner_id];
  if (!atomic_load_explicit(&inbox->pending, memory_order_acquire)) return;
  EnterCriticalSection(&inbox->lock);
  if (!atomic_load_explicit(&inbox->active, memory_order_relaxed) ||
      atomic_load_explicit(&inbox->generation, memory_order_relaxed) !=
          tc->flush_owner_generation) {
    LeaveCriticalSection(&inbox->lock);
    return;
  }
  memcpy(heads, inbox->heads, sizeof(heads));
  memset(inbox->heads, 0, sizeof(inbox->heads));
  memset(inbox->tails, 0, sizeof(inbox->tails));
  memset(inbox->counts, 0, sizeof(inbox->counts));
  inbox->total_count = 0u;
  atomic_store_explicit(&inbox->pending, 0u, memory_order_release);
  LeaveCriticalSection(&inbox->lock);

  for (uint32_t class_id = 0u; class_id < HZ12_CLASS_COUNT; ++class_id) {
    void* head = heads[class_id];
    hz12_flush_owner_ledger_drain_chain(tc, head);
    while (head) {
      void* next = *(void**)head;
      hz12_thread_cache_push(tc, (uint8_t)class_id, head);
      head = next;
    }
  }
}

void hz12_flush_owner_route_stats(H12FlushOwnerRouteStats* out) {
  if (!out) return;
  out->attach_success = atomic_load_explicit(
      &hz12_flush_owner_stats.attach_success, memory_order_relaxed);
  out->attach_reuse = atomic_load_explicit(
      &hz12_flush_owner_stats.attach_reuse, memory_order_relaxed);
  out->attach_full = atomic_load_explicit(
      &hz12_flush_owner_stats.attach_full, memory_order_relaxed);
  out->detach_success = atomic_load_explicit(
      &hz12_flush_owner_stats.detach_success, memory_order_relaxed);
  out->stale_fallback = atomic_load_explicit(
      &hz12_flush_owner_stats.stale_fallback, memory_order_relaxed);
}

int hz12_flush_owner_route_pending(uint32_t owner_id, uint32_t generation,
                                   uint32_t* out_pending) {
  H12FlushOwnerInbox* inbox;
  int matched;
  if (!out_pending || owner_id >= HZ12_SHADOW_MAX_OWNERS || generation == 0u) {
    return 0;
  }
  (void)pthread_once(&hz12_flush_owner_once, hz12_flush_owner_init_once);
  inbox = &hz12_flush_owner_inboxes[owner_id];
  EnterCriticalSection(&inbox->lock);
  matched = atomic_load_explicit(&inbox->generation, memory_order_relaxed) ==
            generation;
  *out_pending = matched ? inbox->total_count : 0u;
  LeaveCriticalSection(&inbox->lock);
  return matched;
}

void* hz12_flush_owner_route_drain_for_class(H12ThreadCache* tc,
                                              uint8_t class_id) {
  if (!tc || class_id >= HZ12_CLASS_COUNT) return NULL;
  hz12_flush_owner_route_drain(tc);
  return hz12_thread_cache_pop(tc, class_id);
}
