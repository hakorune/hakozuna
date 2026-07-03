#include "hz10_remote_stack.h"
#include "hz10_pagemap.h"

#include <stdatomic.h>

/* Returns 1 if this call was the one that set the bit (i.e. it was
 * previously clear -- the slot is now claimed for a remote free), 0 if the
 * bit was already set (a duplicate remote free of the same slot). */
static int hz10_page_pending_try_claim(Hz10FreelistPage* page,
                                      uint32_t slot_index) {
  uint32_t word = slot_index / 64u;
  uint64_t bit = UINT64_C(1) << (slot_index % 64u);
  uint64_t old = atomic_fetch_or_explicit(&page->pending_bits[word], bit,
                                         memory_order_acq_rel);
  return (old & bit) == 0u;
}

static void hz10_page_pending_clear(Hz10FreelistPage* page,
                                   uint32_t slot_index) {
  uint32_t word = slot_index / 64u;
  uint64_t bit = UINT64_C(1) << (slot_index % 64u);
  atomic_fetch_and_explicit(&page->pending_bits[word], ~bit,
                            memory_order_acq_rel);
}

/* Same classification pipeline as HZ10PageMapRoute-L0 (Box 1), scoped to a
 * page the caller already resolved: tail-slack, then misaligned, then
 * interior, then generation. Returns 1 and fills *slot_index_out on VALID,
 * 0 otherwise (caller does not need to distinguish the reject reason --
 * hz10_page_remote_free() only needs accept/reject). */
static int hz10_page_classify_for_remote(const Hz10FreelistPage* page,
                                        const void* ptr,
                                        uint32_t expected_generation,
                                        uint32_t* slot_index_out) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)page->base;
  uint64_t offset = (uint64_t)(addr - base);
  uint64_t span = (uint64_t)page->slot_size * (uint64_t)page->slot_count;

  if (offset >= span) {
    return 0; /* tail slack / out of range */
  }
  if ((offset & (HZ10_MIN_ALIGN - 1u)) != 0u) {
    return 0; /* misaligned */
  }
  if ((offset % page->slot_size) != 0u) {
    return 0; /* interior */
  }
  if (expected_generation != HZ10_GENERATION_ANY &&
      expected_generation != page->generation) {
    return 0; /* stale generation */
  }
  *slot_index_out = (uint32_t)(offset / page->slot_size);
  return 1;
}

int hz10_page_remote_free(Hz10FreelistPage* page, void* ptr,
                          uint32_t expected_generation) {
  uint32_t slot_index = 0u;
  if (!hz10_page_classify_for_remote(page, ptr, expected_generation,
                                    &slot_index)) {
    atomic_fetch_add_explicit(&page->remote_invalid_count, 1u,
                              memory_order_relaxed);
    return 0;
  }

  if (!hz10_page_pending_try_claim(page, slot_index)) {
    atomic_fetch_add_explicit(&page->remote_duplicate_count, 1u,
                              memory_order_relaxed);
    return 0;
  }

  /* Treiber stack push. Safe to stash into *ptr: the pending-bit claim
   * above guarantees no other remote free of this same slot is
   * concurrently doing the same thing, and the owner thread never writes
   * to a slot that isn't in local_free_head. */
  void* old_head =
      atomic_load_explicit(&page->remote_free_head, memory_order_relaxed);
  do {
    *(void**)ptr = old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      &page->remote_free_head, &old_head, ptr, memory_order_release,
      memory_order_relaxed));

  atomic_fetch_add_explicit(&page->remote_push_count, 1u,
                            memory_order_relaxed);
  return 1;
}

uint32_t hz10_page_drain_remote(Hz10FreelistPage* page) {
  void* head = atomic_exchange_explicit(&page->remote_free_head, NULL,
                                       memory_order_acquire);
  uint32_t merged = 0u;
  while (head) {
    void* next = *(void**)head;

    uint64_t offset = (uint64_t)((char*)head - (char*)page->base);
    uint32_t slot_index = (uint32_t)(offset / page->slot_size);
    hz10_page_pending_clear(page, slot_index);

    *(void**)head = page->local_free_head;
    page->local_free_head = head;
    page->free_count += 1u;
    merged += 1u;

    head = next;
  }
  if (merged > 0u) {
    page->drain_count += 1u;
    page->drain_slot_count += merged;
  }
  return merged;
}
