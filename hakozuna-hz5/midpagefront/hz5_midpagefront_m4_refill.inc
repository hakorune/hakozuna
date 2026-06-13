#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
static int hz5_midpagefront_m4_refill_magazine(Hz5MidPageTls* tls,
                                               uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index)) {
    return 0;
  }
  hz5_midpagefront_m4_stat_inc_class(
      g_hz5_midpagefront_m4_stat_refill_call, class_index);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_RELEASE_ON_REFILL
  (void)hz5_midpagefront_m4_release_retired_one(tls, class_index);
#endif
#endif
  if (tls->magazine_count[class_index] != 0u) {
    return 1;
  }

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER
  if (hz5_midpagefront_m8_xfer_has_class(class_index) &&
      hz5_midpagefront_m8_refill_from_xfer(tls, class_index)) {
    return 1;
  }
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
  hz5_midpagefront_m6_flush_raw(tls);
  if (tls->magazine_count[class_index] != 0u) {
    return 1;
  }
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING
  hz5_midpagefront_m10_remote_flush_slots(tls);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_FLUSH_ON_REFILL
  hz5_midpagefront_m6_remote_flush_raw(tls);
#endif

  (void)hz5_midpagefront_drain_remote_class(tls, class_index);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER
  if (hz5_midpagefront_m8_xfer_has_class(class_index) &&
      hz5_midpagefront_m8_refill_from_xfer(tls, class_index)) {
    hz5_midpagefront_m4_stat_inc_class(
        g_hz5_midpagefront_m4_stat_refill_remote_hit, class_index);
    return 1;
  }
#endif
  if (tls->magazine_count[class_index] != 0u) {
    hz5_midpagefront_m4_stat_inc_class(
        g_hz5_midpagefront_m4_stat_refill_remote_hit, class_index);
    return 1;
  }

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  if (hz5_midpagefront_m4_refill_from_overflow(tls, class_index)) {
    return 1;
  }
#endif

  while (tls->magazine_count[class_index] <
         hz5_midpagefront_m4_mag_cap(class_index)) {
    Hz5MidPage* page = NULL;
    void* ptr = hz5_midpagefront_local_pop(tls, class_index, &page);
    if (!ptr) {
      break;
    }
    hz5_midpagefront_m4_stat_inc_class(
        g_hz5_midpagefront_m4_stat_refill_local_pop, class_index);
    uint32_t slot = 0;
    if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
        page->class_index != class_index ||
        !hz5_midpagefront_slot_index(page, ptr, &slot)) {
      continue;
    }
    hz5_midpagefront_m4_magazine_push(tls, class_index, page, slot);
  }
  if (tls->magazine_count[class_index] != 0u) {
    return 1;
  }

  Hz5MidPage* page = hz5_midpagefront_new_page(tls, class_index);
  if (page) {
    hz5_midpagefront_m4_stat_inc_class(
        g_hz5_midpagefront_m4_stat_refill_new_page, class_index);
  }
  return page && tls->magazine_count[class_index] != 0u;
}
#endif
