#include "hz10_page_pool.h"
#include "hz10_pagemap.h"
#include "hz10_platform.h"

#include <stddef.h>

typedef struct Hz10PagePoolNode {
  struct Hz10PagePoolNode* next;
} Hz10PagePoolNode;

static hz10_platform_mutex_t hz10_pool_lock = HZ10_PLATFORM_MUTEX_INIT;
static Hz10PagePoolNode* hz10_pool_head;
static uint32_t hz10_pool_count;
static uint32_t hz10_pool_cap = HZ10_PAGE_POOL_DEFAULT_CAP;
static uint64_t hz10_pool_reuse_count;
static uint64_t hz10_pool_release_count;

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

void hz10_page_pool_reset_for_tests(void) {
  hz10_platform_mutex_lock(&hz10_pool_lock);
  Hz10PagePoolNode* node = hz10_pool_head;
  hz10_pool_head = NULL;
  hz10_pool_count = 0u;
  hz10_pool_cap = HZ10_PAGE_POOL_DEFAULT_CAP;
  hz10_pool_reuse_count = 0u;
  hz10_pool_release_count = 0u;
  hz10_platform_mutex_unlock(&hz10_pool_lock);
  while (node) {
    Hz10PagePoolNode* next = node->next;
    hz10_platform_release((void*)node, HZ10_PAGE_QUANTUM);
    node = next;
  }
}
