#include "hz6_allocator.h"
#include "hz6_allocator_api_descriptor.h"
#include "hz6_allocator_remote_pending_external_dup_index.h"
#include "hz6_allocator_remote_pending_storage_access.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
#define HZ6_REMOTE_PENDING_INDEX_NONE UINT32_MAX

#if HZ6_REMOTE_PENDING_LAZY_STORAGE_L1
void hz6_remote_pending_storage_reset(Hz6Allocator* allocator);

static void hz6_remote_pending_storage_reset_block(
    Hz6RemotePendingStorage* storage) {
  for (size_t class_id = 0; class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
       ++class_id) {
    atomic_flag_clear_explicit(&storage->remote_pending_inbox[class_id].lock,
                               memory_order_relaxed);
    for (size_t front_index = 0; front_index < HZ6_REMOTE_PENDING_FRONT_COUNT;
         ++front_index) {
      storage->remote_pending_inbox[class_id].head_index[front_index] =
          HZ6_REMOTE_PENDING_INDEX_NONE;
      storage->remote_pending_inbox[class_id].key_count[front_index] = 0;
    }
    storage->remote_pending_inbox[class_id].total_count = 0;
  }
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    storage->remote_pending_next[i] = HZ6_REMOTE_PENDING_INDEX_NONE;
    storage->remote_pending_slot_state[i] = HZ6_REMOTE_PENDING_SLOT_NONE;
    storage->remote_pending_published_ptr[i] = NULL;
    storage->remote_pending_published_generation[i] = 0;
    storage->remote_pending_published_bytes[i] = 0;
    storage->remote_pending_published_front_id[i] = 0;
    storage->remote_pending_published_class_id[i] = 0;
    storage->remote_pending_owner_token[i] = (Hz6OwnerToken){0};
  }
#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
  atomic_flag_clear_explicit(&storage->remote_pending_external_lock,
                             memory_order_relaxed);
  for (size_t front_index = 0; front_index < HZ6_REMOTE_PENDING_FRONT_COUNT;
       ++front_index) {
    for (size_t class_id = 0; class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
         ++class_id) {
      storage->remote_pending_external_head[front_index][class_id] =
          HZ6_REMOTE_PENDING_INDEX_NONE;
    }
  }
  storage->remote_pending_external_free_head =
      HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY == 0
          ? HZ6_REMOTE_PENDING_INDEX_NONE
          : 0u;
  for (size_t i = 0; i < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY; ++i) {
    Hz6RemotePendingExternalTicket* ticket =
        &storage->remote_pending_external_tickets[i];
    memset(ticket, 0, sizeof(*ticket));
    ticket->next =
        i + 1u < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY
            ? (uint32_t)(i + 1u)
            : HZ6_REMOTE_PENDING_INDEX_NONE;
  }
  atomic_store_explicit(&storage->remote_pending_external_nonempty_mask, 0u,
                        memory_order_relaxed);
#if HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
  for (size_t i = 0; i < HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY; ++i) {
    storage->remote_pending_external_dup_index[i] = UINT32_MAX;
  }
#endif
#endif
  atomic_store_explicit(&storage->remote_pending_current, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&storage->remote_pending_queued_current, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&storage->remote_pending_claimed_current, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&storage->remote_pending_total_current, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&storage->remote_pending_nonempty_mask, 0u,
                        memory_order_relaxed);
}

static int hz6_remote_pending_storage_ensure(Hz6Allocator* allocator) {
  if (atomic_load_explicit(&allocator->remote_pending_storage,
                           memory_order_acquire)) {
    return 1;
  }
  while (atomic_flag_test_and_set_explicit(
      &allocator->remote_pending_storage_lock, memory_order_acquire)) {
  }
  if (atomic_load_explicit(&allocator->remote_pending_storage,
                           memory_order_acquire)) {
    atomic_flag_clear_explicit(&allocator->remote_pending_storage_lock,
                               memory_order_release);
    return 1;
  }
  Hz6OwnerInboxStorageBlock block =
      hz6_owner_inbox_storage_alloc(sizeof(Hz6RemotePendingStorage));
  if (!block.ptr) {
    atomic_flag_clear_explicit(&allocator->remote_pending_storage_lock,
                               memory_order_release);
    abort();
  }
  hz6_remote_pending_storage_reset_block(
      (Hz6RemotePendingStorage*)block.ptr);
  allocator->remote_pending_storage_block = block;
  atomic_store_explicit(&allocator->remote_pending_storage,
                        (Hz6RemotePendingStorage*)block.ptr,
                        memory_order_release);
  atomic_flag_clear_explicit(&allocator->remote_pending_storage_lock,
                             memory_order_release);
  return 1;
}

static int hz6_remote_pending_storage_ready(Hz6Allocator* allocator) {
  return allocator &&
         atomic_load_explicit(&allocator->remote_pending_storage,
                              memory_order_acquire);
}
#else
static int hz6_remote_pending_storage_ensure(Hz6Allocator* allocator) {
  (void)allocator;
  return 1;
}

static int hz6_remote_pending_storage_ready(Hz6Allocator* allocator) {
  return allocator != NULL;
}
#endif

#if HZ6_DIAGNOSTIC_PROBES
#define HZ6_REMOTE_PENDING_STAT_INC(allocator, field) (++(allocator)->stats.field)
#define HZ6_REMOTE_PENDING_STAT_ADD(allocator, field, value) \
  ((allocator)->stats.field += (value))
#else
#define HZ6_REMOTE_PENDING_STAT_INC(allocator, field) ((void)(allocator))
#define HZ6_REMOTE_PENDING_STAT_ADD(allocator, field, value) \
  ((void)(allocator), (void)(value))
#endif

typedef struct Hz6RemotePendingInboxEntry {
  void* ptr;
  Hz6ObjectDescriptor* descriptor;
  uint32_t generation;
  uint32_t bytes;
  uint16_t front_id;
  uint16_t class_id;
  Hz6OwnerToken owner_token;
} Hz6RemotePendingInboxEntry;

typedef struct Hz6RemotePendingClaim {
  uint32_t descriptor_index;
  void* ptr;
  Hz6ObjectDescriptor* descriptor;
  uint32_t generation;
  uint32_t bytes;
  uint16_t front_id;
  uint16_t class_id;
  Hz6OwnerToken owner_token;
  uint8_t valid;
} Hz6RemotePendingClaim;

static int hz6_remote_pending_front_ordinal(uint16_t front_id,
                                            uint16_t* out) {
  if (!out) {
    return 0;
  }
  switch (front_id) {
    case HZ6_FRONT_LOCAL2P:
      *out = 0;
      return 1;
    case HZ6_FRONT_MIDPAGE:
      *out = 1;
      return 1;
    case HZ6_FRONT_LARGE:
      *out = 2;
      return 1;
    case HZ6_FRONT_TOY:
      *out = 3;
      return 1;
    default:
      return 0;
  }
}

static int hz6_remote_pending_key_bit(uint16_t front_id,
                                      uint16_t class_id,
                                      uint16_t* out_index,
                                      uint64_t* out_bit) {
  uint16_t ordinal = 0;
  if (!out_bit || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !hz6_remote_pending_front_ordinal(front_id, &ordinal)) {
    return 0;
  }
  uint16_t bit_index =
      (uint16_t)(ordinal * HZ6_FRONT_CACHE_CLASS_COUNT + class_id);
  if (bit_index >= 64u) {
    return 0;
  }
  if (out_index) {
    *out_index = bit_index;
  }
  *out_bit = UINT64_C(1) << bit_index;
  return 1;
}

static void hz6_remote_pending_lock(Hz6RemotePendingClassInbox* inbox) {
  while (atomic_flag_test_and_set_explicit(&inbox->lock,
                                           memory_order_acquire)) {
  }
}

static void hz6_remote_pending_unlock(Hz6RemotePendingClassInbox* inbox) {
  atomic_flag_clear_explicit(&inbox->lock, memory_order_release);
}

