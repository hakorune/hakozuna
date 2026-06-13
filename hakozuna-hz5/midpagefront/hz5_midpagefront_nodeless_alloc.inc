#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
static int hz5_midpagefront_nodeless_refill_current(Hz5MidPageTls* tls,
                                                    uint32_t ci) {
  if (!tls || ci >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return 0;
  }

  hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_refill);
  uint64_t bits = 0;
  Hz5MidPage* page = hz5_midpagefront_nodeless_partial_pop(tls, ci, &bits);
  if (page) {
    hz5_midpagefront_stat_inc(
        &g_hz5_midpagefront_stat_refill_partial_hit);
  }
  if (!page) {
    (void)hz5_midpagefront_drain_remote_class(tls, ci);
    page = hz5_midpagefront_nodeless_partial_pop(tls, ci, &bits);
    if (page) {
      hz5_midpagefront_stat_inc(
          &g_hz5_midpagefront_stat_refill_remote_hit);
    }
  }
  if (!page) {
    page = hz5_midpagefront_new_page(tls, ci);
    if (!page) {
      return 0;
    }
    hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_refill_new_page);
    bits = atomic_load_explicit(&page->free_bits, memory_order_acquire);
    bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
  }
  if (bits == 0) {
    return 0;
  }
  tls->current_page[ci] = page;
  tls->current_bits[ci] = bits;
  return 1;
}

static void* hz5_midpagefront_nodeless_alloc_class(uint32_t ci) {
  if (!hz5_midpagefront_class_valid(ci)) {
    return NULL;
  }
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE
  void* cached = hz5_midpagefront_nodeless_ptrcache_pop(tls, ci);
  if (cached) {
    return cached;
  }
#endif
  if (tls->current_bits[ci] == 0 &&
      !hz5_midpagefront_nodeless_refill_current(tls, ci)) {
    return NULL;
  }

  Hz5MidPage* page = tls->current_page[ci];
  uint64_t bits = tls->current_bits[ci];
  if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC || bits == 0) {
    tls->current_bits[ci] = 0;
    return NULL;
  }

  uint64_t mask = bits & (UINT64_C(0) - bits);
  uint32_t slot = (uint32_t)__builtin_ctzll(mask);
  tls->current_bits[ci] = bits & ~mask;
  if (!hz5_midpagefront_mark_active_local(page, slot)) {
    return NULL;
  }
  return (void*)((uintptr_t)page->slab_base +
                 (uintptr_t)slot * (uintptr_t)page->class_size);
}
#endif
