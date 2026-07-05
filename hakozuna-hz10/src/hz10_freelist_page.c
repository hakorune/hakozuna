#include "hz10_freelist_page.h"
#include "hz10_pagemap.h"
#include "hz10_platform.h"

#include <string.h>

#if !defined(_WIN32)
/*
 * How many quanta to reserve in one go (see hz10_freelist_reserve_aligned_
 * quantum() below). Real, measured motivation, not a guess: strace on the
 * slot_count=1/REMOTE_PCT=90 isolating case (current_task.md) showed
 * ~152K mmap + ~152K munmap over 8M ops -- roughly one genuine miss (a
 * class with nothing to reuse, needing a truly fresh quantum) per 53
 * allocations, each paying a full mmap-2x-then-trim round trip. Batching
 * the mmap+trim dance over HZ10_QUANTUM_REGION_COUNT quanta at once
 * amortizes that cost by the same factor -- one big reservation's trim
 * cost divided across 256 quanta instead of paid by each individually.
 * 256 quanta = 16MiB of *virtual* address space per region, reserved via
 * anonymous mmap (demand-paged by the kernel), not 16MiB of real RSS --
 * only the quanta actually handed out and touched ever cost real memory.
 */
#define HZ10_QUANTUM_REGION_COUNT 256u

/* The mutex below guards only the rare refill (one mmap+trim per
 * HZ10_QUANTUM_REGION_COUNT handouts). The common per-quantum bump is
 * lock-free (CAS on the cursor): a perf stat + strace pass on main_r50/
 * main_r90 (current_task.md) found this mutex wrapping EVERY handout, not
 * just the refill, was the dominant cost under REMOTE_PCT>0 across the
 * full main_local0 class range -- 98%+ of syscall time in futex (130K-210K
 * calls over 8M ops), essentially zero at REMOTE_PCT=0. All 24 size
 * classes and all threads share this one reservoir by design (a page is
 * always exactly one HZ10_PAGE_QUANTUM regardless of class), so any
 * per-handout lock serializes unrelated classes against each other. */
static hz10_platform_mutex_t hz10_quantum_region_lock = HZ10_PLATFORM_MUTEX_INIT;
static _Atomic(char*) hz10_quantum_region_cursor;
static _Atomic(char*) hz10_quantum_region_end;
#endif

enum {
  HZ10_METADATA_PENDING_WORDS =
      (HZ10_PAGE_QUANTUM / HZ10_MIN_ALIGN + 63u) / 64u,
  HZ10_METADATA_SLAB_BYTES = HZ10_PAGE_QUANTUM
};

typedef struct Hz10FreelistPageMetaNode {
  Hz10FreelistPage page;
  _Atomic(uint64_t) pending_words_storage[HZ10_METADATA_PENDING_WORDS];
  Hz10RemoteStripeLine spread_storage[HZ10_REMOTE_STRIPE_COUNT];
  struct Hz10FreelistPageMetaNode* next;
} Hz10FreelistPageMetaNode;

_Static_assert(sizeof(Hz10FreelistPageMetaNode) <= HZ10_METADATA_SLAB_BYTES,
               "HZ10 metadata node must fit in one metadata slab");

static hz10_platform_mutex_t hz10_metadata_slab_lock = HZ10_PLATFORM_MUTEX_INIT;
static Hz10FreelistPageMetaNode* hz10_metadata_free_list;
static uint64_t hz10_metadata_slab_count;
static uint64_t hz10_metadata_node_capacity;

static int hz10_metadata_slab_refill_locked(void) {
  void* slab = hz10_platform_reserve_rw(HZ10_METADATA_SLAB_BYTES);
  if (!slab) {
    return 0;
  }

  size_t count =
      (size_t)HZ10_METADATA_SLAB_BYTES / sizeof(Hz10FreelistPageMetaNode);
  char* cursor = (char*)slab;
  for (size_t i = 0; i < count; ++i) {
    Hz10FreelistPageMetaNode* node =
        (Hz10FreelistPageMetaNode*)(cursor +
                                    i * sizeof(Hz10FreelistPageMetaNode));
    node->next = hz10_metadata_free_list;
    hz10_metadata_free_list = node;
  }
  hz10_metadata_slab_count += 1u;
  hz10_metadata_node_capacity += (uint64_t)count;
  return 1;
}