static int hz6_remote_pending_try_lock(Hz6RemotePendingClassInbox* inbox) {
  return !atomic_flag_test_and_set_explicit(&inbox->lock,
                                            memory_order_acquire);
}

#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
static void hz6_remote_pending_external_lock(Hz6Allocator* allocator) {
  while (atomic_flag_test_and_set_explicit(
      &HZ6_RP_EXTERNAL_LOCK(allocator), memory_order_acquire)) {
  }
}

static void hz6_remote_pending_external_unlock(Hz6Allocator* allocator) {
  atomic_flag_clear_explicit(&HZ6_RP_EXTERNAL_LOCK(allocator),
                             memory_order_release);
}

static int hz6_remote_pending_external_key_nonempty(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
  HZ6_REMOTE_PENDING_STAT_INC(allocator, remote_pending_external_key_probe);
  if (!hz6_remote_pending_storage_ready(allocator) ||
      class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  uint16_t front_index = 0;
  if (!hz6_remote_pending_front_ordinal(front_id, &front_index)) {
    return 0;
  }
  uint64_t bit = 0;
  if (!hz6_remote_pending_key_bit(front_id, class_id, NULL, &bit) ||
      (atomic_load_explicit(&HZ6_RP_EXTERNAL_NONEMPTY_MASK(allocator),
                            memory_order_relaxed) &
       bit) == 0) {
    return 0;
  }
  hz6_remote_pending_external_lock(allocator);
  uint32_t head =
      HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id];
  hz6_remote_pending_external_unlock(allocator);
  int hit = head != HZ6_REMOTE_PENDING_INDEX_NONE &&
            head < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY;
  if (hit) {
    HZ6_REMOTE_PENDING_STAT_INC(allocator, remote_pending_external_key_hit);
  } else {
    atomic_fetch_and_explicit(&HZ6_RP_EXTERNAL_NONEMPTY_MASK(allocator),
                              ~bit, memory_order_relaxed);
  }
  return hit;
}

static void hz6_remote_pending_external_note_enqueue(
    Hz6Allocator* allocator) {
#if HZ6_DIAGNOSTIC_PROBES
  size_t current = ++allocator->stats.remote_pending_external_ticket_current;
  if (current >
      allocator->stats.remote_pending_external_ticket_high_water) {
    allocator->stats.remote_pending_external_ticket_high_water = current;
  }
#else
  (void)allocator;
#endif
}

static void hz6_remote_pending_external_note_remove(
    Hz6Allocator* allocator) {
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator->stats.remote_pending_external_ticket_current > 0) {
    --allocator->stats.remote_pending_external_ticket_current;
  }
#else
  (void)allocator;
#endif
}

#if !HZ6_REMOTE_PENDING_EXTERNAL_LOCKED_REVALIDATE_L1
static void hz6_remote_pending_external_note_duplicate_probes(
    Hz6Allocator* allocator,
    size_t probes) {
#if HZ6_DIAGNOSTIC_PROBES
  allocator->stats.remote_pending_external_ticket_duplicate_probe_total +=
      probes;
  if (probes >
      allocator->stats.remote_pending_external_ticket_duplicate_probe_max) {
    allocator->stats.remote_pending_external_ticket_duplicate_probe_max =
        probes;
  }
#else
  (void)allocator;
  (void)probes;
#endif
}
#endif

#else
static int hz6_remote_pending_external_key_nonempty(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
  (void)allocator;
  (void)front_id;
  (void)class_id;
  return 0;
}
#endif

static int hz6_remote_pending_descriptor_index(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    uint32_t* out_index) {
  if (!allocator || !descriptor || !out_index ||
      descriptor < allocator->descriptors ||
      descriptor >= allocator->descriptors + HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    return 0;
  }
  *out_index = (uint32_t)(descriptor - allocator->descriptors);
  return 1;
}

static void hz6_remote_pending_note_current(Hz6Allocator* allocator,
                                            size_t current) {
#if HZ6_DIAGNOSTIC_PROBES
  size_t claimed = atomic_load_explicit(
      &HZ6_RP_CLAIMED_CURRENT(allocator), memory_order_relaxed);
  allocator->stats.remote_pending_current = current;
  allocator->stats.remote_pending_queued_current = current;
  allocator->stats.remote_pending_total_current = current + claimed;
  if (current > allocator->stats.remote_pending_high_water) {
    allocator->stats.remote_pending_high_water = current;
  }
#else
  (void)allocator;
  (void)current;
#endif
}

static void hz6_remote_pending_note_claimed(Hz6Allocator* allocator,
                                            size_t claimed) {
#if HZ6_DIAGNOSTIC_PROBES
  allocator->stats.remote_pending_claimed_current = claimed;
  allocator->stats.remote_pending_total_current =
      allocator->stats.remote_pending_queued_current + claimed;
#else
  (void)allocator;
  (void)claimed;
#endif
}

static void hz6_remote_pending_note_key_enqueue_locked(
    Hz6Allocator* allocator,
    Hz6RemotePendingClassInbox* inbox,
    uint16_t front_id,
    uint16_t class_id) {
  uint16_t front_index = 0;
  uint64_t bit = 0;
  if (!allocator || !inbox ||
      !hz6_remote_pending_front_ordinal(front_id, &front_index) ||
      !hz6_remote_pending_key_bit(front_id, class_id, NULL, &bit)) {
    return;
  }
  if (inbox->key_count[front_index]++ == 0) {
    atomic_fetch_or_explicit(&HZ6_RP_NONEMPTY_MASK(allocator), bit,
                             memory_order_relaxed);
  }
}

static void hz6_remote_pending_note_key_pop_locked(
    Hz6Allocator* allocator,
    Hz6RemotePendingClassInbox* inbox,
    uint16_t front_id,
    uint16_t class_id) {
  uint16_t front_index = 0;
  uint64_t bit = 0;
  if (!allocator || !inbox ||
      !hz6_remote_pending_front_ordinal(front_id, &front_index) ||
      !hz6_remote_pending_key_bit(front_id, class_id, NULL, &bit) ||
      inbox->key_count[front_index] == 0) {
    return;
  }
  if (--inbox->key_count[front_index] == 0) {
    atomic_fetch_and_explicit(&HZ6_RP_NONEMPTY_MASK(allocator), ~bit,
                              memory_order_relaxed);
  }
}

static void hz6_remote_pending_requeue_locked(
    Hz6Allocator* allocator,
    uint16_t class_id,
    uint32_t index) {
  Hz6RemotePendingClassInbox* inbox =
      &HZ6_RP_INBOX(allocator)[class_id];
  uint16_t front_index = 0;
  if (!hz6_remote_pending_front_ordinal(
          HZ6_RP_PUBLISHED_FRONT_ID(allocator)[index],
          &front_index)) {
    return;
  }
  HZ6_RP_SLOT_STATE(allocator)[index] =
      HZ6_REMOTE_PENDING_SLOT_QUEUED;
  HZ6_RP_NEXT(allocator)[index] = inbox->head_index[front_index];
  inbox->head_index[front_index] = index;
  ++inbox->total_count;
  hz6_remote_pending_note_key_enqueue_locked(
      allocator, inbox, HZ6_RP_PUBLISHED_FRONT_ID(allocator)[index],
      HZ6_RP_PUBLISHED_CLASS_ID(allocator)[index]);
  size_t current = atomic_fetch_add_explicit(
                       &HZ6_RP_CURRENT(allocator), 1u,
                       memory_order_relaxed) +
                   1u;
  size_t claimed = atomic_load_explicit(
      &HZ6_RP_CLAIMED_CURRENT(allocator), memory_order_relaxed);
  atomic_store_explicit(&HZ6_RP_QUEUED_CURRENT(allocator), current,
                        memory_order_relaxed);
  atomic_store_explicit(&HZ6_RP_TOTAL_CURRENT(allocator),
                        current + claimed,
                        memory_order_relaxed);
  hz6_remote_pending_note_current(allocator, current);
}

