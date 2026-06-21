#include "h8_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#error "HZ8 v0 implementation currently targets Linux x86_64."
#endif

#include <sys/mman.h>

static pthread_mutex_t h8_span_table_lock = PTHREAD_MUTEX_INITIALIZER;

static H8Span* h8_alloc_span_meta(void) {
  H8Span* span = h8_sys_calloc(1, sizeof(*span));
  return span;
}

static void h8_span_commit_memory(H8Span* span) {
  void* base = span->base;
  if (mprotect(base, H8_SPAN_BYTES, PROT_READ | PROT_WRITE) != 0) {
    perror("mprotect");
    abort();
  }
  atomic_fetch_add_explicit(&h8g.arena_committed_bytes, H8_SPAN_BYTES,
                            memory_order_relaxed);
}

static void h8_span_decommit_memory(H8Span* span) {
  if (mprotect(span->base, H8_SPAN_BYTES, PROT_NONE) != 0) {
    perror("mprotect");
    abort();
  }
  atomic_fetch_sub_explicit(&h8g.arena_committed_bytes, H8_SPAN_BYTES,
                            memory_order_relaxed);
}

H8Span* h8_span_commit_for_class(H8OwnerRecord* owner, uint32_t class_id) {
  h8_init();
  pthread_mutex_lock(&h8_span_table_lock);
  for (size_t i = 0; i < h8g.span_count; ++i) {
    if (atomic_load_explicit(&h8g.spans[i], memory_order_acquire)) {
      continue;
    }
    H8Span* span = h8_alloc_span_meta();
    if (!span) {
      pthread_mutex_unlock(&h8_span_table_lock);
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
    if (!span->live_bits || !span->pending_bits || !span->next_free) {
      h8_sys_free(span->live_bits);
      h8_sys_free(span->pending_bits);
      h8_sys_free(span->next_free);
      h8_sys_free(span);
      pthread_mutex_unlock(&h8_span_table_lock);
      return NULL;
    }
    for (uint32_t slot = 0; slot < span->slot_count; ++slot) {
      span->next_free[slot] = UINT32_MAX;
    }
    h8_span_commit_memory(span);
    atomic_store_explicit(&h8g.spans[i], span, memory_order_release);
    h8_owner_add_owned_span(owner, span);
    pthread_mutex_unlock(&h8_span_table_lock);
    return span;
  }
  pthread_mutex_unlock(&h8_span_table_lock);
  return NULL;
}

void h8_span_retire(H8Span* span) {
  if (!span || h8_span_state_load(span) == H8_SPAN_RETIRED) {
    return;
  }
  pthread_mutex_lock(&h8_span_table_lock);
  size_t index = h8_span_index_from_ptr(span->base);
  h8_span_state_store(span, H8_SPAN_RETIRED, memory_order_relaxed);
  if (index < h8g.span_count &&
      atomic_load_explicit(&h8g.spans[index], memory_order_acquire) == span) {
    atomic_store_explicit(&h8g.spans[index], NULL, memory_order_release);
  }
  h8_span_decommit_memory(span);
  span->next_owned = NULL;
  span->next_owned_class = NULL;
  span->next_orphan = NULL;
  span->next_orphan_class = NULL;
  span->next_pending = NULL;
  h8_sys_free(span->live_bits);
  h8_sys_free(span->pending_bits);
  h8_sys_free(span->next_free);
  h8_sys_free(span);
  pthread_mutex_unlock(&h8_span_table_lock);
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
