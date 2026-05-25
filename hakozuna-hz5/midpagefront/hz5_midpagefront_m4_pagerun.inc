#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
static void hz5_midpagefront_m4_remote_packet_publish(Hz5OwnerToken owner,
                                                      uint32_t class_index,
                                                      Hz5MidPage* page) {
  if (owner.slot == 0 || !hz5_midpagefront_class_valid(class_index) ||
      !page || !hz5_owner_is_alive(owner)) {
    return;
  }
  void* old_head = NULL;
  _Atomic(void*)* inbox =
      &g_hz5_midpagefront_owner_inbox[owner.slot][class_index];
  do {
    old_head = atomic_load_explicit(inbox, memory_order_acquire);
    page->remote_packet_next = (Hz5MidPage*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      inbox, &old_head, page, memory_order_release, memory_order_acquire));
  hz5_midpagefront_owner_mark_pending(owner, class_index);
}

static void hz5_midpagefront_m4_remote_packet_requeue_if_needed(
    Hz5MidPage* page) {
  if (!page || !hz5_owner_is_alive(page->owner)) {
    return;
  }
  uint64_t bits = atomic_load_explicit(&page->remote_packet_bits,
                                       memory_order_acquire);
  if (bits == 0) {
    return;
  }
  uint8_t expected = 0;
  if (atomic_compare_exchange_strong_explicit(
          &page->remote_packet_queued,
          &expected,
          1,
          memory_order_acq_rel,
          memory_order_acquire)) {
    hz5_midpagefront_m4_remote_packet_publish(page->owner,
                                              page->class_index,
                                              page);
  }
}

static void hz5_midpagefront_m4_remote_packet_push(Hz5MidPage* page,
                                                   uint32_t slot) {
  if (!page || slot >= page->slot_count || slot >= 64u ||
      !hz5_owner_is_alive(page->owner)) {
    return;
  }
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
  atomic_fetch_or_explicit(&page->remote_packet_bits,
                           mask,
                           memory_order_release);
  uint8_t expected = 0;
  if (atomic_compare_exchange_strong_explicit(
          &page->remote_packet_queued,
          &expected,
          1,
          memory_order_acq_rel,
          memory_order_acquire)) {
    hz5_midpagefront_m4_remote_packet_publish(page->owner,
                                              page->class_index,
                                              page);
  }
}

static void hz5_midpagefront_m4_remote_packet_push_bits(Hz5MidPage* page,
                                                        uint64_t bits) {
  if (!page || !hz5_owner_is_alive(page->owner)) {
    return;
  }
  bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
  if (bits == 0) {
    return;
  }
  atomic_fetch_or_explicit(&page->remote_packet_bits,
                           bits,
                           memory_order_release);
  uint8_t expected = 0;
  if (atomic_compare_exchange_strong_explicit(
          &page->remote_packet_queued,
          &expected,
          1,
          memory_order_acq_rel,
          memory_order_acquire)) {
    hz5_midpagefront_m4_remote_packet_publish(page->owner,
                                              page->class_index,
                                              page);
  }
}

static uint32_t hz5_midpagefront_m4_remote_packet_drain_page(
    Hz5MidPageTls* tls,
    uint32_t class_index,
    Hz5MidPage* page) {
  if (!tls || !page || !hz5_owner_equal(page->owner, tls->owner) ||
      page->class_index != class_index) {
    return 0;
  }
  uint64_t bits = atomic_exchange_explicit(&page->remote_packet_bits,
                                           0,
                                           memory_order_acq_rel);
  bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M9_BUDGET_DRAIN
  uint64_t remaining_bits = 0;
  uint32_t budget = 0;
  uint16_t cap = hz5_midpagefront_m4_mag_cap(class_index);
  uint16_t count = tls->magazine_count[class_index];
  if (cap > count) {
    budget = (uint32_t)(cap - count);
  }
  if (budget == 0 && bits != 0) {
    remaining_bits = bits;
    bits = 0;
  } else if (budget != 0) {
    uint64_t keep_bits = 0;
    uint64_t scan_bits = bits;
    while (scan_bits != 0 && budget != 0) {
      uint64_t mask = scan_bits & (UINT64_C(0) - scan_bits);
      scan_bits &= ~mask;
      keep_bits |= mask;
      --budget;
    }
    remaining_bits = bits & ~keep_bits;
    bits = keep_bits;
  }
#endif
  uint32_t drained = 0;
  uint64_t claimed_bits = 0;
  while (bits != 0) {
    uint64_t mask = bits & (UINT64_C(0) - bits);
    bits &= ~mask;
    uint32_t slot = (uint32_t)__builtin_ctzll(mask);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M11_REMOTE_DIRECT_CACHE
    if (hz5_midpagefront_m4_slot_state_is(page,
                                          slot,
                                          HZ5_MIDPAGE_SLOT_CACHE)) {
      claimed_bits |= mask;
    }
#else
    if (hz5_midpagefront_m4_transition(page, slot,
                                       HZ5_MIDPAGE_SLOT_REMOTE,
                                       HZ5_MIDPAGE_SLOT_CACHE)) {
      claimed_bits |= mask;
    }
#endif
  }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER
  if (claimed_bits != 0 &&
      hz5_midpagefront_m8_xfer_add_bits(
          tls, class_index, page, claimed_bits)) {
    drained += (uint32_t)__builtin_popcountll(claimed_bits);
  } else
#endif
  {
    uint64_t cache_bits = claimed_bits;
    while (cache_bits != 0) {
      uint64_t mask = cache_bits & (UINT64_C(0) - cache_bits);
      cache_bits &= ~mask;
      uint32_t slot = (uint32_t)__builtin_ctzll(mask);
      hz5_midpagefront_m4_cache_slot(tls, class_index, page, slot);
      ++drained;
    }
  }
  hz5_midpagefront_m4_try_release_empty_page(tls, page);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M9_BUDGET_DRAIN
  if (remaining_bits != 0) {
    atomic_fetch_or_explicit(&page->remote_packet_bits,
                             remaining_bits,
                             memory_order_release);
  }
#endif
  atomic_store_explicit(&page->remote_packet_queued,
                        0,
                        memory_order_release);
  hz5_midpagefront_m4_remote_packet_requeue_if_needed(page);
  return drained;
}

