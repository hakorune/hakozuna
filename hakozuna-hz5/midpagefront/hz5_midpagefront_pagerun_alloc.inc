#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
static Hz5MidPage* hz5_midpagefront_pagerun_refill_current(
    Hz5MidPageTls* tls,
    uint32_t class_index) {
  if (!tls || !hz5_midpagefront_class_valid(class_index)) {
    return NULL;
  }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_RELEASE_ON_REFILL
  (void)hz5_midpagefront_m4_release_retired_one(tls, class_index);
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_FLUSH_ON_REFILL
  hz5_midpagefront_m6_remote_flush_raw(tls);
#endif

  (void)hz5_midpagefront_drain_remote_class(tls, class_index);
  Hz5MidPage* current = tls->pagerun_current[class_index];
  if (current && current->magic == HZ5_MIDPAGEFRONT_MAGIC &&
      hz5_owner_equal(current->owner, tls->owner) &&
      current->pagerun_owner_free_bits != 0) {
    return current;
  }
  Hz5MidPage* page =
      hz5_midpagefront_pagerun_partial_pop(tls, class_index);
  if (!page) {
    page = hz5_midpagefront_new_page(tls, class_index);
  }
  if (!page || page->pagerun_owner_free_bits == 0) {
    return NULL;
  }
  tls->pagerun_current[class_index] = page;
  return page;
}

static void* hz5_midpagefront_pagerun_alloc_class(uint32_t class_index) {
  if (!hz5_midpagefront_class_valid(class_index)) {
    return NULL;
  }
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  Hz5MidPage* page = tls->pagerun_current[class_index];
  if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
      !hz5_owner_equal(page->owner, tls->owner) ||
      page->pagerun_owner_free_bits == 0) {
    page = hz5_midpagefront_pagerun_refill_current(tls, class_index);
    if (!page) {
      return NULL;
    }
  }

  uint64_t bits = page->pagerun_owner_free_bits;
  uint64_t mask = bits & (UINT64_C(0) - bits);
  uint32_t slot = (uint32_t)__builtin_ctzll(mask);
  page->pagerun_owner_free_bits = bits & ~mask;
  if (!hz5_midpagefront_pagerun_cache_to_live(page, mask)) {
    return NULL;
  }
  return (void*)((uintptr_t)page->slab_base +
                 (uintptr_t)slot * (uintptr_t)page->class_size);
}
#endif