static Hz10FreelistPage* hz10_metadata_page_alloc(void) {
  hz10_platform_mutex_lock(&hz10_metadata_slab_lock);
  if (!hz10_metadata_free_list && !hz10_metadata_slab_refill_locked()) {
    hz10_platform_mutex_unlock(&hz10_metadata_slab_lock);
    return NULL;
  }
  Hz10FreelistPageMetaNode* node = hz10_metadata_free_list;
  hz10_metadata_free_list = node->next;
  hz10_platform_mutex_unlock(&hz10_metadata_slab_lock);

  memset(node, 0, sizeof(*node));
  return &node->page;
}

static void hz10_metadata_page_free(Hz10FreelistPage* page) {
  if (!page) {
    return;
  }
  Hz10FreelistPageMetaNode* node = (Hz10FreelistPageMetaNode*)page;
  hz10_platform_mutex_lock(&hz10_metadata_slab_lock);
  node->next = hz10_metadata_free_list;
  hz10_metadata_free_list = node;
  hz10_platform_mutex_unlock(&hz10_metadata_slab_lock);
}

/* Reserves an HZ10_PAGE_QUANTUM-aligned, HZ10_PAGE_QUANTUM-sized RW mapping.
 * Plain mmap only guarantees page-size alignment, not our 64KiB quantum, so
 * getting one aligned quantum at a time would need an over-reserve-then-trim
 * dance (the same shape HZ9's segment allocator uses) on every single call.
 * Instead, this hands out quanta from a much larger region reserved (and
 * trimmed to alignment) once every HZ10_QUANTUM_REGION_COUNT calls -- see
 * the constant's comment above for the measured cost this amortizes.
 * Individual quanta are still released independently (hz10_platform_release
 * on a sub-range of a larger mapping is valid POSIX munmap usage, and the
 * bump cursor only ever moves forward, so nothing is ever handed out twice)
 * -- this only changes how a *fresh* quantum is obtained, not how one is
 * given back. */
static void* hz10_freelist_reserve_aligned_quantum(void) {
  size_t bytes = HZ10_PAGE_QUANTUM;
#if defined(_WIN32)
  /* VirtualAlloc returns allocation-granularity aligned regions already,
   * and Windows cannot MEM_RELEASE arbitrary sub-ranges of a larger
   * reservation the way POSIX munmap can -- so the batching below does
   * not apply here; every call is its own VirtualAlloc, same as before. */
  return hz10_platform_reserve_rw(bytes);
#else
  for (;;) {
    char* cursor =
        atomic_load_explicit(&hz10_quantum_region_cursor, memory_order_acquire);
    char* end =
        atomic_load_explicit(&hz10_quantum_region_end, memory_order_acquire);
    /* A failed CAS already writes the current cursor value into `cursor`
     * for us, so retry directly against it instead of paying a fresh
     * atomic load -- `end` stays valid to compare against across those
     * retries too: cursor can only ever reach `end` exactly (never
     * overshoot it, since HZ10_PAGE_QUANTUM evenly divides the region),
     * so a concurrent refill can only be in progress once cursor == end,
     * which this loop's own condition already detects and falls through
     * to the mutex-guarded refill path below for. */
    while (cursor != end) {
      char* next = cursor + bytes;
      if (atomic_compare_exchange_weak_explicit(
              &hz10_quantum_region_cursor, &cursor, next,
              memory_order_acq_rel, memory_order_relaxed)) {
        return cursor;
      }
    }

    /* Exhausted (or never initialized): the mutex now only guards this
     * rare refill, once every HZ10_QUANTUM_REGION_COUNT handouts. */
    hz10_platform_mutex_lock(&hz10_quantum_region_lock);
    /* Re-check under the lock -- another thread may have refilled already
     * while we were waiting for it. */
    cursor =
        atomic_load_explicit(&hz10_quantum_region_cursor, memory_order_acquire);
    end = atomic_load_explicit(&hz10_quantum_region_end, memory_order_acquire);
    if (cursor == end) {
      size_t region_bytes = bytes * (size_t)HZ10_QUANTUM_REGION_COUNT;
      size_t raw_bytes = region_bytes + bytes;
      void* raw = hz10_platform_reserve_rw(raw_bytes);
      if (!raw) {
        hz10_platform_mutex_unlock(&hz10_quantum_region_lock);
        return NULL;
      }
      uintptr_t raw_addr = (uintptr_t)raw;
      uintptr_t aligned_addr =
          (raw_addr + (bytes - 1u)) & ~(uintptr_t)(bytes - 1u);
      size_t head_trim = (size_t)(aligned_addr - raw_addr);
      size_t tail_trim = raw_bytes - head_trim - region_bytes;
      if (head_trim > 0u) {
        hz10_platform_release(raw, head_trim);
      }
      if (tail_trim > 0u) {
        hz10_platform_release((void*)(aligned_addr + region_bytes), tail_trim);
      }
      /* Publish end before cursor: any thread whose lock-free acquire load
       * observes the new cursor is then guaranteed (same-thread program
       * order + release/acquire) to also observe the new end, never a
       * stale one paired with a fresh cursor. */
      atomic_store_explicit(&hz10_quantum_region_end,
                            (char*)aligned_addr + region_bytes,
                            memory_order_release);
      atomic_store_explicit(&hz10_quantum_region_cursor, (char*)aligned_addr,
                            memory_order_release);
    }
    hz10_platform_mutex_unlock(&hz10_quantum_region_lock);
    /* Loop back and retry the lock-free bump against the (now fresh)
     * region -- does not recurse, at most one extra iteration. */
  }
#endif
}

