#include "h8_internal.h"

#include <string.h>
#include <sched.h>
#include <stdlib.h>

static void h8_fail_invalid_free(void) {
  H8_DEBUG_INC(invalid_count);
}

static void* h8_small_alloc_from_span(H8ThreadCtx* ctx, H8OwnerRecord* owner,
                                      H8Span* span, uint32_t class_id) {
  uint32_t class_size = h8_class_size(class_id);
  uint32_t local_head = atomic_load_explicit(&span->local_free_head,
                                             memory_order_relaxed);
  if (local_head != UINT32_MAX) {
    uint32_t slot = local_head;
    atomic_store_explicit(&span->local_free_head, span->next_free[slot],
                          memory_order_relaxed);
    if (H8_UNLIKELY(h8_bitmap_test(span->pending_bits, slot))) {
      H8_DEBUG_INC(local_alloc_pending_nonzero);
      abort();
    }
    if (H8_UNLIKELY(!h8_owner_live_set(span, slot))) {
      abort();
    }
    atomic_fetch_add_explicit(&span->used_count, 1, memory_order_relaxed);
    H8_DEBUG_INC(local_alloc_count);
    owner->active_spans[class_id] = span;
    return h8_slot_ptr(span, slot);
  }

  uint32_t bump = atomic_load_explicit(&span->bump_index, memory_order_relaxed);
  if (bump < span->slot_count) {
    if (atomic_compare_exchange_strong_explicit(
            &span->bump_index, &bump, bump + 1, memory_order_relaxed,
            memory_order_relaxed)) {
      if (H8_UNLIKELY(h8_bitmap_test(span->pending_bits, bump))) {
        H8_DEBUG_INC(local_alloc_pending_nonzero);
        abort();
      }
      if (H8_UNLIKELY(!h8_owner_live_set(span, bump))) {
        abort();
      }
      atomic_fetch_add_explicit(&span->used_count, 1, memory_order_relaxed);
      H8_DEBUG_INC(local_alloc_count);
      return h8_slot_ptr(span, bump);
    }
  }

  (void)class_size;
  h8_pressure_owner_collect(owner);
  (void)ctx;
  return NULL;
}

static H8Span* h8_find_active_span(H8OwnerRecord* owner, uint32_t class_id) {
  H8Span* hint = owner->active_spans[class_id];
  if (hint && hint->class_id == class_id &&
      hint->owner_slot == owner->slot &&
      hint->owner_generation == owner->generation &&
      h8_span_state_load(hint) == H8_SPAN_OWNED_ACTIVE &&
      atomic_load_explicit(&hint->used_count, memory_order_acquire) <
          hint->slot_count) {
    return hint;
  }
  pthread_mutex_lock(&owner->owned_lock);
  for (H8Span* span = owner->owned_by_class[class_id]; span;
       span = span->next_owned_class) {
    if (h8_span_state_load(span) == H8_SPAN_OWNED_ACTIVE &&
        atomic_load_explicit(&span->used_count, memory_order_acquire) <
            span->slot_count) {
      pthread_mutex_unlock(&owner->owned_lock);
      return span;
    }
  }
  pthread_mutex_unlock(&owner->owned_lock);
  return NULL;
}

void* h8_malloc_inner(size_t size) {
  h8_init();
  if (size == 0) {
    size = 1;
  }
  if (size > H8_MAX_SMALL_SIZE) {
    return h8_sys_malloc(size);
  }
  H8ThreadCtx* ctx = h8_thread_ctx_get();
  if (!ctx) {
    return NULL;
  }
  uint32_t class_id = h8_class_for_size(size);
  H8OwnerRecord* owner = ctx->owner ? ctx->owner : h8_orphan_owner();
  H8Span* span = owner->active_spans[class_id];
  if (span && span->class_id == class_id && span->owner_slot == owner->slot &&
      span->owner_generation == owner->generation &&
      h8_span_state_load(span) == H8_SPAN_OWNED_ACTIVE) {
    void* ptr = h8_small_alloc_from_span(ctx, owner, span, class_id);
    if (ptr) {
      return ptr;
    }
  }
  span = h8_find_active_span(owner, class_id);
  if (!span) {
    h8_pressure_owner_collect(owner);
    span = h8_find_active_span(owner, class_id);
  }
  if (!span && h8_regular_adoption_enabled() &&
      atomic_load_explicit(&h8g.orphan_span_count, memory_order_acquire) > 0) {
    span = h8_orphan_adopt_span(owner, class_id);
  }
  if (!span) {
    span = h8_span_commit_for_class(owner, class_id);
  }
  if (!span) {
    H8_DEBUG_INC(invalid_count);
    return NULL;
  }
  return h8_small_alloc_from_span(ctx, owner, span, class_id);
}

static bool h8_local_free(H8OwnerRecord* owner, H8Span* span, size_t slot) {
  if (span->owner_slot != owner->slot ||
      span->owner_generation != owner->generation ||
      h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
    return false;
  }
  if (!h8_bitmap_test(span->live_bits, slot)) {
    return false;
  }
  if (H8_UNLIKELY(h8_bitmap_test(span->pending_bits, slot))) {
    H8_DEBUG_INC(local_free_pending_nonzero);
    return false;
  }
  if (H8_UNLIKELY(!h8_owner_live_clear(span, slot))) {
    return false;
  }
  span->next_free[slot] = atomic_load_explicit(&span->local_free_head,
                                               memory_order_relaxed);
  atomic_store_explicit(&span->local_free_head, (uint32_t)slot, memory_order_relaxed);
  atomic_fetch_sub_explicit(&span->used_count, 1, memory_order_relaxed);
  H8_DEBUG_INC(local_free_count);
  owner->active_spans[span->class_id] = span;
  return true;
}

void h8_free_inner(void* ptr) {
  h8_init();
  if (!ptr) {
    return;
  }
  if (!h8_arena_contains(ptr)) {
    H8_DEBUG_INC(miss_count);
    h8_sys_free(ptr);
    return;
  }
  size_t slot = 0;
  H8Span* span = h8_span_from_ptr_checked(ptr, &slot);
  if (!span) {
    h8_fail_invalid_free();
    return;
  }
  H8OwnerRecord* owner = h8_owner_current();
  if (h8_local_free(owner, span, slot)) {
    return;
  }
  for (;;) {
    H8PublishResult res = h8_remote_free_publish(ptr);
    if (res == H8_PUBLISH_OK) {
      return;
    }
    if (res != H8_PUBLISH_OWNER_TRANSITION) {
      break;
    }
    sched_yield();
  }
  h8_fail_invalid_free();
}