static int hz6_remote_pending_pop_key(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    Hz6RemotePendingInboxEntry* out) {
  if (!allocator || !out || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  uint16_t front_index = 0;
  if (!hz6_remote_pending_front_ordinal(front_id, &front_index)) {
    return 0;
  }
  Hz6RemotePendingClassInbox* inbox =
      &HZ6_RP_INBOX(allocator)[class_id];
  hz6_remote_pending_lock(inbox);
  uint32_t index = inbox->head_index[front_index];
  if (index == HZ6_REMOTE_PENDING_INDEX_NONE ||
      index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    hz6_remote_pending_unlock(inbox);
    return 0;
  }
  uint16_t entry_front_id =
      HZ6_RP_PUBLISHED_FRONT_ID(allocator)[index];
  uint16_t entry_class_id = HZ6_RP_PUBLISHED_CLASS_ID(allocator)[index];
  inbox->head_index[front_index] = HZ6_RP_NEXT(allocator)[index];
  HZ6_RP_NEXT(allocator)[index] = HZ6_REMOTE_PENDING_INDEX_NONE;
  HZ6_RP_SLOT_STATE(allocator)[index] = HZ6_REMOTE_PENDING_SLOT_NONE;
  --inbox->total_count;
  hz6_remote_pending_note_key_pop_locked(allocator, inbox, entry_front_id,
                                         entry_class_id);
  size_t current = atomic_fetch_sub_explicit(
                       &HZ6_RP_CURRENT(allocator), 1u,
                       memory_order_relaxed) -
                   1u;
  size_t claimed = atomic_load_explicit(
      &HZ6_RP_CLAIMED_CURRENT(allocator), memory_order_relaxed);
  atomic_store_explicit(&HZ6_RP_QUEUED_CURRENT(allocator), current,
                        memory_order_relaxed);
  atomic_store_explicit(&HZ6_RP_TOTAL_CURRENT(allocator),
                        current + claimed,
                        memory_order_relaxed);
  hz6_remote_pending_note_current(allocator, current);
  hz6_remote_pending_unlock(inbox);

  Hz6ObjectDescriptor* descriptor = &allocator->descriptors[index];
  out->ptr = HZ6_RP_PUBLISHED_PTR(allocator)[index];
  out->descriptor = descriptor;
  out->generation = HZ6_RP_PUBLISHED_GENERATION(allocator)[index];
  out->bytes = HZ6_RP_PUBLISHED_BYTES(allocator)[index];
  out->front_id = HZ6_RP_PUBLISHED_FRONT_ID(allocator)[index];
  out->class_id = HZ6_RP_PUBLISHED_CLASS_ID(allocator)[index];
  out->owner_token = HZ6_RP_OWNER_TOKEN(allocator)[index];
  return 1;
}

static void hz6_remote_pending_requeue_entry(
    Hz6Allocator* allocator,
    const Hz6RemotePendingInboxEntry* entry) {
  uint32_t index = 0;
  if (!entry || !hz6_remote_pending_descriptor_index(
                    allocator, entry->descriptor, &index) ||
      entry->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return;
  }
  Hz6RemotePendingClassInbox* inbox =
      &HZ6_RP_INBOX(allocator)[entry->class_id];
  hz6_remote_pending_lock(inbox);
  if (HZ6_RP_SLOT_STATE(allocator)[index] ==
      HZ6_REMOTE_PENDING_SLOT_NONE) {
    hz6_remote_pending_requeue_locked(allocator, entry->class_id, index);
  }
  hz6_remote_pending_unlock(inbox);
}

static void hz6_remote_pending_note_claim_transition(Hz6Allocator* allocator) {
  size_t queued = atomic_fetch_sub_explicit(
                      &HZ6_RP_CURRENT(allocator), 1u,
                      memory_order_relaxed) -
                  1u;
  atomic_store_explicit(&HZ6_RP_QUEUED_CURRENT(allocator), queued,
                        memory_order_relaxed);
  size_t claimed = atomic_fetch_add_explicit(
                       &HZ6_RP_CLAIMED_CURRENT(allocator), 1u,
                       memory_order_relaxed) +
                   1u;
  atomic_store_explicit(&HZ6_RP_TOTAL_CURRENT(allocator),
                        queued + claimed, memory_order_relaxed);
  hz6_remote_pending_note_current(allocator, queued);
  hz6_remote_pending_note_claimed(allocator, claimed);
}

static void hz6_remote_pending_note_claim_commit(Hz6Allocator* allocator) {
  size_t claimed = atomic_fetch_sub_explicit(
                       &HZ6_RP_CLAIMED_CURRENT(allocator), 1u,
                       memory_order_relaxed) -
                   1u;
  size_t queued = atomic_load_explicit(&HZ6_RP_CURRENT(allocator),
                                       memory_order_relaxed);
  atomic_store_explicit(&HZ6_RP_TOTAL_CURRENT(allocator),
                        queued + claimed, memory_order_relaxed);
  hz6_remote_pending_note_current(allocator, queued);
  hz6_remote_pending_note_claimed(allocator, claimed);
}

static void hz6_remote_pending_note_claim_cancel(Hz6Allocator* allocator) {
  size_t queued = atomic_fetch_add_explicit(
                      &HZ6_RP_CURRENT(allocator), 1u,
                      memory_order_relaxed) +
                  1u;
  atomic_store_explicit(&HZ6_RP_QUEUED_CURRENT(allocator), queued,
                        memory_order_relaxed);
  size_t claimed = atomic_fetch_sub_explicit(
                       &HZ6_RP_CLAIMED_CURRENT(allocator), 1u,
                       memory_order_relaxed) -
                   1u;
  atomic_store_explicit(&HZ6_RP_TOTAL_CURRENT(allocator),
                        queued + claimed, memory_order_relaxed);
  hz6_remote_pending_note_current(allocator, queued);
  hz6_remote_pending_note_claimed(allocator, claimed);
}

static void hz6_remote_pending_clear_proof(Hz6Allocator* allocator,
                                           uint32_t index) {
  HZ6_RP_NEXT(allocator)[index] = HZ6_REMOTE_PENDING_INDEX_NONE;
  HZ6_RP_PUBLISHED_PTR(allocator)[index] = NULL;
  HZ6_RP_PUBLISHED_GENERATION(allocator)[index] = 0;
  HZ6_RP_PUBLISHED_BYTES(allocator)[index] = 0;
  HZ6_RP_PUBLISHED_FRONT_ID(allocator)[index] = 0;
  HZ6_RP_PUBLISHED_CLASS_ID(allocator)[index] = 0;
  HZ6_RP_OWNER_TOKEN(allocator)[index] = (Hz6OwnerToken){0};
}

static Hz6RemotePendingReuseStatus hz6_remote_pending_try_claim_exact_key(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    int check_mask,
    Hz6RemotePendingClaim* out) {
  if (!allocator || !out || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return HZ6_REMOTE_PENDING_REUSE_EMPTY;
  }
  if (!hz6_remote_pending_storage_ready(allocator)) {
    return HZ6_REMOTE_PENDING_REUSE_EMPTY;
  }
  memset(out, 0, sizeof(*out));
  if (check_mask && !hz6_allocator_remote_pending_key_maybe_nonempty_raw(
          allocator, front_id, class_id)) {
    return HZ6_REMOTE_PENDING_REUSE_EMPTY;
  }
  uint16_t front_index = 0;
  if (!hz6_remote_pending_front_ordinal(front_id, &front_index)) {
    return HZ6_REMOTE_PENDING_REUSE_EMPTY;
  }
  Hz6RemotePendingClassInbox* inbox =
      &HZ6_RP_INBOX(allocator)[class_id];
  if (!hz6_remote_pending_try_lock(inbox)) {
    return HZ6_REMOTE_PENDING_REUSE_BUSY;
  }
  uint32_t index = inbox->head_index[front_index];
  if (index == HZ6_REMOTE_PENDING_INDEX_NONE ||
      index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    hz6_remote_pending_unlock(inbox);
    return HZ6_REMOTE_PENDING_REUSE_EMPTY;
  }
  if (HZ6_RP_SLOT_STATE(allocator)[index] !=
      HZ6_REMOTE_PENDING_SLOT_QUEUED) {
    hz6_remote_pending_unlock(inbox);
    return HZ6_REMOTE_PENDING_REUSE_INTEGRITY_FAILURE;
  }
  inbox->head_index[front_index] = HZ6_RP_NEXT(allocator)[index];
  HZ6_RP_NEXT(allocator)[index] = HZ6_REMOTE_PENDING_INDEX_NONE;
  HZ6_RP_SLOT_STATE(allocator)[index] =
      HZ6_REMOTE_PENDING_SLOT_CLAIMED;
  hz6_remote_pending_note_key_pop_locked(allocator, inbox, front_id,
                                         class_id);
  out->descriptor_index = index;
  out->ptr = HZ6_RP_PUBLISHED_PTR(allocator)[index];
  out->descriptor = &allocator->descriptors[index];
  out->generation = HZ6_RP_PUBLISHED_GENERATION(allocator)[index];
  out->bytes = HZ6_RP_PUBLISHED_BYTES(allocator)[index];
  out->front_id = HZ6_RP_PUBLISHED_FRONT_ID(allocator)[index];
  out->class_id = HZ6_RP_PUBLISHED_CLASS_ID(allocator)[index];
  out->owner_token = HZ6_RP_OWNER_TOKEN(allocator)[index];
  out->valid = 1;
  hz6_remote_pending_unlock(inbox);
  hz6_remote_pending_note_claim_transition(allocator);
  return HZ6_REMOTE_PENDING_REUSE_OK;
}

static void hz6_remote_pending_cancel_claim(
    Hz6Allocator* allocator,
    const Hz6RemotePendingClaim* claim) {
  if (!allocator || !claim || !claim->valid ||
      claim->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return;
  }
  uint16_t front_index = 0;
  if (!hz6_remote_pending_front_ordinal(claim->front_id, &front_index)) {
    return;
  }
  Hz6RemotePendingClassInbox* inbox =
      &HZ6_RP_INBOX(allocator)[claim->class_id];
  hz6_remote_pending_lock(inbox);
  if (HZ6_RP_SLOT_STATE(allocator)[claim->descriptor_index] ==
      HZ6_REMOTE_PENDING_SLOT_CLAIMED) {
    HZ6_RP_SLOT_STATE(allocator)[claim->descriptor_index] =
        HZ6_REMOTE_PENDING_SLOT_QUEUED;
    HZ6_RP_NEXT(allocator)[claim->descriptor_index] =
        inbox->head_index[front_index];
    inbox->head_index[front_index] = claim->descriptor_index;
    hz6_remote_pending_note_key_enqueue_locked(allocator, inbox,
                                               claim->front_id,
                                               claim->class_id);
    hz6_remote_pending_note_claim_cancel(allocator);
  }
  hz6_remote_pending_unlock(inbox);
}

static void hz6_remote_pending_commit_claim(
    Hz6Allocator* allocator,
    const Hz6RemotePendingClaim* claim) {
  Hz6RemotePendingClassInbox* inbox =
      &HZ6_RP_INBOX(allocator)[claim->class_id];
  hz6_remote_pending_lock(inbox);
  if (HZ6_RP_SLOT_STATE(allocator)[claim->descriptor_index] ==
      HZ6_REMOTE_PENDING_SLOT_CLAIMED) {
    HZ6_RP_SLOT_STATE(allocator)[claim->descriptor_index] =
        HZ6_REMOTE_PENDING_SLOT_NONE;
    if (inbox->total_count != 0) {
      --inbox->total_count;
    }
    hz6_remote_pending_clear_proof(allocator, claim->descriptor_index);
    hz6_remote_pending_note_claim_commit(allocator);
  }
  hz6_remote_pending_unlock(inbox);
}

static Hz6RemotePendingReuseStatus hz6_remote_pending_finish_claim_reuse(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t requested_bytes,
    Hz6RemotePendingClaim* claim,
    void** out_ptr,
    Hz6ObjectDescriptor** out_descriptor) {
  Hz6ObjectDescriptor* descriptor = claim->descriptor;
  int valid = allocator && descriptor && claim->ptr &&
              claim->front_id == front_id && claim->class_id == class_id &&
              descriptor->ptr == claim->ptr &&
              descriptor->generation == claim->generation &&
              descriptor->bytes == claim->bytes &&
              descriptor->class_id == class_id &&
              descriptor->state == HZ6_STATE_REMOTE_PENDING &&
              requested_bytes <= claim->bytes &&
              hz6_owner_is_alive(&allocator->owner, claim->owner_token) &&
              hz6_allocator_descriptor_owner_equal_at(
                  allocator, descriptor, claim->owner_token,
                  HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING);
  if (valid) {
    Hz6RouteResult route =
        hz6_allocator_route_lookup_exact(allocator, claim->ptr);
    if (route.kind != HZ6_ROUTE_VALID || route.descriptor != descriptor ||
        route.generation != claim->generation || route.front_id != front_id ||
        route.class_id != class_id || route.route_allocator != allocator) {
      valid = 0;
    }
  }
  if (!valid) {
    return HZ6_REMOTE_PENDING_REUSE_INTEGRITY_FAILURE;
  }

  if (!hz6_allocator_activate_descriptor(allocator, descriptor,
                                         HZ6_STATE_REMOTE_PENDING,
                                         claim->ptr, claim->generation,
                                         claim->owner_token)) {
    hz6_remote_pending_cancel_claim(allocator, claim);
    return HZ6_REMOTE_PENDING_REUSE_EMPTY;
  }

  hz6_remote_pending_commit_claim(allocator, claim);
  if (out_ptr) {
    *out_ptr = claim->ptr;
  }
  if (out_descriptor) {
    *out_descriptor = descriptor;
  }
  return HZ6_REMOTE_PENDING_REUSE_OK;
}
#endif

void hz6_remote_pending_storage_reset(Hz6Allocator* allocator) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (!allocator) {
    return;
  }
#if HZ6_REMOTE_PENDING_LAZY_STORAGE_L1
  Hz6RemotePendingStorage* storage =
      atomic_load_explicit(&allocator->remote_pending_storage,
                           memory_order_acquire);
  if (!storage) {
    return;
  }
  hz6_remote_pending_storage_reset_block(storage);
  return;
#endif
  for (size_t class_id = 0; class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
       ++class_id) {
    atomic_flag_clear_explicit(
        &HZ6_RP_INBOX(allocator)[class_id].lock,
        memory_order_relaxed);
    for (size_t front_index = 0; front_index < HZ6_REMOTE_PENDING_FRONT_COUNT;
         ++front_index) {
      HZ6_RP_INBOX(allocator)[class_id].head_index[front_index] =
          HZ6_REMOTE_PENDING_INDEX_NONE;
      HZ6_RP_INBOX(allocator)[class_id].key_count[front_index] = 0;
    }
    HZ6_RP_INBOX(allocator)[class_id].total_count = 0;
  }
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    HZ6_RP_NEXT(allocator)[i] = HZ6_REMOTE_PENDING_INDEX_NONE;
    HZ6_RP_SLOT_STATE(allocator)[i] = HZ6_REMOTE_PENDING_SLOT_NONE;
    HZ6_RP_PUBLISHED_PTR(allocator)[i] = NULL;
    HZ6_RP_PUBLISHED_GENERATION(allocator)[i] = 0;
    HZ6_RP_PUBLISHED_BYTES(allocator)[i] = 0;
    HZ6_RP_PUBLISHED_FRONT_ID(allocator)[i] = 0;
    HZ6_RP_PUBLISHED_CLASS_ID(allocator)[i] = 0;
    HZ6_RP_OWNER_TOKEN(allocator)[i] = (Hz6OwnerToken){0};
  }
