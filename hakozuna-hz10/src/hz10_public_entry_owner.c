#include "hz10_public_entry_owner.h"

#include "hz10_platform.h"
#include "hz10_remote_stack.h"

#include <stdatomic.h>
#include <string.h>

#ifndef HZ10_ORPHAN_ACTIVE_ADOPT_SCAN_BUDGET
#define HZ10_ORPHAN_ACTIVE_ADOPT_SCAN_BUDGET 16u
#endif

#ifndef HZ10_OWNER_SLAB_BYTES
#define HZ10_OWNER_SLAB_BYTES HZ10_PAGE_QUANTUM
#endif

static _Thread_local Hz10ThreadOwner* hz10_current_owner;
static hz10_platform_mutex_t hz10_owner_slab_lock = HZ10_PLATFORM_MUTEX_INIT;
static char* hz10_owner_slab_cursor;
static char* hz10_owner_slab_end;

static Hz10OwnerRecord* hz10_owner_record_alloc(void) {
  size_t owner_bytes = (sizeof(Hz10OwnerRecord) + 15u) & ~(size_t)15u;
  hz10_platform_mutex_lock(&hz10_owner_slab_lock);
  if (!hz10_owner_slab_cursor ||
      (size_t)(hz10_owner_slab_end - hz10_owner_slab_cursor) < owner_bytes) {
    size_t slab_bytes = HZ10_OWNER_SLAB_BYTES;
    if (slab_bytes < owner_bytes) {
      slab_bytes = owner_bytes;
    }
    void* slab = hz10_platform_reserve_rw(slab_bytes);
    if (!slab) {
      hz10_platform_mutex_unlock(&hz10_owner_slab_lock);
      return NULL;
    }
    hz10_owner_slab_cursor = (char*)slab;
    hz10_owner_slab_end = hz10_owner_slab_cursor + slab_bytes;
  }
  Hz10OwnerRecord* owner = (Hz10OwnerRecord*)hz10_owner_slab_cursor;
  hz10_owner_slab_cursor += owner_bytes;
  hz10_platform_mutex_unlock(&hz10_owner_slab_lock);
  memset(owner, 0, sizeof(*owner));
  atomic_store_explicit(&owner->state, HZ10_THREAD_OWNER_STATE_LIVE,
                        memory_order_release);
  return owner;
}

static Hz10ThreadOwner* hz10_owner_alloc(void) {
  Hz10OwnerRecord* record = hz10_owner_record_alloc();
  if (!record) {
    return NULL;
  }
  Hz10ThreadOwner* owner =
      (Hz10ThreadOwner*)hz10_platform_reserve_rw(sizeof(Hz10ThreadOwner));
  if (!owner) {
    atomic_store_explicit(&record->state, HZ10_THREAD_OWNER_STATE_EXITED,
                          memory_order_release);
    return NULL;
  }
  memset(owner, 0, sizeof(*owner));
  owner->record = record;
  return owner;
}

static hz10_platform_once_t hz10_owner_key_once = HZ10_PLATFORM_ONCE_INIT;
static hz10_platform_thread_key_t hz10_owner_key;
static int hz10_owner_key_ready;
#if HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION
static hz10_platform_mutex_t hz10_orphan_lock = HZ10_PLATFORM_MUTEX_INIT;
static Hz10PageSublist hz10_orphan_active[HZ10_CLASS_COUNT];

static void hz10_orphan_append_active(uint32_t class_id, Hz10PageSublist* sub) {
  if (!sub->head) return;
  Hz10PageSublist* dst = &hz10_orphan_active[class_id];
  if (dst->tail) {
    dst->tail->next_in_owner_list = sub->head;
    sub->head->prev_in_owner_list = dst->tail;
  } else {
    dst->head = sub->head;
  }
  dst->tail = sub->tail;
  dst->length += sub->length;
  *sub = (Hz10PageSublist){0};
}

static void hz10_orphan_append_one(Hz10PageSublist* dst,
                                   Hz10FreelistPage* page) {
  page->next_in_owner_list = NULL;
  page->prev_in_owner_list = dst->tail;
  page->owner_list_kind = HZ10_FREELIST_PAGE_OWNER_LIST_ACTIVE;
  if (dst->tail) {
    dst->tail->next_in_owner_list = page;
  } else {
    dst->head = page;
  }
  dst->tail = page;
  dst->length += 1u;
}

static Hz10FreelistPage* hz10_orphan_pop_one(Hz10PageSublist* src) {
  Hz10FreelistPage* page = src->head;
  if (!page) return NULL;
  src->head = page->next_in_owner_list;
  if (src->head) {
    src->head->prev_in_owner_list = NULL;
  } else {
    src->tail = NULL;
  }
  src->length -= 1u;
  page->next_in_owner_list = NULL;
  page->prev_in_owner_list = NULL;
  page->owner_list_kind = HZ10_FREELIST_PAGE_OWNER_LIST_NONE;
  return page;
}

