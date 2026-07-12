#include "h8_medium_page8k_remote.h"

#include "h8_medium_domain_shadow.h"
#include "h8_platform.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#if defined(H8_MEDIUM_PAGE8K_REMOTE_L1)

#define H8_PAGE8K_BYTES 8192u
#define H8_PAGE8K_RUN_BYTES 65536u
#define H8_PAGE8K_SLOT_COUNT 8u
#define H8_PAGE8K_REGISTRY_CAP 8192u
#define H8_PAGE8K_OWNER_PAGE_CAP 64u
#define H8_PAGE8K_ORPHAN_RESIDENT_EMPTY_CAP 16u

typedef enum H8Page8KSlotState {
  H8_PAGE8K_SLOT_FREE = 0,
  H8_PAGE8K_SLOT_ALLOCATED = 1,
  H8_PAGE8K_SLOT_REMOTE_PENDING = 2
} H8Page8KSlotState;

typedef enum H8Page8KQueueState {
  H8_PAGE8K_Q_IDLE = 0,
  H8_PAGE8K_Q_QUEUED = 1,
  H8_PAGE8K_Q_DRAINING = 2,
  H8_PAGE8K_Q_DRAINING_DIRTY = 3
} H8Page8KQueueState;

typedef enum H8Page8KOwnerState {
  H8_PAGE8K_OWNER_ALIVE = 0,
  H8_PAGE8K_OWNER_CLOSING = 1,
  H8_PAGE8K_OWNER_DEAD = 2
} H8Page8KOwnerState;

struct H8Page8KRemoteOwner {
  _Atomic uint32_t token;
  _Atomic uint8_t state;
  bool permanent;
  _Atomic(H8Page8KRemotePage*) inbox_head;
  _Atomic size_t inbox_depth;
  H8Page8KRemotePage* pages;
  size_t page_count;
  H8Page8KRemotePage* active_page;
  H8Page8KRemotePage* available_head;
  size_t resident_empty_pages;
  H8Page8KRemoteOwner* global_next;
  H8Page8KRemoteOwner* free_next;
};

struct H8Page8KRemotePage {
  uint8_t* base;
  _Atomic(H8Page8KRemoteOwner*) owner;
  _Atomic uint32_t owner_token;
  _Atomic uint8_t publish_closed;
  _Atomic uint16_t publish_refs;
  uint8_t free_head;
  uint8_t free_next[H8_PAGE8K_SLOT_COUNT];
  _Atomic uint8_t slot_state[H8_PAGE8K_SLOT_COUNT];
  _Atomic uint8_t qstate;
  _Atomic uint8_t pending_mask;
  H8Page8KRemotePage* inbox_next;
  H8Page8KRemotePage* owner_next;
  H8Page8KRemotePage* available_next;
  bool available_indexed;
  bool resident;
  bool orphan_resident_counted;
};

static h8_platform_mutex_t g_owner_list_lock = H8_PLATFORM_MUTEX_INIT;
static h8_platform_mutex_t g_transition_lock = H8_PLATFORM_MUTEX_INIT;
static _Atomic(H8Page8KRemotePage*) g_registry[H8_PAGE8K_REGISTRY_CAP];
static H8Page8KRemoteOwner* g_owner_list;
static H8Page8KRemoteOwner* g_owner_free_list;
static H8Page8KRemoteOwner* g_orphan_owner;
static _Atomic uint32_t g_next_owner_token = 1u;
static _Thread_local H8Page8KRemoteOwner* g_current_owner;
#if defined(H8_PAGE8K_REMOTE_DIAGNOSTIC)
static _Atomic uint64_t g_dispatch_alloc_attempt;
static _Atomic uint64_t g_dispatch_alloc_served;
static _Atomic uint64_t g_dispatch_free_attempt;
static _Atomic uint64_t g_dispatch_free_owner_present;
static _Atomic uint64_t g_dispatch_free_owned;
static _Atomic uint64_t g_dispatch_free_success;
static _Atomic uint64_t g_dispatch_free_miss;
static _Atomic uint64_t g_owner_create;
static _Atomic uint64_t g_range_eligible_alloc;
static _Atomic uint64_t g_range_served_alloc;
static _Atomic uint64_t g_remote_claim_attempt;
static _Atomic uint64_t g_remote_claim_success;
static _Atomic uint64_t g_remote_claim_reject;
static _Atomic uint64_t g_pending_publish;
static _Atomic uint64_t g_queue_notify;
static _Atomic uint64_t g_queue_dirty;
static _Atomic uint64_t g_drain_pages;
static _Atomic uint64_t g_drain_slots;
static _Atomic uint64_t g_duplicate_reject;
static _Atomic uint64_t g_interior_reject;
static _Atomic uint64_t g_lost_notification;
static _Atomic uint64_t g_inbox_depth;
static _Atomic uint64_t g_inbox_depth_max;
static _Atomic uint64_t g_page_cap_reject;
static _Atomic uint64_t g_drain_all_owner_visits;
static _Atomic uint64_t g_drain_all_limit;
static _Atomic uint64_t g_drain_all_skipped_live;
static _Atomic uint64_t g_owner_close;
static _Atomic uint64_t g_orphan_adopt;
static _Atomic uint64_t g_publish_retry;
static _Atomic uint64_t g_page_decommit;
static _Atomic uint64_t g_page_recommit;
static _Atomic uint64_t g_page_decommit_fail;
static _Atomic uint64_t g_orphan_resident_empty;
static _Atomic uint64_t g_orphan_resident_empty_max;
#endif

