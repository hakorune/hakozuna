#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
static void hz5_midpagefront_nodeless_partial_push(Hz5MidPageTls* tls,
                                                   uint32_t class_index,
                                                   Hz5MidPage* page) {
  if (!tls || !page || class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT ||
      page->in_partial) {
    return;
  }
  if (tls->current_page[class_index] == page) {
    return;
  }
  hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_partial_push);
  page->in_partial = 1;
  page->next_partial = tls->partial_pages[class_index];
  tls->partial_pages[class_index] = page;
}

static Hz5MidPage* hz5_midpagefront_nodeless_partial_pop(Hz5MidPageTls* tls,
                                                         uint32_t class_index,
                                                         uint64_t* bits_out) {
  if (bits_out) {
    *bits_out = 0;
  }
  if (!tls || class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return NULL;
  }
  while (tls->partial_pages[class_index]) {
    Hz5MidPage* page = tls->partial_pages[class_index];
    tls->partial_pages[class_index] = page->next_partial;
    page->next_partial = NULL;
    page->in_partial = 0;
    uint64_t bits = atomic_load_explicit(&page->free_bits,
                                         memory_order_acquire);
    bits &= hz5_midpagefront_slot_count_mask(page->slot_count);
    if (bits != 0) {
      if (bits_out) {
        *bits_out = bits;
      }
      return page;
    }
  }
  return NULL;
}

static void hz5_midpagefront_nodeless_publish_free_bit(Hz5MidPageTls* tls,
                                                       Hz5MidPage* page,
                                                       uint64_t mask) {
  if (!tls || !page || mask == 0) {
    return;
  }
  uint32_t class_index = page->class_index;
  if (class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return;
  }
  uint64_t bits = atomic_load_explicit(&page->free_bits,
                                       memory_order_relaxed);
  atomic_store_explicit(&page->free_bits, bits | mask,
                        memory_order_release);
  if (tls->current_page[class_index] == page) {
    tls->current_bits[class_index] |= mask;
    return;
  }
  hz5_midpagefront_nodeless_partial_push(tls, class_index, page);
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE
static int hz5_midpagefront_nodeless_ptrcache_push(Hz5MidPageTls* tls,
                                                   uint32_t class_index,
                                                   void* ptr,
                                                   Hz5MidPage* page,
                                                   uint64_t mask) {
  if (!tls || !ptr || !page || mask == 0 ||
      class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return 0;
  }
  uint32_t count = tls->ptrcache_count[class_index];
  if (count >= HZ5_MIDPAGEFRONT_NODELESS_PTRCACHE_CAP) {
    hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_ptrcache_full);
    return 0;
  }
  Hz5MidPagePtrCacheEntry* entry = &tls->ptrcache[class_index][count];
  entry->ptr = ptr;
  entry->page = page;
  entry->mask = mask;
  tls->ptrcache_count[class_index] = count + 1u;
  hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_ptrcache_push);
  return 1;
}

static void* hz5_midpagefront_nodeless_ptrcache_pop(Hz5MidPageTls* tls,
                                                    uint32_t class_index) {
  if (!tls || class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return NULL;
  }
  while (tls->ptrcache_count[class_index] != 0u) {
    uint32_t count = tls->ptrcache_count[class_index] - 1u;
    tls->ptrcache_count[class_index] = count;
    Hz5MidPagePtrCacheEntry entry = tls->ptrcache[class_index][count];
    if (!entry.ptr || !entry.page ||
        entry.page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
        entry.page->class_index != class_index ||
        entry.mask == 0 ||
        (entry.mask & (entry.mask - UINT64_C(1))) != 0) {
      continue;
    }
    uint32_t slot = (uint32_t)__builtin_ctzll(entry.mask);
    if (slot >= entry.page->slot_count ||
        !hz5_midpagefront_mark_active_local(entry.page, slot)) {
      continue;
    }
    hz5_midpagefront_stat_inc(&g_hz5_midpagefront_stat_ptrcache_hit);
    return entry.ptr;
  }
  return NULL;
}
#endif
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT
static int hz5_midpagefront_hot_push(Hz5MidPageTls* tls,
                                     uint32_t class_index,
                                     void* ptr,
                                     Hz5MidPage* page) {
  if (!tls || !ptr || !page ||
      class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT ||
      tls->hot_ptr[class_index]) {
    return 0;
  }
  tls->hot_page[class_index] = page;
  tls->hot_ptr[class_index] = ptr;
  return 1;
}

static void* hz5_midpagefront_hot_pop(Hz5MidPageTls* tls,
                                      uint32_t class_index,
                                      Hz5MidPage** page_out) {
  if (!tls || class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    if (page_out) {
      *page_out = NULL;
    }
    return NULL;
  }
  void* ptr = tls->hot_ptr[class_index];
  if (!ptr) {
    if (page_out) {
      *page_out = NULL;
    }
    return NULL;
  }
  tls->hot_ptr[class_index] = NULL;
  if (page_out) {
    *page_out = tls->hot_page[class_index];
  }
  tls->hot_page[class_index] = NULL;
  return ptr;
}
#endif
