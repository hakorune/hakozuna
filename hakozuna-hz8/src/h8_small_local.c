#include "h8_internal.h"
#include "h8_medium_domain_shadow.h"
#include "h8_medium_page_backend.h"
#include "h8_used_count.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

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

#if defined(H8_ENABLE_DEBUG_STATS)
static void h8_debug_record_active_full_pending(size_t pending) {
  if (pending <= 1) {
    H8_DEBUG_INC(small_active_full_pending_bucket_1);
  } else if (pending <= 4) {
    H8_DEBUG_INC(small_active_full_pending_bucket_2_4);
  } else if (pending <= 8) {
    H8_DEBUG_INC(small_active_full_pending_bucket_5_8);
  } else if (pending <= 32) {
    H8_DEBUG_INC(small_active_full_pending_bucket_9_32);
  } else {
    H8_DEBUG_INC(small_active_full_pending_bucket_33p);
  }
}
#endif

#if defined(H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1) && \
    !defined(H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT)
#define H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT 8u
#endif

#if H8_REUSABLE_SPAN_MAGAZINE_L1
#if defined(H8_ENABLE_DEBUG_STATS)
static void h8_reusable_span_mag_record_pop_depth(uint32_t class_id,
                                                  size_t depth) {
  if (depth == 0u) {
    atomic_fetch_add_explicit(&h8g.reusable_mag_empty_pop_by_class[class_id], 1,
                              memory_order_relaxed);
  }
  atomic_size_t* low = &h8g.reusable_mag_depth_low_water_by_class[class_id];
  size_t current = atomic_load_explicit(low, memory_order_relaxed);
  while (depth < current &&
         !atomic_compare_exchange_weak_explicit(
             low, &current, depth, memory_order_relaxed, memory_order_relaxed)) {
  }
}
#else
static inline void h8_reusable_span_mag_record_pop_depth(uint32_t class_id,
                                                         size_t depth) {
  (void)class_id;
  (void)depth;
}
#endif

static H8Span* h8_reusable_span_mag_pop(H8ThreadCtx* ctx,
                                       H8OwnerRecord* owner,
                                       uint32_t class_id) {
  H8_DEBUG_INC(reusable_mag_pop_attempt);
  uint8_t* count = &ctx->reusable_span_count[class_id];
  h8_reusable_span_mag_record_pop_depth(class_id, *count);
  while (*count != 0u) {
    H8Span* span = ctx->reusable_span_mag[class_id][--(*count)];
    if (h8_active_hint_matches(span, owner, class_id) &&
        !h8_span_local_exhausted(span)) {
      H8_DEBUG_INC(reusable_mag_pop_hit);
      return span;
    }
    H8_DEBUG_INC(reusable_mag_pop_reject);
  }
  return NULL;
}

static void h8_reusable_span_mag_note_local_free(H8ThreadCtx* ctx,
                                                H8Span* span) {
  const uint32_t class_id = span->class_id;
  H8Span* old = ctx->active_spans[class_id];
  uint8_t* count = &ctx->reusable_span_count[class_id];
  if (old && old != span) {
    if (*count == H8_REUSABLE_SPAN_MAG_CAP) {
      H8_DEBUG_INC(reusable_mag_full_preserve);
      h8_small_available_index_push(ctx, span);
      return;
    }
    ctx->reusable_span_mag[class_id][(*count)++] = old;
    H8_DEBUG_INC(reusable_mag_push);
  }
  ctx->active_spans[class_id] = span;
}
#endif

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
  h8_platform_mutex_lock(&owner->owned_lock);
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
    h8_platform_mutex_unlock(&owner->owned_lock);
    return span;
  }
  h8_platform_mutex_unlock(&owner->owned_lock);
  return NULL;
}