#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
  atomic_flag_clear_explicit(&HZ6_RP_EXTERNAL_LOCK(allocator),
                             memory_order_relaxed);
  for (size_t front_index = 0; front_index < HZ6_REMOTE_PENDING_FRONT_COUNT;
       ++front_index) {
    for (size_t class_id = 0; class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
         ++class_id) {
      HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id] =
          HZ6_REMOTE_PENDING_INDEX_NONE;
    }
  }
  HZ6_RP_EXTERNAL_FREE_HEAD(allocator) =
      HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY == 0
          ? HZ6_REMOTE_PENDING_INDEX_NONE
          : 0u;
  for (size_t i = 0; i < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY; ++i) {
    Hz6RemotePendingExternalTicket* ticket =
        &HZ6_RP_EXTERNAL_TICKETS(allocator)[i];
    memset(ticket, 0, sizeof(*ticket));
    ticket->next =
        i + 1u < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY
            ? (uint32_t)(i + 1u)
            : HZ6_REMOTE_PENDING_INDEX_NONE;
  }
  atomic_store_explicit(&HZ6_RP_EXTERNAL_NONEMPTY_MASK(allocator), 0u,
                        memory_order_relaxed);
#if HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
  hz6_remote_pending_external_dup_index_init(allocator);
