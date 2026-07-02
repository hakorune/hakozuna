#if defined(H8_HZ9_MEDIUM_LOCAL_MAG)
static inline uint64_t h8_hz9_local_mag_bit(size_t slot) {
  return UINT64_C(1) << slot;
}

static inline void h8_hz9_local_mag_debug_class_add(uint32_t class_id,
                                                    atomic_size_t* class_24k,
                                                    atomic_size_t* class_32k,
                                                    atomic_size_t* class_48k,
                                                    atomic_size_t* class_64k,
                                                    size_t value) {
  if (class_id == 2u) {
    atomic_fetch_add_explicit(class_24k, value, memory_order_relaxed);
  } else if (class_id == 3u) {
    atomic_fetch_add_explicit(class_32k, value, memory_order_relaxed);
  } else if (class_id == 4u) {
    atomic_fetch_add_explicit(class_48k, value, memory_order_relaxed);
  } else if (class_id == 5u) {
    atomic_fetch_add_explicit(class_64k, value, memory_order_relaxed);
  }
}

static inline void h8_hz9_local_mag_debug_free_push_class(
    uint32_t class_id) {
  h8_hz9_local_mag_debug_class_add(
      class_id, &h8g.medium_hz9_local_mag_free_push_class_24k,
      &h8g.medium_hz9_local_mag_free_push_class_32k,
      &h8g.medium_hz9_local_mag_free_push_class_48k,
      &h8g.medium_hz9_local_mag_free_push_class_64k, 1u);
}

static inline void h8_hz9_local_mag_debug_alloc_hit_class(
    uint32_t class_id) {
  h8_hz9_local_mag_debug_class_add(
      class_id, &h8g.medium_hz9_local_mag_alloc_hit_class_24k,
      &h8g.medium_hz9_local_mag_alloc_hit_class_32k,
      &h8g.medium_hz9_local_mag_alloc_hit_class_48k,
      &h8g.medium_hz9_local_mag_alloc_hit_class_64k, 1u);
}

static inline void h8_hz9_local_mag_debug_flush_slots_class(
    uint32_t class_id, size_t count) {
  h8_hz9_local_mag_debug_class_add(
      class_id, &h8g.medium_hz9_local_mag_flush_slots_class_24k,
      &h8g.medium_hz9_local_mag_flush_slots_class_32k,
      &h8g.medium_hz9_local_mag_flush_slots_class_48k,
      &h8g.medium_hz9_local_mag_flush_slots_class_64k, count);
}

static inline void* h8_hz9_local_mag_try_alloc(H8ThreadCtx* ctx,
                                               uint32_t class_id,
                                               H8MediumRun* run) {
  if (!ctx || class_id >= H8_MEDIUM_CLASS_COUNT || !run) {
    return NULL;
  }
  if (!h8_hz9_local_mag_enabled_run(run)) {
    return NULL;
  }
  if (ctx->medium_local_mag[class_id].run != run ||
      ctx->medium_local_mag[class_id].mask == 0u) {
    return NULL;
  }
  H8_DEBUG_INC(medium_hz9_local_mag_alloc_attempt);

  uint64_t mask = ctx->medium_local_mag[class_id].mask;
  size_t slot = (size_t)__builtin_ctzll(mask);
  uint64_t bit = h8_hz9_local_mag_bit(slot);
  if (slot >= run->slot_count) {
    H8_DEBUG_INC(medium_hz9_local_mag_state_mismatch);
    mask &= ~bit;
    ctx->medium_local_mag[class_id].mask = mask;
    if (mask == 0u) {
      ctx->medium_local_mag[class_id].run = NULL;
    }
    return NULL;
  }

#if !defined(H8_HZ9_MEDIUM_LOCAL_MAG_UNSAFE_TLS_ONLY)
  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
  if (!h8_slot_state_is_local_mag(state)) {
    H8_DEBUG_INC(medium_hz9_local_mag_state_mismatch);
    mask &= ~bit;
    ctx->medium_local_mag[class_id].mask = mask;
    if (mask == 0u) {
      ctx->medium_local_mag[class_id].run = NULL;
    }
    return NULL;
  }
#endif

  mask &= ~bit;
  ctx->medium_local_mag[class_id].mask = mask;
  if (mask == 0u) {
    ctx->medium_local_mag[class_id].run = NULL;
  }
#if !defined(H8_HZ9_MEDIUM_LOCAL_MAG_UNSAFE_TLS_ONLY)
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
#endif
  H8_DEBUG_INC(medium_hz9_local_mag_alloc_hit);
  h8_hz9_local_mag_debug_alloc_hit_class(class_id);
  return h8_medium_slot_ptr_known(run, slot);
}

static inline H8Hz9LocalMagFreeResult h8_hz9_local_mag_try_free_known(
    H8ThreadCtx* ctx, H8MediumRun* run, size_t slot, uint64_t bit) {
  if (!ctx || !run || run->class_id >= H8_MEDIUM_CLASS_COUNT ||
      slot >= run->slot_count || bit == 0u) {
    H8_DEBUG_INC(medium_hz9_local_mag_not_active);
    return H8_HZ9_LOCAL_MAG_FREE_SKIP;
  }
  if (ctx->active_medium_runs[run->class_id] != run) {
    H8_DEBUG_INC(medium_hz9_local_mag_not_active);
    H8_DEBUG_INC(medium_hz9_local_mag_free_active_mismatch);
    return H8_HZ9_LOCAL_MAG_FREE_SKIP;
  }
  if (!h8_hz9_local_mag_enabled_run(run)) {
    H8_DEBUG_INC(medium_hz9_local_mag_not_active);
    H8_DEBUG_INC(medium_hz9_local_mag_free_class_disabled);
    return H8_HZ9_LOCAL_MAG_FREE_SKIP;
  }
  H8_DEBUG_INC(medium_hz9_local_mag_free_attempt);

  uint32_t class_id = run->class_id;
  if (ctx->medium_local_mag[class_id].run &&
      ctx->medium_local_mag[class_id].run != run) {
    H8_DEBUG_INC(medium_hz9_local_mag_not_active);
    H8_DEBUG_INC(medium_hz9_local_mag_free_mag_run_mismatch);
    return H8_HZ9_LOCAL_MAG_FREE_SKIP;
  }
  if ((ctx->medium_local_mag[class_id].mask & bit) != 0u) {
    H8_DEBUG_INC(medium_hz9_local_mag_state_mismatch);
    return H8_HZ9_LOCAL_MAG_FREE_INVALID;
  }

  ctx->medium_local_mag[class_id].run = run;
  ctx->medium_local_mag[class_id].mask |= bit;
#if !defined(H8_HZ9_MEDIUM_LOCAL_MAG_UNSAFE_TLS_ONLY)
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_LOCAL_MAG,
                        memory_order_release);
#endif
  H8_DEBUG_INC(medium_hz9_local_mag_free_push);
  h8_hz9_local_mag_debug_free_push_class(class_id);
  return H8_HZ9_LOCAL_MAG_FREE_OK;
}
#endif