static Hz10FreelistPage* hz10_orphan_pop_class(uint32_t class_id) {
  Hz10FreelistPage* page;
  hz10_platform_mutex_lock(&hz10_orphan_lock);
  page = hz10_orphan_pop_one(&hz10_orphan_active[class_id]);
  hz10_platform_mutex_unlock(&hz10_orphan_lock);
  return page;
}

static void hz10_orphan_repush_class(uint32_t class_id,
                                     Hz10FreelistPage* page) {
  hz10_platform_mutex_lock(&hz10_orphan_lock);
  hz10_orphan_append_one(&hz10_orphan_active[class_id], page);
  hz10_platform_mutex_unlock(&hz10_orphan_lock);
}
#endif

static void hz10_owner_destructor(void* value) {
  Hz10ThreadOwner* owner = (Hz10ThreadOwner*)value;
  if (!owner) return;
  if (hz10_current_owner == owner) {
    /* pthread key destructors can run before other thread-exit destructors.
     * After the active lists become globally orphan-visible, this exiting
     * thread must not keep treating their pages as owner-local. Late frees
     * in other destructors therefore go through the remote path. */
    hz10_current_owner = NULL;
  }
  atomic_store_explicit(&owner->record->state, HZ10_THREAD_OWNER_STATE_EXITED,
                        memory_order_release);
#if HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION
  hz10_platform_mutex_lock(&hz10_orphan_lock);
  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    hz10_orphan_append_active(c, &owner->classes[c].list.active);
    owner->classes[c].active = NULL;
  }
  hz10_platform_mutex_unlock(&hz10_orphan_lock);
#endif
  hz10_platform_release(owner, sizeof(*owner));
}

static void hz10_owner_key_init(void) {
  hz10_owner_key_ready =
      hz10_platform_thread_key_create(&hz10_owner_key,
                                      hz10_owner_destructor) == 0;
}

static void hz10_owner_register_for_exit(Hz10ThreadOwner* owner) {
  (void)hz10_platform_once(&hz10_owner_key_once, hz10_owner_key_init);
  if (hz10_owner_key_ready) {
    (void)hz10_platform_thread_setspecific(hz10_owner_key, owner);
  }
}

Hz10ThreadOwner* hz10_public_entry_current_owner(void) {
  Hz10ThreadOwner* owner = hz10_current_owner;
  if (owner) return owner;
  owner = hz10_owner_alloc();
  if (!owner) return NULL;
  hz10_current_owner = owner;
  hz10_owner_register_for_exit(owner);
  return owner;
}

Hz10ThreadOwner* hz10_public_entry_current_owner_if_any(void) {
  return hz10_current_owner;
}

Hz10OwnerRecord* hz10_public_entry_owner_record(const Hz10ThreadOwner* owner) {
  return owner ? owner->record : NULL;
}

uint32_t hz10_public_entry_owner_state(const Hz10OwnerRecord* owner) {
  if (!owner) return 0u;
  return atomic_load_explicit(&owner->state, memory_order_acquire);
}

Hz10FreelistPage* hz10_public_entry_try_adopt_orphan_active(
    uint32_t class_id, Hz10ThreadOwner* adopter) {
#if HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION
  if (!adopter || class_id >= HZ10_CLASS_COUNT) return NULL;
  uint32_t scan_budget = HZ10_ORPHAN_ACTIVE_ADOPT_SCAN_BUDGET;
  for (uint32_t scanned = 0u; scanned < scan_budget; ++scanned) {
    Hz10FreelistPage* page = hz10_orphan_pop_class(class_id);
    if (!page) {
      break;
    }

#if HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION
    Hz10OwnerRecord* old_owner =
        (Hz10OwnerRecord*)hz10_freelist_page_owner_thread(page);
    if (page->class_id != class_id ||
        hz10_public_entry_owner_state(old_owner) !=
            HZ10_THREAD_OWNER_STATE_EXITED) {
      hz10_orphan_repush_class(class_id, page);
      continue;
    }

    hz10_freelist_page_set_owner_thread(page,
                                        hz10_public_entry_owner_record(adopter));
    hz10_page_drain_remote(page);

    if (page->local_free_head) {
      hz10_freelist_page_note_adopted(page);
      hz10_class_pages_add(&adopter->classes[class_id].list, page);
      return page;
    }

    hz10_freelist_page_set_owner_thread(page, old_owner);
    hz10_orphan_repush_class(class_id, page);
#else
    hz10_page_drain_remote(page);
    if (page->free_count == page->slot_count) {
      hz10_freelist_page_set_owner_thread(
          page, hz10_public_entry_owner_record(adopter));
      hz10_freelist_page_note_adopted(page);
      hz10_class_pages_add(&adopter->classes[class_id].list, page);
      return page;
    }

    hz10_orphan_repush_class(class_id, page);
#endif
  }
  return NULL;
#else
  (void)class_id;
  (void)adopter;
  return NULL;
#endif
}
