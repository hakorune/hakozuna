#ifndef HZ10_FREELIST_PAGE_H
#define HZ10_FREELIST_PAGE_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#ifndef HZ10_ENABLE_STRIPE_SPREAD
#define HZ10_ENABLE_STRIPE_SPREAD 1
#endif
#ifndef HZ10_STRIPE_SPREAD_MAX_SLOT_SIZE
#define HZ10_STRIPE_SPREAD_MAX_SLOT_SIZE 64u
#endif

#define HZ10_FREELIST_LOCAL_MARKER \
  ((uintptr_t)UINT64_C(0x4831304c46524545)) /* "H10LFREE" */

/* Number of independent Treiber stacks Box 3 (src/hz10_remote_stack.c)
 * spreads remote frees across, keyed by a hash of the freeing thread's own
 * identity -- see hz10_remote_stack.c for why. Must be a power of two (the
 * stripe picker masks instead of modulos). Lives here, not in
 * hz10_remote_stack.h, because the array size has to be known wherever
 * Hz10FreelistPage itself is defined. */
#define HZ10_REMOTE_STRIPE_COUNT 4u
#define HZ10_CACHE_LINE_BYTES 64u

#define HZ10_FREELIST_PAGE_OWNER_LIST_NONE 0u
#define HZ10_FREELIST_PAGE_OWNER_LIST_ACTIVE 1u
#define HZ10_FREELIST_PAGE_OWNER_LIST_RETIRED 2u

typedef struct Hz10RemoteStripeLine {
  _Atomic(void*) head;
  unsigned char pad[HZ10_CACHE_LINE_BYTES - sizeof(_Atomic(void*))];
} Hz10RemoteStripeLine;

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
 * load/store fast path. The only extra metadata written here is a cheap
 * in-slot local-free marker in the second word, used by Box 3 to avoid an
 * O(N) freelist search on every remote drain slot. See tests/README.md.
 */

