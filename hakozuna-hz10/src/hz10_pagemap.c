#include "hz10_pagemap.h"
#include "hz10_platform.h"

#include <stdatomic.h>

#define HZ10_ROOT_SIZE (1u << HZ10_ROOT_BITS)
#define HZ10_LEAF_SIZE (1u << HZ10_LEAF_BITS)
#define HZ10_ROOT_MASK (HZ10_ROOT_SIZE - 1u)
#define HZ10_LEAF_MASK (HZ10_LEAF_SIZE - 1u)

typedef struct H10PageRecord {
  void* base;
  void* owner;
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t generation;
  uint32_t flags;
} H10PageRecord;

typedef struct H10Leaf {
  H10PageRecord entries[HZ10_LEAF_SIZE];
} H10Leaf;

static _Atomic(H10Leaf*) hz10_pagemap_root[HZ10_ROOT_SIZE];
static hz10_platform_mutex_t hz10_pagemap_lock = HZ10_PLATFORM_MUTEX_INIT;

static uint32_t hz10_pagemap_page_index(uintptr_t addr) {
  return (uint32_t)(addr >> HZ10_PAGE_SHIFT);
}

static uint32_t hz10_pagemap_root_index(uint32_t page_index) {
  return (page_index >> HZ10_LEAF_BITS) & HZ10_ROOT_MASK;
}

static uint32_t hz10_pagemap_leaf_index(uint32_t page_index) {
  return page_index & HZ10_LEAF_MASK;
}

static H10Leaf* hz10_pagemap_leaf_load(uint32_t root_idx) {
  return atomic_load_explicit(&hz10_pagemap_root[root_idx], memory_order_acquire);
}

/* Lazily mmap's the leaf backing this root slot. Demand-paging means only
 * the 4KiB pages actually touched by later register() calls ever cost RSS,
 * even though the leaf's virtual reservation is ~24MiB. */
static H10Leaf* hz10_pagemap_ensure_leaf(uint32_t root_idx) {
  H10Leaf* leaf = hz10_pagemap_leaf_load(root_idx);
  if (leaf) {
    return leaf;
  }
  hz10_platform_mutex_lock(&hz10_pagemap_lock);
  leaf = hz10_pagemap_leaf_load(root_idx);
  if (!leaf) {
    void* mem = hz10_platform_reserve_rw(sizeof(H10Leaf));
    if (mem) {
      leaf = (H10Leaf*)mem;
      atomic_store_explicit(&hz10_pagemap_root[root_idx], leaf,
                            memory_order_release);
    }
  }
  hz10_platform_mutex_unlock(&hz10_pagemap_lock);
  return leaf;
}

uint32_t hz10_pagemap_register_with_owner(void* base, uint32_t slot_size,
                                          uint32_t slot_count, void* owner) {
  if (!base || slot_size == 0u || slot_count == 0u) {
    return 0u;
  }
  uintptr_t addr = (uintptr_t)base;
  if ((addr & (HZ10_PAGE_QUANTUM - 1u)) != 0u) {
    return 0u;
  }
  uint64_t span = (uint64_t)slot_size * (uint64_t)slot_count;
  if (span > (uint64_t)HZ10_PAGE_QUANTUM) {
    return 0u;
  }

  uint32_t page_index = hz10_pagemap_page_index(addr);
  uint32_t root_idx = hz10_pagemap_root_index(page_index);
  uint32_t leaf_idx = hz10_pagemap_leaf_index(page_index);

  H10Leaf* leaf = hz10_pagemap_ensure_leaf(root_idx);
  if (!leaf) {
    return 0u;
  }

  H10PageRecord* rec = &leaf->entries[leaf_idx];
  hz10_platform_mutex_lock(&hz10_pagemap_lock);
  uint32_t generation = (rec->base == NULL && rec->generation == 0u)
                            ? 1u
                            : rec->generation + 1u;
  rec->base = base;
  rec->owner = owner;
  rec->slot_count = slot_count;
  rec->generation = generation;
  rec->flags = 0u;
  /* slot_size is written last: route() treats slot_size==0 as "absent" and
   * reads these fields without the lock, so publishing slot_size last keeps
   * a concurrent lock-free reader from ever seeing a half-written record as
   * present (this does not make route() fully race-free, see header notes,
   * but avoids the cheapest way to observe torn state). */
  rec->slot_size = slot_size;
  hz10_platform_mutex_unlock(&hz10_pagemap_lock);
  return generation;
}

uint32_t hz10_pagemap_register(void* base, uint32_t slot_size,
                               uint32_t slot_count) {
  return hz10_pagemap_register_with_owner(base, slot_size, slot_count, NULL);
}

