#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
static void hz5_midpagefront_m4_purge_magazine_page(Hz5MidPageTls* tls,
                                                    uint32_t class_index,
                                                    Hz5MidPage* page) {
  uint16_t out = 0;
  uint16_t count = tls->magazine_count[class_index];
  for (uint16_t i = 0; i < count; ++i) {
    if (tls->magazine[class_index][i].page != page) {
      if (out != i) {
        tls->magazine[class_index][out] = tls->magazine[class_index][i];
      }
      ++out;
    }
  }
  tls->magazine_count[class_index] = out;

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  out = 0;
  count = tls->overflow_count[class_index];
  for (uint16_t i = 0; i < count; ++i) {
    if (tls->overflow[class_index][i].page != page) {
      if (out != i) {
        tls->overflow[class_index][out] = tls->overflow[class_index][i];
      }
      ++out;
    }
  }
  tls->overflow_count[class_index] = out;
#endif
}

static void hz5_midpagefront_purge_free_head_page(Hz5MidPageTls* tls,
                                                  uint32_t class_index,
                                                  Hz5MidPage* page) {
  Hz5MidPageNode** link = &tls->free_head[class_index];
  while (*link) {
    Hz5MidPageNode* node = *link;
    if (node->page == page) {
      *link = node->next;
      continue;
    }
    link = &node->next;
  }
}

static void hz5_midpagefront_remove_owned_page(Hz5MidPageTls* tls,
                                               uint32_t class_index,
                                               Hz5MidPage* page) {
  Hz5MidPage** link = &tls->owned_pages[class_index];
  while (*link) {
    if (*link == page) {
      *link = page->next_owned;
      page->next_owned = NULL;
      return;
    }
    link = &(*link)->next_owned;
  }
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
static void hz5_midpagefront_pagerun_purge_page(Hz5MidPageTls* tls,
                                                uint32_t class_index,
                                                Hz5MidPage* page) {
  if (!tls || !page || !hz5_midpagefront_class_valid(class_index)) {
    return;
  }
  if (tls->pagerun_current[class_index] == page) {
    tls->pagerun_current[class_index] = NULL;
  }
  Hz5MidPage** link = &tls->pagerun_partial[class_index];
  while (*link) {
    Hz5MidPage* cur = *link;
    if (cur == page) {
      *link = cur->pagerun_next_partial;
      cur->pagerun_next_partial = NULL;
      cur->pagerun_in_partial = 0;
      continue;
    }
    link = &cur->pagerun_next_partial;
  }
}
#endif

static int hz5_midpagefront_m4_page_is_empty_releasable(
    Hz5MidPageTls* tls,
    Hz5MidPage* page) {
  if (!tls || !page || page->source_free ||
      page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
      !hz5_owner_equal(page->owner, tls->owner) ||
      page->class_index >= HZ5_MIDPAGEFRONT_CLASS_COUNT) {
    return 0;
  }
  if (atomic_load_explicit(&page->live_count, memory_order_acquire) != 0u) {
    return 0;
  }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
  if (atomic_load_explicit(&page->remote_packet_bits, memory_order_acquire) !=
      0) {
    return 0;
  }
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
  if (atomic_load_explicit(&page->remote_bits, memory_order_acquire) != 0) {
    return 0;
  }
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
  uint64_t all_free = hz5_midpagefront_slot_count_mask(page->slot_count);
  uint64_t live = atomic_load_explicit(&page->active_bits,
                                       memory_order_acquire);
  return live == 0 && page->pagerun_owner_free_bits == all_free;
#else
  uint64_t field_mask = hz5_midpagefront_m4_state_field_mask(page->slot_count);
  uint64_t cache_state = hz5_midpagefront_m4_initial_state(page->slot_count);
  uint64_t state = atomic_load_explicit(&page->slot_state2,
                                        memory_order_acquire);
#if !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_ALLOC_ELIDE && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_FREE_ELIDE
  return (state & field_mask) == cache_state;
#else
  (void)field_mask;
  (void)cache_state;
  (void)state;
  return 1;
#endif
#endif
}

static void hz5_midpagefront_source_free_slab(uint32_t class_index,
                                              Hz5MidPage* page) {
  if (!page || !hz5_midpagefront_class_valid(class_index)) {
    return;
  }
  (void)madvise(page->slab_base,
                HZ5_MIDPAGEFRONT_SLAB_BYTES,
                MADV_DONTNEED);
  pthread_mutex_lock(&g_hz5_midpagefront_region_lock);
  page->source_free = 1;
  page->empty_retained = 0;
  page->retired_empty = 0;
  page->retired_next = NULL;
  page->source_next = g_hz5_midpagefront_source_free[class_index];
  g_hz5_midpagefront_source_free[class_index] = page;
  pthread_mutex_unlock(&g_hz5_midpagefront_region_lock);
}

static size_t hz5_midpagefront_m4_release_retired_one(Hz5MidPageTls* tls,
                                                      uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index)) {
    return 0;
  }
  Hz5MidPage* page = tls->retired_empty[class_index];
  if (!page) {
    return 0;
  }
  tls->retired_empty[class_index] = page->retired_next;
  page->retired_next = NULL;
  page->retired_empty = 0;
  if (tls->retired_empty_count[class_index] != 0u) {
    --tls->retired_empty_count[class_index];
  }
  page->owner = k_hz5_midpagefront_no_owner;
  hz5_midpagefront_source_free_slab(class_index, page);
  return 1;
}

