#include "h8_internal.h"

#if defined(H8_MAGAZINE_TAIL_RECLAIM_SHADOW_L0)

#include <stdio.h>

typedef struct H8MagazineTailShadowState {
  _Atomic uint64_t checkpoints;
  _Atomic uint64_t inventory_spans;
  _Atomic uint64_t inventory_bytes;
  _Atomic uint64_t empty_spans;
  _Atomic uint64_t empty_bytes;
  _Atomic uint64_t tail_spans;
  _Atomic uint64_t tail_empty_spans;
  _Atomic uint64_t hypothetical_decommit_bytes;
  _Atomic uint64_t max_inventory_bytes;
  _Atomic uint64_t max_empty_bytes;
  _Atomic uint64_t max_hypothetical_decommit_bytes;
  _Atomic uint64_t blocked_live;
  _Atomic uint64_t blocked_pending;
  _Atomic uint64_t blocked_state;
  _Atomic uint64_t class_inventory_bytes[H8_MAGAZINE_TAIL_MAX_CLASSES];
  _Atomic uint64_t class_empty_bytes[H8_MAGAZINE_TAIL_MAX_CLASSES];
  _Atomic uint64_t class_max_empty_bytes[H8_MAGAZINE_TAIL_MAX_CLASSES];
} H8MagazineTailShadowState;

static H8MagazineTailShadowState h8_magazine_tail_shadow;

static void h8_mag_tail_add(_Atomic uint64_t* field, uint64_t value) {
  atomic_fetch_add_explicit(field, value, memory_order_relaxed);
}

