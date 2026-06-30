#include "h8_internal.h"

static size_t h8_pressure_collect_budget(H8OwnerRecord* owner) {
  size_t pending =
      atomic_load_explicit(&owner->pending_span_count, memory_order_acquire);
  if (pending == 0) {
    return 0;
  }
  if (pending <= 2) {
    return 1;
  }
  if (pending <= 8) {
    return 2;
  }
  if (pending <= 32) {
    return 4;
  }
  return 8;
}

static size_t h8_pressure_collect_budget_remote(size_t pending) {
  if (pending == 0) {
    return 0;
  }
  if (pending <= 2) {
    return pending;
  }
  if (pending <= 8) {
    return 4;
  }
  if (pending <= 32) {
    return 8;
  }
  if (pending <= 128) {
    return 16;
  }
  return 32;
}

#if defined(H8_REMOTE_PRESSURE_ACTIVE_FULL_BUDGET_L1)
static size_t h8_pressure_collect_budget_active_full(size_t pending) {
  if (pending == 0) {
    return 0;
  }
  if (pending <= 2) {
    return pending;
  }
  if (pending <= 8) {
    return 8;
  }
  if (pending <= 32) {
    return 16;
  }
  if (pending <= 128) {
    return 32;
  }
  return 64;
}
#endif

static size_t h8_pressure_collect_budget_remote_source(
    size_t pending, H8RemotePressureCollectSource source) {
#if defined(H8_REMOTE_PRESSURE_ACTIVE_FULL_BUDGET_L1)
  if (source == H8_REMOTE_PRESSURE_COLLECT_SOURCE_ACTIVE_HIT_FULL) {
    return h8_pressure_collect_budget_active_full(pending);
  }
#else
  (void)source;
#endif
  return h8_pressure_collect_budget_remote(pending);
}

void h8_pressure_owner_collect(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  size_t budget = h8_pressure_collect_budget(owner);
  if (budget == 0) {
    return;
  }
  (void)h8_collect_owner_pending_budget(owner, budget);
}

static void h8_pressure_collect_note_source(H8RemotePressureCollectSource source) {
#if defined(H8_ENABLE_DEBUG_STATS)
  switch (source) {
    case H8_REMOTE_PRESSURE_COLLECT_SOURCE_ACTIVE_HIT_FULL:
      H8_DEBUG_INC(remote_pressure_collect_source_active_hit_full_count);
      break;
    case H8_REMOTE_PRESSURE_COLLECT_SOURCE_ACTIVE_MISS:
      H8_DEBUG_INC(remote_pressure_collect_source_active_miss_count);
      break;
    case H8_REMOTE_PRESSURE_COLLECT_SOURCE_OWNER_EXIT:
      H8_DEBUG_INC(remote_pressure_collect_source_owner_exit_count);
      break;
    case H8_REMOTE_PRESSURE_COLLECT_SOURCE_COUNT:
      break;
  }
#else
  (void)source;
#endif
}

void h8_pressure_owner_collect_remote_pressure(H8OwnerRecord* owner,
                                              H8RemotePressureCollectSource source) {
  if (!owner) {
    return;
  }
  size_t pending_before =
      atomic_load_explicit(&owner->pending_span_count, memory_order_acquire);
  size_t budget =
      h8_pressure_collect_budget_remote_source(pending_before, source);
  if (budget == 0) {
    return;
  }
  h8_pressure_collect_note_source(source);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(small_remote_pressure_collect_call_count);
  H8_DEBUG_ADD(small_remote_pressure_collect_budget_count, budget);
  H8_DEBUG_ADD(small_remote_pressure_collect_pending_before_count, pending_before);
#endif
  size_t collected = h8_collect_owner_pending_budget(owner, budget);
  size_t pending_after =
      atomic_load_explicit(&owner->pending_span_count, memory_order_acquire);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(small_remote_pressure_collect_span_count, collected);
  H8_DEBUG_ADD(small_remote_pressure_collect_pending_after_count, pending_after);
#else
  (void)collected;
  (void)pending_after;
#endif
}

H8Span* h8_pressure_refill(H8OwnerRecord* owner, uint32_t class_id) {
  return h8_span_commit_for_class(owner ? owner : h8_owner_current(), class_id);
}
