#include "h8_internal.h"

#if defined(H8_SMALL_PARTIAL_TRANSITION_DEPOT_L1)

#if defined(H8_SMALL_PARTIAL_TRANSITION_DEPOT_DIAG)
#include <stdio.h>

typedef struct H8SmallPartialDepotDiag {
  _Atomic uint64_t transition[H8_CLASS_COUNT];
  _Atomic uint64_t push[H8_CLASS_COUNT];
  _Atomic uint64_t duplicate_push[H8_CLASS_COUNT];
  _Atomic uint64_t pop_attempt[H8_CLASS_COUNT];
  _Atomic uint64_t pop_hit[H8_CLASS_COUNT];
  _Atomic uint64_t pop_reject[H8_CLASS_COUNT];
  _Atomic uint64_t index_mismatch[H8_CLASS_COUNT];
  _Atomic uint64_t depth_current[H8_CLASS_COUNT];
  _Atomic uint64_t depth_max[H8_CLASS_COUNT];
  _Atomic uint64_t reset_item[H8_CLASS_COUNT];
  _Atomic uint64_t commit_nonempty[H8_CLASS_COUNT];
} H8SmallPartialDepotDiag;

static H8SmallPartialDepotDiag g_partial_diag;

#define H8_PARTIAL_INC(field, class_id)                                  \
  atomic_fetch_add_explicit(&g_partial_diag.field[(class_id)], 1u,       \
                            memory_order_relaxed)

static void h8_small_partial_depth_add(uint32_t class_id) {
  uint64_t depth = atomic_fetch_add_explicit(
                       &g_partial_diag.depth_current[class_id], 1u,
                       memory_order_relaxed) +
                   1u;
  uint64_t current = atomic_load_explicit(
      &g_partial_diag.depth_max[class_id], memory_order_relaxed);
  while (depth > current &&
         !atomic_compare_exchange_weak_explicit(
             &g_partial_diag.depth_max[class_id], &current, depth,
             memory_order_relaxed, memory_order_relaxed)) {
  }
}

static void h8_small_partial_depth_sub(uint32_t class_id) {
  atomic_fetch_sub_explicit(&g_partial_diag.depth_current[class_id], 1u,
                            memory_order_relaxed);
}
#else
#define H8_PARTIAL_INC(field, class_id) ((void)0)
static void h8_small_partial_depth_add(uint32_t class_id) {
  (void)class_id;
}
static void h8_small_partial_depth_sub(uint32_t class_id) {
  (void)class_id;
}
#endif

static bool h8_small_partial_depot_valid(H8Span* span,
                                         H8OwnerRecord* owner,
                                         uint32_t class_id) {
  if (!span || !owner || span->class_id != class_id ||
      span->owner_slot != owner->slot ||
      span->owner_generation != owner->generation ||
      h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
    return false;
  }
  uint32_t head = atomic_load_explicit(
      &span->local_hot.local_free_head_word, memory_order_relaxed);
  uint32_t bump = atomic_load_explicit(&span->local_hot.local_bump_index,
                                       memory_order_relaxed);
  return head != H8_SLOT_NONE || bump < span->slot_count;
}

#if defined(H8_SMALL_PARTIAL_TRANSITION_HINT1_L1C)
static bool h8_small_partial_hint_cross_tier_valid(H8ThreadCtx* ctx,
                                                   H8Span* span,
                                                   uint32_t class_id) {
  if (ctx->active_spans[class_id] == span) return false;
  const uint8_t count = ctx->reusable_span_count[class_id];
  for (uint8_t i = 0; i < count; ++i) {
    if (ctx->reusable_span_mag[class_id][i] == span) return false;
  }
  return true;
}
#endif

void h8_small_partial_depot_push(H8ThreadCtx* ctx, H8Span* span) {
  if (!ctx || !span || span->class_id >= H8_CLASS_COUNT) return;
  const uint32_t class_id = span->class_id;
#if defined(H8_SMALL_PARTIAL_TRANSITION_HINT1_L1C)
  H8Span* old = ctx->small_partial_head[class_id];
  if (old == span) {
    H8_PARTIAL_INC(duplicate_push, class_id);
    return;
  }
  ctx->small_partial_head[class_id] = span;
  if (!old) h8_small_partial_depth_add(class_id);
  H8_PARTIAL_INC(push, class_id);
#else
  if (span->small_partial_indexed) {
    H8_PARTIAL_INC(duplicate_push, class_id);
    return;
  }
  span->next_small_partial = ctx->small_partial_head[class_id];
  span->small_partial_indexed = true;
  ctx->small_partial_head[class_id] = span;
  H8_PARTIAL_INC(push, class_id);
  h8_small_partial_depth_add(class_id);
#endif
}