#endif
#endif
  atomic_store_explicit(&HZ6_RP_CURRENT(allocator), 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&HZ6_RP_QUEUED_CURRENT(allocator), 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&HZ6_RP_CLAIMED_CURRENT(allocator), 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&HZ6_RP_TOTAL_CURRENT(allocator), 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&HZ6_RP_NONEMPTY_MASK(allocator), 0u,
                        memory_order_relaxed);
#else
  (void)allocator;
#endif
}

void hz6_allocator_remote_pending_inbox_init(Hz6Allocator* allocator) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (!allocator) {
    return;
  }
#if HZ6_REMOTE_PENDING_LAZY_STORAGE_L1
  atomic_flag_clear_explicit(&allocator->remote_pending_storage_lock,
                             memory_order_relaxed);
  atomic_store_explicit(&allocator->remote_pending_storage, NULL,
                        memory_order_relaxed);
  allocator->remote_pending_storage_block = (Hz6OwnerInboxStorageBlock){0};
#else
  hz6_remote_pending_storage_reset(allocator);
#endif
#else
  (void)allocator;
#endif
}

int hz6_allocator_remote_pending_enqueue(Hz6Allocator* allocator,
                                         Hz6ObjectDescriptor* descriptor,
                                         void* ptr,
                                         uint32_t generation,
                                         uint16_t front_id,
                                         uint16_t class_id) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  uint64_t bit = 0;
  uint16_t key_index = 0;
  if (!allocator || !descriptor || !ptr ||
      class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !hz6_remote_pending_key_bit(front_id, class_id, &key_index, &bit)) {
    return 0;
  }
  (void)key_index;
  (void)bit;
	  HZ6_REMOTE_PENDING_STAT_INC(allocator, remote_pending_enqueue_attempt);
	  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
	    HZ6_REMOTE_PENDING_STAT_INC(allocator,
	                                remote_pending_enqueue_after_owner_dying);
	    HZ6_REMOTE_PENDING_STAT_INC(allocator, remote_pending_owner_dead);
	    return 0;
	  }
  uint32_t index = 0;
  if (!hz6_remote_pending_descriptor_index(allocator, descriptor, &index) ||
      descriptor->ptr != ptr || descriptor->generation != generation ||
      descriptor->class_id != class_id ||
      descriptor->state != HZ6_STATE_ACTIVE) {
    HZ6_REMOTE_PENDING_STAT_INC(allocator, remote_pending_publish_fail);
    return 0;
  }
  if (!hz6_allocator_descriptor_owner_equal_at(
          allocator, descriptor, allocator->owner.token,
          HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING)) {
    HZ6_REMOTE_PENDING_STAT_INC(allocator, remote_pending_owner_mismatch);
    return 0;
  }
  if (!hz6_remote_pending_storage_ensure(allocator)) {
    HZ6_REMOTE_PENDING_STAT_INC(allocator, remote_pending_publish_fail);
    return 0;
  }

  Hz6RemotePendingClassInbox* inbox =
      &HZ6_RP_INBOX(allocator)[class_id];
  hz6_remote_pending_lock(inbox);
  if (HZ6_RP_SLOT_STATE(allocator)[index] !=
      HZ6_REMOTE_PENDING_SLOT_NONE) {
    HZ6_REMOTE_PENDING_STAT_INC(allocator, remote_pending_duplicate_claim);
    hz6_remote_pending_unlock(inbox);
    return 0;
  }
  if (inbox->total_count >= HZ6_REMOTE_PENDING_INBOX_CLASS_CAPACITY) {
    HZ6_REMOTE_PENDING_STAT_INC(allocator, remote_pending_enqueue_full);
    hz6_remote_pending_unlock(inbox);
    return 0;
  }
  descriptor->state = HZ6_STATE_REMOTE_PENDING;
  HZ6_RP_PUBLISHED_PTR(allocator)[index] = ptr;
  HZ6_RP_PUBLISHED_GENERATION(allocator)[index] = generation;
  HZ6_RP_PUBLISHED_BYTES(allocator)[index] =
      (uint32_t)descriptor->bytes;
  HZ6_RP_PUBLISHED_FRONT_ID(allocator)[index] = front_id;
  HZ6_RP_PUBLISHED_CLASS_ID(allocator)[index] = class_id;
  HZ6_RP_OWNER_TOKEN(allocator)[index] = allocator->owner.token;
  hz6_remote_pending_requeue_locked(allocator, class_id, index);
  HZ6_REMOTE_PENDING_STAT_INC(allocator, remote_pending_enqueue_success);
  hz6_remote_pending_unlock(inbox);
  return 1;
#else
  (void)allocator;
  (void)descriptor;
  (void)ptr;
  (void)generation;
  (void)front_id;
  (void)class_id;
  return 0;
#endif
}

int hz6_allocator_remote_pending_external_ticket_publish(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr,
    uint32_t generation,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1 && \
    HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
  uint64_t bit = 0;
  uint16_t key_index = 0;
  if (!allocator || !descriptor || !ptr ||
      class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !hz6_remote_pending_key_bit(front_id, class_id, &key_index, &bit)) {
    return 0;
  }
  (void)key_index;
	  HZ6_REMOTE_PENDING_STAT_INC(allocator,
	                              remote_pending_external_ticket_attempt);
	  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
	    HZ6_REMOTE_PENDING_STAT_INC(
	        allocator, remote_pending_enqueue_after_owner_dying);
	    HZ6_REMOTE_PENDING_STAT_INC(
	        allocator, remote_pending_external_ticket_owner_mismatch);
	    return 0;
	  }
	  uint32_t inline_index = 0;
  if (hz6_remote_pending_descriptor_index(allocator, descriptor,
                                          &inline_index)) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_storage_mismatch);
    return 0;
  }
  if (descriptor->ptr != ptr || descriptor->generation != generation ||
      descriptor->class_id != class_id ||
      descriptor->state != HZ6_STATE_ACTIVE) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_state_mismatch);
    return 0;
  }
  if (!hz6_allocator_descriptor_owner_equal_at(
          allocator, descriptor, allocator->owner.token,
          HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING)) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_owner_mismatch);
    return 0;
  }
  Hz6OwnerToken storage_owner_token = {0};
