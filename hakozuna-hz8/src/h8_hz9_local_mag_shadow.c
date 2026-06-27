#include "h8_internal.h"

#if defined(H8_HZ9_MEDIUM_LOCAL_MAG_SHADOW)

static inline uint64_t h8_hz9_local_mag_shadow_bit(size_t slot) {
  return UINT64_C(1) << slot;
}

void h8_hz9_local_mag_shadow_flush_class(H8ThreadCtx* ctx, uint32_t class_id,
                                         bool owner_exit) {
  if (!ctx || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8MediumRun* run = ctx->medium_local_mag_shadow_run[class_id];
  uint64_t mask = ctx->medium_local_mag_shadow_mask[class_id];
  if (!run || mask == 0u) {
    ctx->medium_local_mag_shadow_run[class_id] = NULL;
    ctx->medium_local_mag_shadow_mask[class_id] = 0u;
    return;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  size_t count = (size_t)__builtin_popcountll(mask);
  if (owner_exit) {
    H8_DEBUG_ADD(medium_hz9_local_mag_shadow_owner_exit_flush, count);
  } else {
    H8_DEBUG_ADD(medium_hz9_local_mag_shadow_active_switch_flush, count);
  }
#else
  (void)owner_exit;
#endif
  ctx->medium_local_mag_shadow_run[class_id] = NULL;
  ctx->medium_local_mag_shadow_mask[class_id] = 0u;
}

void h8_hz9_local_mag_shadow_flush_owner(H8ThreadCtx* ctx, bool owner_exit) {
  if (!ctx) {
    return;
  }
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    h8_hz9_local_mag_shadow_flush_class(ctx, c, owner_exit);
  }
}

void h8_hz9_local_mag_shadow_note_free(H8MediumRun* run, size_t slot,
                                       bool keep_empty_live) {
  H8ThreadCtx* ctx = h8_tls_ctx;
  if (!ctx || !run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  if (!keep_empty_live || ctx->active_medium_runs[run->class_id] != run) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_INC(medium_hz9_local_mag_shadow_not_active_run);
#endif
    return;
  }
  if (slot >= 64u) {
    return;
  }

  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
#if !defined(H8_MEDIUM_CEILING_ALLOC_NO_SLOT_STATE)
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_INC(medium_hz9_local_mag_shadow_state_mismatch);
#endif
    return;
  }
#else
  (void)state;
#endif

  uint64_t bit = h8_hz9_local_mag_shadow_bit(slot);
  uint64_t pending =
      atomic_load_explicit(&run->pending_bits[slot >> 6u], memory_order_acquire);
  if ((pending & bit) != 0u) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_INC(medium_hz9_local_mag_shadow_pending_block);
#endif
    return;
  }

  H8MediumRun* shadow_run = ctx->medium_local_mag_shadow_run[run->class_id];
  if (shadow_run && shadow_run != run) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_INC(medium_hz9_local_mag_shadow_not_active_run);
#endif
    return;
  }
  if (!shadow_run) {
    ctx->medium_local_mag_shadow_run[run->class_id] = run;
  }

  uint64_t mask = ctx->medium_local_mag_shadow_mask[run->class_id];
  if ((mask & bit) != 0u) {
    return;
  }
  ctx->medium_local_mag_shadow_mask[run->class_id] = mask | bit;
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(medium_hz9_local_mag_shadow_eligible_free);
  H8_DEBUG_INC(medium_hz9_local_mag_shadow_avoid_mark_empty);
#endif
}

void h8_hz9_local_mag_shadow_note_alloc(H8MediumRun* run, size_t slot) {
  H8ThreadCtx* ctx = h8_tls_ctx;
  if (!ctx || !run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  if (ctx->active_medium_runs[run->class_id] != run) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_INC(medium_hz9_local_mag_shadow_not_active_run);
#endif
    return;
  }
  if (slot >= 64u) {
    return;
  }

  H8MediumRun* shadow_run = ctx->medium_local_mag_shadow_run[run->class_id];
  if (shadow_run != run) {
    return;
  }

  uint64_t bit = h8_hz9_local_mag_shadow_bit(slot);
  uint64_t mask = ctx->medium_local_mag_shadow_mask[run->class_id];
  if ((mask & bit) == 0u) {
    return;
  }

  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
#if !defined(H8_MEDIUM_CEILING_ALLOC_NO_SLOT_STATE)
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_INC(medium_hz9_local_mag_shadow_state_mismatch);
#endif
    return;
  }
#else
  (void)state;
#endif

  mask &= ~bit;
  ctx->medium_local_mag_shadow_mask[run->class_id] = mask;
  if (mask == 0u) {
    ctx->medium_local_mag_shadow_run[run->class_id] = NULL;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(medium_hz9_local_mag_shadow_eligible_alloc);
  H8_DEBUG_INC(medium_hz9_local_mag_shadow_avoid_mark_live);
#endif
}

#else
typedef int h8_hz9_local_mag_shadow_translation_unit_silence;
#endif
