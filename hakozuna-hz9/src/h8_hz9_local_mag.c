#include "h8_internal.h"
#include "h8_medium.h"

#if defined(H8_HZ9_MEDIUM_LOCAL_MAG)

static void h8_hz9_local_mag_clear_tls_run(H8MediumRun* run) {
  if (!h8_tls_ctx || !run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  if (h8_tls_ctx->medium_local_mag[run->class_id].run == run) {
    h8_tls_ctx->medium_local_mag[run->class_id].run = NULL;
    h8_tls_ctx->medium_local_mag[run->class_id].mask = 0u;
  }
}

typedef enum H8Hz9LocalMagFlushSource {
  H8_HZ9_LOCAL_MAG_FLUSH_ACTIVE_SWITCH = 0,
  H8_HZ9_LOCAL_MAG_FLUSH_OWNER_EXIT = 1,
  H8_HZ9_LOCAL_MAG_FLUSH_RUN_SCAN = 2
} H8Hz9LocalMagFlushSource;

static size_t h8_hz9_local_mag_flush_mask(
    H8MediumRun* run, uint64_t mask, H8Hz9LocalMagFlushSource source) {
  if (!run || mask == 0u) {
    return 0u;
  }
  uint64_t requested = mask;
  size_t count = 0u;
  while (mask) {
    size_t slot = (size_t)__builtin_ctzll(mask);
    uint64_t bit = h8_hz9_local_mag_bit(slot);
    if (slot < run->slot_count) {
#if defined(H8_HZ9_MEDIUM_LOCAL_MAG_UNSAFE_TLS_ONLY)
      atomic_store_explicit(&run->slot_state[slot], H8_SLOT_FREE | H8_SLOT_NONE,
                            memory_order_release);
      run->allocated_mask &= ~bit;
      run->free_mask |= bit;
      ++count;
#else
      uint32_t state =
          atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
      if (h8_slot_state_is_local_mag(state)) {
        atomic_store_explicit(&run->slot_state[slot],
                              H8_SLOT_FREE | H8_SLOT_NONE,
                              memory_order_release);
        run->allocated_mask &= ~bit;
        run->free_mask |= bit;
        ++count;
      } else {
        H8_DEBUG_INC(medium_hz9_local_mag_state_mismatch);
      }
#endif
    }
    mask &= mask - 1u;
  }
  if (h8_tls_ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
      h8_tls_ctx->medium_local_mag[run->class_id].run == run) {
    h8_tls_ctx->medium_local_mag[run->class_id].mask &= ~requested;
    if (h8_tls_ctx->medium_local_mag[run->class_id].mask == 0u) {
      h8_tls_ctx->medium_local_mag[run->class_id].run = NULL;
    }
  }
  H8_DEBUG_ADD(medium_hz9_local_mag_flush_slots, count);
  h8_hz9_local_mag_debug_flush_slots_class(run->class_id, count);
  if (source == H8_HZ9_LOCAL_MAG_FLUSH_ACTIVE_SWITCH) {
    H8_DEBUG_ADD(medium_hz9_local_mag_flush_active_switch_slots, count);
  } else if (source == H8_HZ9_LOCAL_MAG_FLUSH_OWNER_EXIT) {
    H8_DEBUG_ADD(medium_hz9_local_mag_flush_owner_exit_slots, count);
  } else {
    H8_DEBUG_ADD(medium_hz9_local_mag_flush_run_scan_slots, count);
  }
  return count;
}

void h8_hz9_local_mag_flush_class(H8ThreadCtx* ctx, uint32_t class_id,
                                  bool owner_exit) {
  (void)owner_exit;
  if (!ctx || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8MediumRun* run = ctx->medium_local_mag[class_id].run;
  uint64_t mask = ctx->medium_local_mag[class_id].mask;
  ctx->medium_local_mag[class_id].run = NULL;
  ctx->medium_local_mag[class_id].mask = 0u;
  h8_hz9_local_mag_flush_mask(
      run, mask, owner_exit ? H8_HZ9_LOCAL_MAG_FLUSH_OWNER_EXIT
                            : H8_HZ9_LOCAL_MAG_FLUSH_ACTIVE_SWITCH);
}

void h8_hz9_local_mag_flush_owner(H8ThreadCtx* ctx, H8OwnerRecord* owner,
                                  bool owner_exit) {
  if (!ctx || ctx->owner != owner) {
    return;
  }
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    h8_hz9_local_mag_flush_class(ctx, c, owner_exit);
  }
}

/*
 * Caller contract: run mask mutation requires the run lock, or an equivalent
 * quiescent/unregistered state such as run destroy after unregister.
 */
void h8_hz9_local_mag_flush_run_locked(H8MediumRun* run, bool owner_exit) {
  (void)owner_exit;
  if (!run) {
    return;
  }
  uint64_t mask = 0u;
  for (uint16_t slot = 0; slot < run->slot_count; ++slot) {
    uint32_t state =
        atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
    if (h8_slot_state_is_local_mag(state)) {
      mask |= h8_hz9_local_mag_bit(slot);
    }
  }
  h8_hz9_local_mag_flush_mask(run, mask, H8_HZ9_LOCAL_MAG_FLUSH_RUN_SCAN);
  h8_hz9_local_mag_clear_tls_run(run);
}

#else
typedef int h8_hz9_local_mag_translation_unit_silence;
#endif