#if HZ6_DESCRIPTOR_STORAGE_OWNER16_L1 || HZ6_DIAGNOSTIC_PROBES || \
    HZ6_OWNER_SOURCE_SIDE_META_DRYRUN || HZ6_OWNER_SOURCE_SIDE_META_L2
  Hz6Allocator* storage =
      hz6_allocator_descriptor_storage_owner(allocator, descriptor, NULL);
  if (!storage) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_storage_mismatch);
    return 0;
  }
  storage_owner_token = storage->owner.token;
#else
  HZ6_REMOTE_PENDING_STAT_INC(
      allocator, remote_pending_external_ticket_storage_mismatch);
  return 0;
#endif
  uint16_t front_index = 0;
  if (!hz6_remote_pending_front_ordinal(front_id, &front_index)) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_state_mismatch);
    return 0;
  }
  if (!hz6_remote_pending_storage_ensure(allocator)) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_state_mismatch);
    return 0;
  }

  hz6_remote_pending_external_lock(allocator);
#if HZ6_REMOTE_PENDING_EXTERNAL_LOCKED_REVALIDATE_L1
  if (descriptor->ptr != ptr || descriptor->generation != generation ||
      descriptor->class_id != class_id ||
      descriptor->state != HZ6_STATE_ACTIVE ||
      !hz6_allocator_descriptor_owner_equal_at(
          allocator, descriptor, allocator->owner.token,
          HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING)) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_locked_revalidate_fail);
    hz6_remote_pending_external_unlock(allocator);
    return 0;
  }
  HZ6_REMOTE_PENDING_STAT_INC(
      allocator, remote_pending_external_ticket_duplicate_scan_skip);
#elif HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
  size_t duplicate_probes = 0;
  uint32_t duplicate_ticket = 0;
  if (hz6_remote_pending_external_dup_index_find(allocator, descriptor,
                                                 generation,
                                                 &duplicate_ticket,
                                                 &duplicate_probes)) {
    (void)duplicate_ticket;
    hz6_remote_pending_external_note_duplicate_probes(allocator,
                                                      duplicate_probes);
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_duplicate);
    hz6_remote_pending_external_unlock(allocator);
    return 0;
  }
  hz6_remote_pending_external_note_duplicate_probes(allocator,
                                                    duplicate_probes);
#else
  size_t duplicate_probes = 0;
  for (size_t i = 0; i < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY; ++i) {
    ++duplicate_probes;
    Hz6RemotePendingExternalTicket* ticket =
        &HZ6_RP_EXTERNAL_TICKETS(allocator)[i];
    if (ticket->state != HZ6_REMOTE_PENDING_SLOT_NONE &&
        ticket->descriptor == descriptor &&
        ticket->generation == generation) {
      hz6_remote_pending_external_note_duplicate_probes(allocator,
                                                        duplicate_probes);
      HZ6_REMOTE_PENDING_STAT_INC(
          allocator, remote_pending_external_ticket_duplicate);
      hz6_remote_pending_external_unlock(allocator);
      return 0;
    }
  }
  hz6_remote_pending_external_note_duplicate_probes(allocator,
                                                    duplicate_probes);
#endif
  uint32_t ticket_index = HZ6_RP_EXTERNAL_FREE_HEAD(allocator);
  if (ticket_index == HZ6_REMOTE_PENDING_INDEX_NONE ||
      ticket_index >= HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY) {
    HZ6_REMOTE_PENDING_STAT_INC(allocator,
                                remote_pending_external_ticket_full);
    hz6_remote_pending_external_unlock(allocator);
    return 0;
  }
  Hz6RemotePendingExternalTicket* ticket =
      &HZ6_RP_EXTERNAL_TICKETS(allocator)[ticket_index];
  HZ6_RP_EXTERNAL_FREE_HEAD(allocator) = ticket->next;
  ticket->ptr = ptr;
  ticket->descriptor = descriptor;
  ticket->generation = generation;
  ticket->bytes = descriptor->bytes;
  ticket->owner_token = allocator->owner.token;
  ticket->descriptor_storage_owner_token = storage_owner_token;
  ticket->front_id = front_id;
  ticket->class_id = class_id;
  ticket->state = HZ6_REMOTE_PENDING_SLOT_QUEUED;
  ticket->next = HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id];
  HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id] =
      ticket_index;
  atomic_fetch_or_explicit(&HZ6_RP_EXTERNAL_NONEMPTY_MASK(allocator), bit,
                           memory_order_relaxed);
#if HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
  if (!hz6_remote_pending_external_dup_index_insert(allocator, ticket_index)) {
    HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id] =
        ticket->next;
    if (HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id] ==
        HZ6_REMOTE_PENDING_INDEX_NONE &&
        hz6_remote_pending_key_bit(front_id, class_id, NULL, &bit)) {
      atomic_fetch_and_explicit(&HZ6_RP_EXTERNAL_NONEMPTY_MASK(allocator),
                                ~bit, memory_order_relaxed);
    }
    memset(ticket, 0, sizeof(*ticket));
    ticket->next = HZ6_RP_EXTERNAL_FREE_HEAD(allocator);
    HZ6_RP_EXTERNAL_FREE_HEAD(allocator) = ticket_index;
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_duplicate_index_stale);
    hz6_remote_pending_external_unlock(allocator);
    return 0;
  }
#endif
  descriptor->state = HZ6_STATE_REMOTE_PENDING;
  HZ6_REMOTE_PENDING_STAT_INC(allocator,
                              remote_pending_external_ticket_success);
  hz6_remote_pending_external_note_enqueue(allocator);
  hz6_remote_pending_external_unlock(allocator);
  return 1;
#else
  (void)allocator;
  (void)descriptor;
  (void)ptr;
  (void)generation;
  (void)front_id;
  (void)class_id;
  return 0;
#endif
}

int hz6_allocator_remote_pending_external_ticket_consume_one(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1 && \
    HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
  if (!hz6_remote_pending_storage_ready(allocator) ||
      class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      hz6_allocator_frontcache_count(allocator, class_id) >=
          hz6_allocator_frontcache_capacity(allocator, class_id)) {
    if (allocator && class_id < HZ6_FRONT_CACHE_CLASS_COUNT) {
      ++allocator->stats.remote_pending_external_ticket_frontcache_full;
    }
    return 0;
  }
  uint16_t front_index = 0;
  if (!hz6_remote_pending_front_ordinal(front_id, &front_index)) {
    return 0;
  }

  hz6_remote_pending_external_lock(allocator);
  uint32_t ticket_index =
      HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id];
  if (ticket_index == HZ6_REMOTE_PENDING_INDEX_NONE ||
      ticket_index >= HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY) {
    ++allocator->stats.remote_pending_external_ticket_consume_empty;
    hz6_remote_pending_external_unlock(allocator);
    return 0;
  }
  Hz6RemotePendingExternalTicket* ticket =
      &HZ6_RP_EXTERNAL_TICKETS(allocator)[ticket_index];
  Hz6RemotePendingExternalTicket proof = *ticket;
  HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id] =
      ticket->next;
  uint64_t bit = 0;
  if (HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id] ==
          HZ6_REMOTE_PENDING_INDEX_NONE &&
      hz6_remote_pending_key_bit(front_id, class_id, NULL, &bit)) {
    atomic_fetch_and_explicit(&HZ6_RP_EXTERNAL_NONEMPTY_MASK(allocator), ~bit,
                              memory_order_relaxed);
  }
  ticket->state = HZ6_REMOTE_PENDING_SLOT_CLAIMED;
  hz6_remote_pending_external_unlock(allocator);

  Hz6ObjectDescriptor* descriptor = proof.descriptor;
  int valid = descriptor && proof.ptr &&
              proof.front_id == front_id && proof.class_id == class_id &&
              descriptor->ptr == proof.ptr &&
              descriptor->generation == proof.generation &&
              descriptor->bytes == proof.bytes &&
              descriptor->class_id == class_id &&
              descriptor->state == HZ6_STATE_REMOTE_PENDING;
  if (!valid) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_state_mismatch);
  }
  if (valid && !hz6_owner_is_alive(&allocator->owner, proof.owner_token)) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_owner_mismatch);
    valid = 0;
  }
  if (valid && !hz6_allocator_descriptor_owner_equal_at(
                   allocator, descriptor, proof.owner_token,
                   HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING)) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_owner_mismatch);
    valid = 0;
  }
