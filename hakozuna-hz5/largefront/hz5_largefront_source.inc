// LargeFront slow-path source and lookup helpers.

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_PAYLOAD_SCAVENGE
static int hz5_largefront_payload_scavenge_class(uint32_t class_index) {
  return hz5_largefront_class_valid(class_index) &&
         hz5_largefront_class_bytes(class_index) == 131072u;
}

static void hz5_largefront_payload_scavenge(Hz5LargeSpan* span) {
  if (!span || !span->base || span->class_bytes == 0u ||
      (span->flags & HZ5_LARGEFRONT_SPAN_PAYLOAD_SCAVENGED)) {
    return;
  }
  (void)madvise(span->base, (size_t)span->class_bytes, MADV_DONTNEED);
  span->flags |= HZ5_LARGEFRONT_SPAN_PAYLOAD_SCAVENGED;
}

static void hz5_largefront_payload_reactivate(Hz5LargeSpan* span) {
  if (!span) {
    return;
  }
  span->flags &= (uint16_t)~HZ5_LARGEFRONT_SPAN_PAYLOAD_SCAVENGED;
}
#else
static void hz5_largefront_payload_reactivate(Hz5LargeSpan* span) {
  (void)span;
}
#endif

static size_t hz5_largefront_hash(uintptr_t page_base) {
  uintptr_t x = page_base >> 12;
  x ^= x >> HZ5_LARGEFRONT_MAP_BITS;
  x ^= x >> (HZ5_LARGEFRONT_MAP_BITS * 2u);
  return (size_t)x & (HZ5_LARGEFRONT_MAP_CAP - 1u);
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
static size_t hz5_largefront_region_hash(uintptr_t key) {
  uintptr_t x = key;
  x ^= x >> HZ5_LARGEFRONT_REGION_BUCKET_BITS;
  x ^= x >> (HZ5_LARGEFRONT_REGION_BUCKET_BITS * 2u);
  return (size_t)x & (HZ5_LARGEFRONT_REGION_BUCKET_CAP - 1u);
}

static int hz5_largefront_region_register(void* block,
                                          size_t bytes,
                                          uint32_t class_index,
                                          uint32_t span_count) {
  if (!block || bytes == 0 || !hz5_largefront_class_valid(class_index)) {
    return 0;
  }

  uintptr_t base = (uintptr_t)block;
  uintptr_t end = base + (uintptr_t)bytes;
  if (end <= base) {
    return 0;
  }

  uintptr_t first_key = base >> HZ5_LARGEFRONT_REGION_GRAN_BITS;
  uintptr_t last_key = (end - 1u) >> HZ5_LARGEFRONT_REGION_GRAN_BITS;
  uintptr_t link_count = last_key - first_key + 1u;

  pthread_mutex_lock(&g_hz5_largefront_region_lock);
  if (g_hz5_largefront_region_count >= HZ5_LARGEFRONT_REGION_CAP ||
      link_count >
          (uintptr_t)(HZ5_LARGEFRONT_REGION_LINK_CAP -
                      g_hz5_largefront_region_link_count)) {
    pthread_mutex_unlock(&g_hz5_largefront_region_lock);
    return 0;
  }

  Hz5LargeRegion* region =
      &g_hz5_largefront_regions[g_hz5_largefront_region_count++];
  region->base = base;
  region->end = end;
  region->class_index = class_index;
  region->span_count = span_count;
  region->stride = hz5_largefront_span_stride(class_index);

  for (uintptr_t key = first_key; key <= last_key; ++key) {
    Hz5LargeRegionLink* link =
        &g_hz5_largefront_region_links[g_hz5_largefront_region_link_count++];
    link->key = key;
    link->region = region;
    size_t bucket = hz5_largefront_region_hash(key);
    Hz5LargeRegionLink* old_head = atomic_load_explicit(
        &g_hz5_largefront_region_buckets[bucket], memory_order_acquire);
    link->next = old_head;
    atomic_store_explicit(&g_hz5_largefront_region_buckets[bucket],
                          link,
                          memory_order_release);
  }
  pthread_mutex_unlock(&g_hz5_largefront_region_lock);
  return 1;
}

static Hz5LargeSpan* hz5_largefront_lookup_region_ptr(uintptr_t p) {
  uintptr_t key = p >> HZ5_LARGEFRONT_REGION_GRAN_BITS;
  size_t bucket = hz5_largefront_region_hash(key);
  Hz5LargeRegionLink* link = atomic_load_explicit(
      &g_hz5_largefront_region_buckets[bucket], memory_order_acquire);

  while (link) {
    const Hz5LargeRegion* region = link->region;
    if (link->key == key && region && p >= region->base && p < region->end) {
      uintptr_t offset = p - region->base;
      size_t stride = region->stride;
      if (stride == 0) {
        return NULL;
      }
      uintptr_t span_index = offset / (uintptr_t)stride;
      uintptr_t span_offset = offset % (uintptr_t)stride;
      if (span_index >= region->span_count ||
          span_offset < HZ5_LARGEFRONT_PAGE_SIZE) {
        return NULL;
      }
      Hz5LargeSpan* span =
          (Hz5LargeSpan*)(region->base + span_index * (uintptr_t)stride);
      if (span->magic != HZ5_LARGEFRONT_MAGIC ||
          span->class_index != region->class_index) {
        return NULL;
      }
      return span;
    }
    link = link->next;
  }
  return NULL;
}
#endif

static uint32_t hz5_largefront_source_refill_count_locked(
    uint32_t class_index) {
  uint32_t batch_count = HZ5_LARGEFRONT_SOURCE_BATCH_COUNT;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L1A
  if (hz5_largefront_class_valid(class_index) &&
      hz5_largefront_class_bytes(class_index) == 131072u) {
    size_t mapped_spans = g_hz5_largefront_mapped_spans[class_index];
    if (mapped_spans >= HZ5_LARGEFRONT_POLICY_L1A_HIGH_SPANS) {
      batch_count = 4u;
    } else if (mapped_spans >= HZ5_LARGEFRONT_POLICY_L1A_MID_SPANS) {
      batch_count = 8u;
    } else {
      batch_count = 16u;
    }
  }
#elif BENCHLAB_HZ5_LINUX_LARGEFRONT_ADAPTIVE128
  if (hz5_largefront_class_valid(class_index) &&
      hz5_largefront_class_bytes(class_index) == 131072u) {
    size_t mapped_bytes = g_hz5_largefront_mapped_spans[class_index] *
                          hz5_largefront_span_stride(class_index);
    if (mapped_bytes >= HZ5_LARGEFRONT_ADAPTIVE128_HIGH_BYTES) {
      batch_count = 4u;
    } else if (mapped_bytes >= HZ5_LARGEFRONT_ADAPTIVE128_MID_BYTES) {
      batch_count = 8u;
    }
  }
#else
  (void)class_index;
#endif
  return batch_count == 0u ? 1u : batch_count;
}

static int hz5_largefront_source_refill_locked(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return 0;
  }
  size_t stride = hz5_largefront_span_stride(class_index);
  uint32_t batch_count = hz5_largefront_source_refill_count_locked(class_index);
  if (stride == 0 ||
      batch_count > SIZE_MAX / stride) {
    return 0;
  }
  size_t bytes = stride * (size_t)batch_count;
  void* block = mmap(NULL,
                     bytes,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1,
                     0);
  if (block == MAP_FAILED) {
    return 0;
  }

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
  if (!hz5_largefront_region_register(block, bytes, class_index, batch_count)) {
    (void)munmap(block, bytes);
    return 0;
  }
#endif

  uintptr_t base = (uintptr_t)block;
  for (uint32_t i = 0; i < batch_count; ++i) {
    Hz5LargeRawNode* node = (Hz5LargeRawNode*)(base + (uintptr_t)i * stride);
    node->next = g_hz5_largefront_source_free[class_index];
    g_hz5_largefront_source_free[class_index] = node;
  }
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
  hz5_largefront_obs_note_source_refill(class_index, batch_count);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
  hz5_largefront_policy_note_source_refill(class_index, batch_count);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_ADAPTIVE128 || \
    BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L1A
  g_hz5_largefront_mapped_spans[class_index] += (size_t)batch_count;
#endif
  return 1;
}

