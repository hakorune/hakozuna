#include "h8_internal.h"

#if defined(H8_SMALL_AVAILABLE_INDEX_L1)

#include <stdio.h>

#define H8_SMALL_AVAILABLE_CLASS 8u

#if defined(H8_SMALL_AVAILABLE_INDEX_DIAG)
static _Atomic uint64_t g_push;
static _Atomic uint64_t g_duplicate;
static _Atomic uint64_t g_pop_attempt;
static _Atomic uint64_t g_pop_hit;
static _Atomic uint64_t g_pop_reject;
static _Atomic uint64_t g_reset;
#define H8_AVAILABLE_INC(field) \
  atomic_fetch_add_explicit(&(field), 1u, memory_order_relaxed)
#else
#define H8_AVAILABLE_INC(field) ((void)0)
#endif

static bool h8_small_available_valid(H8Span* span, H8OwnerRecord* owner,
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

void h8_small_available_index_push(H8ThreadCtx* ctx, H8Span* span) {
  if (!ctx || !span || span->class_id != H8_SMALL_AVAILABLE_CLASS) return;
  if (span->small_available_indexed) {
    H8_AVAILABLE_INC(g_duplicate);
    return;
  }
  span->next_orphan_class = ctx->small_available_head[span->class_id];
  span->small_available_indexed = true;
  ctx->small_available_head[span->class_id] = span;
  H8_AVAILABLE_INC(g_push);
}

H8Span* h8_small_available_index_pop(H8ThreadCtx* ctx,
                                     H8OwnerRecord* owner,
                                     uint32_t class_id) {
  if (!ctx || class_id != H8_SMALL_AVAILABLE_CLASS) return NULL;
  H8_AVAILABLE_INC(g_pop_attempt);
  while (ctx->small_available_head[class_id]) {
    H8Span* span = ctx->small_available_head[class_id];
    ctx->small_available_head[class_id] = span->next_orphan_class;
    span->next_orphan_class = NULL;
    span->small_available_indexed = false;
    if (h8_small_available_valid(span, owner, class_id)) {
      H8_AVAILABLE_INC(g_pop_hit);
      return span;
    }
    H8_AVAILABLE_INC(g_pop_reject);
  }
  return NULL;
}

void h8_small_available_index_reset(H8ThreadCtx* ctx) {
  if (!ctx) return;
  for (uint32_t class_id = 0; class_id < H8_CLASS_COUNT; ++class_id) {
    H8Span* span = ctx->small_available_head[class_id];
    while (span) {
      H8Span* next = span->next_orphan_class;
      span->next_orphan_class = NULL;
      span->small_available_indexed = false;
      span = next;
      H8_AVAILABLE_INC(g_reset);
    }
    ctx->small_available_head[class_id] = NULL;
  }
}

H8SmallAvailableIndexStats h8_small_available_index_stats(void) {
  H8SmallAvailableIndexStats out = {0};
#if defined(H8_SMALL_AVAILABLE_INDEX_DIAG)
  out.push = atomic_load_explicit(&g_push, memory_order_relaxed);
  out.duplicate = atomic_load_explicit(&g_duplicate, memory_order_relaxed);
  out.pop_attempt = atomic_load_explicit(&g_pop_attempt, memory_order_relaxed);
  out.pop_hit = atomic_load_explicit(&g_pop_hit, memory_order_relaxed);
  out.pop_reject = atomic_load_explicit(&g_pop_reject, memory_order_relaxed);
  out.reset = atomic_load_explicit(&g_reset, memory_order_relaxed);
#endif
  return out;
}

void h8_small_available_index_dump(void) {
  H8SmallAvailableIndexStats s = h8_small_available_index_stats();
  fprintf(stderr,
          "[H8_SMALL_AVAILABLE] push=%llu duplicate=%llu pop_attempt=%llu "
          "pop_hit=%llu pop_reject=%llu reset=%llu\n",
          (unsigned long long)s.push, (unsigned long long)s.duplicate,
          (unsigned long long)s.pop_attempt, (unsigned long long)s.pop_hit,
          (unsigned long long)s.pop_reject, (unsigned long long)s.reset);
}

#endif