void* h8_malloc_inner(size_t size) {
  if (size == 0) {
    size = 1;
  }
  if (size > H8_MAX_SMALL_SIZE) {
#if defined(H8_MEDIUM_PAGE8K_HZ10_SHADOW_L1) || \
    defined(H8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1)
#if defined(H8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1)
    if (size == 8192u) {
#else
    if (h8_medium_page_backend_accepts_size(size)) {
#endif
      if (!h8_thread_ctx_fast()) {
        return NULL;
      }
      void* page_ptr = h8_medium_page_backend_malloc(size);
      if (page_ptr) {
        return page_ptr;
      }
    }
#endif
    if (size <= H8_MEDIUM_MAX_SIZE) {
      uint32_t medium_class_id = h8_medium_class_for_size_fast(size);
      return h8_medium_malloc_class_inner(medium_class_id);
    }
    if (h8_direct_large_size_supported(size)) {
      void* ptr = h8_direct_large_malloc(size);
      if (ptr) {
        return ptr;
      }
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
  bool remote_pressure_collect_triggered = false;
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
#if H8_REUSABLE_SPAN_MAGAZINE_L1
    span = h8_reusable_span_mag_pop(ctx, owner, class_id);
    if (span) {
      ctx->active_spans[class_id] = span;
      ptr = h8_small_alloc_from_span(span);
      if (ptr) return ptr;
    }
#endif
    span = h8_small_available_index_pop(ctx, owner, class_id);
    if (span) {
      ctx->active_spans[class_id] = span;
      ptr = h8_small_alloc_from_span(span);
      if (ptr) return ptr;
    }
    size_t pending_before =
        atomic_load_explicit(&owner->pending_span_count, memory_order_acquire);
    if (pending_before > 0) {
      H8_DEBUG_INC(small_active_full_pending_nonzero_count);
#if defined(H8_ENABLE_DEBUG_STATS)
      h8_debug_record_active_full_pending(pending_before);
#endif
#if defined(H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1)
      if (pending_before <= H8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT) {
        H8_DEBUG_INC(small_active_full_defer_count);
      } else
#endif
      {
        remote_pressure_collect_triggered = true;
        h8_pressure_owner_collect_remote_pressure(
            owner, H8_REMOTE_PRESSURE_COLLECT_SOURCE_ACTIVE_HIT_FULL);
      }
    } else {
      h8_pressure_owner_collect(owner);
    }
  }
#if !defined(H8_ENABLE_DEBUG_STATS)
  if (!owner) {
    owner = h8_ctx_owner_assume(ctx);
  }
#endif
  H8_DEBUG_INC(local_active_miss);
  span = h8_find_active_span(ctx, owner, class_id);
  if (remote_pressure_collect_triggered && span) {
    H8_DEBUG_INC(small_active_full_collect_helped_count);
    remote_pressure_collect_triggered = false;
  }
  if (!span) {
    H8_DEBUG_INC(local_slow_collect);
    size_t pending_before =
        atomic_load_explicit(&owner->pending_span_count, memory_order_acquire);
    if (pending_before > 0) {
      H8_DEBUG_INC(small_active_full_pending_nonzero_count);
      remote_pressure_collect_triggered = true;
      h8_pressure_owner_collect_remote_pressure(
          owner, H8_REMOTE_PRESSURE_COLLECT_SOURCE_ACTIVE_MISS);
    } else {
      h8_pressure_owner_collect(owner);
    }
    span = h8_find_active_span(ctx, owner, class_id);
    if (remote_pressure_collect_triggered) {
      if (span) {
        H8_DEBUG_INC(small_active_full_collect_helped_count);
      } else {
        H8_DEBUG_INC(small_active_full_collect_no_help_count);
      }
      remote_pressure_collect_triggered = false;
    }
  }
  if (!span && h8_regular_adoption_enabled() &&
      atomic_load_explicit(&h8g.orphan_span_count, memory_order_acquire) > 0) {
    span = h8_orphan_adopt_span(owner, class_id);
  }
  if (!span) {
    H8_DEBUG_INC(local_span_commit);
    h8_small_reuse_visibility_checkpoint(ctx, owner, class_id);
    h8_magazine_tail_shadow_checkpoint(ctx);
    h8_adaptive_shadow_note_small_refill(
        class_id,
        atomic_load_explicit(&owner->pending_span_count, memory_order_relaxed));
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
#if H8_REUSABLE_SPAN_MAGAZINE_L1
  h8_reusable_span_mag_note_local_free(ctx, span);
#else
  ctx->active_spans[span->class_id] = span;
#endif
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
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0) || \
    defined(H8_UNIFIED_MEDIUM_DOMAIN_KIND_L1)
    H8MediumDomainProbe medium_domain_probe =
        h8_medium_domain_shadow_lookup(ptr);
#endif
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_KIND_L1)
    bool page_backend_checked = false;
    bool medium_backend_checked = false;
    if (medium_domain_probe.kind == H8_MEDIUM_DOMAIN_PAGE8K) {
      bool page_backend_owned = false;
      page_backend_checked = true;
      if (h8_medium_page_backend_free(ptr, &page_backend_owned)) {
        return;
      }
      if (page_backend_owned) {
        h8_fail_invalid_free();
        return;
      }
    } else if (medium_domain_probe.kind == H8_MEDIUM_DOMAIN_RUN) {
      bool medium_owned = false;
      medium_backend_checked = true;
      if (h8_medium_free_inner(ptr, &medium_owned)) {
        return;
      }
      if (medium_owned) {
        h8_fail_invalid_free();
        return;
      }
      bool page_backend_owned = false;
      page_backend_checked = true;
      if (h8_medium_page_backend_free(ptr, &page_backend_owned)) {
        return;
      }
      if (page_backend_owned) {
        h8_fail_invalid_free();
        return;
      }
    }
#endif
#if defined(H8_MEDIUM_PAGE8K_HZ10_SHADOW_L1) || \
    defined(H8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1)
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_KIND_L1)
    if (!page_backend_checked) {
#endif
    bool page_backend_owned = false;
    if (h8_medium_page_backend_free(ptr, &page_backend_owned)) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
      h8_medium_domain_shadow_compare(medium_domain_probe,
                                      H8_MEDIUM_DOMAIN_PAGE8K);
#endif
      return;
    }
    if (page_backend_owned) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
      h8_medium_domain_shadow_compare(medium_domain_probe,
                                      H8_MEDIUM_DOMAIN_PAGE8K);
#endif
      h8_fail_invalid_free();
      return;
    }
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_KIND_L1)
    }
