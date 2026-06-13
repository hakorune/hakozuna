#include "hz5_largefront.h"

#include "hz5_config.h"
#include "hz5_internal.h"
#include "hz5_midpagefront.h"
#include "hz5_ownerhub.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "hz5_largefront_config.inc"

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LARGEFRONT_L1

#define HZ5_LARGEFRONT_MAGIC UINT64_C(0x485A354C41524731)
#define HZ5_LARGEFRONT_SPAN_PAYLOAD_SCAVENGED 0x0001u
#define HZ5_LARGEFRONT_PAGE_SIZE ((size_t)4096)
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES
#define HZ5_LARGEFRONT_CLASS_COUNT 8u
#else
#define HZ5_LARGEFRONT_CLASS_COUNT 4u
#endif

#ifndef HZ5_LARGEFRONT_MAP_BITS
#define HZ5_LARGEFRONT_MAP_BITS 22u
#endif

#define HZ5_LARGEFRONT_MAP_CAP ((size_t)1u << HZ5_LARGEFRONT_MAP_BITS)

#ifndef HZ5_LARGEFRONT_REGION_BUCKET_BITS
#define HZ5_LARGEFRONT_REGION_BUCKET_BITS 16u
#endif

#define HZ5_LARGEFRONT_REGION_BUCKET_CAP \
  ((size_t)1u << HZ5_LARGEFRONT_REGION_BUCKET_BITS)

#ifndef HZ5_LARGEFRONT_REGION_GRAN_BITS
#define HZ5_LARGEFRONT_REGION_GRAN_BITS 21u
#endif

#ifndef HZ5_LARGEFRONT_REGION_CAP
#define HZ5_LARGEFRONT_REGION_CAP 65536u
#endif

#ifndef HZ5_LARGEFRONT_REGION_LINK_CAP
#define HZ5_LARGEFRONT_REGION_LINK_CAP 262144u
#endif

#ifndef HZ5_LARGEFRONT_SOURCE_BATCH_COUNT
#define HZ5_LARGEFRONT_SOURCE_BATCH_COUNT 16u
#endif

#include "hz5_largefront_state.inc"

static Hz5LargeTls* hz5_largefront_tls(void) {
  Hz5LargeTls* tls = &g_hz5_largefront_tls;
  if (tls->owner.slot == 0) {
    tls->owner = hz5_owner_current();
  }
  if (tls->remote_batch_cap == 0u) {
    tls->remote_batch_cap = HZ5_LARGEFRONT_REMOTE_BATCH_CAP;
  }
  return tls;
}

static int hz5_largefront_class_valid(uint32_t class_index) {
  return class_index < HZ5_LARGEFRONT_CLASS_COUNT;
}

static uint32_t hz5_largefront_class_bytes(uint32_t class_index) {
  return g_hz5_largefront_classes[class_index];
}

static int hz5_largefront_class_index(size_t size) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES
  if (size <= 4096u || size > 1048576u) {
    return -1;
  }
#else
  if (size <= 65536u || size > 1048576u) {
    return -1;
  }
#endif
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_CLASS_COUNT; ++i) {
    if (size <= g_hz5_largefront_classes[i]) {
      return (int)i;
    }
  }
  return -1;
}

static size_t hz5_largefront_span_stride(uint32_t class_index) {
  return HZ5_LARGEFRONT_PAGE_SIZE +
         (size_t)hz5_largefront_class_bytes(class_index);
}

#include "hz5_largefront_policy.inc"

#include "hz5_largefront_source.inc"

static void hz5_largefront_local_push(Hz5LargeTls* tls,
                                      uint32_t class_index,
                                      Hz5LargeSpan* span) {
  span->next = tls->free_head[class_index];
  tls->free_head[class_index] = span;
  ++tls->free_count[class_index];
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  hz5_largefront_obs_max(&g_hz5_largefront_obs_local_free_highwater[class_index],
                         tls->free_count[class_index]);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
  if (hz5_largefront_payload_scavenge_class(class_index) &&
      tls->free_count[class_index] > HZ5_LARGEFRONT_SCAVENGE_LOCAL_CAP) {
    hz5_largefront_payload_scavenge(span);
  }
#endif
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_BULK_LOCAL
static void hz5_largefront_local_push_list(Hz5LargeTls* tls,
                                           uint32_t class_index,
                                           Hz5LargeSpan* head,
                                           Hz5LargeSpan* tail,
                                           uint32_t count) {
  if (!head || count == 0u) {
    return;
  }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
  Hz5LargeSpan* span = head;
  while (span) {
    Hz5LargeSpan* next = span->next;
    hz5_largefront_local_push(tls, class_index, span);
    span = next;
  }
#else
  if (!tail) {
    tail = head;
    while (tail->next) {
      tail = tail->next;
    }
  }
  tail->next = tls->free_head[class_index];
  tls->free_head[class_index] = head;
  tls->free_count[class_index] += count;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  hz5_largefront_obs_max(&g_hz5_largefront_obs_local_free_highwater[class_index],
                         tls->free_count[class_index]);
#endif
#endif
}
#endif

static Hz5LargeSpan* hz5_largefront_local_pop(Hz5LargeTls* tls,
                                              uint32_t class_index) {
  Hz5LargeSpan* span = tls->free_head[class_index];
  if (!span) {
    return NULL;
  }
  tls->free_head[class_index] = span->next;
  if (tls->free_count[class_index] != 0u) {
    --tls->free_count[class_index];
  }
  span->next = NULL;
  return span;
}

static void hz5_largefront_global_push(uint32_t class_index,
                                       Hz5LargeSpan* span) {
  if (!hz5_largefront_class_valid(class_index) || !span) {
    return;
  }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
  if (hz5_largefront_payload_scavenge_class(class_index)) {
    hz5_largefront_payload_scavenge(span);
  }
#endif
  pthread_mutex_lock(&g_hz5_largefront_global_lock);
  span->next = g_hz5_largefront_global_free[class_index];
  g_hz5_largefront_global_free[class_index] = span;
  pthread_mutex_unlock(&g_hz5_largefront_global_lock);
}

static Hz5LargeSpan* hz5_largefront_global_pop(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_largefront_global_lock);
  Hz5LargeSpan* span = g_hz5_largefront_global_free[class_index];
  if (span) {
    g_hz5_largefront_global_free[class_index] = span->next;
    span->next = NULL;
  }
  pthread_mutex_unlock(&g_hz5_largefront_global_lock);
  return span;
}