#if HZ6_DESCRIPTOR_STORAGE_OWNER16_L1 || HZ6_DIAGNOSTIC_PROBES || \
    HZ6_OWNER_SOURCE_SIDE_META_DRYRUN || HZ6_OWNER_SOURCE_SIDE_META_L2
  if (valid) {
    Hz6Allocator* storage =
        hz6_allocator_descriptor_storage_owner(allocator, descriptor, NULL);
    if (!storage ||
        !hz6_owner_equal(storage->owner.token,
                         proof.descriptor_storage_owner_token)) {
      HZ6_REMOTE_PENDING_STAT_INC(
          allocator, remote_pending_external_ticket_storage_mismatch);
      valid = 0;
    }
  }
#else
  if (valid) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_storage_mismatch);
    valid = 0;
  }
#endif
  if (valid) {
    Hz6RouteResult route =
        hz6_allocator_route_lookup_exact(allocator, proof.ptr);
    if (route.kind != HZ6_ROUTE_VALID || route.descriptor != descriptor ||
        route.generation != proof.generation || route.front_id != front_id ||
        route.class_id != class_id || route.route_allocator != allocator) {
      HZ6_REMOTE_PENDING_STAT_INC(
          allocator, remote_pending_external_ticket_route_mismatch);
      valid = 0;
    }
  }

  if (!valid) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_integrity_abort);
    abort();
  }

  hz6_remote_pending_external_lock(allocator);
  ticket = &HZ6_RP_EXTERNAL_TICKETS(allocator)[ticket_index];
  Hz6FrontCacheEntry front_entry = {0};
  front_entry.ptr = proof.ptr;
  front_entry.descriptor = descriptor;
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  front_entry.source_block = descriptor->source_block;
#endif
  front_entry.generation = proof.generation;
  hz6_frontcache_entry_set_bytes(&front_entry, descriptor->bytes);
  hz6_frontcache_entry_set_class_id(&front_entry, class_id);
  if (!hz6_allocator_frontcache_push(allocator, class_id, front_entry)) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_frontcache_full);
    ticket->state = HZ6_REMOTE_PENDING_SLOT_QUEUED;
    ticket->next =
        HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id];
    HZ6_RP_EXTERNAL_HEAD(allocator)[front_index][class_id] =
        ticket_index;
    if (hz6_remote_pending_key_bit(front_id, class_id, NULL, &bit)) {
      atomic_fetch_or_explicit(&HZ6_RP_EXTERNAL_NONEMPTY_MASK(allocator),
                               bit, memory_order_relaxed);
    }
    hz6_remote_pending_external_unlock(allocator);
    return 0;
  }
  descriptor->state = HZ6_STATE_LOCAL_FREE;
#if HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
  if (!hz6_remote_pending_external_dup_index_remove(
          allocator, ticket_index, proof.descriptor, proof.generation)) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_external_ticket_duplicate_index_stale);
  }
#endif
  memset(ticket, 0, sizeof(*ticket));
  ticket->next = HZ6_RP_EXTERNAL_FREE_HEAD(allocator);
  HZ6_RP_EXTERNAL_FREE_HEAD(allocator) = ticket_index;
  HZ6_REMOTE_PENDING_STAT_INC(allocator,
                              remote_pending_external_ticket_consume);
  hz6_remote_pending_external_note_remove(allocator);
  hz6_remote_pending_external_unlock(allocator);
  return 1;
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  return 0;
#endif
}

size_t hz6_allocator_remote_pending_maintenance_class(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t budget) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT || budget == 0) {
    return 0;
  }
  int inline_gate_hit = hz6_allocator_remote_pending_key_maybe_nonempty_raw(
      allocator, front_id, class_id);
  int external_gate_hit =
      hz6_remote_pending_external_key_nonempty(allocator, front_id, class_id);
  if (!inline_gate_hit && !external_gate_hit) {
    HZ6_REMOTE_PENDING_STAT_INC(allocator,
                                remote_pending_maintenance_entry_gate_miss);
    return 0;
  }
  if (inline_gate_hit) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_maintenance_inline_gate_hit);
  }
  if (external_gate_hit) {
    HZ6_REMOTE_PENDING_STAT_INC(
        allocator, remote_pending_maintenance_external_gate_hit);
  }
  ++allocator->stats.remote_pending_maintenance_check;
  if (atomic_load_explicit(&HZ6_RP_CURRENT(allocator),
                           memory_order_relaxed) == 0 &&
      !external_gate_hit) {
    ++allocator->stats.remote_pending_maintenance_key_race;
    return 0;
  }
  ++allocator->stats.remote_pending_maintenance_armed;
  ++allocator->stats.remote_pending_batch_call;
  size_t drained = 0;
  int external_nonempty = external_gate_hit;
  while (drained < budget) {
    if (hz6_allocator_frontcache_count(allocator, class_id) >=
        hz6_allocator_frontcache_capacity(allocator, class_id)) {
      ++allocator->stats.remote_pending_maintenance_frontcache_full_stop;
      ++allocator->stats.remote_pending_frontcache_full;
      break;
    }
    if (external_nonempty) {
      if (hz6_allocator_remote_pending_external_ticket_consume_one(
          allocator, front_id, class_id)) {
        ++drained;
        external_nonempty = hz6_remote_pending_external_key_nonempty(
            allocator, front_id, class_id);
        continue;
      }
      ++allocator->stats.remote_pending_maintenance_external_miss;
      external_nonempty = 0;
    }
    Hz6RemotePendingInboxEntry entry = {0};
    if (!hz6_remote_pending_pop_key(allocator, front_id, class_id, &entry)) {
      ++allocator->stats.remote_pending_maintenance_inline_empty;
      break;
    }
    Hz6ObjectDescriptor* descriptor = entry.descriptor;
    int valid = descriptor && descriptor->ptr == entry.ptr &&
                descriptor->generation == entry.generation &&
                descriptor->bytes == entry.bytes &&
                descriptor->class_id == entry.class_id &&
                entry.front_id == front_id &&
                entry.class_id == class_id;
    if (valid && descriptor->state != HZ6_STATE_REMOTE_PENDING) {
      ++allocator->stats.remote_pending_state_mismatch;
      valid = 0;
    }
    if (valid && !hz6_allocator_descriptor_owner_equal_at(
                     allocator, descriptor, entry.owner_token,
                     HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING)) {
      ++allocator->stats.remote_pending_owner_mismatch;
      valid = 0;
    }
    if (valid) {
      Hz6RouteResult route =
          hz6_allocator_route_lookup_exact(allocator, entry.ptr);
      if (route.kind != HZ6_ROUTE_VALID || route.descriptor != descriptor ||
          route.generation != entry.generation || route.class_id != class_id ||
          route.front_id != entry.front_id ||
          route.route_allocator != allocator) {
        ++allocator->stats.remote_pending_route_mismatch;
        valid = 0;
      }
    }
    if (!valid) {
      if (descriptor && descriptor->generation != entry.generation) {
        ++allocator->stats.remote_pending_generation_mismatch;
      }
      continue;
    }

    Hz6FrontCacheEntry front_entry = {0};
    front_entry.ptr = entry.ptr;
    front_entry.descriptor = descriptor;
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
    front_entry.source_block = descriptor->source_block;
#endif
    front_entry.generation = entry.generation;
    hz6_frontcache_entry_set_bytes(&front_entry, descriptor->bytes);
    hz6_frontcache_entry_set_class_id(&front_entry, class_id);
    if (!hz6_allocator_frontcache_push(allocator, class_id, front_entry)) {
      hz6_remote_pending_requeue_entry(allocator, &entry);
      ++allocator->stats.remote_pending_frontcache_full;
      break;
    }
    descriptor->state = HZ6_STATE_LOCAL_FREE;
    ++allocator->stats.remote_pending_frontcache_push;
    ++drained;
  }
  if (drained == 0) {
    ++allocator->stats.remote_pending_maintenance_noop;
  }
  allocator->stats.remote_pending_batch_items += drained;
  return drained;
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  (void)budget;
  return 0;
