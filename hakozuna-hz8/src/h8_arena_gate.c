#include "h8_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#error "HZ8 v0 implementation currently targets Linux x86_64."
#endif

#include <sys/mman.h>
#if defined(H8_ENABLE_DEBUG_STATS)
#include <time.h>
#endif

static pthread_mutex_t h8_span_table_lock = PTHREAD_MUTEX_INITIALIZER;

#if defined(H8_ENABLE_DEBUG_STATS)
static uint64_t h8_debug_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}
#endif

static H8Span* h8_alloc_span_meta(void) {
  H8Span* span = h8_sys_calloc(1, sizeof(*span));
  return span;
}

static void h8_span_commit_memory(H8Span* span) {
  (void)span;
  atomic_fetch_add_explicit(&h8g.arena_committed_bytes, H8_SPAN_BYTES,
                            memory_order_relaxed);
}

static void h8_span_decommit_memory(H8Span* span) {
  if (madvise(span->base, H8_SPAN_BYTES, MADV_DONTNEED) != 0) {
    perror("madvise");
    abort();
  }
  atomic_fetch_sub_explicit(&h8g.arena_committed_bytes, H8_SPAN_BYTES,
                            memory_order_relaxed);
}

H8Span* h8_span_commit_for_class(H8OwnerRecord* owner, uint32_t class_id) {
  h8_init();
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t total_start = h8_debug_now_ns();
#endif
  size_t i =
      atomic_fetch_add_explicit(&h8g.span_alloc_cursor, 1, memory_order_relaxed);
  if (i >= h8g.span_count) {
    return NULL;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t meta_start = h8_debug_now_ns();
#endif
  H8Span* span = h8_alloc_span_meta();
  if (!span) {
    return NULL;
  }
  span->base = h8_span_base_from_index(i);
  span->class_id = (uint16_t)class_id;
  span->slot_count = (uint16_t)h8_slot_count_for_class(class_id);
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
  atomic_store_explicit(&span->bump_index, 0, memory_order_relaxed);
  atomic_store_explicit(&span->local_free_head, UINT32_MAX, memory_order_relaxed);
  atomic_store_explicit(&span->used_count, 0, memory_order_relaxed);
  span->live_bits = h8_sys_calloc(h8_word_count_for_slots(span->slot_count),
                                  sizeof(_Atomic uint64_t));
  span->pending_bits = h8_sys_calloc(h8_word_count_for_slots(span->slot_count),
                                     sizeof(_Atomic uint64_t));
  span->next_free = h8_sys_calloc(span->slot_count, sizeof(uint32_t));
  bool use_slot_state = h8_slot_state_authority_enabled()
#if defined(H8_ENABLE_DEBUG_STATS)
                        || true
#endif
      ;
  if (use_slot_state) {
    span->slot_state = h8_sys_calloc(span->slot_count, sizeof(_Atomic uint32_t));
  }
  if (!span->live_bits || !span->pending_bits || !span->next_free ||
      (use_slot_state && !span->slot_state)) {
    h8_sys_free(span->live_bits);
    h8_sys_free(span->pending_bits);
    h8_sys_free(span->next_free);
    h8_sys_free(span->slot_state);
    h8_sys_free(span);
    return NULL;
  }
  for (uint32_t slot = 0; slot < span->slot_count; ++slot) {
    span->next_free[slot] = UINT32_MAX;
  }
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

void h8_span_retire(H8Span* span) {
  if (!span || h8_span_state_load(span) == H8_SPAN_RETIRED) {
    return;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t total_start = h8_debug_now_ns();
  uint64_t lock_start = total_start;
#endif
  pthread_mutex_lock(&h8_span_table_lock);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(span_retire_lock_wait_ns,
               (size_t)(h8_debug_now_ns() - lock_start));
#endif
  size_t index = h8_span_index_from_ptr(span->base);
  h8_span_state_store(span, H8_SPAN_RETIRED, memory_order_relaxed);
  if (index < h8g.span_count &&
      atomic_load_explicit(&h8g.spans[index], memory_order_acquire) == span) {
    atomic_store_explicit(&h8g.spans[index], NULL, memory_order_release);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t madvise_start = h8_debug_now_ns();
#endif
  h8_span_decommit_memory(span);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(span_retire_madvise_ns,
               (size_t)(h8_debug_now_ns() - madvise_start));
#endif
  span->next_owned = NULL;
  span->next_owned_class = NULL;
  span->next_orphan = NULL;
  span->next_orphan_class = NULL;
  span->next_pending = NULL;
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t free_start = h8_debug_now_ns();
#endif
  h8_sys_free(span->live_bits);
  h8_sys_free(span->pending_bits);
  h8_sys_free(span->next_free);
  h8_sys_free(span->slot_state);
  h8_sys_free(span);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(span_retire_meta_free_ns,
               (size_t)(h8_debug_now_ns() - free_start));
#endif
  pthread_mutex_unlock(&h8_span_table_lock);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(span_retire_count);
  H8_DEBUG_ADD(span_retire_total_ns,
               (size_t)(h8_debug_now_ns() - total_start));
#endif
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
  size_t slot = h8_slot_index_from_ptr(span, ptr);
  if (slot >= span->slot_count) {
    return NULL;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return span;
}