#endif
#endif
#if defined(H8_LARGE_DIRECT_OWNED_L1)
    bool direct_maybe = h8_direct_large_maybe_contains_hot(ptr);
    bool direct_owned = false;
    if (direct_maybe && h8_direct_large_free_exact_inner(ptr, &direct_owned)) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
      h8_medium_domain_shadow_compare(medium_domain_probe,
                                      H8_MEDIUM_DOMAIN_NONE);
#endif
      return;
    }
#endif
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_KIND_L1)
    if (!medium_backend_checked) {
#endif
      bool medium_owned = false;
      if (h8_medium_free_inner(ptr, &medium_owned)) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
        h8_medium_domain_shadow_compare(medium_domain_probe,
                                        H8_MEDIUM_DOMAIN_RUN);
#endif
        return;
      }
      if (medium_owned) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
        h8_medium_domain_shadow_compare(medium_domain_probe,
                                        H8_MEDIUM_DOMAIN_RUN);
#endif
        h8_fail_invalid_free();
        return;
      }
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_KIND_L1)
    }
#endif
#if defined(H8_LARGE_DIRECT_OWNED_L1)
    if (direct_maybe && h8_direct_large_free_inner(ptr, &direct_owned)) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
      h8_medium_domain_shadow_compare(medium_domain_probe,
                                      H8_MEDIUM_DOMAIN_NONE);
#endif
      return;
    }
    if (direct_owned) {
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
      h8_medium_domain_shadow_compare(medium_domain_probe,
                                      H8_MEDIUM_DOMAIN_NONE);
#endif
      h8_fail_invalid_free();
      return;
    }
#endif
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_SHADOW_L0)
    h8_medium_domain_shadow_compare(medium_domain_probe,
                                    H8_MEDIUM_DOMAIN_NONE);
#endif
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
#if defined(H8_REMOTE_TRANSITION_BACKOFF_L1)
  size_t transition_retries = 0;
#endif
  for (;;) {
    H8PublishResult res = h8_remote_free_publish(ptr);
    if (res == H8_PUBLISH_OK) {
      return;
    }
    if (res != H8_PUBLISH_OWNER_TRANSITION) {
      break;
    }
#if defined(H8_REMOTE_TRANSITION_BACKOFF_L1)
    transition_retries++;
    if ((transition_retries & 63u) == 0) {
      h8_platform_sleep_ns(1000000ull);
      continue;
    }
#endif
    h8_platform_yield();
  }
  h8_fail_invalid_free();
}

static bool h8_small_usable_size(void* ptr, size_t* usable_out) {
  if (!h8_arena_contains(ptr)) {
    return false;
  }
  H8Span* span = atomic_load_explicit(
      &h8g.spans[h8_span_index_from_ptr(ptr)], memory_order_acquire);
  if (!span || h8_span_state_load(span) == H8_SPAN_RETIRED) {
    return false;
  }
  size_t slot = 0;
  if (!h8_slot_index_from_ptr_checked(span, ptr, &slot)) {
    return false;
  }
  uint32_t state = h8_slot_state_load_hot(span, slot);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT) ||
      h8_bitmap_test(span->pending_bits, slot)) {
    return false;
  }
  *usable_out = h8_class_size(span->class_id);
  return true;
}

