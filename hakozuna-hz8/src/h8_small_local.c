#include "h8_internal.h"
#include "h8_used_count.h"

#include <string.h>
#include <sched.h>
#include <stdlib.h>

static void h8_fail_invalid_free(void) {
  H8_DEBUG_INC(invalid_count);
}

static inline H8OwnerRecord* h8_ctx_owner_assume(H8ThreadCtx* ctx) {
  H8OwnerRecord* owner = ctx->owner;
  if (H8_UNLIKELY(!owner)) {
#if defined(H8_ENABLE_DEBUG_STATS)
    abort();
#else
    __builtin_unreachable();
#endif
  }
  return owner;
}

static void h8_owner_used_add(H8Span* span, size_t count) {
  H8_DEBUG_INC(local_used_touch_alloc);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(local_used_count_load_alloc);
  H8_DEBUG_INC(local_used_count_store_alloc);
  h8_used_count_mirror_add(span, count);
#else
  (void)span;
  (void)count;
#endif
}

static bool h8_owner_used_sub(H8Span* span, size_t count) {
  H8_DEBUG_INC(local_used_touch_free);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(local_used_count_load_free);
  H8_DEBUG_INC(local_used_count_store_free);
  if (H8_UNLIKELY(!h8_used_count_mirror_sub(span, count))) {
    H8_DEBUG_INC(local_used_count_underflow);
    abort();
  }
#else
  (void)span;
  (void)count;
#endif
  return true;
}

static inline void* h8_small_alloc_from_span(H8Span* span) {
  H8_DEBUG_INC(local_free_head_touch_alloc);
  uint32_t local_head = atomic_load_explicit(&span->local_hot.local_free_head_word,
                                             memory_order_relaxed);
  if (local_head != H8_SLOT_NONE) {
    uint32_t slot = local_head;
    _Atomic uint32_t* slot_state = span->slot_state;
#if defined(H8_ENABLE_DEBUG_STATS)
    h8_slot_shadow_expect(span, slot, H8_SLOT_FREE >> H8_SLOT_TAG_SHIFT);
#endif
    uint32_t state = h8_slot_state_load_ptr_hot(slot_state, slot);
    uint32_t next = h8_slot_state_decode_next(h8_slot_state_payload(state));
    atomic_store_explicit(&span->local_hot.local_free_head_word, next,
                          memory_order_relaxed);
    H8_DEBUG_INC(local_freelist_pop);
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_INC(local_pending_check_alloc);
    if (H8_UNLIKELY(h8_bitmap_test(span->pending_bits, slot))) {
      H8_DEBUG_INC(local_alloc_pending_nonzero);
      abort();
    }
#endif
    H8_DEBUG_INC(local_live_touch_alloc);
    h8_debug_local_live_word(slot);
#if defined(H8_ENABLE_DEBUG_STATS)
    if (H8_UNLIKELY(!h8_owner_live_set(span, slot))) {
      abort();
    }
#endif
    h8_slot_state_store_allocated_ptr_hot(slot_state, slot);
    h8_owner_used_add(span, 1);
    H8_DEBUG_INC(local_alloc_count);
    return h8_slot_ptr(span, slot);
  }

  uint32_t bump = atomic_load_explicit(&span->local_hot.local_bump_index,
                                       memory_order_relaxed);
  if (bump < span->slot_count) {
#if defined(H8_ENABLE_DEBUG_STATS)
    h8_slot_shadow_expect(span, bump, H8_SLOT_NEVER_USED >> H8_SLOT_TAG_SHIFT);
#endif
    atomic_store_explicit(&span->local_hot.local_bump_index, bump + 1,
                          memory_order_relaxed);
    H8_DEBUG_INC(local_bump_alloc);
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_INC(local_pending_check_alloc);
    if (H8_UNLIKELY(h8_bitmap_test(span->pending_bits, bump))) {
      H8_DEBUG_INC(local_alloc_pending_nonzero);
      abort();
    }
#endif
    H8_DEBUG_INC(local_live_touch_alloc);
    h8_debug_local_live_word(bump);
#if defined(H8_ENABLE_DEBUG_STATS)
    if (H8_UNLIKELY(!h8_owner_live_set(span, bump))) {
      abort();
    }
#endif
    h8_slot_state_store_allocated_hot(span, bump);
    h8_owner_used_add(span, 1);
    H8_DEBUG_INC(local_alloc_count);
    return h8_slot_ptr(span, bump);
  }

  return NULL;
}

