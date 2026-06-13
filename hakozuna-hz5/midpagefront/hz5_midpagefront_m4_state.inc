#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
static uint16_t hz5_midpagefront_m4_mag_cap(uint32_t class_index) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_FLAT_MAG_CAP
  (void)class_index;
  return HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX;
#else
  static const uint16_t caps[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      64u, 64u, 32u, 16u, 8u
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
      ,
      4u
#endif
  };
  return class_index < HZ5_MIDPAGEFRONT_CLASS_COUNT ? caps[class_index] : 0u;
#endif
}

static uint64_t hz5_midpagefront_m4_state_shift(uint32_t slot) {
  return (uint64_t)slot * UINT64_C(2);
}

static uint64_t hz5_midpagefront_m4_state_mask(uint32_t slot) {
  return UINT64_C(3) << hz5_midpagefront_m4_state_shift(slot);
}

static uint64_t hz5_midpagefront_m4_initial_state(uint32_t slot_count) {
  uint64_t state = 0;
  if (slot_count > 32u) {
    slot_count = 32u;
  }
  for (uint32_t slot = 0; slot < slot_count; ++slot) {
    state |= (uint64_t)HZ5_MIDPAGE_SLOT_CACHE
             << hz5_midpagefront_m4_state_shift(slot);
  }
  return state;
}

static uint64_t hz5_midpagefront_m4_state_field_mask(uint32_t slot_count) {
  uint64_t mask = 0;
  if (slot_count > 32u) {
    slot_count = 32u;
  }
  for (uint32_t slot = 0; slot < slot_count; ++slot) {
    mask |= hz5_midpagefront_m4_state_mask(slot);
  }
  return mask;
}

static int hz5_midpagefront_m4_transition(Hz5MidPage* page,
                                          uint32_t slot,
                                          Hz5MidPageSlotState expected,
                                          Hz5MidPageSlotState desired) {
  if (!page || slot >= page->slot_count || slot >= 32u) {
    return 0;
  }
  uint64_t shift = hz5_midpagefront_m4_state_shift(slot);
  uint64_t mask = hz5_midpagefront_m4_state_mask(slot);
  uint64_t old = atomic_load_explicit(&page->slot_state2,
                                      memory_order_acquire);
  for (;;) {
    if (((old & mask) >> shift) != (uint64_t)expected) {
      return 0;
    }
    uint64_t next = (old & ~mask) | ((uint64_t)desired << shift);
    if (atomic_compare_exchange_weak_explicit(&page->slot_state2,
                                              &old,
                                              next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      return 1;
    }
  }
}

static int hz5_midpagefront_m4_transition_owner_local(
    Hz5MidPage* page,
    uint32_t slot,
    Hz5MidPageSlotState expected,
    Hz5MidPageSlotState desired) {
  if (!page || slot >= page->slot_count || slot >= 32u) {
    return 0;
  }
  uint64_t shift = hz5_midpagefront_m4_state_shift(slot);
  uint64_t mask = hz5_midpagefront_m4_state_mask(slot);
  uint64_t old = atomic_load_explicit(&page->slot_state2,
                                      memory_order_acquire);
  if (((old & mask) >> shift) != (uint64_t)expected) {
    return 0;
  }
  uint64_t next = (old & ~mask) | ((uint64_t)desired << shift);
  atomic_store_explicit(&page->slot_state2, next, memory_order_release);
  return 1;
}

static void hz5_midpagefront_m4_note_alloc(Hz5MidPage* page) {
  atomic_fetch_add_explicit(&page->live_count, 1u, memory_order_relaxed);
}

static int hz5_midpagefront_m4_note_free(Hz5MidPage* page) {
  uint32_t old = atomic_load_explicit(&page->live_count,
                                      memory_order_relaxed);
  for (;;) {
    if (old == 0u) {
      return 0;
    }
    if (atomic_compare_exchange_weak_explicit(&page->live_count,
                                              &old,
                                              old - 1u,
                                              memory_order_relaxed,
                                              memory_order_relaxed)) {
      return 1;
    }
  }
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE || \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M11_REMOTE_DIRECT_CACHE
static int hz5_midpagefront_m4_slot_state_is(Hz5MidPage* page,
                                             uint32_t slot,
                                             Hz5MidPageSlotState expected) {
  if (!page || slot >= page->slot_count || slot >= 32u) {
    return 0;
  }
  uint64_t shift = hz5_midpagefront_m4_state_shift(slot);
  uint64_t mask = hz5_midpagefront_m4_state_mask(slot);
  uint64_t old = atomic_load_explicit(&page->slot_state2,
                                      memory_order_acquire);
  return ((old & mask) >> shift) == (uint64_t)expected;
}
#endif
#endif
