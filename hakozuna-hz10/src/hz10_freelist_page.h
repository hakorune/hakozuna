#ifndef HZ10_FREELIST_PAGE_H
#define HZ10_FREELIST_PAGE_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

/*
 * HZ10ThreadLocalFreelistPage-L0: Layer 0 of the HZ10 design -- one class,
 * one page, an intrusive freelist (a free slot's own first bytes hold the
 * next-free pointer, so alloc/free are a plain pop/push of local_free_head,
 * no atomics, no per-op touch of anything outside this struct).
 *
 * Design rule (docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md): route metadata is
 * static after page publication. hz10_freelist_page_create() registers with
 * HZ10PageMapRoute-L0 (Box 1) exactly once; hz10_freelist_page_destroy()
 * releases exactly once. alloc()/free() never touch the pagemap.
 *
 * Known, accepted gap: alloc()/free() trust the caller. A same-thread
 * double-free is not rejected here -- fail-closed validation for untrusted
 * pointers is the route boundary's job (Layer 1), not this local, plain
 * load/store fast path. See tests/README.md.
 */

typedef struct Hz10FreelistPage {
  void* base;             /* HZ10_PAGE_QUANTUM-aligned payload */
  void* local_free_head;  /* intrusive freelist head; NULL == empty */
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t free_count;
  uint32_t generation;    /* returned by hz10_pagemap_register() at create */

  /*
   * HZ10RemoteStackDrain-L0 (Box 3, src/hz10_remote_stack.{h,c}): a
   * foreign thread cannot touch local_free_head directly without breaking
   * Box 2's "owner thread only, plain load/store" guarantee, so a remote
   * free instead pushes onto remote_free_head (a Treiber stack, single
   * CAS) and the owner drains it into local_free_head at its own pace.
   * pending_bits (one bit per slot) is set by a successful remote push and
   * cleared at drain, so a second remote free of the same slot before it
   * drains is rejected as a duplicate instead of corrupting the freelist.
   * These fields are owned by hz10_remote_stack.c; hz10_freelist_page.c
   * only initializes/tears them down and drains once at destroy time.
   */
  _Atomic(void*) remote_free_head;
  _Atomic(uint64_t)* pending_bits;
  uint32_t pending_words;   /* (slot_count + 63) / 64 */
  _Atomic(uint64_t) remote_push_count;
  _Atomic(uint64_t) remote_duplicate_count;
  _Atomic(uint64_t) remote_invalid_count;
  uint64_t drain_count;      /* owner-only, plain: incremented at drain */
  uint64_t drain_slot_count; /* owner-only, plain: total slots ever drained */
  uint64_t drain_invalid_count; /* owner-only: remote frees rejected at drain */
} Hz10FreelistPage;

/*
 * Creates a page of slot_count slots of slot_size bytes each. Requires
 * slot_size >= sizeof(void*) (the freelist needs room for the intrusive
 * next-pointer) and slot_size * slot_count <= HZ10_PAGE_QUANTUM (Box 1's
 * single-quantum registration limit). Returns NULL on any failure (bad
 * arguments, mmap failure, or pagemap registration failure).
 */
Hz10FreelistPage* hz10_freelist_page_create(uint32_t slot_size,
                                            uint32_t slot_count);

/* Releases the page from the pagemap and unmaps its payload. */
void hz10_freelist_page_destroy(Hz10FreelistPage* page);

/*
 * Pool-agnostic hooks for HZ10BoundedPagePool-L0 (Box 4,
 * src/hz10_pooled_page.{h,c}): this header/module still knows nothing
 * about page pooling -- these two functions just let a caller supply
 * memory instead of mmap'ing fresh, or take memory back instead of
 * unmap'ing it, keeping the "where does memory come from" question
 * entirely outside Box 2.
 */

/* Same as hz10_freelist_page_create(), but if base is non-NULL, uses that
 * (already HZ10_PAGE_QUANTUM-aligned, HZ10_PAGE_QUANTUM-sized) block
 * instead of reserving a fresh mapping. base == NULL behaves identically
 * to hz10_freelist_page_create(). */
Hz10FreelistPage* hz10_freelist_page_create_with_base(void* base,
                                                      uint32_t slot_size,
                                                      uint32_t slot_count);

/* Same as hz10_freelist_page_destroy(), but returns the underlying base
 * pointer to the caller instead of unmapping it (NULL if page was NULL).
 * The caller now owns that HZ10_PAGE_QUANTUM block and must either reuse
 * it or release it (hz10_platform_release(base, HZ10_PAGE_QUANTUM)). */
void* hz10_freelist_page_destroy_reclaim_base(Hz10FreelistPage* page);

/*
 * alloc()/free() are the real hot path (called every malloc/free, unlike
 * create/destroy which run once per page), so they are `static inline` in
 * the header rather than exported .c functions -- the same reason real
 * fast allocators (mimalloc, tcmalloc) expose their hot path header-only:
 * a caller in a different translation unit still gets these inlined
 * without depending on a build-flag trick (e.g. -flto) to get there. Box
 * 1's hz10_pagemap_route() stays a regular function on purpose, since it's
 * a boundary operation, not a per-op one.
 */

/* Same-thread fast path. Plain loads/stores only. NULL if empty. */
static inline void* hz10_freelist_page_alloc(Hz10FreelistPage* page) {
  void* slot = page->local_free_head;
  if (!slot) {
    return NULL;
  }
  page->local_free_head = *(void**)slot;
  page->free_count -= 1u;
  return slot;
}

/* Same-thread fast path. Plain loads/stores only. Caller must pass a
 * pointer this page actually handed out and has not already freed. */
static inline void hz10_freelist_page_free(Hz10FreelistPage* page,
                                          void* ptr) {
  *(void**)ptr = page->local_free_head;
  page->local_free_head = ptr;
  page->free_count += 1u;
}

#endif
