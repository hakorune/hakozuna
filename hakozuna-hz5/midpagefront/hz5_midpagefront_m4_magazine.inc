static int hz5_midpagefront_m4_magazine_push(Hz5MidPageTls* tls,
                                             uint32_t class_index,
                                             Hz5MidPage* page,
                                             uint32_t slot) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index) ||
      slot >= page->slot_count) {
    return 0;
  }
  uint16_t count = tls->magazine_count[class_index];
  uint16_t cap = hz5_midpagefront_m4_mag_cap(class_index);
  if (count >= cap || count >= HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX) {
    return 0;
  }
  hz5_midpagefront_m4_stat_inc_class(
      g_hz5_midpagefront_m4_stat_mag_push, class_index);
  tls->magazine[class_index][count].page = page;
  tls->magazine[class_index][count].slot = (uint16_t)slot;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG || \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY || \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  tls->magazine[class_index][count].ptr =
      (void*)((uintptr_t)page->slab_base +
              (uintptr_t)slot * (uintptr_t)page->class_size);
#endif
  tls->magazine_count[class_index] = (uint16_t)(count + 1u);
  return 1;
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
static int hz5_midpagefront_m4_overflow_push(Hz5MidPageTls* tls,
                                             uint32_t class_index,
                                             Hz5MidPage* page,
                                             uint32_t slot) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index) ||
      slot >= page->slot_count) {
    return 0;
  }
  uint16_t count = tls->overflow_count[class_index];
  if (count >= HZ5_MIDPAGEFRONT_M4_OVERFLOW_CAP_MAX) {
    return 0;
  }
  tls->overflow[class_index][count].page = page;
  tls->overflow[class_index][count].slot = (uint16_t)slot;
  tls->overflow[class_index][count].ptr =
      (void*)((uintptr_t)page->slab_base +
              (uintptr_t)slot * (uintptr_t)page->class_size);
  tls->overflow_count[class_index] = (uint16_t)(count + 1u);
  return 1;
}

static int hz5_midpagefront_m4_refill_from_overflow(Hz5MidPageTls* tls,
                                                    uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index)) {
    return 0;
  }
  uint16_t cap = hz5_midpagefront_m4_mag_cap(class_index);
  while (tls->magazine_count[class_index] < cap &&
         tls->magazine_count[class_index] <
             HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX &&
         tls->overflow_count[class_index] != 0u) {
    uint16_t next = (uint16_t)(tls->overflow_count[class_index] - 1u);
    Hz5MidPageMagEntry entry = tls->overflow[class_index][next];
    tls->overflow_count[class_index] = next;
    if (!entry.page || entry.slot >= entry.page->slot_count ||
        entry.page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
        entry.page->class_index != class_index) {
      continue;
    }
    if (!hz5_midpagefront_m4_magazine_push(tls,
                                           class_index,
                                           entry.page,
                                           entry.slot)) {
      (void)hz5_midpagefront_m4_overflow_push(tls,
                                              class_index,
                                              entry.page,
                                              entry.slot);
      break;
    }
  }
  return tls->magazine_count[class_index] != 0u;
}
#endif

static void hz5_midpagefront_m4_cache_slot(Hz5MidPageTls* tls,
                                           uint32_t class_index,
                                           Hz5MidPage* page,
                                           uint32_t slot) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index) ||
      slot >= page->slot_count) {
    return;
  }
  hz5_midpagefront_m4_stat_inc_class(
      g_hz5_midpagefront_m4_stat_cache_slot, class_index);
  if (hz5_midpagefront_m4_magazine_push(tls, class_index, page, slot)) {
    return;
  }
  hz5_midpagefront_m4_stat_inc_class(
      g_hz5_midpagefront_m4_stat_mag_full, class_index);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  if (hz5_midpagefront_m4_overflow_push(tls, class_index, page, slot)) {
    return;
  }
#endif
  void* ptr = (void*)((uintptr_t)page->slab_base +
                      (uintptr_t)slot * (uintptr_t)page->class_size);
  hz5_midpagefront_local_push(tls, class_index, ptr, page);
}
