// MidPage remote experiment helpers (M6/M7/M8/M10/M11).

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    HZ5_MIDPAGEFRONT_BATCH_STATE_HELPERS
typedef struct Hz5MidPageM6Batch {
  Hz5MidPage* page;
  uint64_t bits;
} Hz5MidPageM6Batch;

static int hz5_midpagefront_m6_batch_add(Hz5MidPageM6Batch* batch,
                                         uint32_t* count,
                                         Hz5MidPage* page,
                                         uint64_t mask) {
  if (!batch || !count || !page || mask == 0) {
    return 0;
  }
  for (uint32_t i = 0; i < *count; ++i) {
    if (batch[i].page == page) {
      batch[i].bits |= mask;
      return 1;
    }
  }
  if (*count >= HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    return 0;
  }
  batch[*count].page = page;
  batch[*count].bits = mask;
  *count += 1u;
  return 1;
}

static uint64_t hz5_midpagefront_m6_state_bits(uint64_t slots,
                                               Hz5MidPageSlotState state) {
  uint64_t out = 0;
  while (slots != 0) {
    uint64_t mask = slots & (UINT64_C(0) - slots);
    slots &= ~mask;
    uint32_t slot = (uint32_t)__builtin_ctzll(mask);
    out |= (uint64_t)state << hz5_midpagefront_m4_state_shift(slot);
  }
  return out;
}

static int hz5_midpagefront_m6_transition_owner_local_batch(
    Hz5MidPage* page,
    uint64_t slots,
    Hz5MidPageSlotState expected,
    Hz5MidPageSlotState desired) {
  if (!page) {
    return 0;
  }
  slots &= hz5_midpagefront_slot_count_mask(page->slot_count);
  if (slots == 0) {
    return 0;
  }
  uint64_t state_mask = hz5_midpagefront_m6_state_bits(
      slots, HZ5_MIDPAGE_SLOT_DEAD);
  uint64_t expected_bits = hz5_midpagefront_m6_state_bits(slots, expected);
  uint64_t desired_bits = hz5_midpagefront_m6_state_bits(slots, desired);
  uint64_t old = atomic_load_explicit(&page->slot_state2,
                                      memory_order_acquire);
  if ((old & state_mask) != expected_bits) {
    return 0;
  }
  uint64_t next = (old & ~state_mask) | desired_bits;
  atomic_store_explicit(&page->slot_state2, next, memory_order_release);
  return 1;
}