#include "hz5_largefront_transfer128.inc"

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_HOLD
static int hz5_largefront_remote_hold_push(Hz5LargeTls* tls,
                                           uint32_t class_index,
                                           Hz5LargeSpan* span) {
  if (!tls || !span || class_index >= HZ5_LARGEFRONT_CLASS_COUNT ||
      HZ5_LARGEFRONT_REMOTE_HOLD_CAP == 0u ||
      tls->remote_hold_count[class_index] >=
          HZ5_LARGEFRONT_REMOTE_HOLD_CAP) {
    return 0;
  }
  span->next = NULL;
  if (tls->remote_hold_tail[class_index]) {
    tls->remote_hold_tail[class_index]->next = span;
  } else {
    tls->remote_hold_head[class_index] = span;
  }
  tls->remote_hold_tail[class_index] = span;
  ++tls->remote_hold_count[class_index];
  return 1;
}

static Hz5LargeSpan* hz5_largefront_remote_hold_drain_budget(
    Hz5LargeTls* tls,
    uint32_t class_index,
    uint32_t budget,
    uint32_t* drained_out) {
  uint32_t drained = 0;
  Hz5LargeSpan* taken = NULL;
  if (!tls || class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }
  Hz5LargeSpan* span = tls->remote_hold_head[class_index];
  while (span && (!taken || (budget != 0u && drained < budget))) {
    tls->remote_hold_head[class_index] = span->next;
    if (!tls->remote_hold_head[class_index]) {
      tls->remote_hold_tail[class_index] = NULL;
    }
    if (tls->remote_hold_count[class_index] != 0u) {
      --tls->remote_hold_count[class_index];
    }
    span->next = NULL;

    if (!hz5_owner_equal(span->owner, tls->owner)) {
      hz5_largefront_mark_orphan(span);
      span = tls->remote_hold_head[class_index];
      continue;
    }
    if (!taken &&
        hz5_largefront_state_cas(span,
                                 (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                 (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
      hz5_largefront_payload_reactivate(span);
      taken = span;
      span = tls->remote_hold_head[class_index];
      continue;
    }
    if (budget != 0u && drained < budget &&
        hz5_largefront_remote_pending_to_local(span)) {
      hz5_largefront_local_push(tls, class_index, span);
      ++drained;
      span = tls->remote_hold_head[class_index];
      continue;
    }
    hz5_largefront_mark_orphan(span);
    span = tls->remote_hold_head[class_index];
  }
  if (drained_out) {
    *drained_out = drained;
  }
  return taken;
}
#endif

static int hz5_largefront_state_cas(Hz5LargeSpan* span,
                                    unsigned char from,
                                    unsigned char to) {
  unsigned char expected = from;
  return atomic_compare_exchange_strong_explicit(&span->state,
                                                 &expected,
                                                 to,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire);
}

static int hz5_largefront_remote_pending_to_local(Hz5LargeSpan* span) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TRUST_REMOTE_STATE
  if (atomic_load_explicit(&span->state, memory_order_acquire) !=
      (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING) {
    return 0;
  }
  atomic_store_explicit(&span->state,
                        (unsigned char)HZ5_LARGESPAN_LOCAL_FREE,
                        memory_order_release);
  return 1;
#else
  return hz5_largefront_state_cas(
      span,
      (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
      (unsigned char)HZ5_LARGESPAN_LOCAL_FREE);
#endif
}

static int hz5_largefront_owner_local_state_transition(Hz5LargeSpan* span,
                                                       unsigned char from,
                                                       unsigned char to) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_FAST_STATE
  if (atomic_load_explicit(&span->state, memory_order_acquire) != from) {
    return 0;
  }
  atomic_store_explicit(&span->state, to, memory_order_release);
  return 1;
#else
  return hz5_largefront_state_cas(span, from, to);
#endif
}

static int hz5_largefront_activate_local_for_owner(Hz5LargeTls* tls,
                                                   Hz5LargeSpan* span) {
  if (!hz5_largefront_owner_local_state_transition(
          span,
          (unsigned char)HZ5_LARGESPAN_LOCAL_FREE,
          (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
    return 0;
  }
  span->owner = tls->owner;
  hz5_largefront_payload_reactivate(span);
  return 1;
}

static int hz5_largefront_activate_global_for_owner(Hz5LargeTls* tls,
                                                    Hz5LargeSpan* span) {
  if (!hz5_largefront_state_cas(span,
                                (unsigned char)HZ5_LARGESPAN_LOCAL_FREE,
                                (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
    return 0;
  }
  span->owner = tls->owner;
  hz5_largefront_payload_reactivate(span);
  return 1;
}

static void hz5_largefront_mark_orphan(Hz5LargeSpan* span) {
  if (!span) {
    return;
  }
  atomic_store_explicit(&span->state,
                        (unsigned char)HZ5_LARGESPAN_ORPHAN,
                        memory_order_release);
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
static int hz5_largefront_can_publish_to_owner(Hz5OwnerToken owner,
                                               uint32_t class_index,
                                               const Hz5LargeSpan* span) {
  return owner.slot != 0 && hz5_largefront_class_valid(class_index) && span &&
         hz5_owner_is_alive(owner);
}

static int hz5_largefront_remote_publish_one(Hz5OwnerToken owner,
                                             uint32_t class_index,
                                             Hz5LargeSpan* span) {
  if (!hz5_largefront_can_publish_to_owner(owner, class_index, span)) {
    return 0;
  }

  void* old_head = NULL;
  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    span->next = (Hz5LargeSpan*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, span, memory_order_release, memory_order_acquire));
  hz5_ownerhub_mark_pending(owner, HZ5_OWNERHUB_FRONT_LARGE, class_index);
  return 1;
}

static int hz5_largefront_remote_publish_list(Hz5OwnerToken owner,
                                              uint32_t class_index,
                                              Hz5LargeSpan* head,
                                              Hz5LargeSpan* tail) {
  if (!hz5_largefront_can_publish_to_owner(owner, class_index, head) || !tail) {
    return 0;
  }

  void* old_head = NULL;
  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    tail->next = (Hz5LargeSpan*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, head, memory_order_release, memory_order_acquire));
  hz5_ownerhub_mark_pending(owner, HZ5_OWNERHUB_FRONT_LARGE, class_index);
  return 1;
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
static Hz5LargeRemoteChunk* hz5_largefront_tls_chunk_alloc(Hz5LargeTls* tls) {
  if (!tls) {
    return NULL;
  }
  if (!tls->chunk_pool) {
    size_t bytes = sizeof(Hz5LargeRemoteChunk) *
                   (size_t)HZ5_LARGEFRONT_REMOTE_CHUNK_POOL_CAP;
    void* mem = mmap(NULL,
                     bytes,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1,
                     0);
    if (mem == MAP_FAILED) {
      return NULL;
    }
    tls->chunk_pool = (Hz5LargeRemoteChunk*)mem;
    tls->chunk_cap = HZ5_LARGEFRONT_REMOTE_CHUNK_POOL_CAP;
    tls->chunk_next = 0;
  }
  if (tls->chunk_next >= tls->chunk_cap) {
    return NULL;
  }
  return &tls->chunk_pool[tls->chunk_next++];
}

static int hz5_largefront_remote_publish_chunk(Hz5LargeTls* tls,
                                               Hz5OwnerToken owner,
                                               uint32_t class_index,
                                               Hz5LargeSpan* head,
                                               uint32_t count) {
  if (!hz5_largefront_can_publish_to_owner(owner, class_index, head) ||
      count == 0u || count > HZ5_LARGEFRONT_REMOTE_CHUNK_CAP ||
      hz5_largefront_class_bytes(class_index) != 131072u) {
    return 0;
  }
  Hz5LargeRemoteChunk* chunk = hz5_largefront_tls_chunk_alloc(tls);
  if (!chunk) {
    return 0;
  }

  chunk->next = NULL;
  chunk->count = (uint16_t)count;
  chunk->class_index = (uint16_t)class_index;
  chunk->owner = owner;
  Hz5LargeSpan* span = head;
  for (uint32_t i = 0; i < count; ++i) {
    if (!span) {
      chunk->count = (uint16_t)i;
      break;
    }
    Hz5LargeSpan* next = span->next;
    span->next = NULL;
    chunk->spans[i] = span;
    span = next;
  }

  Hz5LargeRemoteChunk* old_head = NULL;
  _Atomic(Hz5LargeRemoteChunk*)* inbox =
      &g_hz5_largefront_owner_chunk_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    chunk->next = old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, chunk, memory_order_release, memory_order_acquire));
  hz5_ownerhub_mark_pending(owner, HZ5_OWNERHUB_FRONT_LARGE, class_index);
  return 1;
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_BATCH
static void hz5_largefront_remote_batch_flush(Hz5LargeTls* tls) {
  if (!tls || tls->remote_batch_count == 0u) {
    return;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  hz5_largefront_policy_note_remote_flush(tls->remote_batch_class,
                                          tls->remote_batch_count,
                                          tls->remote_batch_cap);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L1B
  hz5_largefront_policy_l1b_note_remote_flush(tls,
                                              tls->remote_batch_class,
                                              tls->remote_batch_count);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
  if (hz5_largefront_remote_publish_chunk(tls,
                                          tls->remote_batch_owner,
                                          tls->remote_batch_class,
                                          tls->remote_batch_head,
                                          tls->remote_batch_count)) {
    tls->remote_batch_owner = k_hz5_largefront_no_owner;
    tls->remote_batch_class = 0;
    tls->remote_batch_count = 0;
    tls->remote_batch_head = NULL;
    tls->remote_batch_tail = NULL;
    return;
  }
#endif
  if (!hz5_largefront_remote_publish_list(tls->remote_batch_owner,
                                          tls->remote_batch_class,
                                          tls->remote_batch_head,
                                          tls->remote_batch_tail)) {
    Hz5LargeSpan* span = tls->remote_batch_head;
    while (span) {
      Hz5LargeSpan* next = span->next;
      hz5_largefront_mark_orphan(span);
      span = next;
    }
  }
  tls->remote_batch_owner = k_hz5_largefront_no_owner;
  tls->remote_batch_class = 0;
  tls->remote_batch_count = 0;
  tls->remote_batch_head = NULL;
  tls->remote_batch_tail = NULL;
}

static void hz5_largefront_remote_batch_push(Hz5LargeTls* tls,
                                             Hz5LargeSpan* span) {
  if (!hz5_owner_is_alive(span->owner)) {
    hz5_largefront_mark_orphan(span);
    return;
  }
  uint32_t class_index = span->class_index;
  if (tls->remote_batch_count != 0u &&
      (!hz5_owner_equal(tls->remote_batch_owner, span->owner) ||
       tls->remote_batch_class != class_index)) {
    hz5_largefront_remote_batch_flush(tls);
  }

  span->next = NULL;
  if (tls->remote_batch_count == 0u) {
    tls->remote_batch_owner = span->owner;
    tls->remote_batch_class = class_index;
    tls->remote_batch_head = span;
  } else {
    tls->remote_batch_tail->next = span;
  }
  tls->remote_batch_tail = span;
  ++tls->remote_batch_count;

  uint32_t cap = tls->remote_batch_cap == 0u
                     ? HZ5_LARGEFRONT_REMOTE_BATCH_CAP
                     : tls->remote_batch_cap;
  if (tls->remote_batch_count >= cap) {
    hz5_largefront_remote_batch_flush(tls);
  }
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_POP_BUDGET
static Hz5LargeSpan* hz5_largefront_owner_inbox_pop_one(Hz5OwnerToken owner,
                                                        uint32_t class_index) {
  if (owner.slot == 0 || class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    return NULL;
  }
  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[owner.slot][class_index];
  void* head = atomic_load_explicit(inbox, memory_order_acquire);
  while (head) {
    Hz5LargeSpan* span = (Hz5LargeSpan*)head;
    void* next = span->next;
    if (atomic_compare_exchange_weak_explicit(inbox,
                                              &head,
                                              next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      span->next = NULL;
      return span;
    }
  }
  hz5_ownerhub_clear_pending(owner, HZ5_OWNERHUB_FRONT_LARGE, class_index);
  return NULL;
}

static Hz5LargeSpan* hz5_largefront_drain_remote_class_pop_budget(
    Hz5LargeTls* tls,
    uint32_t class_index,
    uint32_t budget,
    uint32_t* drained_out) {
  uint32_t drained = 0;
  Hz5LargeSpan* taken = NULL;
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }

  while (!taken || (budget != 0u && drained < budget)) {
    Hz5LargeSpan* span =
        hz5_largefront_owner_inbox_pop_one(tls->owner, class_index);
    if (!span) {
      break;
    }
    if (!hz5_owner_equal(span->owner, tls->owner)) {
      hz5_largefront_mark_orphan(span);
      continue;
    }
    if (!taken &&
        hz5_largefront_state_cas(span,
                                 (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                 (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
      hz5_largefront_payload_reactivate(span);
      taken = span;
      continue;
    }
    if (budget != 0u && drained < budget &&
        hz5_largefront_remote_pending_to_local(span)) {
      hz5_largefront_local_push(tls, class_index, span);
      ++drained;
      continue;
    }
    hz5_largefront_mark_orphan(span);
  }

  if (drained_out) {
    *drained_out = drained;
  }
  return taken;
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
static Hz5LargeSpan* hz5_largefront_drain_chunk_inbox(
    Hz5LargeTls* tls,
    uint32_t class_index,
    uint32_t* drained_out) {
  uint32_t drained = 0;
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }

  _Atomic(Hz5LargeRemoteChunk*)* inbox =
      &g_hz5_largefront_owner_chunk_inbox[tls->owner.slot][class_index];
  hz5_ownerhub_clear_pending(tls->owner,
                             HZ5_OWNERHUB_FRONT_LARGE,
                             class_index);
  Hz5LargeRemoteChunk* chunk =
      atomic_exchange_explicit(inbox, NULL, memory_order_acq_rel);
  Hz5LargeSpan* taken = NULL;
  while (chunk) {
    Hz5LargeRemoteChunk* next_chunk = chunk->next;
    for (uint32_t i = 0; i < chunk->count; ++i) {
      Hz5LargeSpan* span = chunk->spans[i];
      if (!span) {
        continue;
      }
      if (!hz5_owner_equal(span->owner, tls->owner)) {
        hz5_largefront_mark_orphan(span);
        continue;
      }
      if (!taken &&
          hz5_largefront_state_cas(span,
                                   (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                   (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
        hz5_largefront_payload_reactivate(span);
        taken = span;
        continue;
      }
      if (hz5_largefront_remote_pending_to_local(span)) {
        hz5_largefront_local_push(tls, class_index, span);
        ++drained;
      } else {
        hz5_largefront_mark_orphan(span);
      }
    }
    chunk = next_chunk;
  }
  if (drained_out) {
    *drained_out = drained;
  }
  return taken;
}
#endif

static Hz5LargeSpan* hz5_largefront_drain_remote_class_budget(
    Hz5LargeTls* tls,
    uint32_t class_index,
    uint32_t budget,
    uint32_t* drained_out) {
  uint32_t drained = 0;
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_LARGEFRONT_CLASS_COUNT) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
  Hz5LargeSpan* chunk_taken =
      hz5_largefront_drain_chunk_inbox(tls, class_index, &drained);
  if (chunk_taken) {
    if (drained_out) {
      *drained_out = drained;
    }
    return chunk_taken;
  }
#endif

  _Atomic(void*)* inbox =
      &g_hz5_largefront_owner_inbox[tls->owner.slot][class_index];
  hz5_ownerhub_clear_pending(tls->owner,
                             HZ5_OWNERHUB_FRONT_LARGE,
                             class_index);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_EMPTY_GATED
  if (!atomic_load_explicit(inbox, memory_order_acquire)) {
    if (drained_out) {
      *drained_out = 0;
    }
    return NULL;
  }
#endif
  void* head = atomic_exchange_explicit(inbox, NULL, memory_order_acq_rel);
  Hz5LargeSpan* taken = NULL;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  uint32_t policy_take_first = 0;
  uint32_t policy_to_local = 0;
  uint32_t policy_republished = 0;
  uint32_t policy_held = 0;
  uint32_t policy_orphaned = 0;
  uint32_t policy_state_fail = 0;
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_BULK_LOCAL
  Hz5LargeSpan* bulk_local_head = NULL;
  Hz5LargeSpan* bulk_local_tail = NULL;
  uint32_t bulk_local_count = 0;
#define HZ5_LARGEFRONT_FLUSH_BULK_LOCAL()                                      \
  do {                                                                         \
    hz5_largefront_local_push_list(tls,                                        \
                                   class_index,                                \
                                   bulk_local_head,                            \
                                   bulk_local_tail,                            \
                                   bulk_local_count);                          \
    bulk_local_head = NULL;                                                    \
    bulk_local_tail = NULL;                                                    \
    bulk_local_count = 0;                                                      \
  } while (0)
#define HZ5_LARGEFRONT_STAGE_LOCAL_SPAN(span_)                                 \
  do {                                                                         \
    (span_)->next = bulk_local_head;                                           \
    if (!bulk_local_head) {                                                    \
      bulk_local_tail = (span_);                                               \
    }                                                                          \
    bulk_local_head = (span_);                                                 \
    ++bulk_local_count;                                                        \
  } while (0)
#else
#define HZ5_LARGEFRONT_FLUSH_BULK_LOCAL() ((void)0)
#define HZ5_LARGEFRONT_STAGE_LOCAL_SPAN(span_)                                 \
  hz5_largefront_local_push(tls, class_index, (span_))
#endif
  while (head) {
    Hz5LargeSpan* span = (Hz5LargeSpan*)head;
    head = span->next;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TAKE_ONLY
    if (taken) {
      HZ5_LARGEFRONT_FLUSH_BULK_LOCAL();
      Hz5LargeSpan* tail = span;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      uint32_t republished = 1;
#endif
      while (tail->next) {
        tail = tail->next;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
        ++republished;
#endif
      }
      (void)hz5_largefront_remote_publish_list(tls->owner,
                                               class_index,
                                               span,
                                               tail);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      policy_republished += republished;
      hz5_largefront_policy_note_owner_drain(class_index,
                                             policy_take_first +
                                                 policy_republished,
                                             policy_take_first,
                                             policy_to_local,
                                             policy_republished,
                                             policy_held,
                                             policy_orphaned,
                                             policy_state_fail);
#endif
      if (drained_out) {
        *drained_out = drained;
      }
      return taken;
    }
#endif
    if (budget != 0u && drained >= budget) {
      HZ5_LARGEFRONT_FLUSH_BULK_LOCAL();
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      uint32_t republished = 0;
#endif
      Hz5LargeSpan* tail = span;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L7
      if (hz5_largefront_class_bytes(class_index) == 131072u) {
        uint32_t remainder_count = 0;
        for (Hz5LargeSpan* it = span; it; it = it->next) {
          ++remainder_count;
        }
        if (remainder_count >=
            HZ5_LARGEFRONT_POLICY_L7_REMAINDER_LOCAL_THRESHOLD) {
          Hz5LargeSpan* remainder = span;
          while (remainder) {
            Hz5LargeSpan* next = remainder->next;
            remainder->next = NULL;
            if (!hz5_owner_equal(remainder->owner, tls->owner)) {
              hz5_largefront_mark_orphan(remainder);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
              ++policy_orphaned;
#endif
            } else if (hz5_largefront_remote_pending_to_local(remainder)) {
              HZ5_LARGEFRONT_STAGE_LOCAL_SPAN(remainder);
              ++drained;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
              ++policy_to_local;
#endif
            } else {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
              ++policy_state_fail;
#endif
              hz5_largefront_mark_orphan(remainder);
            }
            remainder = next;
          }
          HZ5_LARGEFRONT_FLUSH_BULK_LOCAL();
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
          hz5_largefront_policy_note_owner_drain(class_index,
                                                 drained + policy_take_first +
                                                     policy_held +
                                                     policy_orphaned +
                                                     policy_state_fail,
                                                 policy_take_first,
                                                 policy_to_local,
                                                 policy_republished,
                                                 policy_held,
                                                 policy_orphaned,
                                                 policy_state_fail);
#endif
          if (drained_out) {
            *drained_out = drained;
          }
          return taken;
        }
      }
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_HOLD
      Hz5LargeSpan* remainder = span;
      while (remainder) {
        Hz5LargeSpan* next = remainder->next;
        if (!hz5_owner_equal(remainder->owner, tls->owner)) {
          remainder->next = NULL;
          hz5_largefront_mark_orphan(remainder);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
          ++policy_orphaned;
#endif
        } else if (!hz5_largefront_remote_hold_push(tls,
                                                    class_index,
                                                    remainder)) {
          remainder->next = next;
          break;
        }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
        ++policy_held;
#endif
        remainder = next;
      }
      if (!remainder) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
        hz5_largefront_policy_note_owner_drain(class_index,
                                               drained + policy_take_first +
                                                   policy_held +
                                                   policy_orphaned,
                                               policy_take_first,
                                               policy_to_local,
                                               policy_republished,
                                               policy_held,
                                               policy_orphaned,
                                               policy_state_fail);
#endif
        if (drained_out) {
          *drained_out = drained;
        }
        return taken;
      }
      span = remainder;
      tail = span;
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      republished = 1;
#endif
      while (tail->next) {
        tail = tail->next;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
        ++republished;
#endif
      }
      (void)hz5_largefront_remote_publish_list(tls->owner,
                                               class_index,
                                               span,
                                               tail);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      policy_republished += republished;
      hz5_largefront_policy_note_owner_drain(class_index,
                                             drained + policy_take_first +
                                                 policy_republished +
                                                 policy_held +
                                                 policy_orphaned,
                                             policy_take_first,
                                             policy_to_local,
                                             policy_republished,
                                             policy_held,
                                             policy_orphaned,
                                             policy_state_fail);
#endif
      if (drained_out) {
        *drained_out = drained;
      }
      return taken;
    }
    if (!hz5_owner_equal(span->owner, tls->owner)) {
      hz5_largefront_mark_orphan(span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      ++policy_orphaned;
#endif
      continue;
    }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_TAKE_FIRST
    if (!taken &&
        hz5_largefront_state_cas(span,
                                 (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING,
                                 (unsigned char)HZ5_LARGESPAN_ACTIVE)) {
      span->next = NULL;
      hz5_largefront_payload_reactivate(span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_remote_take_first[class_index]);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      ++policy_take_first;
#endif
      taken = span;
      continue;
    }
#endif
    if (hz5_largefront_remote_pending_to_local(span)) {
      HZ5_LARGEFRONT_STAGE_LOCAL_SPAN(span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_remote_to_local[class_index]);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      ++policy_to_local;
#endif
    } else {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
      ++policy_state_fail;
#endif
    }
    ++drained;
  }
  HZ5_LARGEFRONT_FLUSH_BULK_LOCAL();
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  hz5_largefront_policy_note_owner_drain(class_index,
                                         drained + policy_take_first +
                                             policy_orphaned +
                                             policy_state_fail,
                                         policy_take_first,
                                         policy_to_local,
                                         policy_republished,
                                         policy_held,
                                         policy_orphaned,
                                         policy_state_fail);
#endif
  if (drained_out) {
    *drained_out = drained;
  }
#undef HZ5_LARGEFRONT_STAGE_LOCAL_SPAN
#undef HZ5_LARGEFRONT_FLUSH_BULK_LOCAL
  return taken;
}

static Hz5LargeSpan* hz5_largefront_drain_remote_class(Hz5LargeTls* tls,
                                                       uint32_t class_index) {
  return hz5_largefront_drain_remote_class_budget(tls,
                                                  class_index,
                                                  0u,
                                                  NULL);
}

void hz5_largefront_ownerhub_drain_some(uint32_t budget) {
  Hz5LargeTls* tls = hz5_largefront_tls();
  uint32_t remaining = budget;
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_CLASS_COUNT; ++i) {
    if (remaining == 0u) {
      break;
    }
    uint32_t drained = 0;
    (void)hz5_largefront_drain_remote_class_budget(tls,
                                                   i,
                                                   remaining,
                                                   &drained);
    remaining -= drained > remaining ? remaining : drained;
  }
}
#endif

static Hz5LargeSpan* hz5_largefront_new_span(Hz5LargeTls* tls,
                                             uint32_t class_index) {
  uint32_t class_bytes = hz5_largefront_class_bytes(class_index);
  void* raw = hz5_largefront_source_alloc_raw(class_index);
  if (!raw) {
    return NULL;
  }

  Hz5LargeSpan* span = (Hz5LargeSpan*)raw;
  span->magic = 0;
  span->raw = raw;
  span->base = (void*)((uintptr_t)raw + HZ5_LARGEFRONT_PAGE_SIZE);
  span->class_bytes = class_bytes;
  span->page_count = (uint16_t)(class_bytes / HZ5_LARGEFRONT_PAGE_SIZE);
  span->class_index = (uint16_t)class_index;
  span->owner = tls->owner;
  atomic_store_explicit(&span->state,
                        (unsigned char)HZ5_LARGESPAN_ACTIVE,
                        memory_order_release);
  span->generation = 1;
  span->flags = 0;
  span->next = NULL;

  if (!hz5_largefront_map_insert(span)) {
    hz5_largefront_source_free_raw(class_index, raw);
    return NULL;
  }
  span->magic = HZ5_LARGEFRONT_MAGIC;
  return span;
}

void* hz5_largefront_alloc(size_t size, size_t align) {
  if (align > 16u) {
    return NULL;
  }
  int class_index = hz5_largefront_class_index(size);
  if (class_index < 0) {
    return NULL;
  }

  Hz5LargeTls* tls = hz5_largefront_tls();
  uint32_t ci = (uint32_t)class_index;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  hz5_largefront_counter_inc(&g_hz5_largefront_obs_alloc_calls[ci]);
#endif
  Hz5LargeSpan* span = NULL;

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_FIRST_128K && \
    BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
  if (hz5_largefront_class_bytes(ci) == 131072u) {
    hz5_ownerhub_note_alloc_miss(tls->owner, HZ5_OWNERHUB_FRONT_LARGE, ci);
#if HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET > 0u
    span = hz5_largefront_drain_remote_class_budget(
        tls,
        ci,
        HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
        NULL);
#else
    span = hz5_largefront_drain_remote_class(tls, ci);
#endif
    if (span) {
      return span->base;
    }
  }
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_FIRST_GATED_128K && \
    BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
  if (hz5_largefront_class_bytes(ci) == 131072u &&
      tls->owner.slot != 0) {
    _Atomic(void*)* inbox = &g_hz5_largefront_owner_inbox[tls->owner.slot][ci];
    if (atomic_load_explicit(inbox, memory_order_acquire)) {
      hz5_ownerhub_note_alloc_miss(tls->owner, HZ5_OWNERHUB_FRONT_LARGE, ci);
#if HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET > 0u
      span = hz5_largefront_drain_remote_class_budget(
          tls,
          ci,
          HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
          NULL);
#else
      span = hz5_largefront_drain_remote_class(tls, ci);
#endif
      if (span) {
        return span->base;
      }
    }
  }
#endif

  span = hz5_largefront_local_pop(tls, ci);

  if (span) {
    if (hz5_largefront_activate_local_for_owner(tls, span)) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(&g_hz5_largefront_obs_local_pop_hit[ci]);
#endif
      return span->base;
    }
    return NULL;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_TLS_FIRST
  span = hz5_largefront_transfer128_tls_pop_for_owner(tls, ci);
  if (span) {
    return span->base;
  }
#else
  hz5_largefront_transfer128_tls_flush(tls);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_OWNER_SHARD
  span = hz5_largefront_transfer128_owner_shard_pop(tls, ci);
  if (span) {
    return span->base;
  }
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_CONSUMER_SHARD
  span = hz5_largefront_transfer128_consumer_shard_pop(tls, ci);
  if (span) {
    return span->base;
  }
#endif
  span = hz5_largefront_transfer128_pop_for_owner(tls, ci);
  if (span) {
    return span->base;
  }
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_HOLD
  span = hz5_largefront_remote_hold_drain_budget(
      tls,
      ci,
      HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
      NULL);
  if (span) {
    return span->base;
  }
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
  hz5_ownerhub_note_alloc_miss(tls->owner, HZ5_OWNERHUB_FRONT_LARGE, ci);
#if HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET > 0u
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DRAIN_POP_BUDGET
  span = hz5_largefront_drain_remote_class_pop_budget(
      tls,
      ci,
      HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
      NULL);
#else
  span = hz5_largefront_drain_remote_class_budget(
      tls,
      ci,
      HZ5_LARGEFRONT_ALLOC_DRAIN_LOCAL_BUDGET,
      NULL);
#endif
#else
  span = hz5_largefront_drain_remote_class(tls, ci);
#endif
  if (span) {
    return span->base;
  }
  hz5_ownerhub_drain_cross_fronts(tls->owner, HZ5_OWNERHUB_FRONT_LARGE);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
  hz5_midpagefront_owner_drain_some(2u);
#endif
  span = hz5_largefront_local_pop(tls, ci);
  if (span) {
    if (hz5_largefront_activate_local_for_owner(tls, span)) {
      return span->base;
    }
    return NULL;
  }
#endif

  while ((span = hz5_largefront_global_pop(ci)) != NULL) {
    if (hz5_largefront_activate_global_for_owner(tls, span)) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(&g_hz5_largefront_obs_global_pop_hit[ci]);
#endif
      return span->base;
    }
  }

  span = hz5_largefront_new_span(tls, ci);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  if (span) {
    hz5_largefront_counter_inc(&g_hz5_largefront_obs_new_span[ci]);
  }
#endif
  return span ? span->base : NULL;
}

Hz5LargeFrontFreeResult hz5_largefront_free(void* ptr) {
  Hz5LargeSpan* span = hz5_largefront_span_for_ptr(ptr);
  if (!span) {
    return HZ5_LARGEFRONT_FREE_NOT_OWNED;
  }
  if (ptr != span->base) {
    return HZ5_LARGEFRONT_FREE_INVALID;
  }

  Hz5LargeTls* tls = hz5_largefront_tls();
  if (hz5_owner_equal(span->owner, tls->owner)) {
    if (!hz5_largefront_owner_local_state_transition(
            span,
            (unsigned char)HZ5_LARGESPAN_ACTIVE,
            (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
      return HZ5_LARGEFRONT_FREE_INVALID;
    }
    hz5_largefront_local_push(tls, span->class_index, span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
    hz5_largefront_counter_inc(
        &g_hz5_largefront_obs_free_local[span->class_index]);
#endif
  } else {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128
    if (hz5_largefront_transfer128_class(span->class_index)) {
      if (!hz5_largefront_state_cas(
              span,
              (unsigned char)HZ5_LARGESPAN_ACTIVE,
              (unsigned char)HZ5_LARGESPAN_TRANSFER_FREE)) {
        return HZ5_LARGEFRONT_FREE_INVALID;
      }
      if (!hz5_largefront_transfer128_push(tls, span)) {
        hz5_largefront_mark_orphan(span);
      }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_free_remote[span->class_index]);
#endif
      return HZ5_LARGEFRONT_FREE_OK;
    }
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_GLOBAL_128K
    if (hz5_largefront_class_bytes(span->class_index) == 131072u) {
      if (!hz5_largefront_state_cas(
              span,
              (unsigned char)HZ5_LARGESPAN_ACTIVE,
              (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
        return HZ5_LARGEFRONT_FREE_INVALID;
      }
      hz5_largefront_global_push(span->class_index, span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_free_global[span->class_index]);
#endif
      return HZ5_LARGEFRONT_FREE_OK;
    }
#endif
    if (hz5_owner_is_alive(span->owner)) {
      if (!hz5_largefront_state_cas(
              span,
              (unsigned char)HZ5_LARGESPAN_ACTIVE,
              (unsigned char)HZ5_LARGESPAN_REMOTE_PENDING)) {
        return HZ5_LARGEFRONT_FREE_INVALID;
      }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_BATCH
      hz5_largefront_remote_batch_push(tls, span);
#else
      if (!hz5_largefront_remote_publish_one(span->owner,
                                             span->class_index,
                                             span)) {
        hz5_largefront_mark_orphan(span);
      }
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
      hz5_largefront_counter_inc(
          &g_hz5_largefront_obs_free_remote[span->class_index]);
#endif
      return HZ5_LARGEFRONT_FREE_OK;
    }
#endif
    if (!hz5_largefront_state_cas(span,
                                  (unsigned char)HZ5_LARGESPAN_ACTIVE,
                                  (unsigned char)HZ5_LARGESPAN_LOCAL_FREE)) {
      return HZ5_LARGEFRONT_FREE_INVALID;
    }
    hz5_largefront_global_push(span->class_index, span);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
    hz5_largefront_counter_inc(
        &g_hz5_largefront_obs_free_global[span->class_index]);
#endif
  }
  return HZ5_LARGEFRONT_FREE_OK;
}

int hz5_largefront_can_handle(size_t size, size_t align) {
  return align <= 16u && hz5_largefront_class_index(size) >= 0;
}

int hz5_largefront_owns(void* ptr) {
  return hz5_largefront_span_for_ptr(ptr) != NULL;
}

size_t hz5_largefront_usable_size(void* ptr) {
  Hz5LargeSpan* span = hz5_largefront_span_for_ptr(ptr);
  if (!span || ptr != span->base) {
    return 0;
  }
  return span->class_bytes;
}

#else

void hz5_largefront_ownerhub_drain_some(uint32_t budget) {
  (void)budget;
}

void* hz5_largefront_alloc(size_t size, size_t align) {
  (void)size;
  (void)align;
  return NULL;
}

Hz5LargeFrontFreeResult hz5_largefront_free(void* ptr) {
  (void)ptr;
  return HZ5_LARGEFRONT_FREE_NOT_OWNED;
}

int hz5_largefront_can_handle(size_t size, size_t align) {
  (void)size;
  (void)align;
  return 0;
}

int hz5_largefront_owns(void* ptr) {
  (void)ptr;
  return 0;
}

size_t hz5_largefront_usable_size(void* ptr) {
  (void)ptr;
  return 0;
}

#endif
