#include "hz10_remote_stack.h"
#include "hz10_pagemap.h"

#include <stdatomic.h>

/*
 * Contention mitigation, not batching: hz10_remote_stack_drain_bench
 * measured 4 producer threads remote-freeing concurrently costing ~25x a
 * single-threaded local free, on one shared remote_free_head cache line
 * (CAS/cache-line-ownership traffic, not retried CAS loops -- the Treiber
 * push itself is cheap when uncontended). True cross-thread batching (stage
 * several frees locally, flush as one push) was considered and rejected for
 * this iteration: it needs a guaranteed eventual flush (a thread that
 * remote-frees exactly once and then does nothing else could leave that
 * free stuck in a thread-local staging buffer forever, silently breaking
 * the "accepted means at least drainable" contract every other box and
 * smoke test relies on) and HZ10 has no thread-lifecycle/timer hook to
 * guarantee that yet. Splitting remote_free_head into
 * HZ10_REMOTE_STRIPE_COUNT independent Treiber stacks, keyed by a hash of
 * the freeing thread, spreads the SAME number of CAS operations across
 * independent list heads. The current page layout keeps those heads compact
 * instead of cache-line-padding each stripe; HZ10RemoteGapAttribution-L0
 * recorded that before the pending/stripe colocation box. Revisit true
 * batching later if this isn't enough and a thread-lifecycle mechanism
 * exists to hang a flush guarantee on.
 */
static _Thread_local char hz10_remote_stripe_token_storage;

static uint32_t hz10_remote_pick_stripe(void) {
  uintptr_t addr = (uintptr_t)&hz10_remote_stripe_token_storage;
  addr ^= addr >> 16;
  addr ^= addr >> 8;
  return (uint32_t)addr & (HZ10_REMOTE_STRIPE_COUNT - 1u);
}

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

static int hz10_page_local_freelist_contains(const Hz10FreelistPage* page,
                                             const void* ptr) {
  const void* node = page->local_free_head;
  uint32_t steps = 0u;
  while (node && steps < page->slot_count) {
    if (node == ptr) {
      return 1;
    }
    node = *(void* const*)node;
    steps += 1u;
  }
  return 0;
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

int hz10_page_remote_free_claim(Hz10FreelistPage* page, void* ptr,
                                uint32_t expected_generation,
                                uint32_t* slot_index_out) {
  uint32_t slot_index = 0u;
  if (!hz10_page_classify_for_remote(page, ptr, expected_generation,
                                    &slot_index)) {
    atomic_fetch_add_explicit(&page->remote_invalid_count, 1u,
                              memory_order_relaxed);
    return 0;
  }

  /* A slot already on the owner's local freelist has its first word in use
   * as the local next pointer. Do not Treiber-push through that same word:
   * reject before publishing so an invalid foreign double-free cannot corrupt
   * the owner's local chain. */
  if (hz10_freelist_page_is_marked_local_free(page, ptr)) {
    atomic_fetch_add_explicit(&page->remote_invalid_count, 1u,
                              memory_order_relaxed);
    return 0;
  }

  if (!hz10_page_pending_try_claim(page, slot_index)) {
    atomic_fetch_add_explicit(&page->remote_duplicate_count, 1u,
                              memory_order_relaxed);
    return 0;
  }

  if (slot_index_out) {
    *slot_index_out = slot_index;
  }
  return 1;
}

void hz10_page_remote_free_publish(Hz10FreelistPage* page, void* ptr) {
  /* Debug-only: keep the successful-publish counter out of the release hot
   * path. The CAS below is the load-bearing publication point; adding a
   * second uncontended RMW here measured as a real remote-row cost. */
#if defined(HZ10_ENABLE_DEBUG_STATS)
  atomic_fetch_add_explicit(&page->remote_push_count, 1u,
                            memory_order_relaxed);
#endif

  /* Treiber stack push, onto this thread's stripe. Safe to stash into
   * *ptr: the pending-bit claim (in claim(), already done) guarantees no
   * other remote free of this same slot is concurrently doing the same
   * thing, and the owner thread never writes to a slot that isn't in
   * local_free_head. */
  _Atomic(void*)* stack =
      hz10_freelist_page_remote_head(page, hz10_remote_pick_stripe());
  void* old_head = atomic_load_explicit(stack, memory_order_relaxed);
  do {
    *(void**)ptr = old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      stack, &old_head, ptr, memory_order_release, memory_order_relaxed));
  /* Nothing may follow this CAS -- see the module comment in the header. */
}

int hz10_page_remote_free(Hz10FreelistPage* page, void* ptr,
                          uint32_t expected_generation) {
  uint32_t slot_index;
  if (!hz10_page_remote_free_claim(page, ptr, expected_generation,
                                  &slot_index)) {
    return 0;
  }
  hz10_page_remote_free_publish(page, ptr);
  return 1;
}

uint32_t hz10_page_drain_remote(Hz10FreelistPage* page) {
  uint32_t merged = 0u;
  for (uint32_t s = 0u; s < HZ10_REMOTE_STRIPE_COUNT; ++s) {
    _Atomic(void*)* stack = hz10_freelist_page_remote_head(page, s);

    /* Relaxed peek before paying for the exchange: a caller that scans
     * many candidate pages (src/hz10_class_pages.h) calls this on pages
     * that usually have nothing pending in any stripe, and an atomic RMW
     * (exchange) costs more than a plain load on most architectures. A
     * stale NULL read here just means this call skips a drain that a push
     * landed a moment too late for -- correctness-neutral, the next drain
     * call picks it up, same as any lock-free peek-before-acting fast
     * path. */
    if (atomic_load_explicit(stack, memory_order_relaxed) == NULL) {
      continue;
    }
    void* head = atomic_exchange_explicit(stack, NULL, memory_order_acquire);
    while (head) {
      void* next = *(void**)head;

      uint64_t offset = (uint64_t)((char*)head - (char*)page->base);
      uint32_t slot_index = (uint32_t)(offset / page->slot_size);
      hz10_page_pending_clear(page, slot_index);

      if (hz10_freelist_page_is_marked_local_free(page, head) &&
          hz10_page_local_freelist_contains(page, head)) {
        page->drain_invalid_count += 1u;
        atomic_fetch_add_explicit(&page->remote_invalid_count, 1u,
                                  memory_order_relaxed);
        head = next;
        continue;
      }

      hz10_freelist_page_mark_local_free(page, head);
      *(void**)head = page->local_free_head;
      page->local_free_head = head;
      page->free_count += 1u;
      merged += 1u;

      head = next;
    }
  }
  if (merged > 0u) {
    page->drain_count += 1u;
    page->drain_slot_count += merged;
  }
  return merged;
}

int hz10_page_classify_for_remote_probe(const Hz10FreelistPage* page,
                                        const void* ptr,
                                        uint32_t expected_generation,
                                        uint32_t* slot_index_out) {
  return hz10_page_classify_for_remote(page, ptr, expected_generation,
                                      slot_index_out);
}