static bool h8_active_hint_matches(H8Span* span, H8OwnerRecord* owner,
                                   uint32_t class_id) {
  if (span->class_id != class_id) {
    H8_DEBUG_INC(local_active_hint_class_mismatch);
    return false;
  }
  if (span->owner_slot != owner->slot) {
    H8_DEBUG_INC(local_active_hint_owner_mismatch);
    return false;
  }
  if (span->owner_generation != owner->generation) {
    H8_DEBUG_INC(local_active_hint_generation_mismatch);
    return false;
  }
  if (h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
    H8_DEBUG_INC(local_active_hint_state_mismatch);
    return false;
  }
  H8_DEBUG_INC(local_active_hint_trusted);
  return true;
}

static bool h8_span_local_exhausted(H8Span* span) {
  uint32_t head = atomic_load_explicit(&span->local_hot.local_free_head_word,
                                       memory_order_relaxed);
  if (head != H8_SLOT_NONE) {
    return false;
  }
  uint32_t bump = atomic_load_explicit(&span->local_hot.local_bump_index,
                                       memory_order_relaxed);
  return bump >= span->slot_count;
}

static H8Span* h8_find_active_span(H8ThreadCtx* ctx, H8OwnerRecord* owner,
                                   uint32_t class_id) {
  H8_DEBUG_INC(local_find_scan);
  H8Span* hint = ctx->active_spans[class_id];
  bool hint_full = false;
  if (!hint) {
    H8_DEBUG_INC(local_active_hint_null);
  } else if (!h8_active_hint_matches(hint, owner, class_id)) {
    H8_DEBUG_INC(local_active_hint_state_blocked);
  } else if (h8_span_local_exhausted(hint)) {
    H8_DEBUG_INC(local_active_hint_full);
    hint_full = true;
  } else {
    return hint;
  }
  if (hint_full &&
      atomic_load_explicit(&owner->pending_span_count, memory_order_acquire) == 0) {
    H8_DEBUG_INC(local_find_skip_scan_no_pending);
    return NULL;
  }
  pthread_mutex_lock(&owner->owned_lock);
  for (H8Span* span = owner->owned_by_class[class_id]; span;
       span = span->next_owned_class) {
    H8_DEBUG_INC(local_find_scan_span);
    if (h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
      H8_DEBUG_INC(local_find_scan_span_state_blocked);
      continue;
    }
    if (h8_span_local_exhausted(span)) {
      H8_DEBUG_INC(local_find_scan_span_full);
      continue;
    }
    H8_DEBUG_INC(local_find_scan_span_usable);
    pthread_mutex_unlock(&owner->owned_lock);
    return span;
  }
  pthread_mutex_unlock(&owner->owned_lock);
  return NULL;
}

void* h8_malloc_inner(size_t size) {
  if (size == 0) {
    size = 1;
  }
  if (size > H8_MAX_SMALL_SIZE) {
    if (h8_medium_size_supported(size)) {
      return h8_medium_malloc_inner(size);
    }
    return h8_sys_malloc(size);
  }
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  if (!ctx) {
    return NULL;
  }
  uint32_t class_id = h8_class_for_size(size);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8OwnerRecord* owner = h8_ctx_owner_assume(ctx);
#else
  H8OwnerRecord* owner = NULL;
#endif
  H8Span* span = ctx->active_spans[class_id];
  bool active_hint_ok = false;
  if (span) {
#if defined(H8_ENABLE_DEBUG_STATS)
    active_hint_ok = h8_active_hint_matches(span, owner, class_id);
#else
    active_hint_ok = true;
#endif
  }
  if (active_hint_ok) {
    H8_DEBUG_INC(local_active_hit);
    H8_DEBUG_INC(local_used_count_full_check);
    void* ptr = h8_small_alloc_from_span(span);
    if (ptr) {
      return ptr;
    }
#if !defined(H8_ENABLE_DEBUG_STATS)
    owner = h8_ctx_owner_assume(ctx);
#endif
    h8_pressure_owner_collect(owner);
  }
#if !defined(H8_ENABLE_DEBUG_STATS)
  if (!owner) {
    owner = h8_ctx_owner_assume(ctx);
  }
#endif
  H8_DEBUG_INC(local_active_miss);
  span = h8_find_active_span(ctx, owner, class_id);
  if (!span) {
    H8_DEBUG_INC(local_slow_collect);
    h8_pressure_owner_collect(owner);
    span = h8_find_active_span(ctx, owner, class_id);
  }
  if (!span && h8_regular_adoption_enabled() &&
      atomic_load_explicit(&h8g.orphan_span_count, memory_order_acquire) > 0) {
    span = h8_orphan_adopt_span(owner, class_id);
  }
  if (!span) {
    H8_DEBUG_INC(local_span_commit);
    span = h8_span_commit_for_class(owner, class_id);
  }
  if (!span) {
    H8_DEBUG_INC(invalid_count);
    return NULL;
  }
  ctx->active_spans[class_id] = span;
  H8_DEBUG_INC(local_used_count_full_check);
  return h8_small_alloc_from_span(span);
}

