#include "h8_adaptive_shadow.h"

#if defined(H8_ADAPTIVE_TRANSFER_SHADOW_L0)

#include <stdatomic.h>
#include <stdio.h>

typedef struct H8AdaptiveShadowState {
  _Atomic uint64_t recommendation[H8_ADAPTIVE_SHADOW_MODE_COUNT];
  _Atomic uint64_t small_refill[H8_CLASS_COUNT];
  _Atomic uint64_t small_refill_pending[H8_CLASS_COUNT];
  _Atomic uint64_t medium_source[H8_MEDIUM_CLASS_COUNT];
  _Atomic uint64_t medium_rss_pressure[H8_MEDIUM_CLASS_COUNT];
  _Atomic uint64_t remote_collect;
  _Atomic uint64_t remote_pending_before;
  _Atomic uint64_t remote_pending_after;
} H8AdaptiveShadowState;

static H8AdaptiveShadowState h8_adaptive_shadow;

static void h8_adaptive_shadow_recommend(H8AdaptiveShadowMode mode) {
  atomic_fetch_add_explicit(&h8_adaptive_shadow.recommendation[mode], 1u,
                            memory_order_relaxed);
}

static bool h8_adaptive_shadow_rss_high(size_t bytes, size_t budget) {
  return budget > 0u && bytes >= budget - budget / 4u;
}

void h8_adaptive_shadow_note_small_refill(uint32_t class_id, size_t pending) {
  if (class_id >= H8_CLASS_COUNT) {
    return;
  }
  atomic_fetch_add_explicit(&h8_adaptive_shadow.small_refill[class_id], 1u,
                            memory_order_relaxed);
  if (pending > 0u) {
    atomic_fetch_add_explicit(
        &h8_adaptive_shadow.small_refill_pending[class_id], 1u,
        memory_order_relaxed);
    h8_adaptive_shadow_recommend(H8_ADAPTIVE_SHADOW_TRANSFER_PRESSURE);
    return;
  }
  h8_adaptive_shadow_recommend(H8_ADAPTIVE_SHADOW_BALANCED);
}

void h8_adaptive_shadow_note_medium_source(uint32_t class_id,
                                           size_t resident_bytes,
                                           size_t resident_budget) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  atomic_fetch_add_explicit(&h8_adaptive_shadow.medium_source[class_id], 1u,
                            memory_order_relaxed);
  h8_adaptive_shadow_recommend(
      h8_adaptive_shadow_rss_high(resident_bytes, resident_budget)
          ? H8_ADAPTIVE_SHADOW_RSS_PRESSURE
          : H8_ADAPTIVE_SHADOW_TRANSFER_PRESSURE);
}

void h8_adaptive_shadow_note_remote_collect(size_t pending_before,
                                            size_t pending_after) {
  atomic_fetch_add_explicit(&h8_adaptive_shadow.remote_collect, 1u,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8_adaptive_shadow.remote_pending_before,
                            pending_before, memory_order_relaxed);
  atomic_fetch_add_explicit(&h8_adaptive_shadow.remote_pending_after,
                            pending_after, memory_order_relaxed);
  h8_adaptive_shadow_recommend(H8_ADAPTIVE_SHADOW_TRANSFER_PRESSURE);
}

void h8_adaptive_shadow_note_medium_residency(uint32_t class_id,
                                              size_t resident_bytes,
                                              size_t resident_budget,
                                              bool rejected) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  if (rejected || h8_adaptive_shadow_rss_high(resident_bytes, resident_budget)) {
    atomic_fetch_add_explicit(
        &h8_adaptive_shadow.medium_rss_pressure[class_id], 1u,
        memory_order_relaxed);
    h8_adaptive_shadow_recommend(H8_ADAPTIVE_SHADOW_RSS_PRESSURE);
    return;
  }
  h8_adaptive_shadow_recommend(H8_ADAPTIVE_SHADOW_BALANCED);
}

H8AdaptiveShadowSnapshot h8_adaptive_shadow_snapshot(void) {
  H8AdaptiveShadowSnapshot snapshot = {0};
  for (uint32_t i = 0u; i < H8_ADAPTIVE_SHADOW_MODE_COUNT; ++i) {
    snapshot.recommendation[i] = atomic_load_explicit(
        &h8_adaptive_shadow.recommendation[i], memory_order_relaxed);
  }
  for (uint32_t i = 0u; i < H8_CLASS_COUNT; ++i) {
    snapshot.small_refill[i] = atomic_load_explicit(
        &h8_adaptive_shadow.small_refill[i], memory_order_relaxed);
    snapshot.small_refill_pending[i] = atomic_load_explicit(
        &h8_adaptive_shadow.small_refill_pending[i], memory_order_relaxed);
  }
  for (uint32_t i = 0u; i < H8_MEDIUM_CLASS_COUNT; ++i) {
    snapshot.medium_source[i] = atomic_load_explicit(
        &h8_adaptive_shadow.medium_source[i], memory_order_relaxed);
    snapshot.medium_rss_pressure[i] = atomic_load_explicit(
        &h8_adaptive_shadow.medium_rss_pressure[i], memory_order_relaxed);
  }
  snapshot.remote_collect = atomic_load_explicit(
      &h8_adaptive_shadow.remote_collect, memory_order_relaxed);
  snapshot.remote_pending_before = atomic_load_explicit(
      &h8_adaptive_shadow.remote_pending_before, memory_order_relaxed);
  snapshot.remote_pending_after = atomic_load_explicit(
      &h8_adaptive_shadow.remote_pending_after, memory_order_relaxed);
  return snapshot;
}

void h8_adaptive_shadow_dump(void) {
  H8AdaptiveShadowSnapshot snapshot = h8_adaptive_shadow_snapshot();
  fprintf(stderr,
          "[h8 adaptive shadow] balanced=%llu transfer=%llu rss=%llu "
          "remote_collect=%llu pending_before=%llu pending_after=%llu\n",
          (unsigned long long)
              snapshot.recommendation[H8_ADAPTIVE_SHADOW_BALANCED],
          (unsigned long long)
              snapshot.recommendation[H8_ADAPTIVE_SHADOW_TRANSFER_PRESSURE],
          (unsigned long long)
              snapshot.recommendation[H8_ADAPTIVE_SHADOW_RSS_PRESSURE],
          (unsigned long long)snapshot.remote_collect,
          (unsigned long long)snapshot.remote_pending_before,
          (unsigned long long)snapshot.remote_pending_after);
}

#endif
