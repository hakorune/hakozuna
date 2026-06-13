static uint32_t hz5_midpagefront_drain_remote_class(Hz5MidPageTls* tls,
                                                    uint32_t class_index) {
  if (!tls || tls->owner.slot == 0 ||
      class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return 0;
  }
  hz5_midpagefront_owner_clear_pending(tls->owner, class_index);
  void* head = atomic_exchange_explicit(
      &g_hz5_midpagefront_owner_inbox[tls->owner.slot][class_index],
      NULL,
      memory_order_acq_rel);
  uint32_t drained = 0;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
  while (head) {
    Hz5MidPage* page = (Hz5MidPage*)head;
    head = page->remote_packet_next;
    page->remote_packet_next = NULL;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
    drained += hz5_midpagefront_pagerun_remote_drain_page(
        tls, class_index, page);
#else
    drained += hz5_midpagefront_m4_remote_packet_drain_page(
        tls, class_index, page);
#endif
  }
  return drained;
#else
  while (head) {
    Hz5MidPageNode* node = (Hz5MidPageNode*)head;
    head = node->next;
    if (node->page && hz5_owner_equal(node->page->owner, tls->owner)) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
      uint32_t slot = 0;
      if (hz5_midpagefront_slot_index(node->page, node, &slot) &&
          hz5_midpagefront_m4_transition(node->page, slot,
                                         HZ5_MIDPAGE_SLOT_REMOTE,
                                         HZ5_MIDPAGE_SLOT_CACHE)) {
        hz5_midpagefront_m4_cache_slot(tls, class_index, node->page, slot);
        ++drained;
      }
#else
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
      uint32_t slot = 0;
      if (hz5_midpagefront_slot_index(node->page, node, &slot)) {
        uint64_t mask = hz5_midpagefront_slot_mask(slot);
        uint64_t remote = atomic_load_explicit(&node->page->remote_bits,
                                               memory_order_acquire);
        if ((remote & mask) != 0) {
          for (;;) {
            uint64_t next_remote = remote & ~mask;
            if (atomic_compare_exchange_weak_explicit(
                    &node->page->remote_bits,
                    &remote,
                    next_remote,
                    memory_order_acq_rel,
                    memory_order_acquire)) {
              hz5_midpagefront_nodeless_publish_free_bit(tls, node->page,
                                                         mask);
              hz5_midpagefront_stat_inc(
                  &g_hz5_midpagefront_stat_remote_drained);
              ++drained;
              break;
            }
            if ((remote & mask) == 0) {
              break;
            }
          }
        }
      }
#else
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
      uint32_t slot = 0;
      if (hz5_midpagefront_slot_index(node->page, node, &slot)) {
        uint64_t mask = hz5_midpagefront_slot_mask(slot);
        uint64_t remote = atomic_load_explicit(&node->page->remote_bits,
                                               memory_order_acquire);
        uint64_t active = atomic_load_explicit(&node->page->active_bits,
                                               memory_order_acquire);
        if ((remote & mask) != 0 && (active & mask) != 0) {
          for (;;) {
            if ((remote & mask) == 0) {
              break;
            }
            uint64_t next_remote = remote & ~mask;
            if (atomic_compare_exchange_weak_explicit(
                    &node->page->remote_bits,
                    &remote,
                    next_remote,
                    memory_order_acq_rel,
                    memory_order_acquire)) {
              break;
            }
          }
          atomic_store_explicit(&node->page->active_bits,
                                active & ~mask,
                                memory_order_release);
          hz5_midpagefront_local_push(tls, class_index, node, node->page);
          ++drained;
        }
      }
#else
      hz5_midpagefront_local_push(tls, class_index, node, node->page);
      ++drained;
#endif
#endif
#endif
    }
  }
  return drained;
#endif
}
