#include "h8_internal.h"

#include <string.h>

void h8_stats_snapshot(H8Stats* out) {
  size_t small_span_count = 0;
  for (size_t i = 0; i < h8g.span_count; ++i) {
    if (h8g.spans &&
        atomic_load_explicit(&h8g.spans[i], memory_order_acquire)) {
      ++small_span_count;
    }
  }
  out->arena_reserved_bytes = h8g.arena_bytes;
  out->arena_committed_bytes =
      atomic_load_explicit(&h8g.arena_committed_bytes, memory_order_acquire);
  out->small_span_count = small_span_count;
  out->owner_count =
      atomic_load_explicit(&h8g.owner_count, memory_order_acquire);
  out->orphan_span_count =
      atomic_load_explicit(&h8g.orphan_span_count, memory_order_acquire);
  out->local_alloc_count =
      atomic_load_explicit(&h8g.local_alloc_count, memory_order_acquire);
  out->local_free_count =
      atomic_load_explicit(&h8g.local_free_count, memory_order_acquire);
  out->remote_publish_count =
      atomic_load_explicit(&h8g.remote_publish_count, memory_order_acquire);
  out->remote_collect_count =
      atomic_load_explicit(&h8g.remote_collect_count, memory_order_acquire);
  out->owner_exit_count =
      atomic_load_explicit(&h8g.owner_exit_count, memory_order_acquire);
  out->pending_enqueue_count =
      atomic_load_explicit(&h8g.pending_enqueue_count, memory_order_acquire);
  out->pending_dequeue_count =
      atomic_load_explicit(&h8g.pending_dequeue_count, memory_order_acquire);
  out->orphan_handoff_count =
      atomic_load_explicit(&h8g.orphan_handoff_count, memory_order_acquire);
  out->handoff_success_count =
      atomic_load_explicit(&h8g.handoff_success_count, memory_order_acquire);
}

H8Stats h8_stats(void) {
  H8Stats stats;
  memset(&stats, 0, sizeof(stats));
  h8_stats_snapshot(&stats);
  return stats;
}