int hz10_pagemap_release(void* base) {
  if (!base) {
    return 0;
  }
  uintptr_t addr = (uintptr_t)base;
  if ((addr & (HZ10_PAGE_QUANTUM - 1u)) != 0u) {
    return 0;
  }
  uint32_t page_index = hz10_pagemap_page_index(addr);
  uint32_t root_idx = hz10_pagemap_root_index(page_index);
  uint32_t leaf_idx = hz10_pagemap_leaf_index(page_index);

  H10Leaf* leaf = hz10_pagemap_leaf_load(root_idx);
  if (!leaf) {
    return 0;
  }

  H10PageRecord* rec = &leaf->entries[leaf_idx];
  hz10_platform_mutex_lock(&hz10_pagemap_lock);
  int released = 0;
  if (rec->base == base && rec->slot_size != 0u) {
    /* generation and base survive on purpose: a later register() at this
     * same address must be able to bump generation further, and a stale
     * route() against the pre-release generation must keep failing. */
    rec->slot_size = 0u;
    released = 1;
  }
  hz10_platform_mutex_unlock(&hz10_pagemap_lock);
  return released;
}

static H10RouteResult hz10_pagemap_result(H10RouteKind kind,
                                          H10RouteReason reason,
                                          void* page_base, uint32_t slot_size,
                                          uint32_t slot_count,
                                          uint32_t slot_index,
                                          uint32_t generation, void* owner) {
  H10RouteResult result;
  result.kind = kind;
  result.reason = reason;
  result.page_base = page_base;
  result.slot_size = slot_size;
  result.slot_count = slot_count;
  result.slot_index = slot_index;
  result.generation = generation;
  result.owner = owner;
  return result;
}

H10RouteResult hz10_pagemap_route(const void* ptr,
                                  uint32_t expected_generation) {
  if (!ptr) {
    return hz10_pagemap_result(H10_ROUTE_MISS, H10_REASON_ROOT_ABSENT, NULL,
                              0u, 0u, 0u, 0u, NULL);
  }

  uintptr_t addr = (uintptr_t)ptr;
  uint32_t page_index = hz10_pagemap_page_index(addr);
  uint32_t root_idx = hz10_pagemap_root_index(page_index);
  uint32_t leaf_idx = hz10_pagemap_leaf_index(page_index);

  H10Leaf* leaf = hz10_pagemap_leaf_load(root_idx);
  if (!leaf) {
    return hz10_pagemap_result(H10_ROUTE_MISS, H10_REASON_ROOT_ABSENT, NULL,
                              0u, 0u, 0u, 0u, NULL);
  }

  /* Lock-free read: acceptable single-threaded-scope simplification for
   * L0, see header notes and current_task.md. */
  const H10PageRecord* rec = &leaf->entries[leaf_idx];
  uint32_t slot_size = rec->slot_size;
  if (slot_size == 0u) {
    return hz10_pagemap_result(H10_ROUTE_MISS, H10_REASON_LEAF_ABSENT, NULL,
                              0u, 0u, 0u, 0u, NULL);
  }

  void* base = rec->base;
  void* owner = rec->owner;
  uint32_t slot_count = rec->slot_count;
  uint32_t generation = rec->generation;
  uint64_t span = (uint64_t)slot_size * (uint64_t)slot_count;

  /* Unsigned subtraction: a "before base" pointer wraps to a huge value and
   * is caught by the tail-slack bound below without a separate check. */
  uint64_t offset = (uint64_t)(addr - (uintptr_t)base);

  if (offset >= span) {
    return hz10_pagemap_result(H10_ROUTE_INVALID, H10_REASON_TAIL_SLACK, base,
                              slot_size, slot_count, 0u, generation, owner);
  }
  if ((offset & (HZ10_MIN_ALIGN - 1u)) != 0u) {
    return hz10_pagemap_result(H10_ROUTE_INVALID, H10_REASON_MISALIGNED, base,
                              slot_size, slot_count, 0u, generation, owner);
  }
  if ((offset % slot_size) != 0u) {
    return hz10_pagemap_result(H10_ROUTE_INVALID, H10_REASON_INTERIOR, base,
                              slot_size, slot_count, 0u, generation, owner);
  }
  if (expected_generation != HZ10_GENERATION_ANY &&
      expected_generation != generation) {
    return hz10_pagemap_result(H10_ROUTE_INVALID, H10_REASON_GENERATION_STALE,
                              base, slot_size, slot_count, 0u, generation,
                              owner);
  }

  return hz10_pagemap_result(H10_ROUTE_VALID, H10_REASON_NONE, base,
                            slot_size, slot_count,
                            (uint32_t)(offset / slot_size), generation,
                            owner);
}

void hz10_pagemap_reset_for_tests(void) {
  hz10_platform_mutex_lock(&hz10_pagemap_lock);
  for (uint32_t i = 0; i < HZ10_ROOT_SIZE; ++i) {
    H10Leaf* leaf = atomic_load_explicit(&hz10_pagemap_root[i],
                                        memory_order_acquire);
    if (leaf) {
      hz10_platform_release((void*)leaf, sizeof(H10Leaf));
      atomic_store_explicit(&hz10_pagemap_root[i], NULL, memory_order_release);
    }
  }
  hz10_platform_mutex_unlock(&hz10_pagemap_lock);
}