static int hz5_midpagefront_m7_transition_remote_batch(
    Hz5MidPage* page,
    uint64_t slots) {
  if (!page) {
    return 0;
  }
  slots &= hz5_midpagefront_slot_count_mask(page->slot_count);
  if (slots == 0) {
    return 0;
  }
  uint64_t state_mask = hz5_midpagefront_m6_state_bits(
      slots, HZ5_MIDPAGE_SLOT_DEAD);
  uint64_t expected_bits = hz5_midpagefront_m6_state_bits(
      slots, HZ5_MIDPAGE_SLOT_LIVE);
  uint64_t desired_bits = hz5_midpagefront_m6_state_bits(
      slots, HZ5_MIDPAGE_SLOT_REMOTE);
  uint64_t old = atomic_load_explicit(&page->slot_state2,
                                      memory_order_acquire);
  for (;;) {
    if ((old & state_mask) != expected_bits) {
      return 0;
    }
    uint64_t next = (old & ~state_mask) | desired_bits;
    if (atomic_compare_exchange_weak_explicit(&page->slot_state2,
                                              &old,
                                              next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      return 1;
    }
  }
}

static uint64_t hz5_midpagefront_m7_claim_remote_bits(Hz5MidPage* page,
                                                       uint64_t bits) {
  if (!page) {
    return 0;
  }
  bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
  if (bits == 0) {
    return 0;
  }
  if (hz5_midpagefront_m7_transition_remote_batch(page, bits)) {
    return bits;
  }
  uint64_t claimed = 0;
  uint64_t pending = bits;
  while (pending != 0) {
    uint64_t mask = pending & (UINT64_C(0) - pending);
    pending &= ~mask;
    uint32_t slot = (uint32_t)__builtin_ctzll(mask);
    if (hz5_midpagefront_m4_transition(page,
                                       slot,
                                       HZ5_MIDPAGE_SLOT_LIVE,
                                       HZ5_MIDPAGE_SLOT_REMOTE)) {
      claimed |= mask;
    }
  }
  return claimed;
}

#if HZ5_MIDPAGEFRONT_M6_ANY_DEFERRED_FREE
static void hz5_midpagefront_m6_promote_owner_batch(
    Hz5MidPageTls* tls,
    Hz5MidPageM6Batch* batch,
    uint32_t count) {
  if (!tls || !batch) {
    return;
  }
  for (uint32_t i = 0; i < count; ++i) {
    Hz5MidPage* page = batch[i].page;
    uint64_t bits = batch[i].bits;
    if (!page || bits == 0 ||
        !hz5_owner_equal(page->owner, tls->owner) ||
        !hz5_midpagefront_m6_transition_owner_local_batch(
            page, bits, HZ5_MIDPAGE_SLOT_LIVE,
            HZ5_MIDPAGE_SLOT_CACHE)) {
      continue;
    }
    while (bits != 0) {
      uint64_t mask = bits & (UINT64_C(0) - bits);
      bits &= ~mask;
      uint32_t slot = (uint32_t)__builtin_ctzll(mask);
      hz5_midpagefront_m4_cache_slot(tls, page->class_index, page, slot);
    }
  }
}
#endif

#if HZ5_MIDPAGEFRONT_M6_ANY_DEFERRED_FREE
static void hz5_midpagefront_m6_flush_raw_entries(Hz5MidPageTls* tls,
                                                  void** raw_free,
                                                  uint16_t* raw_count) {
  if (!tls || !raw_free || !raw_count || *raw_count == 0u) {
    return;
  }

  uint16_t count = *raw_count;
  *raw_count = 0;
  Hz5MidPageM6Batch owner_batch[HZ5_MIDPAGEFRONT_M6_RAW_CAP];
  uint32_t owner_batch_count = 0;
  memset(owner_batch, 0, sizeof(owner_batch));
  for (uint16_t i = 0; i < count; ++i) {
    void* ptr = raw_free[i];
    raw_free[i] = NULL;
    Hz5MidPage* page = hz5_midpagefront_page_for_ptr(ptr);
    if (!page) {
      continue;
    }
    uint32_t slot = 0;
    if (!hz5_midpagefront_slot_index(page, ptr, &slot)) {
      continue;
    }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
    hz5_midpagefront_pagerun_flush_entry(tls, page, slot);
    continue;
#endif
    if (hz5_owner_equal(page->owner, tls->owner)) {
      (void)hz5_midpagefront_m6_batch_add(
          owner_batch,
          &owner_batch_count,
          page,
          hz5_midpagefront_slot_mask(slot));
      continue;
    }

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M11_REMOTE_DIRECT_CACHE
    if (!hz5_midpagefront_m4_transition(page,
                                        slot,
                                        HZ5_MIDPAGE_SLOT_LIVE,
                                        HZ5_MIDPAGE_SLOT_CACHE)) {
      continue;
    }
#else
    if (!hz5_midpagefront_m4_transition(page,
                                        slot,
                                        HZ5_MIDPAGE_SLOT_LIVE,
                                        HZ5_MIDPAGE_SLOT_REMOTE)) {
      continue;
    }
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
    hz5_midpagefront_m4_remote_packet_push(page, slot);
#else
    hz5_midpagefront_remote_batch_push(tls, page, ptr);
#endif
  }
  hz5_midpagefront_m6_promote_owner_batch(
      tls, owner_batch, owner_batch_count);
}
#endif

#if HZ5_MIDPAGEFRONT_M6_ANY_DEFERRED_FREE
static void hz5_midpagefront_m6_defer_free_entries(Hz5MidPageTls* tls,
                                                   void** raw_free,
                                                   uint16_t* raw_count,
                                                   void* ptr) {
  if (!tls || !raw_free || !raw_count || !ptr) {
    return;
  }
  uint16_t count = *raw_count;
  if (count >= HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    hz5_midpagefront_m6_flush_raw_entries(tls, raw_free, raw_count);
    count = *raw_count;
  }
  if (count < HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    raw_free[count] = ptr;
    *raw_count = (uint16_t)(count + 1u);
  }
  if (*raw_count >= HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    hz5_midpagefront_m6_flush_raw_entries(tls, raw_free, raw_count);
  }
}
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
static void hz5_midpagefront_m6_flush_raw(Hz5MidPageTls* tls) {
  hz5_midpagefront_m6_flush_raw_entries(
      tls, tls->m6_raw_free, &tls->m6_raw_count);
}

static void hz5_midpagefront_m6_defer_free(Hz5MidPageTls* tls, void* ptr) {
  hz5_midpagefront_m6_defer_free_entries(
      tls, tls->m6_raw_free, &tls->m6_raw_count, ptr);
}
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE
static void hz5_midpagefront_m6_remote_flush_raw(Hz5MidPageTls* tls) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
  hz5_midpagefront_m6_flush_raw(tls);
#else
  hz5_midpagefront_m6_flush_raw_entries(
      tls,
      g_hz5_midpagefront_m6_remote_tls.raw_free,
      &g_hz5_midpagefront_m6_remote_tls.raw_count);
#endif
}

static void hz5_midpagefront_m6_remote_defer_free(Hz5MidPageTls* tls,
                                                  void* ptr) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
  hz5_midpagefront_m6_defer_free(tls, ptr);
#else
  hz5_midpagefront_m6_defer_free_entries(
      tls,
      g_hz5_midpagefront_m6_remote_tls.raw_free,
      &g_hz5_midpagefront_m6_remote_tls.raw_count,
      ptr);
