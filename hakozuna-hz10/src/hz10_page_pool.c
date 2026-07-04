#include "hz10_page_pool.h"
#include "hz10_pagemap.h"
#include "hz10_platform.h"

#include <stddef.h>

typedef struct Hz10PagePoolNode {
  struct Hz10PagePoolNode* next;
  uint64_t released_at_ns; /* hz10_platform_now_ns() at release() time, for
                            * hz10_page_pool_purge_idle()'s aging sweep */
} Hz10PagePoolNode;

static hz10_platform_mutex_t hz10_pool_lock = HZ10_PLATFORM_MUTEX_INIT;
static Hz10PagePoolNode* hz10_pool_head;
static uint32_t hz10_pool_count;
static uint32_t hz10_pool_cap = HZ10_PAGE_POOL_DEFAULT_CAP;
static uint64_t hz10_pool_reuse_count;
static uint64_t hz10_pool_release_count;
static uint64_t hz10_pool_purged_count;

void* hz10_page_pool_try_acquire(void) {
  hz10_platform_mutex_lock(&hz10_pool_lock);
  Hz10PagePoolNode* node = hz10_pool_head;
  if (node) {
    hz10_pool_head = node->next;
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
  Hz10PagePoolNode** link = &hz10_pool_head;
  while (*link) {
    Hz10PagePoolNode* node = *link;
    uint64_t idle_ns = now - node->released_at_ns; /* unsigned: a clock
                                                     * that ever went
                                                     * backwards would wrap
                                                     * to huge, purging
                                                     * eagerly rather than
                                                     * never -- fail-safe
                                                     * either way */
    if (idle_ns >= max_idle_ns) {
      *link = node->next;
      hz10_pool_count -= 1u;
      node->next = purged_head;
      purged_head = node;
    } else {
      link = &node->next;
    }
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