static size_t hz5_midpagefront_m4_release_retired_all(Hz5MidPageTls* tls) {
  size_t released = 0;
  if (!tls) {
    return 0;
  }
  for (uint32_t ci = 0; ci < HZ5_MIDPAGEFRONT_CLASS_COUNT; ++ci) {
    while (tls->retired_empty[ci]) {
      released += hz5_midpagefront_m4_release_retired_one(tls, ci);
    }
  }
  return released;
}

static size_t hz5_midpagefront_m4_release_empty_owned_all(Hz5MidPageTls* tls) {
  size_t released = 0;
  if (!tls) {
    return 0;
  }
  for (uint32_t ci = 0; ci < HZ5_MIDPAGEFRONT_CLASS_COUNT; ++ci) {
    Hz5MidPage* page = tls->owned_pages[ci];
    while (page) {
      Hz5MidPage* next = page->next_owned;
      if (hz5_midpagefront_m4_page_is_empty_releasable(tls, page)) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER
        hz5_midpagefront_m8_purge_xfer_page(tls, ci, page);
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
        hz5_midpagefront_pagerun_purge_page(tls, ci, page);
#endif
        hz5_midpagefront_m4_purge_magazine_page(tls, ci, page);
        hz5_midpagefront_purge_free_head_page(tls, ci, page);
        hz5_midpagefront_remove_owned_page(tls, ci, page);
        if (page->empty_retained &&
            tls->empty_retained_count[ci] != 0u) {
          --tls->empty_retained_count[ci];
        }
        page->empty_retained = 0;
        if (page->retired_empty &&
            tls->retired_empty_count[ci] != 0u) {
          --tls->retired_empty_count[ci];
        }
        page->retired_empty = 0;
        page->retired_next = NULL;
        page->owner = k_hz5_midpagefront_no_owner;
        hz5_midpagefront_source_free_slab(ci, page);
        ++released;
      }
      page = next;
    }
  }
  return released;
}

static size_t hz5_midpagefront_m4_release_checkpoint(Hz5MidPageTls* tls) {
  size_t released = 0;
  released += hz5_midpagefront_m4_release_empty_owned_all(tls);
  released += hz5_midpagefront_m4_release_retired_all(tls);
  return released;
}

static void hz5_midpagefront_m4_try_release_empty_page(Hz5MidPageTls* tls,
                                                       Hz5MidPage* page) {
  if (!hz5_midpagefront_m4_page_is_empty_releasable(tls, page)) {
    return;
  }

  uint32_t class_index = page->class_index;
  if (!page->empty_retained) {
    page->empty_retained = 1;
    ++tls->empty_retained_count[class_index];
  }
  if (tls->empty_retained_count[class_index] <=
      HZ5_MIDPAGEFRONT_EMPTY_RETAIN_CAP) {
    return;
  }

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER
  hz5_midpagefront_m8_purge_xfer_page(tls, class_index, page);
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
  hz5_midpagefront_pagerun_purge_page(tls, class_index, page);
#endif
  hz5_midpagefront_m4_purge_magazine_page(tls, class_index, page);
  hz5_midpagefront_purge_free_head_page(tls, class_index, page);
  hz5_midpagefront_remove_owned_page(tls, class_index, page);
  if (page->empty_retained &&
      tls->empty_retained_count[class_index] != 0u) {
    --tls->empty_retained_count[class_index];
  }
  page->empty_retained = 0;
  if (!page->retired_empty) {
    page->retired_empty = 1;
    page->retired_next = tls->retired_empty[class_index];
    tls->retired_empty[class_index] = page;
    ++tls->retired_empty_count[class_index];
  }
}
#else
static void hz5_midpagefront_m4_try_release_empty_page(Hz5MidPageTls* tls,
                                                       Hz5MidPage* page) {
  (void)tls;
  (void)page;
}
#endif