/* Lays down the intrusive chain: slot i's first sizeof(void*) bytes hold a
 * pointer to slot i-1, so the head-of-chain (last one linked, slot 0) is
 * popped first. */
static void hz10_freelist_page_init_chain(Hz10FreelistPage* page) {
  void* head = NULL;
  for (uint32_t i = page->slot_count; i > 0u; --i) {
    void* slot = (char*)page->base + (size_t)(i - 1u) * (size_t)page->slot_size;
    hz10_freelist_page_mark_local_free(page, slot);
    *(void**)slot = head;
    head = slot;
  }
  page->local_free_head = head;
  page->free_count = page->slot_count;
}

/* Shared by hz10_freelist_page_create_with_base() and
 * ..._with_base_and_owner(): allocates the struct/pending_bits storage first
 * (so a self-referential owner tag has something to point at), then registers --
 * with_owner picks which Box 1 registration call to make, so the caller
 * never pays for two registrations of the same page. */
static Hz10FreelistPage* hz10_freelist_page_create_common(void* base,
                                                         uint32_t slot_size,
                                                         uint32_t slot_count,
                                                         int with_owner) {
  if (slot_size < sizeof(void*) || slot_count == 0u) {
    return NULL;
  }
  uint64_t span = (uint64_t)slot_size * (uint64_t)slot_count;
  if (span > (uint64_t)HZ10_PAGE_QUANTUM) {
    return NULL;
  }

  int owns_base = 0;
  if (!base) {
    base = hz10_freelist_reserve_aligned_quantum();
    if (!base) {
      return NULL;
    }
    owns_base = 1;
  }

  Hz10FreelistPage* page = hz10_metadata_page_alloc();
  if (!page) {
    if (owns_base) {
      hz10_platform_release(base, HZ10_PAGE_QUANTUM);
    }
    return NULL;
  }

  uint32_t pending_words = (slot_count + 63u) / 64u;
  _Atomic(uint64_t)* pending_bits = &page->pending_inline_word;
  if (pending_words > 1u) {
    Hz10FreelistPageMetaNode* node = (Hz10FreelistPageMetaNode*)page;
    pending_bits = node->pending_words_storage;
#if HZ10_ENABLE_STRIPE_SPREAD
    if (slot_size <= HZ10_STRIPE_SPREAD_MAX_SLOT_SIZE) {
      page->remote_free_spread = node->spread_storage;
    }
#endif
  }

  uint32_t generation =
      with_owner ? hz10_pagemap_register_with_owner(base, slot_size,
                                                    slot_count, page)
                : hz10_pagemap_register(base, slot_size, slot_count);
  if (generation == 0u) {
    hz10_metadata_page_free(page);
    if (owns_base) {
      hz10_platform_release(base, HZ10_PAGE_QUANTUM);
    }
    return NULL;
  }

  page->base = base;
  page->slot_size = slot_size;
  page->slot_count = slot_count;
  page->generation = generation;
  page->pending_bits = pending_bits;
  page->pending_words = pending_words;
  hz10_freelist_page_init_chain(page);
  return page;
}

