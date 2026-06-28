#include "h8_internal.h"
#include "h8_used_count.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static h8_platform_mutex_t h8_span_table_lock = H8_PLATFORM_MUTEX_INIT;

#if defined(H8_ENABLE_DEBUG_STATS)
static uint64_t h8_debug_now_ns(void) {
  return h8_platform_now_ns();
}
#endif

static void* h8_aligned_zero_alloc(size_t align, size_t size, void** raw_out) {
  size_t total = size + align - 1u + sizeof(void*);
  uint8_t* raw = h8_sys_calloc(1, total);
  if (!raw) {
    return NULL;
  }
  uintptr_t start = (uintptr_t)(raw + sizeof(void*));
  uintptr_t aligned = (start + align - 1u) & ~(uintptr_t)(align - 1u);
  *raw_out = raw;
  return (void*)aligned;
}

static H8Span* h8_alloc_span_meta(uint16_t slot_count) {
  size_t words = h8_word_count_for_slots(slot_count);
  size_t off = h8_round_up_size(sizeof(H8Span), sizeof(uint64_t));
  size_t live_off = off;
#if defined(H8_ENABLE_DEBUG_STATS)
  off += words * sizeof(_Atomic uint64_t);
  off = h8_round_up_size(off, sizeof(uint64_t));
#endif
  size_t pending_off = off;
  off += words * sizeof(_Atomic uint64_t);
  off = h8_round_up_size(off, sizeof(uint32_t));
  size_t slot_state_off = off;
  off += (size_t)slot_count * sizeof(_Atomic uint32_t);

  void* raw = NULL;
  uint8_t* block =
      h8_aligned_zero_alloc(H8_CACHELINE_BYTES, off, &raw);
  if (!block) {
    return NULL;
  }
  H8Span* span = (H8Span*)block;
  span->meta_alloc_base = raw;
#if defined(H8_ENABLE_DEBUG_STATS)
  span->live_bits = (_Atomic uint64_t*)(void*)(block + live_off);
#else
  (void)live_off;
#endif
  span->pending_bits = (_Atomic uint64_t*)(void*)(block + pending_off);
  span->slot_state = (_Atomic uint32_t*)(void*)(block + slot_state_off);
  span->meta_bundled = true;
  return span;
}

static size_t h8_owner_next_span_index(H8OwnerRecord* owner) {
  if (owner->span_chunk_next < owner->span_chunk_end) {
    return owner->span_chunk_next++;
  }
  size_t start = atomic_fetch_add_explicit(&h8g.span_alloc_cursor,
                                           H8_OWNER_SPAN_CHUNK,
                                           memory_order_relaxed);
  size_t end = start + H8_OWNER_SPAN_CHUNK;
  if (end > h8g.span_count) {
    end = h8g.span_count;
  }
  owner->span_chunk_next = start + 1u;
  owner->span_chunk_end = end;
  return start;
}

static void h8_span_commit_memory(H8Span* span) {
  (void)span;
  atomic_fetch_add_explicit(&h8g.arena_committed_bytes, H8_SPAN_BYTES,
                            memory_order_relaxed);
}

static void h8_span_decommit_range(void* base, size_t bytes) {
  if (h8_platform_purge(base, bytes) != 0) {
    perror("h8_platform_purge");
    abort();
  }
  atomic_fetch_sub_explicit(&h8g.arena_committed_bytes, bytes,
                            memory_order_relaxed);
}