#endif
}
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING
static void hz5_midpagefront_m10_remote_flush_slots(Hz5MidPageTls* tls) {
  (void)tls;
  Hz5MidPageM10SlotTls* stls = &g_hz5_midpagefront_m10_slot_tls;
  uint16_t count = stls->slot_count;
  stls->slot_count = 0;
  for (uint16_t i = 0; i < count; ++i) {
    Hz5MidPage* page = stls->slot_free[i].page;
    uint32_t slot = stls->slot_free[i].slot;
    stls->slot_free[i].page = NULL;
    stls->slot_free[i].slot = 0;
    if (!page || slot >= page->slot_count ||
        !hz5_owner_is_alive(page->owner)) {
      continue;
    }
    if (!hz5_midpagefront_m4_transition(page,
                                        slot,
                                        HZ5_MIDPAGE_SLOT_LIVE,
                                        HZ5_MIDPAGE_SLOT_REMOTE)) {
      continue;
    }
    hz5_midpagefront_m4_remote_packet_push(page, slot);
  }
}

static Hz5MidPageFrontFreeResult hz5_midpagefront_m10_remote_slot_defer(
    Hz5MidPageTls* tls,
    Hz5MidPage* page,
    uint32_t slot) {
  if (!tls || !page || slot >= page->slot_count || slot >= UINT16_MAX ||
      !hz5_owner_is_alive(page->owner)) {
    return HZ5_MIDPAGEFRONT_FREE_INVALID;
  }
  Hz5MidPageM10SlotTls* stls = &g_hz5_midpagefront_m10_slot_tls;
  uint16_t count = stls->slot_count;
  if (count >= HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    hz5_midpagefront_m10_remote_flush_slots(tls);
    count = stls->slot_count;
  }
  if (count < HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    stls->slot_free[count].page = page;
    stls->slot_free[count].slot = (uint16_t)slot;
    stls->slot_count = (uint16_t)(count + 1u);
  }
  if (stls->slot_count >= HZ5_MIDPAGEFRONT_M6_RAW_CAP) {
    hz5_midpagefront_m10_remote_flush_slots(tls);
  }
  return HZ5_MIDPAGEFRONT_FREE_OK;
}
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET
static void hz5_midpagefront_m7_remote_flush(Hz5MidPageTls* tls) {
  (void)tls;
  Hz5MidPageM7RemoteTls* rtls = &g_hz5_midpagefront_m7_remote_tls;
  uint16_t count = rtls->batch_count;
  rtls->batch_count = 0;
  rtls->ticket_count = 0;
  for (uint16_t i = 0; i < count; ++i) {
    Hz5MidPage* page = rtls->batch[i].page;
    uint64_t bits = rtls->batch[i].bits;
    rtls->batch[i].page = NULL;
    rtls->batch[i].bits = 0;
    if (!page || bits == 0 || !hz5_owner_is_alive(page->owner)) {
      continue;
    }
    bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
    if (bits == 0) {
      continue;
    }
    bits = hz5_midpagefront_m7_claim_remote_bits(page, bits);
    if (bits == 0) {
      continue;
    }
    hz5_midpagefront_m4_remote_packet_push_bits(page, bits);
  }
}

static Hz5MidPageFrontFreeResult hz5_midpagefront_m7_remote_ticket_defer(
    Hz5MidPageTls* tls,
    Hz5MidPage* page,
    uint32_t slot) {
  if (!tls || !page || slot >= page->slot_count || slot >= 64u ||
      !hz5_owner_is_alive(page->owner)) {
    return HZ5_MIDPAGEFRONT_FREE_INVALID;
  }
  Hz5MidPageM7RemoteTls* rtls = &g_hz5_midpagefront_m7_remote_tls;
  uint64_t bit = hz5_midpagefront_slot_mask(slot);
  for (uint16_t i = 0; i < rtls->batch_count; ++i) {
    if (rtls->batch[i].page == page) {
      if (rtls->batch[i].bits & bit) {
        return HZ5_MIDPAGEFRONT_FREE_INVALID;
      }
      rtls->batch[i].bits |= bit;
      ++rtls->ticket_count;
      if (rtls->ticket_count >= HZ5_MIDPAGEFRONT_M6_RAW_CAP ||
          rtls->batch[i].bits ==
              hz5_midpagefront_slot_count_mask(page->slot_count)) {
        hz5_midpagefront_m7_remote_flush(tls);
      }
      return HZ5_MIDPAGEFRONT_FREE_OK;
    }
  }
  if (rtls->batch_count >= HZ5_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP) {
    hz5_midpagefront_m7_remote_flush(tls);
  }
  if (rtls->batch_count >= HZ5_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP) {
    return HZ5_MIDPAGEFRONT_FREE_INVALID;
  }
  uint16_t idx = rtls->batch_count++;
  rtls->batch[idx].page = page;
  rtls->batch[idx].bits = bit;
  ++rtls->ticket_count;
  if (rtls->ticket_count >= HZ5_MIDPAGEFRONT_M6_RAW_CAP ||
      bit == hz5_midpagefront_slot_count_mask(page->slot_count)) {
    hz5_midpagefront_m7_remote_flush(tls);
  }
  return HZ5_MIDPAGEFRONT_FREE_OK;
}
#endif
#endif

