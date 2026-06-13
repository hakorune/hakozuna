#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
static int hz5_midpagefront_source_refill_locked(uint32_t class_index) {
  if (!hz5_midpagefront_class_valid(class_index) ||
      g_hz5_midpagefront_region_count >= HZ5_MIDPAGEFRONT_REGION_CAP) {
    return 0;
  }

  void* block = _aligned_malloc(HZ5_MIDPAGEFRONT_REGION_BYTES,
                                HZ5_MIDPAGEFRONT_REGION_BYTES);
  if (!block) {
    return 0;
  }
  Hz5MidPage* pages = (Hz5MidPage*)_aligned_malloc(
      sizeof(Hz5MidPage) * HZ5_MIDPAGEFRONT_REGION_SLAB_COUNT, 64u);
  if (!pages) {
    _aligned_free(block);
    return 0;
  }
  memset(pages, 0,
         sizeof(Hz5MidPage) * HZ5_MIDPAGEFRONT_REGION_SLAB_COUNT);

  uintptr_t base = (uintptr_t)block;
  if ((base & (uintptr_t)(HZ5_MIDPAGEFRONT_REGION_BYTES - 1u)) != 0) {
    _aligned_free(pages);
    _aligned_free(block);
    return 0;
  }

  Hz5MidPageRegion* region =
      &g_hz5_midpagefront_regions[g_hz5_midpagefront_region_count++];
  region->base = base;
  region->pages = pages;
  region->class_index = class_index;
  region->slab_count = (uint32_t)HZ5_MIDPAGEFRONT_REGION_SLAB_COUNT;
  if (!hz5_midpagefront_region_map_insert(base, region)) {
    --g_hz5_midpagefront_region_count;
    _aligned_free(pages);
    _aligned_free(block);
    return 0;
  }

  for (uint32_t i = 0; i < HZ5_MIDPAGEFRONT_REGION_SLAB_COUNT; ++i) {
    Hz5MidPage* page = &pages[i];
    page->slab_base =
        (void*)(base + (uintptr_t)i * HZ5_MIDPAGEFRONT_SLAB_BYTES);
    page->class_index = (uint16_t)class_index;
    page->source_free = 1;
    page->source_next = g_hz5_midpagefront_source_free[class_index];
    g_hz5_midpagefront_source_free[class_index] = page;
  }
  return 1;
}

static void* hz5_midpagefront_source_alloc_slab(uint32_t class_index,
                                                Hz5MidPage** page_out) {
  if (!hz5_midpagefront_class_valid(class_index) || !page_out) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_midpagefront_region_lock);
  if (!g_hz5_midpagefront_source_free[class_index] &&
      !hz5_midpagefront_source_refill_locked(class_index)) {
    pthread_mutex_unlock(&g_hz5_midpagefront_region_lock);
    return NULL;
  }
  Hz5MidPage* page = g_hz5_midpagefront_source_free[class_index];
  g_hz5_midpagefront_source_free[class_index] = page->source_next;
  page->source_next = NULL;
  page->source_free = 0;
  page->empty_retained = 0;
  page->retired_empty = 0;
  page->retired_next = NULL;
  pthread_mutex_unlock(&g_hz5_midpagefront_region_lock);

  uintptr_t slab_base = (uintptr_t)page->slab_base;
  uintptr_t region_base =
      slab_base & ~(uintptr_t)(HZ5_MIDPAGEFRONT_REGION_BYTES - 1u);
  Hz5MidPageRegion* region =
      hz5_midpagefront_lookup_region(region_base);
  if (!region || !region->pages || region->class_index != class_index) {
    return NULL;
  }
  uintptr_t slab_index =
      (slab_base - region_base) / HZ5_MIDPAGEFRONT_SLAB_BYTES;
  if (slab_index >= region->slab_count) {
    return NULL;
  }
  if (page != &region->pages[slab_index]) {
    return NULL;
  }
  *page_out = page;
  return page->slab_base;
}
#endif

static Hz5MidPage* hz5_midpagefront_new_page(Hz5MidPageTls* tls,
                                             uint32_t class_index) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
  Hz5MidPage* page = NULL;
  void* slab = hz5_midpagefront_source_alloc_slab(class_index, &page);
  if (!slab || !page) {
    return NULL;
  }
#else
  void* slab = _aligned_malloc(HZ5_MIDPAGEFRONT_SLAB_BYTES,
                               HZ5_MIDPAGEFRONT_SLAB_BYTES);
  if (!slab) {
    return NULL;
  }
  Hz5MidPage* page =
      (Hz5MidPage*)_aligned_malloc(sizeof(Hz5MidPage), 64u);
  if (!page) {
    _aligned_free(slab);
    return NULL;
  }
#endif

  uint32_t class_size = g_hz5_midpagefront_classes[class_index];
  uint16_t slot_count =
      (uint16_t)(HZ5_MIDPAGEFRONT_SLAB_BYTES / (size_t)class_size);
  page->magic = HZ5_MIDPAGEFRONT_MAGIC;
  page->slab_base = slab;
  page->class_size = class_size;
  page->class_index = (uint16_t)class_index;
  page->slot_count = slot_count;
  page->owner = tls->owner;
  page->empty_retained = 0;
  page->retired_empty = 0;
  page->retired_next = NULL;
  atomic_store_explicit(&page->active_bits, 0, memory_order_relaxed);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  atomic_store_explicit(
      &page->free_bits,
      hz5_midpagefront_slot_count_mask(slot_count),
      memory_order_relaxed);
  page->in_partial = 0;
  page->next_partial = NULL;
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
  atomic_store_explicit(&page->remote_bits, 0, memory_order_relaxed);
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
  atomic_store_explicit(&page->slot_state2,
                        hz5_midpagefront_m4_initial_state(slot_count),
                        memory_order_relaxed);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
  atomic_store_explicit(&page->remote_packet_bits, 0, memory_order_relaxed);
  atomic_store_explicit(&page->remote_packet_queued, 0, memory_order_relaxed);
  page->remote_packet_next = NULL;
#endif
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
  page->pagerun_owner_free_bits =
      hz5_midpagefront_slot_count_mask(slot_count);
  page->pagerun_in_partial = 0;
  page->pagerun_next_partial = NULL;
#endif
  page->next_owned = tls->owned_pages[class_index];

#if !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
  if (!hz5_midpagefront_map_insert((uintptr_t)slab, page)) {
    _aligned_free(page);
    _aligned_free(slab);
    return NULL;
  }
#endif

  tls->owned_pages[class_index] = page;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
  (void)class_size;
#if !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
  hz5_midpagefront_m4_seed_page(tls, class_index, page);
#endif
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  (void)class_size;
#else
  for (uint32_t slot = slot_count; slot > 0; --slot) {
    void* ptr = (void*)((uintptr_t)slab +
                        (uintptr_t)(slot - 1u) * class_size);
    hz5_midpagefront_local_push(tls, class_index, ptr, page);
  }
#endif
  return page;
}
