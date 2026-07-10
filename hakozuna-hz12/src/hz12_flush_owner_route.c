#include "hz12_flush_owner_route.h"

#include "hz12_shadow.h"
#include "hz12_span.h"
#include "hz12_thread_cache.h"

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
  _Atomic uint8_t pending;
} H12FlushOwnerInbox;

static pthread_once_t hz12_flush_owner_once = PTHREAD_ONCE_INIT;
static _Atomic uint32_t hz12_flush_owner_next;
static H12FlushOwnerInbox
    hz12_flush_owner_inboxes[HZ12_SHADOW_MAX_OWNERS];

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
                                     void* head, void* tail, uint32_t count) {
  H12FlushOwnerInbox* inbox;
  if (owner_id >= HZ12_SHADOW_MAX_OWNERS || class_id >= HZ12_CLASS_COUNT ||
      !head || !tail || count == 0u) {
    hz12_flush_owner_return_chain(class_id, head);
    return;
  }
  inbox = &hz12_flush_owner_inboxes[owner_id];
  EnterCriticalSection(&inbox->lock);
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
  owner_id = atomic_fetch_add_explicit(&hz12_flush_owner_next, 1u,
                                       memory_order_relaxed);
  if (owner_id >= HZ12_SHADOW_MAX_OWNERS) return;
  tc->flush_owner_id = owner_id;
  tc->flush_owner_valid = 1u;
}

void hz12_flush_owner_route_batch(H12ThreadCache* tc, uint8_t class_id,
                                  void** items, uint32_t count) {
  void* heads[HZ12_SHADOW_MAX_OWNERS];
  void* tails[HZ12_SHADOW_MAX_OWNERS];
  uint32_t counts[HZ12_SHADOW_MAX_OWNERS];
  void* local[HZ12_CACHE_CAP];
  uint32_t local_count = 0u;

  if (!tc || !items || count == 0u || class_id >= HZ12_CLASS_COUNT) return;

  if (tc->flush_owner_valid &&
      h12_shadow_batch_all_owner(items, count, tc->flush_owner_id)) {
    hz12_returned_push_range(class_id, items, count);
    return;
  }

  memset(heads, 0, sizeof(heads));
  memset(tails, 0, sizeof(tails));
  memset(counts, 0, sizeof(counts));

  for (uint32_t i = 0u; i < count; ++i) {
    void* ptr = items[i];
    uint32_t owner_id;
    if (!ptr) continue;
    if (!hz12_arena_contains(ptr) ||
        !h12_shadow_owner_for_ptr(ptr, &owner_id) ||
        !tc->flush_owner_valid || owner_id == tc->flush_owner_id) {
      local[local_count++] = ptr;
      continue;
    }
    *(void**)ptr = heads[owner_id];
    if (!tails[owner_id]) tails[owner_id] = ptr;
    heads[owner_id] = ptr;
    counts[owner_id] += 1u;
  }

  if (local_count != 0u) {
    hz12_returned_push_range(class_id, local, local_count);
  }
  for (uint32_t owner_id = 0u; owner_id < HZ12_SHADOW_MAX_OWNERS;
       ++owner_id) {
    if (counts[owner_id] != 0u) {
      hz12_flush_owner_publish(owner_id, class_id, heads[owner_id],
                               tails[owner_id], counts[owner_id]);
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
  memcpy(heads, inbox->heads, sizeof(heads));
  memset(inbox->heads, 0, sizeof(inbox->heads));
  memset(inbox->tails, 0, sizeof(inbox->tails));
  memset(inbox->counts, 0, sizeof(inbox->counts));
  inbox->total_count = 0u;
  atomic_store_explicit(&inbox->pending, 0u, memory_order_release);
  LeaveCriticalSection(&inbox->lock);

  for (uint32_t class_id = 0u; class_id < HZ12_CLASS_COUNT; ++class_id) {
    void* head = heads[class_id];
    while (head) {
      void* next = *(void**)head;
      hz12_thread_cache_push(tc, (uint8_t)class_id, head);
      head = next;
    }
  }
}