static void hz5_midpagefront_m4_remote_packet_drain_if_pending(
    Hz5MidPageTls* tls,
    uint32_t class_index) {
  if (!tls || tls->owner.slot == 0 ||
      !hz5_midpagefront_class_valid(class_index)) {
    return;
  }
  void* head = atomic_load_explicit(
      &g_hz5_midpagefront_owner_inbox[tls->owner.slot][class_index],
      memory_order_acquire);
  if (head) {
    (void)hz5_midpagefront_drain_remote_class(tls, class_index);
  }
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
static void hz5_midpagefront_pagerun_partial_push_once(
    Hz5MidPageTls* tls,
    uint32_t class_index,
    Hz5MidPage* page) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index) ||
      page->pagerun_owner_free_bits == 0 || page->source_free ||
      page->pagerun_in_partial) {
    return;
  }
  page->pagerun_in_partial = 1;
  page->pagerun_next_partial = tls->pagerun_partial[class_index];
  tls->pagerun_partial[class_index] = page;
}

static Hz5MidPage* hz5_midpagefront_pagerun_partial_pop(
    Hz5MidPageTls* tls,
    uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index)) {
    return NULL;
  }
  while (tls->pagerun_partial[class_index]) {
    Hz5MidPage* page = tls->pagerun_partial[class_index];
    tls->pagerun_partial[class_index] = page->pagerun_next_partial;
    page->pagerun_next_partial = NULL;
    page->pagerun_in_partial = 0;
    if (page->magic == HZ5_MIDPAGEFRONT_MAGIC &&
        page->class_index == class_index &&
        hz5_owner_equal(page->owner, tls->owner) &&
        page->pagerun_owner_free_bits != 0) {
      return page;
    }
  }
  return NULL;
}

static int hz5_midpagefront_pagerun_live_to_cache(Hz5MidPage* page,
                                                  uint64_t mask) {
  if (!page || mask == 0) {
    return 0;
  }
  uint64_t old = atomic_fetch_and_explicit(&page->active_bits,
                                           ~mask,
                                           memory_order_acq_rel);
  if ((old & mask) == 0) {
    return 0;
  }
  uint32_t n = (uint32_t)__builtin_popcountll(mask);
  if (n != 0) {
    atomic_fetch_sub_explicit(&page->live_count,
                              n,
                              memory_order_acq_rel);
  }
  return 1;
}

static int hz5_midpagefront_pagerun_cache_to_live(Hz5MidPage* page,
                                                  uint64_t mask) {
  if (!page || mask == 0) {
    return 0;
  }
  uint64_t old = atomic_fetch_or_explicit(&page->active_bits,
                                          mask,
                                          memory_order_acq_rel);
  if ((old & mask) != 0) {
    return 0;
  }
  uint32_t n = (uint32_t)__builtin_popcountll(mask);
  if (n != 0) {
    atomic_fetch_add_explicit(&page->live_count,
                              n,
                              memory_order_acq_rel);
  }
  return 1;
}