typedef struct Hz10FreelistPage {
  void* base;             /* HZ10_PAGE_QUANTUM-aligned payload */
  void* local_free_head;  /* intrusive freelist head; NULL == empty */
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t free_count;
  uint32_t generation;    /* returned by hz10_pagemap_register() at create */

  /*
   * Inert storage, same rule as owner_thread_token below: the creator's
   * size-class index for this page (HZ10FrontCache-L1,
   * docs/HZ10_FRONT_CACHE_DESIGN_L0.md), so a public free that already
   * routed to this page can index per-class thread state without
   * recomputing the class from slot_size. Box 2 never reads it. Placed
   * here (not after the list pointers) so it shares the first cache line
   * with base/local_free_head/slot_size/owner_thread_token -- note this
   * moves owner_thread_token from offset 32 to 40, called out per the
   * design doc and re-checked against the stage-cost bench.
   */
  uint32_t class_id;
  uint32_t owner_list_kind;
  uint32_t adopted_count;

  /*
   * Inert storage for whoever creates this page (e.g. a multi-class public
   * entry module) to record "which thread owns me" -- Box 2 never reads or
   * interprets this itself, alloc()/free() do not touch it, and it plays no
   * role in this module's own correctness. It exists so a caller that
   * recovers this page via hz10_pagemap_route()'s owner field (this page's
   * own pointer, if registered with hz10_pagemap_register_with_owner) can
   * decide same-thread (fast local free) vs. foreign-thread (Box 3 remote
   * free) without Box 2 needing to know what a "thread" is.
   */
  _Atomic(void*) owner_thread_token;

  /*
   * Also inert storage, same rule as owner_thread_token: a doubly-linked
   * list pointer pair for whoever wants to track more than one page per
   * class (src/hz10_class_pages.{h,c}) and evict/reclaim from either end
   * in O(1). Box 2 never walks or otherwise touches this list itself.
   */
  struct Hz10FreelistPage* next_in_owner_list;
  struct Hz10FreelistPage* prev_in_owner_list;

  /*
   * HZ10RemoteStackDrain-L0 (Box 3, src/hz10_remote_stack.{h,c}): a
   * foreign thread cannot touch local_free_head directly without breaking
   * Box 2's "owner thread only, plain load/store" guarantee, so a remote
   * free instead pushes onto one of remote_free_head's stripes (a Treiber
   * stack, single CAS) and the owner drains all stripes into
   * local_free_head at its own pace. Splitting into HZ10_REMOTE_STRIPE_COUNT
   * independent stacks spreads list heads logically; the current compact
   * struct layout does not cache-line-pad them, so this is not a cache-line
   * spreading guarantee. HZ10RemoteGapAttribution-L0 recorded that point
   * before opening any locality-shaped follow-up.
   * pending_bits (one bit per slot) is set by a successful remote push and
   * cleared at drain, so a second remote free of the same slot before it
   * drains is rejected as a duplicate instead of corrupting the freelist.
   * For the common slot_count<=64 case, the single pending word is stored
   * inline next to remote_free_head to keep the producer claim/publish and
   * owner drain/clear metadata on one shared line. Larger pages keep the
   * old heap-backed pending array. Tiny high-slot-count classes
   * (slot_size<=HZ10_STRIPE_SPREAD_MAX_SLOT_SIZE) also move the stripe
   * heads into a separate 64B-strided array so small-class producer
   * contention does not false-share one compact stripe-head line.
   * These fields are owned by hz10_remote_stack.c; hz10_freelist_page.c
   * only initializes/tears them down and drains once at destroy time.
   */
  _Atomic(void*) remote_free_head[HZ10_REMOTE_STRIPE_COUNT];
  Hz10RemoteStripeLine* remote_free_spread; /* tiny slot_count>64, else NULL */
  _Atomic(uint64_t) pending_inline_word;
  _Atomic(uint64_t)* pending_bits;
  uint32_t pending_words;   /* (slot_count + 63) / 64 */
  _Atomic(uint64_t) remote_push_count; /* debug-only successful publish */
  _Atomic(uint64_t) remote_duplicate_count;
  _Atomic(uint64_t) remote_invalid_count;
  uint64_t drain_count;      /* owner-only, plain: incremented at drain */
  uint64_t drain_slot_count; /* owner-only, plain: total slots ever drained */
  uint64_t drain_invalid_count; /* owner-only: remote frees rejected at drain */

  /*
   * HZ10RetiredReadyQueue-L0 (src/hz10_retired_ready.{h,c}): an event-
   * driven HINT layer on top of Box 6's polling harvest sweep, wired into
   * hz10_malloc/hz10_free -- deliberately NOT a replacement for harvest's
   * authoritative free_count == slot_count / local_free_head check, so a
   * page it reports as ready is only ever a CANDIDATE -- the caller must
   * still re-verify with the existing check before treating it as
   * reclaimable. A false positive (or a missed one) is a wasted/skipped
   * opportunity, never a correctness issue, because nothing downstream
   * trusts this layer's count on its own.
   *
   * Two real bugs were found (via ASan/TSan under sustained,
   * continuously-running workloads -- see current_task.md) and fixed
   * before this was trustworthy; both are recorded here because the
   * invariants they depend on are easy to accidentally violate later:
   *
   * (1) src/hz10_remote_stack.h's hz10_page_remote_free() is split into
   *     claim() + publish() specifically so hz10_retired_ready_note_
   *     remote_free() (called between them, see hz10_public_entry.c's
   *     hz10_free()) always runs BEFORE the slot becomes visible to the
   *     owner's drain -- a claimed-but-not-published slot cannot be
   *     merged, so free_count cannot reach slot_count, so the owner
   *     cannot conclude the page is idle and destroy/recycle it, while
   *     note_remote_free() is still touching it. Do not introduce a path
   *     that marks a page reclaimable by any means other than free_count
   *     == slot_count, or this invariant breaks.
   * (2) even with (1), a ready-queue ENTRY can still go stale by ABA: the
   *     page could be destroyed and its address recycled for a brand-new
   *     page between being pushed and being popped (Box 6's budgeted
   *     cursor walk never has this problem -- it only ever walks its own
   *     thread-local retired list, never a cross-thread-pushed
   *     reference). retired_ready_generation exists to catch this: the
   *     owner must compare a popped candidate's CURRENT page->generation
   *     against the value captured at mark() time, before touching any
   *     other field (including unlinking it from `retired`) -- a
   *     mismatch means this address now belongs to a different page
   *     entirely, and the popped reference must simply be dropped, not
   *     processed as if it were still the original page.
   *
   * retired_ready_flag is set by the owner only, at the moment a page
   * becomes retired, and read (never written) by a foreign thread's free
   * to decide whether that free should also touch
   * retired_ready_outstanding. Both are untouched, and cost nothing, for
   * a page that is never retired this way -- Box 2/3's existing
   * alloc/free/drain paths do not read or write either field.
   * retired_ready_next is a Treiber-stack link distinct from
   * next_in_owner_list/prev_in_owner_list: a page can be linked into Box
   * 6's retired doubly-linked list AND awaiting a ready-stack push at
   * the same time, so reusing either of those two fields for this would
   * corrupt one or the other. retired_ready_stack is an opaque (to Box
   * 2) pointer to whichever Hz10RetiredReadyStack this page should push
   * itself onto if its count reaches zero -- set by the owner alongside
   * the flag, read by a foreign thread's free so it does not need its
   * own separate lookup of "which stack does this page belong to."
   * retired_ready_on_stack is a THIRD, separate state from
   * retired_ready_flag (found necessary, not assumed, after ASan caught
   * a real use-after-free under bench/hz10_public_entry_steady_state_
   * bench.c's long-running workload): retired_ready_flag is cleared
   * BEFORE the Treiber-stack push in hz10_retired_ready_note_remote_
   * free(), so it cannot also serve as "is this page currently linked
   * into the ready stack" -- and Box 6's budgeted cursor walk, which
   * knows nothing about ready-stack membership, could otherwise destroy
   * a page (via its own, independent free_count==slot_count check)
   * while that same page was still linked into the ready stack via
   * retired_ready_next, leaving a dangling entry for a later pop() to
   * dereference. Set (relaxed) right before the push, cleared (relaxed)
   * right after a successful pop -- the budgeted walk must skip any
   * node with this set, leaving it for the ready-queue path to finish.
   * retired_ready_generation is set by the owner at mark() time, from
   * this page's own (stable, Box-1-assigned) `generation` field -- see
   * bug (2) above for why the ready-queue's pop path must compare the
   * two before trusting a popped candidate.
   */
  _Atomic(int) retired_ready_flag;
  _Atomic(uint32_t) retired_ready_outstanding;
  struct Hz10FreelistPage* retired_ready_next;
  void* retired_ready_stack;
  _Atomic(int) retired_ready_on_stack;
  uint32_t retired_ready_generation;
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

/* Sets owner_thread_token (see the struct comment above). Not required by
 * Box 2 itself; a higher-level module calls this once right after create. */
static inline void hz10_freelist_page_set_owner_thread(Hz10FreelistPage* page,
                                                      void* token) {
  atomic_store_explicit(&page->owner_thread_token, token,
                        memory_order_release);
}

static inline void* hz10_freelist_page_owner_thread(
    const Hz10FreelistPage* page) {
  return atomic_load_explicit(&page->owner_thread_token, memory_order_acquire);
}

/* Sets class_id (see the struct comment above). Same inert-storage rule as
 * set_owner_thread: Box 2 itself never reads this. create() zero-fills it
 * (calloc), so a caller that never sets it sees class_id == 0. */
static inline void hz10_freelist_page_set_class_id(Hz10FreelistPage* page,
                                                  uint32_t class_id) {
  page->class_id = class_id;
}

static inline void hz10_freelist_page_note_adopted(Hz10FreelistPage* page) {
  page->adopted_count += 1u;
}

static inline _Atomic(void*)* hz10_freelist_page_remote_head(
    Hz10FreelistPage* page, uint32_t stripe) {
  return page->remote_free_spread
             ? &page->remote_free_spread[stripe].head
             : &page->remote_free_head[stripe];
}

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

/* Same as hz10_freelist_page_create_with_base(), but registers with Box
 * 1's owner-tag-carrying hz10_pagemap_register_with_owner(owner ==
 * the returned Hz10FreelistPage* itself) in the same registration call,
 * instead of a caller doing a second register() afterward to attach the
 * tag (that second registration was a real, measured cost for
 * small-slot_count classes recreated often -- see current_task.md). */
Hz10FreelistPage* hz10_freelist_page_create_with_base_and_owner(
    void* base, uint32_t slot_size, uint32_t slot_count);

/* Same as hz10_freelist_page_destroy(), but returns the underlying base
 * pointer to the caller instead of unmapping it (NULL if page was NULL).
 * The caller now owns that HZ10_PAGE_QUANTUM block and must either reuse
 * it or release it (hz10_platform_release(base, HZ10_PAGE_QUANTUM)). */
void* hz10_freelist_page_destroy_reclaim_base(Hz10FreelistPage* page);

typedef struct Hz10FreelistMetadataStats {
  uint64_t slab_count;
  uint64_t node_capacity;
  uint64_t free_nodes;
  uint64_t live_nodes;
  uint32_t node_bytes;
  uint32_t slab_bytes;
} Hz10FreelistMetadataStats;

/* Diagnostic-only snapshot for shim/macro RSS attribution. This walks the
 * private metadata free list under its lock, so it is intentionally not a
 * hot-path API. */
void hz10_freelist_metadata_stats(Hz10FreelistMetadataStats* stats_out);

/* LD_PRELOAD shim atfork hooks for this module's private metadata/quantum
 * reservoirs. See hz10_pagemap_atfork_* for the matching route locks. */
void hz10_freelist_page_atfork_prepare(void);
void hz10_freelist_page_atfork_parent(void);
void hz10_freelist_page_atfork_child(void);

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
static inline uintptr_t* hz10_freelist_page_slot_marker_ptr(
    const Hz10FreelistPage* page, void* ptr) {
  if (page->slot_size < (uint32_t)(2u * sizeof(void*))) {
    return NULL;
  }
  return (uintptr_t*)((char*)ptr + sizeof(void*));
}

static inline void hz10_freelist_page_mark_local_free(
    const Hz10FreelistPage* page, void* ptr) {
  uintptr_t* marker = hz10_freelist_page_slot_marker_ptr(page, ptr);
  if (marker) {
    *marker = HZ10_FREELIST_LOCAL_MARKER;
  }
}

static inline void hz10_freelist_page_clear_local_free_marker(
    const Hz10FreelistPage* page, void* ptr) {
  uintptr_t* marker = hz10_freelist_page_slot_marker_ptr(page, ptr);
  if (marker) {
    *marker = 0u;
  }
}

static inline int hz10_freelist_page_is_marked_local_free(
    const Hz10FreelistPage* page, void* ptr) {
  uintptr_t* marker = hz10_freelist_page_slot_marker_ptr(page, ptr);
  return marker && *marker == HZ10_FREELIST_LOCAL_MARKER;
}

static inline void* hz10_freelist_page_alloc(Hz10FreelistPage* page) {
  void* slot = page->local_free_head;
  if (!slot) {
    return NULL;
  }
  page->local_free_head = *(void**)slot;
  hz10_freelist_page_clear_local_free_marker(page, slot);
  page->free_count -= 1u;
  return slot;
}

/* Same-thread fast path. Plain loads/stores only. Caller must pass a
 * pointer this page actually handed out and has not already freed. */
static inline void hz10_freelist_page_free(Hz10FreelistPage* page,
                                          void* ptr) {
  hz10_freelist_page_mark_local_free(page, ptr);
  *(void**)ptr = page->local_free_head;
  page->local_free_head = ptr;
  page->free_count += 1u;
}

#endif
