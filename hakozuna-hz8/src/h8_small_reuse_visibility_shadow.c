#include "h8_internal.h"

#if defined(H8_SMALL_REUSE_VISIBILITY_SHADOW_L0)

#include <stdio.h>

typedef struct H8SmallReuseVisibilityState {
  _Atomic uint64_t checkpoints;
  _Atomic uint64_t owned_scan_spans;
  _Atomic uint64_t usable_spans;
  _Atomic uint64_t hidden_usable_spans;
  _Atomic uint64_t hidden_usable_bytes;
  _Atomic uint64_t max_hidden_usable_spans;
  _Atomic uint64_t max_hidden_usable_bytes;
  _Atomic uint64_t full_spans;
  _Atomic uint64_t blocked_state;
} H8SmallReuseVisibilityState;

static H8SmallReuseVisibilityState h8_small_reuse_visibility;

static void h8_visibility_add(_Atomic uint64_t* field, uint64_t value) {
  atomic_fetch_add_explicit(field, value, memory_order_relaxed);
}

static void h8_visibility_max(_Atomic uint64_t* field, uint64_t value) {
  uint64_t current = atomic_load_explicit(field, memory_order_relaxed);
  while (value > current &&
         !atomic_compare_exchange_weak_explicit(
             field, &current, value, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}

static bool h8_visibility_in_magazine(H8ThreadCtx* ctx, uint32_t class_id,
                                      H8Span* span) {
#if H8_REUSABLE_SPAN_MAGAZINE_L1
  size_t count = ctx->reusable_span_count[class_id];
  for (size_t i = 0; i < count; ++i) {
    if (ctx->reusable_span_mag[class_id][i] == span) return true;
  }
#else
  (void)ctx;
  (void)class_id;
  (void)span;
#endif
  return false;
}

void h8_small_reuse_visibility_checkpoint(H8ThreadCtx* ctx,
                                          H8OwnerRecord* owner,
                                          uint32_t class_id) {
  if (!ctx || !owner || class_id >= H8_CLASS_COUNT) return;
  uint64_t hidden = 0;
  h8_visibility_add(&h8_small_reuse_visibility.checkpoints, 1u);
  h8_platform_mutex_lock(&owner->owned_lock);
  for (H8Span* span = owner->owned_by_class[class_id]; span;
       span = span->next_owned_class) {
    h8_visibility_add(&h8_small_reuse_visibility.owned_scan_spans, 1u);
    if (h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
      h8_visibility_add(&h8_small_reuse_visibility.blocked_state, 1u);
      continue;
    }
    uint32_t head = atomic_load_explicit(
        &span->local_hot.local_free_head_word, memory_order_relaxed);
    uint32_t bump = atomic_load_explicit(&span->local_hot.local_bump_index,
                                         memory_order_relaxed);
    if (head == H8_SLOT_NONE && bump >= span->slot_count) {
      h8_visibility_add(&h8_small_reuse_visibility.full_spans, 1u);
      continue;
    }
    h8_visibility_add(&h8_small_reuse_visibility.usable_spans, 1u);
    if (span != ctx->active_spans[class_id] &&
        !h8_visibility_in_magazine(ctx, class_id, span)) {
      ++hidden;
    }
  }
  h8_platform_mutex_unlock(&owner->owned_lock);
  h8_visibility_add(&h8_small_reuse_visibility.hidden_usable_spans, hidden);
  h8_visibility_add(&h8_small_reuse_visibility.hidden_usable_bytes,
                    hidden * H8_SPAN_BYTES);
  h8_visibility_max(&h8_small_reuse_visibility.max_hidden_usable_spans,
                    hidden);
  h8_visibility_max(&h8_small_reuse_visibility.max_hidden_usable_bytes,
                    hidden * H8_SPAN_BYTES);
}

#define H8_VIS_LOAD(field) atomic_load_explicit(                            \
    &h8_small_reuse_visibility.field, memory_order_relaxed)

H8SmallReuseVisibilitySnapshot h8_small_reuse_visibility_snapshot(void) {
  H8SmallReuseVisibilitySnapshot out = {0};
  out.checkpoints = H8_VIS_LOAD(checkpoints);
  out.owned_scan_spans = H8_VIS_LOAD(owned_scan_spans);
  out.usable_spans = H8_VIS_LOAD(usable_spans);
  out.hidden_usable_spans = H8_VIS_LOAD(hidden_usable_spans);
  out.hidden_usable_bytes = H8_VIS_LOAD(hidden_usable_bytes);
  out.max_hidden_usable_spans = H8_VIS_LOAD(max_hidden_usable_spans);
  out.max_hidden_usable_bytes = H8_VIS_LOAD(max_hidden_usable_bytes);
  out.full_spans = H8_VIS_LOAD(full_spans);
  out.blocked_state = H8_VIS_LOAD(blocked_state);
  return out;
}

void h8_small_reuse_visibility_dump(void) {
  H8SmallReuseVisibilitySnapshot s = h8_small_reuse_visibility_snapshot();
  fprintf(stderr,
          "[H8_SMALL_REUSE_VIS] checkpoints=%llu scanned=%llu usable=%llu "
          "hidden=%llu hidden_bytes=%llu max_hidden=%llu max_hidden_bytes=%llu "
          "full=%llu blocked_state=%llu\n",
          (unsigned long long)s.checkpoints,
          (unsigned long long)s.owned_scan_spans,
          (unsigned long long)s.usable_spans,
          (unsigned long long)s.hidden_usable_spans,
          (unsigned long long)s.hidden_usable_bytes,
          (unsigned long long)s.max_hidden_usable_spans,
          (unsigned long long)s.max_hidden_usable_bytes,
          (unsigned long long)s.full_spans,
          (unsigned long long)s.blocked_state);
}

#endif