H8Span* h8_span_commit_for_class(H8OwnerRecord* owner, uint32_t class_id) {
  h8_init();
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t total_start = h8_debug_now_ns();
#endif
  size_t i = h8_owner_next_span_index(owner);
  if (i >= h8g.span_count) {
    return NULL;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t meta_start = h8_debug_now_ns();
#endif
  uint16_t slot_count = (uint16_t)h8_slot_count_for_class(class_id);
  H8Span* span = h8_alloc_span_meta(slot_count);
  if (!span) {
    return NULL;
  }
  span->base = h8_span_base_from_index(i);
  span->class_id = (uint16_t)class_id;
  span->slot_count = slot_count;
  h8_span_owner_word_store(
      span,
      h8_owner_word_make((uint8_t)owner->slot, (uint16_t)owner->generation,
                         H8_SPAN_OWNED_ACTIVE, 1),
      memory_order_relaxed);
  atomic_store_explicit(&span->publish_closed, 0, memory_order_relaxed);
  atomic_store_explicit(&span->publish_refs, 0, memory_order_relaxed);
  atomic_store_explicit(&span->qstate, H8_Q_IDLE, memory_order_relaxed);
  atomic_store_explicit(&span->pending_count, 0, memory_order_relaxed);
  atomic_store_explicit(&span->pending_word_mask, 0, memory_order_relaxed);
  atomic_store_explicit(&span->local_hot.local_bump_index, 0,
                        memory_order_relaxed);
  atomic_store_explicit(&span->local_hot.local_free_head_word, H8_SLOT_NONE,
                        memory_order_relaxed);
  h8_used_count_init(span);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(span_commit_meta_ns, (size_t)(h8_debug_now_ns() - meta_start));
  uint64_t mprotect_start = h8_debug_now_ns();
#endif
  h8_span_commit_memory(span);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(span_commit_mprotect_ns,
               (size_t)(h8_debug_now_ns() - mprotect_start));
#endif
  atomic_store_explicit(&h8g.spans[i], span, memory_order_release);
  h8_owner_add_owned_span(owner, span);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(span_commit_total_ns, (size_t)(h8_debug_now_ns() - total_start));
#endif
  return span;
}

H8Span* h8_span_retire_logical(H8Span* span) {
  if (!span || h8_span_state_load(span) == H8_SPAN_RETIRED) {
    return NULL;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t total_start = h8_debug_now_ns();
  uint64_t lock_start = total_start;
#endif
  h8_platform_mutex_lock(&h8_span_table_lock);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(span_retire_lock_wait_ns,
               (size_t)(h8_debug_now_ns() - lock_start));
#endif
  size_t index = h8_span_index_from_ptr(span->base);
  if (h8_span_state_load(span) == H8_SPAN_RETIRED) {
    h8_platform_mutex_unlock(&h8_span_table_lock);
    return NULL;
  }
  h8_span_state_store(span, H8_SPAN_RETIRED, memory_order_release);
  if (index < h8g.span_count &&
      atomic_load_explicit(&h8g.spans[index], memory_order_acquire) == span) {
    atomic_store_explicit(&h8g.spans[index], NULL, memory_order_release);
  }
  h8_platform_mutex_unlock(&h8_span_table_lock);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(span_retire_count);
  H8_DEBUG_ADD(span_retire_total_ns,
               (size_t)(h8_debug_now_ns() - total_start));
#endif
  return span;
}

static void h8_span_free_retired_meta(H8Span* span) {
  span->next_owned = NULL;
  span->next_owned_class = NULL;
  span->next_orphan = NULL;
  span->next_orphan_class = NULL;
  span->next_pending = NULL;
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t free_start = h8_debug_now_ns();
#endif
  if (!span->meta_bundled) {
    h8_sys_free(span->live_bits);
    h8_sys_free(span->pending_bits);
    h8_sys_free(span->slot_state);
  }
  h8_sys_free(span->meta_alloc_base ? span->meta_alloc_base : span);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(span_retire_meta_free_ns,
               (size_t)(h8_debug_now_ns() - free_start));
#endif
}

static void h8_span_record_purge_run(size_t spans) {
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(span_purge_run_count);
  H8_DEBUG_ADD(span_purge_run_spans_total, spans);
  if (spans == 1) {
    H8_DEBUG_INC(span_purge_singleton_runs);
  }
  size_t cur = atomic_load_explicit(&h8g.span_purge_run_max, memory_order_relaxed);
  while (spans > cur &&
         !atomic_compare_exchange_weak_explicit(&h8g.span_purge_run_max, &cur,
                                                spans, memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
#else
  (void)spans;
#endif
}

static void h8_span_purge_range(uint8_t* base, size_t spans) {
  size_t bytes = spans * H8_SPAN_BYTES;
  h8_span_record_purge_run(spans);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t madvise_start = h8_debug_now_ns();
#endif
  h8_span_decommit_range(base, bytes);
#if defined(H8_ENABLE_DEBUG_STATS)
  size_t elapsed = (size_t)(h8_debug_now_ns() - madvise_start);
  H8_DEBUG_INC(span_purge_madvise_calls);
  H8_DEBUG_ADD(span_purge_madvise_bytes, bytes);
  H8_DEBUG_ADD(span_purge_madvise_ns, elapsed);
  H8_DEBUG_ADD(span_retire_madvise_ns, elapsed);
#endif
}

void h8_span_purge_retired_batch(H8Span* spans) {
  uint8_t* run_base = NULL;
  size_t run_spans = 0;
  H8Span* span = spans;
  while (span) {
    H8Span* next = span->next_owned;
    uint8_t* base = span->base;
    if (!run_base) {
      run_base = base;
      run_spans = 1;
    } else if (base + H8_SPAN_BYTES == run_base) {
      run_base = base;
      ++run_spans;
    } else if (run_base + run_spans * H8_SPAN_BYTES == base) {
      ++run_spans;
    } else {
      h8_span_purge_range(run_base, run_spans);
      run_base = base;
      run_spans = 1;
    }
    h8_span_free_retired_meta(span);
    span = next;
  }
  if (run_base) {
    h8_span_purge_range(run_base, run_spans);
  }
}

void h8_span_retire(H8Span* span) {
  H8Span* retired = h8_span_retire_logical(span);
  if (retired) {
    retired->next_owned = NULL;
    h8_span_purge_retired_batch(retired);
  }
}

H8Span* h8_span_from_ptr_checked(void* ptr, size_t* slot_out) {
  if (!ptr || !h8_arena_contains(ptr)) {
    return NULL;
  }
  size_t index = h8_span_index_from_ptr(ptr);
  H8Span* span = atomic_load_explicit(&h8g.spans[index], memory_order_acquire);
  if (!span || h8_span_state_load(span) == H8_SPAN_RETIRED) {
    return NULL;
  }
  size_t slot = 0;
  if (!h8_slot_index_from_ptr_checked(span, ptr, &slot)) {
    return NULL;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return span;
}