#endif
}

Hz6RemotePendingReuseStatus hz6_allocator_remote_pending_try_reuse(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t requested_bytes,
    void** out_ptr,
    Hz6ObjectDescriptor** out_descriptor) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (out_ptr) {
    *out_ptr = NULL;
  }
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  Hz6RemotePendingClaim claim;
  Hz6RemotePendingReuseStatus status = hz6_remote_pending_try_claim_exact_key(
      allocator, front_id, class_id, 1, &claim);
  if (status != HZ6_REMOTE_PENDING_REUSE_OK) {
    return status;
  }

  return hz6_remote_pending_finish_claim_reuse(
      allocator, front_id, class_id, requested_bytes, &claim, out_ptr,
      out_descriptor);
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  (void)requested_bytes;
  if (out_ptr) {
    *out_ptr = NULL;
  }
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  return HZ6_REMOTE_PENDING_REUSE_EMPTY;
#endif
}

Hz6RemotePendingReuseStatus
hz6_allocator_remote_pending_try_reuse_known_nonempty(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t requested_bytes,
    void** out_ptr,
    Hz6ObjectDescriptor** out_descriptor) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (out_ptr) {
    *out_ptr = NULL;
  }
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  Hz6RemotePendingClaim claim;
  Hz6RemotePendingReuseStatus status = hz6_remote_pending_try_claim_exact_key(
      allocator, front_id, class_id, 0, &claim);
  if (status != HZ6_REMOTE_PENDING_REUSE_OK) {
    return status;
  }

  return hz6_remote_pending_finish_claim_reuse(
      allocator, front_id, class_id, requested_bytes, &claim, out_ptr,
      out_descriptor);
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  (void)requested_bytes;
  if (out_ptr) {
    *out_ptr = NULL;
  }
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  return HZ6_REMOTE_PENDING_REUSE_EMPTY;
#endif
}

int hz6_allocator_remote_pending_key_maybe_nonempty_raw(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (!allocator) {
    return 0;
  }
  if (!hz6_remote_pending_storage_ready(allocator)) {
    return 0;
  }
  uint64_t bit = 0;
  if (!hz6_remote_pending_key_bit(front_id, class_id, NULL, &bit)) {
    return 0;
  }
  return (atomic_load_explicit(&HZ6_RP_NONEMPTY_MASK(allocator),
                               memory_order_relaxed) &
          bit) != 0;
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  return 0;
#endif
}

int hz6_allocator_remote_pending_key_nonempty(Hz6Allocator* allocator,
                                              uint16_t front_id,
                                              uint16_t class_id) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (!allocator) {
    return 0;
  }
  ++allocator->stats.remote_pending_key_nonempty_load;
  int hit = hz6_allocator_remote_pending_key_maybe_nonempty_raw(
                allocator, front_id, class_id) ||
            hz6_remote_pending_external_key_nonempty(allocator, front_id,
                                                     class_id);
  if (hit) {
    ++allocator->stats.remote_pending_key_nonempty_hit;
  }
  return hit;
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  return 0;
#endif
}

void hz6_allocator_remote_pending_note_frontcache_miss(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (hz6_allocator_remote_pending_key_nonempty(allocator, front_id,
                                                class_id)) {
    ++allocator->stats.pending_same_key_on_frontcache_miss;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

void hz6_allocator_remote_pending_note_front_dispatch(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (hz6_allocator_remote_pending_key_nonempty(allocator, front_id,
                                                class_id)) {
    ++allocator->stats.pending_same_key_on_front_dispatch;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

void hz6_allocator_remote_pending_note_source_alloc(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (hz6_allocator_remote_pending_key_nonempty(allocator, front_id,
                                                class_id)) {
    ++allocator->stats.pending_same_key_on_source_alloc;
    ++allocator->stats.source_alloc_with_matching_pending;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

void hz6_allocator_remote_pending_note_before_maintenance(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_V2_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (hz6_allocator_remote_pending_key_nonempty(allocator, front_id,
                                                class_id)) {
    ++allocator->stats.pending_same_key_before_maintenance;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

void hz6_allocator_remote_pending_note_after_maintenance(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_V2_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (hz6_allocator_remote_pending_key_nonempty(allocator, front_id,
                                                class_id)) {
    ++allocator->stats.pending_same_key_after_maintenance;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

void hz6_allocator_remote_pending_note_maintenance_reuse_success(
    Hz6Allocator* allocator,
    size_t drained) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_V2_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (!allocator || drained == 0) {
    return;
  }
  ++allocator->stats.pending_maintenance_immediate_reuse_success;
  if (drained > 1u) {
    allocator->stats.pending_maintenance_batch_surplus += drained - 1u;
  }
#else
  (void)allocator;
  (void)drained;
#endif
}

void hz6_allocator_remote_pending_note_prefill_attempt(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_V2_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  int nonempty =
      hz6_allocator_remote_pending_key_nonempty(allocator, front_id,
                                                class_id);
  ++allocator->stats.remote_pending_direct_source_boundary_attempt;
  if (nonempty) {
    ++allocator->stats.remote_pending_direct_source_boundary_gate_hit;
    ++allocator->stats.pending_same_key_on_prefill_attempt;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

void hz6_allocator_remote_pending_note_prefill_commit(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_V2_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  int inline_hit = hz6_allocator_remote_pending_key_maybe_nonempty_raw(
      allocator, front_id, class_id);
  int external_hit =
      hz6_remote_pending_external_key_nonempty(allocator, front_id, class_id);
  ++allocator->stats.remote_pending_key_nonempty_load;
  if (inline_hit || external_hit) {
    ++allocator->stats.remote_pending_key_nonempty_hit;
    ++allocator->stats.prefill_commit_with_matching_pending;
    if (inline_hit) {
      ++allocator->stats.prefill_commit_with_inline_pending;
    }
    if (external_hit) {
      ++allocator->stats.prefill_commit_with_external_pending;
    }
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

void hz6_allocator_remote_pending_note_source_block_commit(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_V2_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  int inline_hit = hz6_allocator_remote_pending_key_maybe_nonempty_raw(
      allocator, front_id, class_id);
  int external_hit =
      hz6_remote_pending_external_key_nonempty(allocator, front_id, class_id);
  ++allocator->stats.remote_pending_key_nonempty_load;
  if (inline_hit || external_hit) {
    ++allocator->stats.remote_pending_key_nonempty_hit;
    ++allocator->stats.source_block_commit_with_matching_pending;
    if (inline_hit) {
      ++allocator->stats.source_block_commit_with_inline_pending;
    }
    if (external_hit) {
      ++allocator->stats.source_block_commit_with_external_pending;
    }
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

void hz6_allocator_remote_pending_note_direct_source_attempt(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_V2_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  int nonempty =
      hz6_allocator_remote_pending_key_nonempty(allocator, front_id,
                                                class_id);
  ++allocator->stats.remote_pending_direct_source_boundary_attempt;
  if (nonempty) {
    ++allocator->stats.remote_pending_direct_source_boundary_gate_hit;
    ++allocator->stats.direct_source_attempt_with_matching_pending;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

void hz6_allocator_remote_pending_note_direct_source_commit(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_REUSE_DEMAND_AUDIT_V2_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (hz6_allocator_remote_pending_key_nonempty(allocator, front_id,
                                                class_id)) {
    ++allocator->stats.direct_source_commit_with_matching_pending;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}
