#include "h8_internal.h"
#if defined(H8_RECLAIM_ADAPTER_SHADOW_L0)

#include <stdio.h>

typedef struct H8ReclaimShadowState {
  _Atomic uint64_t owner_exit_spans;
  _Atomic uint64_t owner_spans;
  _Atomic uint64_t owner_span_bytes;
  _Atomic uint64_t reclaimable_spans;
  _Atomic uint64_t reclaimable_bytes;
  _Atomic uint64_t blocked_active;
  _Atomic uint64_t blocked_live;
  _Atomic uint64_t blocked_pending;
  _Atomic uint64_t blocked_state;
} H8ReclaimShadowState;

static H8ReclaimShadowState h8_reclaim_shadow;

static void h8_reclaim_shadow_add(_Atomic uint64_t* field, uint64_t value) {
  atomic_fetch_add_explicit(field, value, memory_order_relaxed);
}

void h8_reclaim_shadow_note_owner_exit_span(H8ThreadCtx* ctx, H8Span* span,
                                            uint64_t used) {
  H8Span* active = NULL;
  if (!span || span->class_id >= H8_CLASS_COUNT) return;
  if (ctx && ctx->owner && ctx->active_spans[span->class_id] == span) {
    active = span;
  }
  h8_reclaim_shadow_add(&h8_reclaim_shadow.owner_exit_spans, 1u);
  h8_reclaim_shadow_add(&h8_reclaim_shadow.owner_spans, 1u);
  h8_reclaim_shadow_add(&h8_reclaim_shadow.owner_span_bytes, H8_SPAN_BYTES);
  if (h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
    h8_reclaim_shadow_add(&h8_reclaim_shadow.blocked_state, 1u);
  } else if (used != 0u) {
    h8_reclaim_shadow_add(&h8_reclaim_shadow.blocked_live, 1u);
  } else if (!h8_span_pending_quiescent(span)) {
    h8_reclaim_shadow_add(&h8_reclaim_shadow.blocked_pending, 1u);
  } else if (active) {
    h8_reclaim_shadow_add(&h8_reclaim_shadow.blocked_active, 1u);
  } else {
    h8_reclaim_shadow_add(&h8_reclaim_shadow.reclaimable_spans, 1u);
    h8_reclaim_shadow_add(&h8_reclaim_shadow.reclaimable_bytes, H8_SPAN_BYTES);
  }
}

#define H8_RECLAIM_LOAD(field) atomic_load_explicit(                         \
    &h8_reclaim_shadow.field, memory_order_relaxed)

H8ReclaimShadowSnapshot h8_reclaim_shadow_snapshot(void) {
  H8ReclaimShadowSnapshot out = {0};
  out.owner_exit_spans = H8_RECLAIM_LOAD(owner_exit_spans);
  out.owner_spans = H8_RECLAIM_LOAD(owner_spans);
  out.owner_span_bytes = H8_RECLAIM_LOAD(owner_span_bytes);
  out.reclaimable_spans = H8_RECLAIM_LOAD(reclaimable_spans);
  out.reclaimable_bytes = H8_RECLAIM_LOAD(reclaimable_bytes);
  out.blocked_active = H8_RECLAIM_LOAD(blocked_active);
  out.blocked_live = H8_RECLAIM_LOAD(blocked_live);
  out.blocked_pending = H8_RECLAIM_LOAD(blocked_pending);
  out.blocked_state = H8_RECLAIM_LOAD(blocked_state);
  return out;
}

void h8_reclaim_shadow_dump(void) {
  H8ReclaimShadowSnapshot s = h8_reclaim_shadow_snapshot();
  fprintf(stderr,
          "[h8 reclaim shadow] owner_exit_spans=%llu owner_spans=%llu "
          "owner_bytes=%llu reclaimable_spans=%llu reclaimable_bytes=%llu "
          "blocked_active=%llu blocked_live=%llu blocked_pending=%llu "
          "blocked_state=%llu\n",
          (unsigned long long)s.owner_exit_spans,
          (unsigned long long)s.owner_spans,
          (unsigned long long)s.owner_span_bytes,
          (unsigned long long)s.reclaimable_spans,
          (unsigned long long)s.reclaimable_bytes,
          (unsigned long long)s.blocked_active,
          (unsigned long long)s.blocked_live,
          (unsigned long long)s.blocked_pending,
          (unsigned long long)s.blocked_state);
}

#endif