H8Span* h8_small_partial_depot_pop(H8ThreadCtx* ctx,
                                   H8OwnerRecord* owner,
                                   uint32_t class_id) {
  if (!ctx || class_id >= H8_CLASS_COUNT) return NULL;
  H8_PARTIAL_INC(pop_attempt, class_id);
#if defined(H8_SMALL_PARTIAL_TRANSITION_HINT1_L1C)
  H8Span* span = ctx->small_partial_head[class_id];
  if (!span) return NULL;
  ctx->small_partial_head[class_id] = NULL;
  h8_small_partial_depth_sub(class_id);
  if (h8_small_partial_depot_valid(span, owner, class_id) &&
      h8_small_partial_hint_cross_tier_valid(ctx, span, class_id)) {
    H8_PARTIAL_INC(pop_hit, class_id);
    return span;
  }
  H8_PARTIAL_INC(pop_reject, class_id);
  return NULL;
#else
  while (ctx->small_partial_head[class_id]) {
    H8Span* span = ctx->small_partial_head[class_id];
    ctx->small_partial_head[class_id] = span->next_small_partial;
    span->next_small_partial = NULL;
    if (!span->small_partial_indexed) {
      H8_PARTIAL_INC(index_mismatch, class_id);
    }
    span->small_partial_indexed = false;
    h8_small_partial_depth_sub(class_id);
    if (h8_small_partial_depot_valid(span, owner, class_id)) {
      H8_PARTIAL_INC(pop_hit, class_id);
      return span;
    }
    H8_PARTIAL_INC(pop_reject, class_id);
  }
  return NULL;
#endif
}

void h8_small_partial_depot_reset(H8ThreadCtx* ctx) {
  if (!ctx) return;
  for (uint32_t class_id = 0; class_id < H8_CLASS_COUNT; ++class_id) {
#if defined(H8_SMALL_PARTIAL_TRANSITION_HINT1_L1C)
    if (ctx->small_partial_head[class_id]) {
      ctx->small_partial_head[class_id] = NULL;
      h8_small_partial_depth_sub(class_id);
      H8_PARTIAL_INC(reset_item, class_id);
    }
#else
    H8Span* span = ctx->small_partial_head[class_id];
    while (span) {
      H8Span* next = span->next_small_partial;
      span->next_small_partial = NULL;
      if (!span->small_partial_indexed) {
        H8_PARTIAL_INC(index_mismatch, class_id);
      }
      span->small_partial_indexed = false;
      h8_small_partial_depth_sub(class_id);
      H8_PARTIAL_INC(reset_item, class_id);
      span = next;
    }
    ctx->small_partial_head[class_id] = NULL;
#endif
  }
}

void h8_small_partial_depot_discard(H8ThreadCtx* ctx,
                                    H8Span* span,
                                    uint32_t class_id) {
#if defined(H8_SMALL_PARTIAL_TRANSITION_HINT1_L1C)
  if (ctx && class_id < H8_CLASS_COUNT &&
      ctx->small_partial_head[class_id] == span) {
    ctx->small_partial_head[class_id] = NULL;
    h8_small_partial_depth_sub(class_id);
  }
#else
  (void)ctx;
  (void)span;
  (void)class_id;
#endif
}

#if defined(H8_SMALL_PARTIAL_TRANSITION_DEPOT_DIAG)
void h8_small_partial_depot_note_transition(uint32_t class_id) {
  if (class_id < H8_CLASS_COUNT) H8_PARTIAL_INC(transition, class_id);
}

void h8_small_partial_depot_commit_checkpoint(H8ThreadCtx* ctx,
                                               uint32_t class_id) {
  if (ctx && class_id < H8_CLASS_COUNT &&
      ctx->small_partial_head[class_id] != NULL) {
    H8_PARTIAL_INC(commit_nonempty, class_id);
  }
}

void h8_small_partial_depot_dump(void) {
  for (uint32_t class_id = 0; class_id < H8_CLASS_COUNT; ++class_id) {
    uint64_t transition = atomic_load_explicit(
        &g_partial_diag.transition[class_id], memory_order_relaxed);
    uint64_t push = atomic_load_explicit(&g_partial_diag.push[class_id],
                                         memory_order_relaxed);
    uint64_t pop_attempt = atomic_load_explicit(
        &g_partial_diag.pop_attempt[class_id], memory_order_relaxed);
    if (transition == 0 && push == 0 && pop_attempt == 0) continue;
    fprintf(stderr,
            "[H8_SMALL_PARTIAL] class=%u transition=%llu push=%llu "
            "duplicate=%llu pop_attempt=%llu pop_hit=%llu pop_reject=%llu "
            "index_mismatch=%llu depth_current=%llu depth_max=%llu "
            "reset_item=%llu commit_nonempty=%llu\n",
            class_id, (unsigned long long)transition,
            (unsigned long long)push,
            (unsigned long long)atomic_load_explicit(
                &g_partial_diag.duplicate_push[class_id],
                memory_order_relaxed),
            (unsigned long long)pop_attempt,
            (unsigned long long)atomic_load_explicit(
                &g_partial_diag.pop_hit[class_id], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_partial_diag.pop_reject[class_id], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_partial_diag.index_mismatch[class_id],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_partial_diag.depth_current[class_id],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_partial_diag.depth_max[class_id], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_partial_diag.reset_item[class_id], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_partial_diag.commit_nonempty[class_id],
                memory_order_relaxed));
  }
}
#endif

#endif
