#include "h8_internal.h"

#if defined(H8_SMALL_TRANSITION_INVENTORY_L1)

#include <stdio.h>
#include <stdlib.h>

#if defined(H8_SMALL_TRANSITION_INVENTORY_DIAG)
static _Atomic uint64_t g_candidate;
static _Atomic uint64_t g_push;
static _Atomic uint64_t g_duplicate;
static _Atomic uint64_t g_pop_attempt;
static _Atomic uint64_t g_pop_hit;
static _Atomic uint64_t g_pop_reject;
static _Atomic uint64_t g_remote_push;
static _Atomic uint64_t g_direct_activate;
static _Atomic uint64_t g_reset;
static _Atomic uint64_t g_depth_max;

#define H8_TRANSITION_INC(field) \
  atomic_fetch_add_explicit(&(field), 1u, memory_order_relaxed)

static void h8_transition_record_depth(uint32_t depth) {
  uint64_t current = atomic_load_explicit(&g_depth_max, memory_order_relaxed);
  while ((uint64_t)depth > current &&
         !atomic_compare_exchange_weak_explicit(
             &g_depth_max, &current, depth, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}
#else
#define H8_TRANSITION_INC(field) ((void)0)
static inline void h8_transition_record_depth(uint32_t depth) {
  (void)depth;
}
#endif

static bool h8_small_transition_valid(H8Span* span, H8OwnerRecord* owner,
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

static bool h8_small_transition_inventory_note(H8ThreadCtx* ctx,
                                               H8Span* span,
                                               uint32_t old_free_head) {
  if (!ctx || !span || span->class_id >= H8_CLASS_COUNT ||
      ctx->active_spans[span->class_id] == span ||
      old_free_head != H8_SLOT_NONE) {
    return false;
  }
  uint32_t current_head = atomic_load_explicit(
      &span->local_hot.local_free_head_word, memory_order_relaxed);
  if (current_head == H8_SLOT_NONE) return false;
  uint32_t bump = atomic_load_explicit(&span->local_hot.local_bump_index,
                                       memory_order_relaxed);
  if (bump < span->slot_count) return false;

  H8_TRANSITION_INC(g_candidate);
  if (span->transition_indexed) {
    H8_TRANSITION_INC(g_duplicate);
    return false;
  }
  H8Span* active = ctx->active_spans[span->class_id];
  if (active) {
    uint32_t active_head = atomic_load_explicit(
        &active->local_hot.local_free_head_word, memory_order_relaxed);
    uint32_t active_bump = atomic_load_explicit(
        &active->local_hot.local_bump_index, memory_order_relaxed);
    if (active_head == H8_SLOT_NONE && active_bump >= active->slot_count) {
      ctx->active_spans[span->class_id] = span;
      H8_TRANSITION_INC(g_direct_activate);
      return false;
    }
  }
  uint32_t class_id = span->class_id;
  span->next_transition_available = ctx->small_transition_head[class_id];
  span->transition_indexed = true;
  ctx->small_transition_head[class_id] = span;
  uint32_t depth = ++ctx->small_transition_depth[class_id];
  h8_transition_record_depth(depth);
  H8_TRANSITION_INC(g_push);
  return true;
}

void h8_small_transition_inventory_note_local_free(H8ThreadCtx* ctx,
                                                   H8Span* span,
                                                   uint32_t old_free_head) {
  (void)h8_small_transition_inventory_note(ctx, span, old_free_head);
}

void h8_small_transition_inventory_note_remote_collect(H8ThreadCtx* ctx,
                                                       H8Span* span,
                                                       uint32_t old_free_head) {
  if (h8_small_transition_inventory_note(ctx, span, old_free_head)) {
    H8_TRANSITION_INC(g_remote_push);
  }
}

H8Span* h8_small_transition_inventory_pop(H8ThreadCtx* ctx,
                                          H8OwnerRecord* owner,
                                          uint32_t class_id) {
  if (!ctx || class_id >= H8_CLASS_COUNT) return NULL;
  H8_TRANSITION_INC(g_pop_attempt);
  while (ctx->small_transition_head[class_id]) {
    H8Span* span = ctx->small_transition_head[class_id];
    ctx->small_transition_head[class_id] = span->next_transition_available;
    span->next_transition_available = NULL;
    span->transition_indexed = false;
    if (ctx->small_transition_depth[class_id] == 0u) abort();
    --ctx->small_transition_depth[class_id];
    if (h8_small_transition_valid(span, owner, class_id)) {
      H8_TRANSITION_INC(g_pop_hit);
      return span;
    }
    H8_TRANSITION_INC(g_pop_reject);
  }
  return NULL;
}

void h8_small_transition_inventory_reset(H8ThreadCtx* ctx) {
  if (!ctx) return;
  for (uint32_t class_id = 0; class_id < H8_CLASS_COUNT; ++class_id) {
    H8Span* span = ctx->small_transition_head[class_id];
    while (span) {
      H8Span* next = span->next_transition_available;
      span->next_transition_available = NULL;
      span->transition_indexed = false;
      span = next;
      H8_TRANSITION_INC(g_reset);
    }
    ctx->small_transition_head[class_id] = NULL;
    ctx->small_transition_depth[class_id] = 0;
  }
}

H8SmallTransitionInventoryStats h8_small_transition_inventory_stats(void) {
  H8SmallTransitionInventoryStats out = {0};
#if defined(H8_SMALL_TRANSITION_INVENTORY_DIAG)
  out.candidate = atomic_load_explicit(&g_candidate, memory_order_relaxed);
  out.push = atomic_load_explicit(&g_push, memory_order_relaxed);
  out.duplicate = atomic_load_explicit(&g_duplicate, memory_order_relaxed);
  out.pop_attempt = atomic_load_explicit(&g_pop_attempt, memory_order_relaxed);
  out.pop_hit = atomic_load_explicit(&g_pop_hit, memory_order_relaxed);
  out.pop_reject = atomic_load_explicit(&g_pop_reject, memory_order_relaxed);
  out.remote_push = atomic_load_explicit(&g_remote_push, memory_order_relaxed);
  out.direct_activate =
      atomic_load_explicit(&g_direct_activate, memory_order_relaxed);
  out.reset = atomic_load_explicit(&g_reset, memory_order_relaxed);
  out.depth_max = atomic_load_explicit(&g_depth_max, memory_order_relaxed);
#endif
  return out;
}

void h8_small_transition_inventory_dump(void) {
  H8SmallTransitionInventoryStats s = h8_small_transition_inventory_stats();
  fprintf(stderr,
          "[H8_SMALL_TRANSITION] candidate=%llu push=%llu duplicate=%llu "
          "pop_attempt=%llu pop_hit=%llu pop_reject=%llu remote_push=%llu "
          "direct_activate=%llu reset=%llu "
          "depth_max=%llu\n",
          (unsigned long long)s.candidate, (unsigned long long)s.push,
          (unsigned long long)s.duplicate, (unsigned long long)s.pop_attempt,
          (unsigned long long)s.pop_hit, (unsigned long long)s.pop_reject,
          (unsigned long long)s.remote_push,
          (unsigned long long)s.direct_activate, (unsigned long long)s.reset,
          (unsigned long long)s.depth_max);
}

#endif