static void* hz5_largefront_source_alloc_raw(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return NULL;
  }
  pthread_mutex_lock(&g_hz5_largefront_source_lock);
  if (!g_hz5_largefront_source_free[class_index]) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
    hz5_largefront_policy_note_source_empty(class_index);
#endif
    if (!hz5_largefront_source_refill_locked(class_index)) {
      pthread_mutex_unlock(&g_hz5_largefront_source_lock);
      return NULL;
    }
  }
  if (!g_hz5_largefront_source_free[class_index]) {
    pthread_mutex_unlock(&g_hz5_largefront_source_lock);
    return NULL;
  }
  Hz5LargeRawNode* node = g_hz5_largefront_source_free[class_index];
  g_hz5_largefront_source_free[class_index] = node->next;
  pthread_mutex_unlock(&g_hz5_largefront_source_lock);
  return node;
}

static void hz5_largefront_source_free_raw(uint32_t class_index, void* raw) {
  if (!hz5_largefront_class_valid(class_index) || !raw) {
    return;
  }
  Hz5LargeRawNode* node = (Hz5LargeRawNode*)raw;
  pthread_mutex_lock(&g_hz5_largefront_source_lock);
  node->next = g_hz5_largefront_source_free[class_index];
  g_hz5_largefront_source_free[class_index] = node;
  pthread_mutex_unlock(&g_hz5_largefront_source_lock);
}