static void h8_mag_tail_max(_Atomic uint64_t* field, uint64_t value) {
  uint64_t current = atomic_load_explicit(field, memory_order_relaxed);
  while (value > current &&
         !atomic_compare_exchange_weak_explicit(
             field, &current, value, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}

void h8_magazine_tail_shadow_checkpoint(H8ThreadCtx* ctx) {
  if (!ctx) return;
  h8_mag_tail_add(&h8_magazine_tail_shadow.checkpoints, 1u);
#if H8_REUSABLE_SPAN_MAGAZINE_L1
  uint64_t inventory_bytes = 0;
  uint64_t empty_bytes = 0;
  uint64_t hypothetical_bytes = 0;
  uint64_t class_empty_bytes[H8_MAGAZINE_TAIL_MAX_CLASSES] = {0};
  for (uint32_t class_id = 0; class_id < H8_CLASS_COUNT; ++class_id) {
    size_t count = ctx->reusable_span_count[class_id];
    size_t tail_count = (count + 1u) / 2u;
    for (size_t i = 0; i < count; ++i) {
      H8Span* span = ctx->reusable_span_mag[class_id][i];
      if (!span) continue;
      h8_mag_tail_add(&h8_magazine_tail_shadow.inventory_spans, 1u);
      inventory_bytes += H8_SPAN_BYTES;
      h8_mag_tail_add(&h8_magazine_tail_shadow.inventory_bytes, H8_SPAN_BYTES);
      h8_mag_tail_add(
          &h8_magazine_tail_shadow.class_inventory_bytes[class_id],
          H8_SPAN_BYTES);
      if (i < tail_count) {
        h8_mag_tail_add(&h8_magazine_tail_shadow.tail_spans, 1u);
      }
      if (h8_span_state_load(span) != H8_SPAN_OWNED_ACTIVE) {
        h8_mag_tail_add(&h8_magazine_tail_shadow.blocked_state, 1u);
        continue;
      }
      if (!h8_span_pending_quiescent(span)) {
        h8_mag_tail_add(&h8_magazine_tail_shadow.blocked_pending, 1u);
        continue;
      }
      if (h8_slot_allocated_count_quiescent(span) != 0u) {
        h8_mag_tail_add(&h8_magazine_tail_shadow.blocked_live, 1u);
        continue;
      }
      h8_mag_tail_add(&h8_magazine_tail_shadow.empty_spans, 1u);
      empty_bytes += H8_SPAN_BYTES;
      class_empty_bytes[class_id] += H8_SPAN_BYTES;
      h8_mag_tail_add(&h8_magazine_tail_shadow.empty_bytes, H8_SPAN_BYTES);
      h8_mag_tail_add(&h8_magazine_tail_shadow.class_empty_bytes[class_id],
                      H8_SPAN_BYTES);
      if (i < tail_count) {
        h8_mag_tail_add(&h8_magazine_tail_shadow.tail_empty_spans, 1u);
        h8_mag_tail_add(
            &h8_magazine_tail_shadow.hypothetical_decommit_bytes,
            H8_SPAN_BYTES);
        hypothetical_bytes += H8_SPAN_BYTES;
      }
    }
  }
  h8_mag_tail_max(&h8_magazine_tail_shadow.max_inventory_bytes,
                  inventory_bytes);
  h8_mag_tail_max(&h8_magazine_tail_shadow.max_empty_bytes, empty_bytes);
  h8_mag_tail_max(&h8_magazine_tail_shadow.max_hypothetical_decommit_bytes,
                  hypothetical_bytes);
  for (size_t i = 0; i < H8_MAGAZINE_TAIL_MAX_CLASSES; ++i) {
    h8_mag_tail_max(&h8_magazine_tail_shadow.class_max_empty_bytes[i],
                    class_empty_bytes[i]);
  }
#endif
}

#define H8_MAG_TAIL_LOAD(field) atomic_load_explicit(                       \
    &h8_magazine_tail_shadow.field, memory_order_relaxed)

H8MagazineTailShadowSnapshot h8_magazine_tail_shadow_snapshot(void) {
  H8MagazineTailShadowSnapshot out = {0};
  out.checkpoints = H8_MAG_TAIL_LOAD(checkpoints);
  out.inventory_spans = H8_MAG_TAIL_LOAD(inventory_spans);
  out.inventory_bytes = H8_MAG_TAIL_LOAD(inventory_bytes);
  out.empty_spans = H8_MAG_TAIL_LOAD(empty_spans);
  out.empty_bytes = H8_MAG_TAIL_LOAD(empty_bytes);
  out.tail_spans = H8_MAG_TAIL_LOAD(tail_spans);
  out.tail_empty_spans = H8_MAG_TAIL_LOAD(tail_empty_spans);
  out.hypothetical_decommit_bytes =
      H8_MAG_TAIL_LOAD(hypothetical_decommit_bytes);
  out.max_inventory_bytes = H8_MAG_TAIL_LOAD(max_inventory_bytes);
  out.max_empty_bytes = H8_MAG_TAIL_LOAD(max_empty_bytes);
  out.max_hypothetical_decommit_bytes =
      H8_MAG_TAIL_LOAD(max_hypothetical_decommit_bytes);
  out.blocked_live = H8_MAG_TAIL_LOAD(blocked_live);
  out.blocked_pending = H8_MAG_TAIL_LOAD(blocked_pending);
  out.blocked_state = H8_MAG_TAIL_LOAD(blocked_state);
  for (size_t i = 0; i < H8_MAGAZINE_TAIL_MAX_CLASSES; ++i) {
    out.class_inventory_bytes[i] = atomic_load_explicit(
        &h8_magazine_tail_shadow.class_inventory_bytes[i],
        memory_order_relaxed);
    out.class_empty_bytes[i] = atomic_load_explicit(
        &h8_magazine_tail_shadow.class_empty_bytes[i], memory_order_relaxed);
    out.class_max_empty_bytes[i] = atomic_load_explicit(
        &h8_magazine_tail_shadow.class_max_empty_bytes[i],
        memory_order_relaxed);
  }
  return out;
}

void h8_magazine_tail_shadow_dump(void) {
  H8MagazineTailShadowSnapshot s = h8_magazine_tail_shadow_snapshot();
  fprintf(stderr,
          "[H8_MAG_TAIL] checkpoints=%llu inventory_spans=%llu "
          "inventory_bytes=%llu empty_spans=%llu empty_bytes=%llu "
          "tail_spans=%llu tail_empty=%llu hypothetical_decommit_bytes=%llu "
          "max_inventory_bytes=%llu max_empty_bytes=%llu max_hypothetical_bytes=%llu "
          "blocked_live=%llu blocked_pending=%llu blocked_state=%llu\n",
          (unsigned long long)s.checkpoints,
          (unsigned long long)s.inventory_spans,
          (unsigned long long)s.inventory_bytes,
          (unsigned long long)s.empty_spans,
          (unsigned long long)s.empty_bytes,
          (unsigned long long)s.tail_spans,
          (unsigned long long)s.tail_empty_spans,
          (unsigned long long)s.hypothetical_decommit_bytes,
          (unsigned long long)s.max_inventory_bytes,
          (unsigned long long)s.max_empty_bytes,
          (unsigned long long)s.max_hypothetical_decommit_bytes,
          (unsigned long long)s.blocked_live,
          (unsigned long long)s.blocked_pending,
          (unsigned long long)s.blocked_state);
  fprintf(stderr, "[H8_MAG_TAIL_CLASS] inventory_bytes=");
  for (size_t i = 0; i < H8_MAGAZINE_TAIL_MAX_CLASSES; ++i) {
    fprintf(stderr, "%s%llu", i ? "," : "",
            (unsigned long long)s.class_inventory_bytes[i]);
  }
  fprintf(stderr, " empty_bytes=");
  for (size_t i = 0; i < H8_MAGAZINE_TAIL_MAX_CLASSES; ++i) {
    fprintf(stderr, "%s%llu", i ? "," : "",
            (unsigned long long)s.class_empty_bytes[i]);
  }
  fprintf(stderr, " max_empty_bytes=");
  for (size_t i = 0; i < H8_MAGAZINE_TAIL_MAX_CLASSES; ++i) {
    fprintf(stderr, "%s%llu", i ? "," : "",
            (unsigned long long)s.class_max_empty_bytes[i]);
  }
  fprintf(stderr, "\n");
}

#endif