Hz10FreelistPage* hz10_freelist_page_create_with_base(void* base,
                                                      uint32_t slot_size,
                                                      uint32_t slot_count) {
  return hz10_freelist_page_create_common(base, slot_size, slot_count, 0);
}

Hz10FreelistPage* hz10_freelist_page_create_with_base_and_owner(
    void* base, uint32_t slot_size, uint32_t slot_count) {
  return hz10_freelist_page_create_common(base, slot_size, slot_count, 1);
}

Hz10FreelistPage* hz10_freelist_page_create(uint32_t slot_size,
                                            uint32_t slot_count) {
  return hz10_freelist_page_create_with_base(NULL, slot_size, slot_count);
}

void* hz10_freelist_page_destroy_reclaim_base(Hz10FreelistPage* page) {
  if (!page) {
    return NULL;
  }
  /* Deliberately does not drain the remote stack (src/hz10_remote_stack.h)
   * itself -- Box 2 has no dependency on Box 3's module, only the reverse
   * (hz10_remote_stack.c includes this header for the struct). A caller
   * that has exposed this page to hz10_page_remote_free() must call
   * hz10_page_drain_remote(page) itself before destroy, or any
   * already-pushed-but-undrained remote free is silently lost. See
   * tests/README.md's HZ10RemoteStackDrain-L0 notes. */
  hz10_pagemap_release(page->base);
  void* base = page->base;
  hz10_metadata_page_free(page);
  return base;
}

void hz10_freelist_page_destroy(Hz10FreelistPage* page) {
  void* base = hz10_freelist_page_destroy_reclaim_base(page);
  if (base) {
    hz10_platform_release(base, HZ10_PAGE_QUANTUM);
  }
}

void hz10_freelist_metadata_stats(Hz10FreelistMetadataStats* stats_out) {
  if (!stats_out) {
    return;
  }
  Hz10FreelistMetadataStats stats = {0};
  hz10_platform_mutex_lock(&hz10_metadata_slab_lock);
  stats.slab_count = hz10_metadata_slab_count;
  stats.node_capacity = hz10_metadata_node_capacity;
  for (Hz10FreelistPageMetaNode* node = hz10_metadata_free_list; node;
       node = node->next) {
    stats.free_nodes += 1u;
  }
  hz10_platform_mutex_unlock(&hz10_metadata_slab_lock);
  stats.live_nodes = stats.node_capacity - stats.free_nodes;
  stats.node_bytes = (uint32_t)sizeof(Hz10FreelistPageMetaNode);
  stats.slab_bytes = (uint32_t)HZ10_METADATA_SLAB_BYTES;
  *stats_out = stats;
}

void hz10_freelist_page_atfork_prepare(void) {
  hz10_platform_mutex_lock(&hz10_metadata_slab_lock);
#if !defined(_WIN32)
  hz10_platform_mutex_lock(&hz10_quantum_region_lock);
#endif
}

void hz10_freelist_page_atfork_parent(void) {
#if !defined(_WIN32)
  hz10_platform_mutex_unlock(&hz10_quantum_region_lock);
#endif
  hz10_platform_mutex_unlock(&hz10_metadata_slab_lock);
}

void hz10_freelist_page_atfork_child(void) {
#if !defined(_WIN32)
  hz10_platform_mutex_unlock(&hz10_quantum_region_lock);
#endif
  hz10_platform_mutex_unlock(&hz10_metadata_slab_lock);
}
