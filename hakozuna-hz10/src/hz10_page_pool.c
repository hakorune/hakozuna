#include "hz10_page_pool.h"
#include "hz10_pagemap.h"
#include "hz10_platform.h"

#include <stdatomic.h>
#include <stddef.h>

typedef struct Hz10PagePoolNode {
  struct Hz10PagePoolNode* next;
  uint64_t released_at_ns; /* hz10_platform_now_ns() at release() time, for
                            * hz10_page_pool_purge_idle()'s aging sweep */
} Hz10PagePoolNode;

static hz10_platform_mutex_t hz10_pool_lock = HZ10_PLATFORM_MUTEX_INIT;
/* _Atomic so hz10_page_pool_try_acquire() can peek at emptiness without
 * taking the mutex (see below); every actual mutation still happens under
 * hz10_pool_lock, same discipline as src/hz10_remote_stack.c's
 * remote_free_head peek. */
static _Atomic(Hz10PagePoolNode*) hz10_pool_head;
static uint32_t hz10_pool_count;
static uint32_t hz10_pool_cap = HZ10_PAGE_POOL_DEFAULT_CAP;
static uint64_t hz10_pool_reuse_count;
static uint64_t hz10_pool_release_count;
static uint64_t hz10_pool_purged_count;

void* hz10_page_pool_try_acquire(void) {
  /* Real, measured motivation (current_task.md): a perf stat + strace pass
   * on main_r50/main_r90 found this function's mutex was THE dominant
   * futex cost (98%+ of syscall time), because callers on the current
   * hz10_public_entry.c path never call hz10_pooled_page_destroy() -- the
   * pool is provably always empty here, so every call was paying a
   * lock/unlock pair to find nothing. This relaxed peek skips the mutex
   * entirely on the common empty case; a stale NULL read just means this
   * call misses a page another thread released a moment ago, no different
   * from being called a moment earlier -- correctness-neutral. */
  if (atomic_load_explicit(&hz10_pool_head, memory_order_relaxed) == NULL) {
    return NULL;
  }
  hz10_platform_mutex_lock(&hz10_pool_lock);
  Hz10PagePoolNode* node =
      atomic_load_explicit(&hz10_pool_head, memory_order_relaxed);
  if (node) {
    atomic_store_explicit(&hz10_pool_head, node->next, memory_order_relaxed);
    hz10_pool_count -= 1u;
    hz10_pool_reuse_count += 1u;
  }
  hz10_platform_mutex_unlock(&hz10_pool_lock);
  return (void*)node;
}

int hz10_page_pool_release(void* base) {
  if (!base) {
    return 0;
  }
  hz10_platform_mutex_lock(&hz10_pool_lock);
  int cached = hz10_pool_count < hz10_pool_cap;
  if (cached) {
    Hz10PagePoolNode* node = (Hz10PagePoolNode*)base;
    node->next = hz10_pool_head;
    node->released_at_ns = hz10_platform_now_ns();
    hz10_pool_head = node;
    hz10_pool_count += 1u;
  } else {
    hz10_pool_release_count += 1u;
  }
  hz10_platform_mutex_unlock(&hz10_pool_lock);
  if (!cached) {
    hz10_platform_release(base, HZ10_PAGE_QUANTUM);
  }
  return cached;
}

uint32_t hz10_page_pool_purge_idle(uint64_t max_idle_ns) {
  hz10_platform_mutex_lock(&hz10_pool_lock);
  uint64_t now = hz10_platform_now_ns();
  Hz10PagePoolNode* purged_head = NULL;
  /* Local, non-atomic walk under the lock (hz10_pool_head is only _Atomic
   * for the lock-free peek in try_acquire above): unlink idle nodes in
   * place (a `prev` pointer, same shape as the original pre-_Atomic
   * version) instead of rebuilding a parallel "kept" list, and only
   * publish hz10_pool_head back if something was actually purged -- the
   * common case (nothing idle yet) does zero writes here, not a full
   * list rebuild plus an unconditional store. */
  Hz10PagePoolNode* prev = NULL;
  Hz10PagePoolNode* node =
      atomic_load_explicit(&hz10_pool_head, memory_order_relaxed);
  Hz10PagePoolNode* new_head = node;
  while (node) {
    Hz10PagePoolNode* next = node->next;
    uint64_t idle_ns = now - node->released_at_ns; /* unsigned: a clock
                                                     * that ever went
                                                     * backwards would wrap
                                                     * to huge, purging
                                                     * eagerly rather than
                                                     * never -- fail-safe
                                                     * either way */
    if (idle_ns >= max_idle_ns) {
      hz10_pool_count -= 1u;
      if (prev) {
        prev->next = next;
      } else {
        new_head = next;
      }
      node->next = purged_head;
      purged_head = node;
    } else {
      prev = node;
    }
    node = next;
  }
  if (purged_head) {
    atomic_store_explicit(&hz10_pool_head, new_head, memory_order_relaxed);
  }
  hz10_platform_mutex_unlock(&hz10_pool_lock);

  uint32_t purged = 0u;
  while (purged_head) {
    Hz10PagePoolNode* next = purged_head->next;
    hz10_platform_release((void*)purged_head, HZ10_PAGE_QUANTUM);
    purged_head = next;
    purged += 1u;
  }
  if (purged > 0u) {
    hz10_platform_mutex_lock(&hz10_pool_lock);
    hz10_pool_purged_count += purged;
    hz10_platform_mutex_unlock(&hz10_pool_lock);
  }
  return purged;
}

uint32_t hz10_page_pool_set_cap(uint32_t cap) {
  hz10_platform_mutex_lock(&hz10_pool_lock);
  uint32_t previous = hz10_pool_cap;
  hz10_pool_cap = cap;
  hz10_platform_mutex_unlock(&hz10_pool_lock);
  return previous;
}

uint32_t hz10_page_pool_cached_count(void) {
  hz10_platform_mutex_lock(&hz10_pool_lock);
  uint32_t count = hz10_pool_count;
  hz10_platform_mutex_unlock(&hz10_pool_lock);
  return count;
}

uint64_t hz10_page_pool_reuse_count(void) {
  hz10_platform_mutex_lock(&hz10_pool_lock);
  uint64_t count = hz10_pool_reuse_count;
  hz10_platform_mutex_unlock(&hz10_pool_lock);
  return count;
}

uint64_t hz10_page_pool_release_count(void) {
  hz10_platform_mutex_lock(&hz10_pool_lock);
  uint64_t count = hz10_pool_release_count;
  hz10_platform_mutex_unlock(&hz10_pool_lock);
  return count;
}

uint64_t hz10_page_pool_purged_count(void) {
  hz10_platform_mutex_lock(&hz10_pool_lock);
  uint64_t count = hz10_pool_purged_count;
  hz10_platform_mutex_unlock(&hz10_pool_lock);
  return count;
}

void hz10_page_pool_reset_for_tests(void) {
  hz10_platform_mutex_lock(&hz10_pool_lock);
  Hz10PagePoolNode* node = hz10_pool_head;
  hz10_pool_head = NULL;
  hz10_pool_count = 0u;
  hz10_pool_cap = HZ10_PAGE_POOL_DEFAULT_CAP;
  hz10_pool_reuse_count = 0u;
  hz10_pool_release_count = 0u;
  hz10_pool_purged_count = 0u;
  hz10_platform_mutex_unlock(&hz10_pool_lock);
  while (node) {
    Hz10PagePoolNode* next = node->next;
    hz10_platform_release((void*)node, HZ10_PAGE_QUANTUM);
    node = next;
  }
}