static Hz5LargeSpan* hz5_largefront_lookup_page(uintptr_t page_base) {
  size_t idx = hz5_largefront_hash(page_base);
  for (size_t probe = 0; probe < HZ5_LARGEFRONT_MAP_CAP; ++probe) {
    Hz5LargeMapEntry* entry =
        &g_hz5_largefront_map[(idx + probe) & (HZ5_LARGEFRONT_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->page_base, memory_order_acquire);
    if (current == page_base) {
      return atomic_load_explicit(&entry->span, memory_order_acquire);
    }
    if (current == 0) {
      return NULL;
    }
  }
  return NULL;
}

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_BASE_DIRECTMAP
static Hz5LargeSpan* hz5_largefront_lookup_base_direct(uintptr_t page_base) {
  Hz5LargeMapEntry* entry =
      &g_hz5_largefront_base_directmap[hz5_largefront_hash(page_base)];
  uintptr_t current =
      atomic_load_explicit(&entry->page_base, memory_order_acquire);
  if (current != page_base) {
    return NULL;
  }
  return atomic_load_explicit(&entry->span, memory_order_acquire);
}

static void hz5_largefront_base_direct_insert(uintptr_t page_base,
                                              Hz5LargeSpan* span) {
  Hz5LargeMapEntry* entry =
      &g_hz5_largefront_base_directmap[hz5_largefront_hash(page_base)];
  atomic_store_explicit(&entry->span, span, memory_order_relaxed);
  atomic_store_explicit(&entry->page_base, page_base, memory_order_release);
}
#endif

static int hz5_largefront_map_insert_one(uintptr_t page_base,
                                         Hz5LargeSpan* span) {
  size_t idx = hz5_largefront_hash(page_base);
  for (size_t probe = 0; probe < HZ5_LARGEFRONT_MAP_CAP; ++probe) {
    Hz5LargeMapEntry* entry =
        &g_hz5_largefront_map[(idx + probe) & (HZ5_LARGEFRONT_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->page_base, memory_order_acquire);
    if (current == page_base) {
      atomic_store_explicit(&entry->span, span, memory_order_release);
      return 1;
    }
    if (current == 0) {
      atomic_store_explicit(&entry->span, span, memory_order_relaxed);
      atomic_store_explicit(&entry->page_base, page_base,
                            memory_order_release);
      return 1;
    }
  }
  return 0;
}

static int hz5_largefront_map_insert(Hz5LargeSpan* span) {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
  // Region lookup remains the ownership fallback. With the base fast map also
  // enabled, keep a best-effort exact-base map so normal free(ptr) avoids the
  // variable stride division in the region path.
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_BASE_FASTMAP
  if (span && span->base) {
    uintptr_t base = (uintptr_t)span->base;
    pthread_mutex_lock(&g_hz5_largefront_map_lock);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_BASE_DIRECTMAP
    hz5_largefront_base_direct_insert(base, span);
#endif
    (void)hz5_largefront_map_insert_one(base, span);
    pthread_mutex_unlock(&g_hz5_largefront_map_lock);
  }
#else
  (void)span;
#endif
  return 1;
#else
  uintptr_t base = (uintptr_t)span->base;
  pthread_mutex_lock(&g_hz5_largefront_map_lock);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_BASE_DIRECTMAP
  hz5_largefront_base_direct_insert(base, span);
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_MAP_BASE_ONLY
  if (!hz5_largefront_map_insert_one(base, span)) {
    pthread_mutex_unlock(&g_hz5_largefront_map_lock);
    return 0;
  }
#else
  for (uint32_t i = 0; i < span->page_count; ++i) {
    uintptr_t page_base = base + ((uintptr_t)i * HZ5_LARGEFRONT_PAGE_SIZE);
    if (!hz5_largefront_map_insert_one(page_base, span)) {
      pthread_mutex_unlock(&g_hz5_largefront_map_lock);
      return 0;
    }
  }
#endif
  pthread_mutex_unlock(&g_hz5_largefront_map_lock);
  return 1;
#endif
}

static Hz5LargeSpan* hz5_largefront_span_for_ptr(void* ptr) {
  if (!ptr) {
    return NULL;
  }
  uintptr_t p = (uintptr_t)ptr;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_DIRECT_HEADER_LOOKUP
  if ((p & (uintptr_t)(HZ5_LARGEFRONT_PAGE_SIZE - 1u)) == 0u &&
      p >= HZ5_LARGEFRONT_PAGE_SIZE) {
    Hz5LargeSpan* direct =
        (Hz5LargeSpan*)(p - (uintptr_t)HZ5_LARGEFRONT_PAGE_SIZE);
    if (direct->magic == HZ5_LARGEFRONT_MAGIC &&
        direct->base == ptr &&
        hz5_largefront_class_valid(direct->class_index)) {
      return direct;
    }
  }
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_BASE_FASTMAP
  uintptr_t page_base = p & ~(uintptr_t)(HZ5_LARGEFRONT_PAGE_SIZE - 1u);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_BASE_DIRECTMAP
  Hz5LargeSpan* direct_span = hz5_largefront_lookup_base_direct(page_base);
  if (direct_span && direct_span->magic == HZ5_LARGEFRONT_MAGIC &&
      ptr == direct_span->base) {
    return direct_span;
  }
#endif
  Hz5LargeSpan* base_span = hz5_largefront_lookup_page(page_base);
  if (base_span && base_span->magic == HZ5_LARGEFRONT_MAGIC &&
      ptr == base_span->base) {
    return base_span;
  }
#endif
  Hz5LargeSpan* span = hz5_largefront_lookup_region_ptr(p);
#else
  uintptr_t page_base = p & ~(uintptr_t)(HZ5_LARGEFRONT_PAGE_SIZE - 1u);
  Hz5LargeSpan* span = hz5_largefront_lookup_page(page_base);
#endif
  if (!span || span->magic != HZ5_LARGEFRONT_MAGIC) {
    return NULL;
  }
  uintptr_t base = (uintptr_t)span->base;
  uintptr_t end = base + (uintptr_t)span->class_bytes;
  if (p < base || p >= end) {
    return NULL;
  }
  return span;
}