static bool h8_local_free(H8ThreadCtx* ctx, H8OwnerRecord* owner, H8Span* span,
                          size_t slot) {
  if (span->owner_slot != owner->slot ||
      span->owner_generation != owner->generation) {
    H8_DEBUG_INC(local_free_reject_owner);
    return false;
  }
  if (h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
    H8_DEBUG_INC(local_free_reject_state);
    return false;
  }
  uint32_t state = h8_slot_state_load_hot(span, slot);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
    H8_DEBUG_INC(local_free_reject_live);
    return false;
  }
  H8_DEBUG_INC(local_pending_check_free);
  if (H8_UNLIKELY(h8_bitmap_test(span->pending_bits, slot))) {
    H8_DEBUG_INC(local_free_pending_nonzero);
    return false;
  }
  H8_DEBUG_INC(local_live_touch_free);
  h8_debug_local_live_word(slot);
#if defined(H8_ENABLE_DEBUG_STATS)
  if (H8_UNLIKELY(!h8_owner_live_clear(span, slot))) {
    H8_DEBUG_INC(local_free_reject_live);
    return false;
  }
#endif
  H8_DEBUG_INC(local_free_head_touch_free);
  uint32_t old_head = atomic_load_explicit(&span->local_hot.local_free_head_word,
                                           memory_order_relaxed);
  h8_slot_state_store_free_hot(span, slot, old_head);
  atomic_store_explicit(&span->local_hot.local_free_head_word, (uint32_t)slot,
                        memory_order_relaxed);
  if (H8_UNLIKELY(!h8_owner_used_sub(span, 1))) {
    abort();
  }
  H8_DEBUG_INC(local_free_count);
  H8_DEBUG_INC(local_free_hit);
  ctx->active_spans[span->class_id] = span;
  return true;
}

void h8_free_inner(void* ptr) {
  if (!ptr) {
    return;
  }
  if (H8_UNLIKELY(!atomic_load_explicit(&h8g.ready, memory_order_acquire))) {
    H8_DEBUG_INC(miss_count);
    h8_sys_free(ptr);
    return;
  }
  if (!h8_arena_contains(ptr)) {
    bool medium_owned = false;
    if (h8_medium_free_inner(ptr, &medium_owned)) {
      return;
    }
    if (medium_owned) {
      h8_fail_invalid_free();
      return;
    }
    H8_DEBUG_INC(miss_count);
    h8_sys_free(ptr);
    return;
  }
  H8Span* span = atomic_load_explicit(
      &h8g.spans[h8_span_index_from_ptr(ptr)], memory_order_acquire);
  if (!span || h8_span_state_load(span) == H8_SPAN_RETIRED) {
    h8_fail_invalid_free();
    return;
  }
  size_t slot = 0;
  if (!h8_slot_index_from_ptr_checked(span, ptr, &slot)) {
    h8_fail_invalid_free();
    return;
  }
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  if (!ctx) {
    h8_fail_invalid_free();
    return;
  }
  H8OwnerRecord* owner = h8_ctx_owner_assume(ctx);
  if (h8_local_free(ctx, owner, span, slot)) {
    return;
  }
  H8PublishResult first = h8_remote_free_publish_known(span, slot);
  if (first == H8_PUBLISH_OK) {
    return;
  }
  if (first != H8_PUBLISH_OWNER_TRANSITION) {
    h8_fail_invalid_free();
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