bool h8_usable_size_inner(void* ptr, size_t* usable_out, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  if (usable_out) {
    *usable_out = 0;
  }
  if (!ptr) {
    return false;
  }
  h8_init();
  if (h8_arena_contains(ptr)) {
    if (owned_out) {
      *owned_out = true;
    }
    return h8_small_usable_size(ptr, usable_out);
  }
#if defined(H8_LARGE_DIRECT_OWNED_L1)
  bool direct_maybe = h8_direct_large_maybe_contains_hot(ptr);
  bool direct_owned = false;
  if (direct_maybe &&
      h8_direct_large_usable_size_exact_inner(ptr, usable_out,
                                              &direct_owned)) {
    if (owned_out) {
      *owned_out = true;
    }
    return true;
  }
  if (direct_owned) {
    if (owned_out) {
      *owned_out = true;
    }
    return false;
  }
#endif
#if defined(H8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1)
  bool page_owned = false;
  if (h8_medium_page_backend_usable_size(ptr, usable_out, &page_owned)) {
    if (owned_out) *owned_out = true;
    return true;
  }
  if (page_owned) {
    if (owned_out) *owned_out = true;
    return false;
  }
#endif
  bool medium_owned = false;
  if (h8_medium_usable_size_inner(ptr, usable_out, &medium_owned)) {
    if (owned_out) {
      *owned_out = true;
    }
    return true;
  }
  if (medium_owned) {
    if (owned_out) {
      *owned_out = true;
    }
    return false;
  }
#if defined(H8_LARGE_DIRECT_OWNED_L1)
  if (direct_maybe &&
      h8_direct_large_usable_size_inner(ptr, usable_out, &direct_owned)) {
    if (owned_out) {
      *owned_out = true;
    }
    return true;
  }
  if (direct_owned && owned_out) {
    *owned_out = true;
  }
#endif
  return false;
}

void* h8_realloc_inner(void* ptr, size_t size) {
  if (!ptr) {
    return h8_malloc_inner(size);
  }
  if (size == 0) {
    h8_free_inner(ptr);
    return NULL;
  }
  if (H8_UNLIKELY(!atomic_load_explicit(&h8g.ready, memory_order_acquire))) {
    return h8_sys_realloc(ptr, size);
  }

  size_t old_size = 0;
  bool owned = false;
  if (h8_arena_contains(ptr)) {
    owned = true;
    if (!h8_small_usable_size(ptr, &old_size)) {
      errno = EINVAL;
      return NULL;
    }
  } else {
#if defined(H8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1)
    bool page_owned = false;
    bool page_valid = h8_medium_page_backend_usable_size(
        ptr, &old_size, &page_owned);
    if (page_valid) {
      owned = true;
    } else if (page_owned) {
      errno = EINVAL;
      return NULL;
    }
#endif
#if defined(H8_LARGE_DIRECT_OWNED_L1)
    bool direct_maybe = h8_direct_large_maybe_contains_hot(ptr);
    bool direct_owned = false;
    if (direct_maybe &&
        h8_direct_large_usable_size_exact_inner(ptr, &old_size,
                                                &direct_owned)) {
      owned = true;
    } else if (direct_owned) {
      errno = EINVAL;
      return NULL;
    }
#endif
    bool medium_owned = false;
    if (!owned && h8_medium_usable_size_inner(ptr, &old_size, &medium_owned)) {
      owned = true;
    } else if (medium_owned) {
      errno = EINVAL;
      return NULL;
    } else if (!owned) {
#if defined(H8_LARGE_DIRECT_OWNED_L1)
      if (direct_maybe &&
          h8_direct_large_usable_size_inner(ptr, &old_size, &direct_owned)) {
        owned = true;
      } else if (direct_owned) {
        errno = EINVAL;
        return NULL;
      }
#endif
    }
  }

  if (!owned) {
    return h8_sys_realloc(ptr, size);
  }

#if defined(H8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1)
  if (size == old_size && h8_medium_page_backend_route(ptr) == H8_ROUTE_VALID) {
    return ptr;
  }
#endif

  void* next = h8_malloc_inner(size);
  if (!next) {
    return NULL;
  }
  size_t copy_size = old_size < size ? old_size : size;
  memcpy(next, ptr, copy_size);
  h8_free_inner(ptr);
  return next;
}