#if defined(H8_PAGE8K_REMOTE_DIAGNOSTIC)
#define H8_PAGE8K_STAT_INC(field) \
  atomic_fetch_add_explicit(&g_##field, 1u, memory_order_relaxed)
#define H8_PAGE8K_STAT_ADD(field, value) \
  atomic_fetch_add_explicit(&g_##field, (value), memory_order_relaxed)
#define H8_PAGE8K_STAT_STORE(field, value) \
  atomic_store_explicit(&g_##field, (value), memory_order_relaxed)
static void h8_page8k_stat_max(_Atomic uint64_t* target, uint64_t value) {
  uint64_t old = atomic_load_explicit(target, memory_order_relaxed);
  while (old < value && !atomic_compare_exchange_weak_explicit(
                            target, &old, value, memory_order_relaxed,
                            memory_order_relaxed)) {
  }
}
#define H8_PAGE8K_STAT_MAX(field, value) h8_page8k_stat_max(&g_##field, (value))
#else
#define H8_PAGE8K_STAT_INC(field) ((void)0)
#define H8_PAGE8K_STAT_ADD(field, value) ((void)0)
#define H8_PAGE8K_STAT_STORE(field, value) ((void)0)
#define H8_PAGE8K_STAT_MAX(field, value) ((void)0)
#endif

static bool h8_page8k_registry_add(H8Page8KRemotePage* page) {
  bool added = false;
  size_t mask = H8_PAGE8K_REGISTRY_CAP - 1u;
  size_t start = ((uintptr_t)page->base >> 16u) & mask;
  for (size_t probe = 0; probe < H8_PAGE8K_REGISTRY_CAP; ++probe) {
    size_t index = (start + probe) & mask;
    H8Page8KRemotePage* expected = NULL;
    if (atomic_compare_exchange_strong_explicit(
            &g_registry[index], &expected, page, memory_order_release,
            memory_order_relaxed)) {
      added = true;
      break;
    }
  }
  return added;
}

static H8Page8KRemotePage* h8_page8k_classify(const void* ptr) {
  uintptr_t address = (uintptr_t)ptr;
  H8Page8KRemotePage* result = NULL;
  uintptr_t base = address & ~((uintptr_t)H8_PAGE8K_RUN_BYTES - 1u);
  size_t mask = H8_PAGE8K_REGISTRY_CAP - 1u;
  size_t start = (base >> 16u) & mask;
  for (size_t probe = 0; probe < H8_PAGE8K_REGISTRY_CAP; ++probe) {
    H8Page8KRemotePage* page = atomic_load_explicit(
        &g_registry[(start + probe) & mask], memory_order_acquire);
    if (!page) break;
    if ((uintptr_t)page->base == base) {
      result = page;
      break;
    }
  }
  return result;
}

static bool h8_page8k_slot_index(const H8Page8KRemotePage* page,
                                 const void* ptr, uint8_t* slot_out) {
  uintptr_t offset = (uintptr_t)ptr - (uintptr_t)page->base;
  if ((offset % H8_PAGE8K_BYTES) != 0u) return false;
  uint8_t slot = (uint8_t)(offset / H8_PAGE8K_BYTES);
  if (slot >= H8_PAGE8K_SLOT_COUNT) return false;
  if (slot_out) *slot_out = slot;
  return true;
}

static H8RouteKind h8_page8k_route(const void* ptr, bool* owned_out) {
  if (owned_out) *owned_out = false;
  H8Page8KRemotePage* page = h8_page8k_classify(ptr);
  if (!page) return H8_ROUTE_MISS;
  if (owned_out) *owned_out = true;
  uint8_t slot = 0;
  if (!h8_page8k_slot_index(page, ptr, &slot)) return H8_ROUTE_INVALID;
  uint8_t state = atomic_load_explicit(&page->slot_state[slot],
                                       memory_order_acquire);
  return state == H8_PAGE8K_SLOT_ALLOCATED ? H8_ROUTE_VALID
                                           : H8_ROUTE_INVALID;
}

H8RouteKind h8_page8k_remote_route_current(const void* ptr) {
  return h8_page8k_route(ptr, NULL);
}

bool h8_page8k_remote_usable_size_current(const void* ptr, size_t* usable_out,
                                          bool* owned_out) {
  if (usable_out) *usable_out = 0;
  H8RouteKind route = h8_page8k_route(ptr, owned_out);
  if (route != H8_ROUTE_VALID) return false;
  if (usable_out) *usable_out = H8_PAGE8K_BYTES;
  return true;
}

static H8Page8KRemoteOwner* h8_page8k_page_owner(
    const H8Page8KRemotePage* page) {
  return atomic_load_explicit(&page->owner, memory_order_acquire);
}

#include "h8_medium_page8k_residency.inc"

static void h8_page8k_queue_push(H8Page8KRemoteOwner* owner,
                                 H8Page8KRemotePage* page) {
  H8Page8KRemotePage* head = atomic_load_explicit(
      &owner->inbox_head, memory_order_relaxed);
  do {
    page->inbox_next = head;
  } while (!atomic_compare_exchange_weak_explicit(
      &owner->inbox_head, &head, page, memory_order_release,
      memory_order_relaxed));
  size_t depth =
      atomic_fetch_add_explicit(&owner->inbox_depth, 1u, memory_order_relaxed) +
      1u;
  (void)depth;
  H8_PAGE8K_STAT_STORE(inbox_depth, depth);
  H8_PAGE8K_STAT_MAX(inbox_depth_max, depth);
}

static void h8_page8k_queue(H8Page8KRemotePage* page) {
  H8Page8KRemoteOwner* owner = h8_page8k_page_owner(page);
  uint8_t state = atomic_load_explicit(&page->qstate, memory_order_acquire);
  for (;;) {
    if (state == H8_PAGE8K_Q_IDLE) {
      uint8_t expected = H8_PAGE8K_Q_IDLE;
      if (atomic_compare_exchange_weak_explicit(
              &page->qstate, &expected, H8_PAGE8K_Q_QUEUED,
              memory_order_acq_rel, memory_order_acquire)) {
        h8_page8k_queue_push(owner, page);
        H8_PAGE8K_STAT_INC(queue_notify);
        return;
      }
      state = expected;
      continue;
    }
    if (state == H8_PAGE8K_Q_DRAINING) {
      uint8_t expected = H8_PAGE8K_Q_DRAINING;
      if (atomic_compare_exchange_weak_explicit(
              &page->qstate, &expected, H8_PAGE8K_Q_DRAINING_DIRTY,
              memory_order_acq_rel, memory_order_acquire)) {
        H8_PAGE8K_STAT_INC(queue_dirty);
        return;
      }
      state = expected;
      continue;
    }
    return;
  }
}

/* Owner-only index. Remote publishers never mutate these links. */
static void h8_page8k_available_push(H8Page8KRemotePage* page) {
  H8Page8KRemoteOwner* owner = h8_page8k_page_owner(page);
  if (owner->active_page == page || page->available_indexed ||
      page->free_head == UINT8_MAX)
    return;
  page->available_next = owner->available_head;
  owner->available_head = page;
  page->available_indexed = true;
}

static H8Page8KRemotePage* h8_page8k_available_pop(
    H8Page8KRemoteOwner* owner) {
  H8Page8KRemotePage* page = owner->available_head;
  if (!page) return NULL;
  owner->available_head = page->available_next;
  page->available_next = NULL;
  page->available_indexed = false;
  return page;
}

static void h8_page8k_available_remove(H8Page8KRemoteOwner* owner,
                                       H8Page8KRemotePage* page) {
  if (!page->available_indexed) return;
  H8Page8KRemotePage** link = &owner->available_head;
  while (*link && *link != page) link = &(*link)->available_next;
  if (*link == page) *link = page->available_next;
  page->available_next = NULL;
  page->available_indexed = false;
}

static void h8_page8k_owner_prepare(H8Page8KRemoteOwner* owner,
                                    uint32_t token, bool permanent) {
  atomic_store_explicit(&owner->token, token, memory_order_relaxed);
  atomic_store_explicit(&owner->state, H8_PAGE8K_OWNER_ALIVE,
                        memory_order_release);
  owner->permanent = permanent;
  atomic_store_explicit(&owner->inbox_head, NULL, memory_order_relaxed);
  atomic_store_explicit(&owner->inbox_depth, 0u, memory_order_relaxed);
  owner->pages = NULL;
  owner->page_count = 0u;
  owner->active_page = NULL;
  owner->available_head = NULL;
  owner->resident_empty_pages = 0u;
  owner->global_next = NULL;
  owner->free_next = NULL;
}

static H8Page8KRemoteOwner* h8_page8k_orphan_owner(void) {
  h8_platform_mutex_lock(&g_owner_list_lock);
  if (!g_orphan_owner) {
    H8Page8KRemoteOwner* owner = calloc(1u, sizeof(*owner));
    if (owner) {
      atomic_init(&owner->inbox_head, NULL);
      atomic_init(&owner->inbox_depth, 0u);
      atomic_init(&owner->token, UINT32_MAX);
      atomic_init(&owner->state, H8_PAGE8K_OWNER_ALIVE);
      h8_page8k_owner_prepare(owner, UINT32_MAX, true);
      owner->global_next = g_owner_list;
      g_owner_list = owner;
      g_orphan_owner = owner;
    }
  }
  H8Page8KRemoteOwner* owner = g_orphan_owner;
  h8_platform_mutex_unlock(&g_owner_list_lock);
  return owner;
}

static H8Page8KRemoteOwner* h8_page8k_publish_acquire(
    H8Page8KRemotePage* page) {
  for (;;) {
    if (atomic_load_explicit(&page->publish_closed, memory_order_seq_cst) != 0u) {
      H8_PAGE8K_STAT_INC(publish_retry);
      h8_platform_yield();
      continue;
    }
    uint32_t token =
        atomic_load_explicit(&page->owner_token, memory_order_acquire);
    H8Page8KRemoteOwner* owner = h8_page8k_page_owner(page);
    if (!owner || atomic_load_explicit(&owner->state, memory_order_acquire) !=
                      H8_PAGE8K_OWNER_ALIVE ||
        atomic_load_explicit(&owner->token, memory_order_acquire) != token) {
      H8_PAGE8K_STAT_INC(publish_retry);
      h8_platform_yield();
      continue;
    }
    atomic_fetch_add_explicit(&page->publish_refs, 1u, memory_order_seq_cst);
    atomic_thread_fence(memory_order_seq_cst);
    if (atomic_load_explicit(&page->publish_closed, memory_order_seq_cst) == 0u &&
        h8_page8k_page_owner(page) == owner &&
        atomic_load_explicit(&page->owner_token, memory_order_acquire) == token &&
        atomic_load_explicit(&owner->token, memory_order_acquire) == token &&
        atomic_load_explicit(&owner->state, memory_order_acquire) ==
            H8_PAGE8K_OWNER_ALIVE)
      return owner;
    atomic_fetch_sub_explicit(&page->publish_refs, 1u, memory_order_seq_cst);
    H8_PAGE8K_STAT_INC(publish_retry);
  }
}

static void h8_page8k_publish_release(H8Page8KRemotePage* page) {
  atomic_fetch_sub_explicit(&page->publish_refs, 1u, memory_order_seq_cst);
}

H8Page8KRemoteOwner* h8_page8k_remote_owner_create(uint32_t token) {
  h8_platform_mutex_lock(&g_owner_list_lock);
  H8Page8KRemoteOwner* owner = g_owner_free_list;
  if (owner) {
    g_owner_free_list = owner->free_next;
    owner->free_next = NULL;
  }
  h8_platform_mutex_unlock(&g_owner_list_lock);
  if (!owner) {
    owner = calloc(1u, sizeof(*owner));
    if (owner) {
      atomic_init(&owner->token, token);
      atomic_init(&owner->state, H8_PAGE8K_OWNER_ALIVE);
      atomic_init(&owner->inbox_head, NULL);
      atomic_init(&owner->inbox_depth, 0u);
    }
  }
  if (!owner) {
    return NULL;
  }
  h8_page8k_owner_prepare(owner, token, false);
  h8_platform_mutex_lock(&g_owner_list_lock);
  owner->global_next = g_owner_list;
  g_owner_list = owner;
  h8_platform_mutex_unlock(&g_owner_list_lock);
  return owner;
}

void h8_page8k_remote_owner_destroy(H8Page8KRemoteOwner* owner) {
  if (!owner) return;
  if (owner->pages && !owner->permanent)
    (void)h8_page8k_remote_owner_close(owner);
  h8_platform_mutex_lock(&g_owner_list_lock);
  H8Page8KRemoteOwner** link = &g_owner_list;
  while (*link && *link != owner) link = &(*link)->global_next;
  if (*link == owner) *link = owner->global_next;
  atomic_store_explicit(&owner->state, H8_PAGE8K_OWNER_DEAD,
                        memory_order_seq_cst);
  owner->global_next = NULL;
  owner->free_next = g_owner_free_list;
  g_owner_free_list = owner;
  h8_platform_mutex_unlock(&g_owner_list_lock);
}

H8Page8KRemotePage* h8_page8k_remote_page_create(
    H8Page8KRemoteOwner* owner) {
  if (!owner) return NULL;
  H8Page8KRemotePage* page = calloc(1u, sizeof(*page));
  if (!page) return NULL;
  page->base = h8_platform_reserve_rw_aligned(H8_PAGE8K_RUN_BYTES,
                                               H8_PAGE8K_RUN_BYTES);
  if (!page->base) {
    free(page);
    return NULL;
  }
  atomic_init(&page->owner, owner);
  atomic_init(&page->owner_token,
              atomic_load_explicit(&owner->token, memory_order_relaxed));
  atomic_init(&page->publish_closed, 0u);
  atomic_init(&page->publish_refs, 0u);
  page->resident = true;
  page->free_head = 0u;
  for (uint8_t i = 0; i < H8_PAGE8K_SLOT_COUNT; ++i) {
    page->free_next[i] = (uint8_t)(i + 1u);
    atomic_init(&page->slot_state[i], H8_PAGE8K_SLOT_FREE);
  }
  page->free_next[H8_PAGE8K_SLOT_COUNT - 1u] = UINT8_MAX;
  atomic_init(&page->qstate, H8_PAGE8K_Q_IDLE);
  atomic_init(&page->pending_mask, 0u);
  if (!h8_page8k_registry_add(page)) {
    h8_platform_release(page->base, H8_PAGE8K_RUN_BYTES);
    free(page);
    return NULL;
  }
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_KIND_L1)
  h8_medium_domain_shadow_register_page8k(page->base, page);
#endif
  page->owner_next = owner->pages;
  owner->pages = page;
  ++owner->page_count;
  if (!owner->active_page) owner->active_page = page;
  return page;
}

void* h8_page8k_remote_alloc(H8Page8KRemoteOwner* owner,
                             H8Page8KRemotePage* page) {
  if (!owner || !page || h8_page8k_page_owner(page) != owner ||
      page->free_head == UINT8_MAX)
    return NULL;
  if (!h8_page8k_recommit(page)) return NULL;
  uint8_t slot = page->free_head;
  page->free_head = page->free_next[slot];
  uint8_t expected = H8_PAGE8K_SLOT_FREE;
  if (!atomic_compare_exchange_strong_explicit(
          &page->slot_state[slot], &expected, H8_PAGE8K_SLOT_ALLOCATED,
          memory_order_acq_rel, memory_order_acquire))
    return NULL;
  if (page->free_head == UINT8_MAX && owner->active_page == page)
    owner->active_page = NULL;
  return page->base + (size_t)slot * H8_PAGE8K_BYTES;
}

bool h8_page8k_remote_free(H8Page8KRemoteOwner* current_owner, void* ptr,
                           bool* owned_out) {
  if (owned_out) *owned_out = false;
  H8Page8KRemotePage* page = h8_page8k_classify(ptr);
  if (!page) return false;
  if (owned_out) *owned_out = true;
  uintptr_t offset = (uintptr_t)ptr - (uintptr_t)page->base;
  if ((offset % H8_PAGE8K_BYTES) != 0u) {
    H8_PAGE8K_STAT_INC(interior_reject);
    return false;
  }
  uint8_t slot = (uint8_t)(offset / H8_PAGE8K_BYTES);
  if (slot >= H8_PAGE8K_SLOT_COUNT) return false;

  uint8_t expected = H8_PAGE8K_SLOT_ALLOCATED;
  H8Page8KRemoteOwner* page_owner = h8_page8k_page_owner(page);
  if (current_owner == page_owner) {
    if (!atomic_compare_exchange_strong_explicit(
            &page->slot_state[slot], &expected, H8_PAGE8K_SLOT_FREE,
            memory_order_acq_rel, memory_order_acquire)) {
      H8_PAGE8K_STAT_INC(duplicate_reject);
      return false;
    }
    page->free_next[slot] = page->free_head;
    page->free_head = slot;
    h8_page8k_available_push(page);
    return true;
  }

  page_owner = h8_page8k_publish_acquire(page);
  H8_PAGE8K_STAT_INC(remote_claim_attempt);
  if (!atomic_compare_exchange_strong_explicit(
          &page->slot_state[slot], &expected, H8_PAGE8K_SLOT_REMOTE_PENDING,
          memory_order_acq_rel, memory_order_acquire)) {
    H8_PAGE8K_STAT_INC(remote_claim_reject);
    H8_PAGE8K_STAT_INC(duplicate_reject);
    h8_page8k_publish_release(page);
    return false;
  }
  H8_PAGE8K_STAT_INC(remote_claim_success);
  atomic_fetch_or_explicit(&page->pending_mask, (uint8_t)(1u << slot),
                           memory_order_release);
  H8_PAGE8K_STAT_INC(pending_publish);
  h8_page8k_queue(page);
  h8_page8k_publish_release(page);
  return true;
}

size_t h8_page8k_remote_drain(H8Page8KRemoteOwner* owner) {
  if (!owner) return 0u;
  H8Page8KRemotePage* list = atomic_exchange_explicit(
      &owner->inbox_head, NULL, memory_order_acq_rel);
  atomic_store_explicit(&owner->inbox_depth, 0u, memory_order_relaxed);
  H8_PAGE8K_STAT_STORE(inbox_depth, 0u);
  for (H8Page8KRemotePage* page = list; page; page = page->inbox_next)
    atomic_store_explicit(&page->qstate, H8_PAGE8K_Q_DRAINING,
                          memory_order_release);

  size_t drained = 0u;
  while (list) {
    H8Page8KRemotePage* page = list;
    list = page->inbox_next;
    page->inbox_next = NULL;
    uint8_t mask = atomic_exchange_explicit(&page->pending_mask, 0u,
                                            memory_order_acq_rel);
    for (uint8_t slot = 0; slot < H8_PAGE8K_SLOT_COUNT; ++slot) {
      if ((mask & (uint8_t)(1u << slot)) == 0u) continue;
      uint8_t expected = H8_PAGE8K_SLOT_REMOTE_PENDING;
      if (!atomic_compare_exchange_strong_explicit(
              &page->slot_state[slot], &expected, H8_PAGE8K_SLOT_FREE,
              memory_order_acq_rel, memory_order_acquire))
        continue;
      page->free_next[slot] = page->free_head;
      page->free_head = slot;
      ++drained;
    }
    H8_PAGE8K_STAT_INC(drain_pages);
    h8_page8k_available_push(page);

    uint8_t expected = H8_PAGE8K_Q_DRAINING;
    bool idle =
        atomic_load_explicit(&page->pending_mask, memory_order_acquire) == 0u &&
        atomic_compare_exchange_strong_explicit(
            &page->qstate, &expected, H8_PAGE8K_Q_IDLE,
            memory_order_acq_rel, memory_order_acquire);
    if (!idle) {
      atomic_store_explicit(&page->qstate, H8_PAGE8K_Q_QUEUED,
                            memory_order_release);
      h8_page8k_queue_push(owner, page);
    }
    h8_page8k_orphan_trim(page);
  }
  H8_PAGE8K_STAT_ADD(drain_slots, drained);
  return drained;
}

bool h8_page8k_remote_quiescent(const H8Page8KRemotePage* page) {
  return page && atomic_load_explicit(&page->pending_mask, memory_order_acquire) == 0u &&
         atomic_load_explicit(&page->qstate, memory_order_acquire) == H8_PAGE8K_Q_IDLE;
}

H8Page8KRemoteStats h8_page8k_remote_stats(void) {
  H8Page8KRemoteStats out;
  memset(&out, 0, sizeof(out));
#if defined(H8_PAGE8K_REMOTE_DIAGNOSTIC)
#define H8_PAGE8K_LOAD(field) \
  out.field = atomic_load_explicit(&g_##field, memory_order_relaxed)
  H8_PAGE8K_LOAD(dispatch_alloc_attempt);
  H8_PAGE8K_LOAD(dispatch_alloc_served);
  H8_PAGE8K_LOAD(dispatch_free_attempt);
  H8_PAGE8K_LOAD(dispatch_free_owner_present);
  H8_PAGE8K_LOAD(dispatch_free_owned);
  H8_PAGE8K_LOAD(dispatch_free_success);
  H8_PAGE8K_LOAD(dispatch_free_miss);
  H8_PAGE8K_LOAD(owner_create);
  H8_PAGE8K_LOAD(range_eligible_alloc);
  H8_PAGE8K_LOAD(range_served_alloc);
  H8_PAGE8K_LOAD(remote_claim_attempt);
  H8_PAGE8K_LOAD(remote_claim_success);
  H8_PAGE8K_LOAD(remote_claim_reject);
  H8_PAGE8K_LOAD(pending_publish);
  H8_PAGE8K_LOAD(queue_notify);
  H8_PAGE8K_LOAD(queue_dirty);
  H8_PAGE8K_LOAD(drain_pages);
  H8_PAGE8K_LOAD(drain_slots);
  H8_PAGE8K_LOAD(duplicate_reject);
  H8_PAGE8K_LOAD(interior_reject);
  H8_PAGE8K_LOAD(lost_notification);
  H8_PAGE8K_LOAD(inbox_depth);
  H8_PAGE8K_LOAD(inbox_depth_max);
  H8_PAGE8K_LOAD(page_cap_reject);
  H8_PAGE8K_LOAD(drain_all_owner_visits);
  H8_PAGE8K_LOAD(drain_all_limit);
  H8_PAGE8K_LOAD(drain_all_skipped_live);
  H8_PAGE8K_LOAD(owner_close);
  H8_PAGE8K_LOAD(orphan_adopt);
  H8_PAGE8K_LOAD(publish_retry);
  H8_PAGE8K_LOAD(page_decommit);
  H8_PAGE8K_LOAD(page_recommit);
  H8_PAGE8K_LOAD(page_decommit_fail);
  H8_PAGE8K_LOAD(orphan_resident_empty);
  H8_PAGE8K_LOAD(orphan_resident_empty_max);
#undef H8_PAGE8K_LOAD
#endif
  return out;
}

bool h8_page8k_remote_owner_close(H8Page8KRemoteOwner* owner) {
  if (!owner || owner->permanent) return false;
  uint8_t expected = H8_PAGE8K_OWNER_ALIVE;
  if (!atomic_compare_exchange_strong_explicit(
          &owner->state, &expected, H8_PAGE8K_OWNER_CLOSING,
          memory_order_acq_rel, memory_order_acquire))
    return expected == H8_PAGE8K_OWNER_DEAD;

  H8Page8KRemoteOwner* orphan = h8_page8k_orphan_owner();
  if (!orphan) {
    atomic_store_explicit(&owner->state, H8_PAGE8K_OWNER_ALIVE,
                          memory_order_release);
    return false;
  }

  h8_platform_mutex_lock(&g_transition_lock);
  for (H8Page8KRemotePage* page = owner->pages; page;
       page = page->owner_next)
    atomic_store_explicit(&page->publish_closed, 1u, memory_order_seq_cst);
  atomic_thread_fence(memory_order_seq_cst);
  for (H8Page8KRemotePage* page = owner->pages; page;
       page = page->owner_next) {
    while (atomic_load_explicit(&page->publish_refs, memory_order_seq_cst) != 0u)
      h8_platform_yield();
  }
  for (size_t pass = 0u; pass < 4u; ++pass) {
    if (h8_page8k_remote_drain(owner) == 0u) break;
  }

  H8Page8KRemotePage* pages = owner->pages;
  owner->pages = NULL;
  owner->active_page = NULL;
  owner->available_head = NULL;
  owner->page_count = 0u;
  while (pages) {
    H8Page8KRemotePage* page = pages;
    pages = page->owner_next;
    page->available_indexed = false;
    page->available_next = NULL;
    atomic_store_explicit(&page->owner, orphan, memory_order_release);
    atomic_store_explicit(
        &page->owner_token,
        atomic_load_explicit(&orphan->token, memory_order_relaxed),
        memory_order_release);
    page->owner_next = orphan->pages;
    orphan->pages = page;
    ++orphan->page_count;
    h8_page8k_available_push(page);
    h8_page8k_orphan_trim(page);
    atomic_store_explicit(&page->publish_closed, 0u, memory_order_release);
  }
  atomic_store_explicit(&owner->state, H8_PAGE8K_OWNER_DEAD,
                        memory_order_release);
  H8_PAGE8K_STAT_INC(owner_close);
  h8_platform_mutex_unlock(&g_transition_lock);
  return true;
}

bool h8_page8k_remote_adopt_one(H8Page8KRemoteOwner* owner) {
  if (!owner || atomic_load_explicit(&owner->state, memory_order_acquire) !=
                    H8_PAGE8K_OWNER_ALIVE)
    return false;
  H8Page8KRemoteOwner* orphan = h8_page8k_orphan_owner();
  if (!orphan) return false;

  h8_platform_mutex_lock(&g_transition_lock);
  (void)h8_page8k_remote_drain(orphan);
  H8Page8KRemotePage* page = h8_page8k_available_pop(orphan);
  if (!page) {
    h8_platform_mutex_unlock(&g_transition_lock);
    return false;
  }
  atomic_store_explicit(&page->publish_closed, 1u, memory_order_seq_cst);
  atomic_thread_fence(memory_order_seq_cst);
  while (atomic_load_explicit(&page->publish_refs, memory_order_seq_cst) != 0u)
    h8_platform_yield();
  (void)h8_page8k_remote_drain(orphan);
  h8_page8k_available_remove(orphan, page);
  h8_page8k_orphan_resident_remove(page);

  H8Page8KRemotePage** link = &orphan->pages;
  while (*link && *link != page) link = &(*link)->owner_next;
  if (*link != page) {
    atomic_store_explicit(&page->publish_closed, 0u, memory_order_release);
    h8_platform_mutex_unlock(&g_transition_lock);
    return false;
  }
  *link = page->owner_next;
  if (orphan->page_count != 0u) --orphan->page_count;

  atomic_store_explicit(&page->owner, owner, memory_order_release);
  atomic_store_explicit(
      &page->owner_token,
      atomic_load_explicit(&owner->token, memory_order_relaxed),
      memory_order_release);
  page->owner_next = owner->pages;
  owner->pages = page;
  ++owner->page_count;
  if (!owner->active_page) owner->active_page = page;
  else h8_page8k_available_push(page);
  atomic_store_explicit(&page->publish_closed, 0u, memory_order_release);
  H8_PAGE8K_STAT_INC(orphan_adopt);
  h8_platform_mutex_unlock(&g_transition_lock);
  return true;
}

void h8_page8k_remote_thread_shutdown(void) {
  H8Page8KRemoteOwner* owner = g_current_owner;
  if (!owner) return;
  g_current_owner = NULL;
  (void)h8_page8k_remote_owner_close(owner);
  h8_page8k_remote_owner_destroy(owner);
}

bool h8_page8k_remote_accepts_size(size_t size) {
#if defined(H8_MEDIUM_PAGE8K_RANGE4097_L1)
  return size >= 4097u && size <= H8_PAGE8K_BYTES;
#else
  return size == H8_PAGE8K_BYTES;
#endif
}

static H8Page8KRemoteOwner* h8_page8k_current_owner(void) {
  if (!g_current_owner) {
    H8_PAGE8K_STAT_INC(owner_create);
    uint32_t token =
        atomic_fetch_add_explicit(&g_next_owner_token, 1u, memory_order_relaxed);
    g_current_owner = h8_page8k_remote_owner_create(token);
  }
  return g_current_owner;
}

void* h8_page8k_remote_malloc_current(size_t size) {
  if (!h8_page8k_remote_accepts_size(size)) return NULL;
  H8_PAGE8K_STAT_INC(dispatch_alloc_attempt);
#if defined(H8_MEDIUM_PAGE8K_RANGE4097_L1)
  H8_PAGE8K_STAT_INC(range_eligible_alloc);
#endif
  H8Page8KRemoteOwner* owner = h8_page8k_current_owner();
  if (!owner) return NULL;
  if (owner->active_page) {
    void* ptr = h8_page8k_remote_alloc(owner, owner->active_page);
    if (ptr) {
      H8_PAGE8K_STAT_INC(dispatch_alloc_served);
      H8_PAGE8K_STAT_INC(range_served_alloc);
      return ptr;
    }
  }
  for (;;) {
    H8Page8KRemotePage* page = h8_page8k_available_pop(owner);
    if (!page) break;
    owner->active_page = page;
    void* ptr = h8_page8k_remote_alloc(owner, page);
    if (ptr) {
      H8_PAGE8K_STAT_INC(dispatch_alloc_served);
      H8_PAGE8K_STAT_INC(range_served_alloc);
      return ptr;
    }
    owner->active_page = NULL;
  }
  h8_page8k_remote_drain(owner);
  if (owner->active_page) {
    void* ptr = h8_page8k_remote_alloc(owner, owner->active_page);
    if (ptr) {
      H8_PAGE8K_STAT_INC(dispatch_alloc_served);
      H8_PAGE8K_STAT_INC(range_served_alloc);
      return ptr;
    }
  }
  for (;;) {
    H8Page8KRemotePage* page = h8_page8k_available_pop(owner);
    if (!page) break;
    owner->active_page = page;
    void* ptr = h8_page8k_remote_alloc(owner, page);
    if (ptr) {
      H8_PAGE8K_STAT_INC(dispatch_alloc_served);
      H8_PAGE8K_STAT_INC(range_served_alloc);
      return ptr;
    }
    owner->active_page = NULL;
  }
  if (h8_page8k_remote_adopt_one(owner) && owner->active_page) {
    void* ptr = h8_page8k_remote_alloc(owner, owner->active_page);
    if (ptr) {
      H8_PAGE8K_STAT_INC(dispatch_alloc_served);
      H8_PAGE8K_STAT_INC(range_served_alloc);
      return ptr;
    }
  }
  if (owner->page_count >= H8_PAGE8K_OWNER_PAGE_CAP) {
    H8_PAGE8K_STAT_INC(page_cap_reject);
    return NULL;
  }
  H8Page8KRemotePage* page = h8_page8k_remote_page_create(owner);
  if (!page) return NULL;
  void* ptr = h8_page8k_remote_alloc(owner, page);
  if (ptr) {
    H8_PAGE8K_STAT_INC(dispatch_alloc_served);
    H8_PAGE8K_STAT_INC(range_served_alloc);
  }
  return ptr;
}

bool h8_page8k_remote_free_current(void* ptr, bool* owned_out) {
#if defined(H8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1)
  H8_PAGE8K_STAT_INC(dispatch_free_attempt);
  if (g_current_owner) H8_PAGE8K_STAT_INC(dispatch_free_owner_present);
  bool owned = false;
  bool freed = h8_page8k_remote_free(g_current_owner, ptr, &owned);
  if (owned) H8_PAGE8K_STAT_INC(dispatch_free_owned);
  else H8_PAGE8K_STAT_INC(dispatch_free_miss);
  if (freed) H8_PAGE8K_STAT_INC(dispatch_free_success);
  if (owned_out) *owned_out = owned;
  return freed;
#else
  H8Page8KRemoteOwner* owner = h8_page8k_current_owner();
  if (!owner) {
    if (owned_out) *owned_out = false;
    return false;
  }
  return h8_page8k_remote_free(owner, ptr, owned_out);
#endif
}

size_t h8_page8k_remote_drain_all_control(void) {
  size_t total = 0u;
  size_t owner_visits = 0u;
  h8_platform_mutex_lock(&g_owner_list_lock);
  for (H8Page8KRemoteOwner* owner = g_owner_list;
       owner && owner_visits < 1024u;
       owner = owner->global_next) {
    ++owner_visits;
    if (!owner->permanent && owner != g_current_owner) {
      H8_PAGE8K_STAT_INC(drain_all_skipped_live);
      continue;
    }
    for (size_t pass = 0u; pass < 4u; ++pass) {
      size_t drained = h8_page8k_remote_drain(owner);
      total += drained;
      if (drained == 0u) break;
      if (pass == 3u) H8_PAGE8K_STAT_INC(drain_all_limit);
    }
  }
  H8_PAGE8K_STAT_ADD(drain_all_owner_visits, owner_visits);
  if (owner_visits == 1024u) H8_PAGE8K_STAT_INC(drain_all_limit);
  h8_platform_mutex_unlock(&g_owner_list_lock);
  return total;
}

#endif