static void hz5_midpagefront_pagerun_requeue_if_needed(Hz5MidPage* page) {
  if (!page || !hz5_owner_is_alive(page->owner)) {
    return;
  }
  uint64_t bits = atomic_load_explicit(&page->remote_bits,
                                       memory_order_acquire);
  if (bits == 0) {
    return;
  }
  uint8_t expected = 0;
  if (atomic_compare_exchange_strong_explicit(
          &page->remote_packet_queued,
          &expected,
          1,
          memory_order_acq_rel,
          memory_order_acquire)) {
    hz5_midpagefront_m4_remote_packet_publish(page->owner,
                                              page->class_index,
                                              page);
  }
}

static void hz5_midpagefront_pagerun_remote_publish(Hz5MidPage* page) {
  if (!page || !hz5_owner_is_alive(page->owner)) {
    return;
  }
  uint8_t expected = 0;
  if (atomic_compare_exchange_strong_explicit(
          &page->remote_packet_queued,
          &expected,
          1,
          memory_order_acq_rel,
          memory_order_acquire)) {
    hz5_midpagefront_m4_remote_packet_publish(page->owner,
                                              page->class_index,
                                              page);
  }
}

static void hz5_midpagefront_pagerun_accept_cache_bits(
    Hz5MidPageTls* tls,
    uint32_t class_index,
    Hz5MidPage* page,
    uint64_t bits) {
  if (!tls || !page || bits == 0 ||
      !hz5_midpagefront_class_valid(class_index)) {
    return;
  }
  page->pagerun_owner_free_bits |=
      bits & hz5_midpagefront_slot_count_mask(page->slot_count);
  if (tls->pagerun_current[class_index] != page) {
    hz5_midpagefront_pagerun_partial_push_once(tls, class_index, page);
  }
}

static uint32_t hz5_midpagefront_pagerun_remote_drain_page(
    Hz5MidPageTls* tls,
    uint32_t class_index,
    Hz5MidPage* page) {
  if (!tls || !page || !hz5_owner_equal(page->owner, tls->owner) ||
      page->class_index != class_index) {
    return 0;
  }
  uint64_t bits = atomic_exchange_explicit(&page->remote_bits,
                                           0,
                                           memory_order_acq_rel);
  bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
  if (bits != 0) {
    hz5_midpagefront_pagerun_accept_cache_bits(
        tls, class_index, page, bits);
  }
  atomic_store_explicit(&page->remote_packet_queued,
                        0,
                        memory_order_release);
  hz5_midpagefront_pagerun_requeue_if_needed(page);
  hz5_midpagefront_m4_try_release_empty_page(tls, page);
  return (uint32_t)__builtin_popcountll(bits);
}

static Hz5MidPageFrontFreeResult hz5_midpagefront_pagerun_free_local(
    Hz5MidPageTls* tls,
    Hz5MidPage* page,
    uint32_t slot) {
  if (!tls || !page || slot >= page->slot_count || slot >= 64u) {
    return HZ5_MIDPAGEFRONT_FREE_INVALID;
  }
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
  if (!hz5_midpagefront_pagerun_live_to_cache(page, mask)) {
    return HZ5_MIDPAGEFRONT_FREE_INVALID;
  }
  hz5_midpagefront_pagerun_accept_cache_bits(
      tls, page->class_index, page, mask);
  hz5_midpagefront_m4_try_release_empty_page(tls, page);
  return HZ5_MIDPAGEFRONT_FREE_OK;
}

static void hz5_midpagefront_pagerun_flush_entry(Hz5MidPageTls* tls,
                                                 Hz5MidPage* page,
                                                 uint32_t slot) {
  if (!tls || !page || slot >= page->slot_count || slot >= 64u) {
    return;
  }
  uint64_t mask = hz5_midpagefront_slot_mask(slot);
  if (!hz5_midpagefront_pagerun_live_to_cache(page, mask)) {
    return;
  }
  if (hz5_owner_equal(page->owner, tls->owner)) {
    hz5_midpagefront_pagerun_accept_cache_bits(
        tls, page->class_index, page, mask);
    hz5_midpagefront_m4_try_release_empty_page(tls, page);
    return;
  }
  atomic_fetch_or_explicit(&page->remote_bits,
                           mask,
                           memory_order_release);
  hz5_midpagefront_pagerun_remote_publish(page);
}
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_ON_HIT
static int hz5_midpagefront_m4_should_drain_on_hit(Hz5MidPageTls* tls,
                                                   uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index) ||
      HZ5_MIDPAGEFRONT_REMOTE_DRAIN_HIT_INTERVAL == 0u) {
    return 0;
  }
  uint16_t count = (uint16_t)(tls->remote_drain_hit_count[class_index] + 1u);
  if (count < (uint16_t)HZ5_MIDPAGEFRONT_REMOTE_DRAIN_HIT_INTERVAL) {
    tls->remote_drain_hit_count[class_index] = count;
    return 0;
  }
  tls->remote_drain_hit_count[class_index] = 0;
  return 1;
}
#endif
#endif
