// MidPage region / class / slot lookup helpers.

static int hz5_midpagefront_class_valid(uint32_t class_index) {
  return class_index < HZ5_MIDPAGEFRONT_CLASS_COUNT;
}

static int hz5_midpagefront_request_bucket(size_t size) {
  if (size <= 2048u) {
    return -1;
  }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
  if (size > 65536u) {
    return -1;
  }
#else
  if (size > 32768u) {
    return -1;
  }
#endif
  if (size <= 3072u) {
    return 0;
  }
  if (size <= 4096u) {
    return 1;
  }
  if (size <= 8192u) {
    return 2;
  }
  if (size <= 16384u) {
    return 3;
  }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
  if (size <= 32768u) {
    return 4;
  }
  return 5;
#else
  return 4;
#endif
}

// MidPageFront owns the ordinary malloc gap between SmallFront and MidFront.
// Keep this range narrow: 64K exact/overaligned and LargeFront rows are separate
// routes with different RSS and remote-free behavior.
static int hz5_midpagefront_class_index(size_t size) {
  static const uint8_t strict_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      0u, 1u, 2u, 3u, 4u
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
      ,
      5u
#endif
  };
  static const uint8_t band4_16_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      1u, 1u, 3u, 3u, 4u
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
      ,
      5u
#endif
  };
  static const uint8_t band8_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      2u, 2u, 2u, 4u, 4u
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
      ,
      5u
#endif
  };
  static const uint8_t band16_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      3u, 3u, 3u, 3u, 4u
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
      ,
      5u
#endif
  };
  static const uint8_t band4_8_16_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      1u, 1u, 2u, 3u, 4u
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
      ,
      5u
#endif
  };
  static const uint8_t band4_8_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      1u, 1u, 2u, 4u, 4u
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
      ,
      5u
#endif
  };
  static const uint8_t band8_16_32_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      2u, 2u, 2u, 3u, 4u
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
      ,
      5u
#endif
  };
  static const uint8_t wide32k_map[HZ5_MIDPAGEFRONT_CLASS_COUNT] = {
      4u, 4u, 4u, 4u, 4u
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
      ,
      5u
#endif
  };

  int bucket = hz5_midpagefront_request_bucket(size);
  if (bucket < 0) {
    return -1;
  }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_WIDE_32K_CLASS
  return (int)wide32k_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 1
  return (int)band4_16_32_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 2
  return (int)band8_32_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 3
  return (int)band16_32_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 4
  return (int)band4_8_16_32_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 5
  return (int)band4_8_32_map[bucket];
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_COARSE_BANDS == 6
  return (int)band8_16_32_map[bucket];
#else
  return (int)strict_map[bucket];
#endif
}

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
static size_t hz5_midpagefront_region_hash(uintptr_t region_base) {
  uintptr_t x = region_base >> 26;
  x ^= x >> HZ5_MIDPAGEFRONT_REGION_MAP_BITS;
  x ^= x >> (HZ5_MIDPAGEFRONT_REGION_MAP_BITS * 2u);
  return (size_t)x & (HZ5_MIDPAGEFRONT_REGION_MAP_CAP - 1u);
}

static Hz5MidPageRegion* hz5_midpagefront_lookup_region(uintptr_t region_base) {
  size_t idx = hz5_midpagefront_region_hash(region_base);
  for (size_t probe = 0; probe < HZ5_MIDPAGEFRONT_REGION_MAP_CAP; ++probe) {
    Hz5MidPageRegionMapEntry* entry =
        &g_hz5_midpagefront_region_map
            [(idx + probe) & (HZ5_MIDPAGEFRONT_REGION_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->region_base, memory_order_acquire);
    if (current == region_base) {
      return atomic_load_explicit(&entry->region, memory_order_acquire);
    }
    if (current == 0) {
      return NULL;
    }
  }
  return NULL;
}

static int hz5_midpagefront_region_map_insert(uintptr_t region_base,
                                              Hz5MidPageRegion* region) {
  size_t idx = hz5_midpagefront_region_hash(region_base);
  for (size_t probe = 0; probe < HZ5_MIDPAGEFRONT_REGION_MAP_CAP; ++probe) {
    Hz5MidPageRegionMapEntry* entry =
        &g_hz5_midpagefront_region_map
            [(idx + probe) & (HZ5_MIDPAGEFRONT_REGION_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->region_base, memory_order_acquire);
    if (current == region_base) {
      atomic_store_explicit(&entry->region, region, memory_order_release);
      return 1;
    }
    if (current == 0) {
      atomic_store_explicit(&entry->region, region, memory_order_relaxed);
      atomic_store_explicit(&entry->region_base, region_base,
                            memory_order_release);
      return 1;
    }
  }
  return 0;
}
#else
static size_t hz5_midpagefront_hash(uintptr_t slab_base) {
  uintptr_t x = slab_base >> 16;
  x ^= x >> HZ5_MIDPAGEFRONT_MAP_BITS;
  x ^= x >> (HZ5_MIDPAGEFRONT_MAP_BITS * 2u);
  return (size_t)x & (HZ5_MIDPAGEFRONT_MAP_CAP - 1u);
}

static Hz5MidPage* hz5_midpagefront_lookup_slab(uintptr_t slab_base) {
  size_t idx = hz5_midpagefront_hash(slab_base);
  for (size_t probe = 0; probe < HZ5_MIDPAGEFRONT_MAP_CAP; ++probe) {
    Hz5MidPageMapEntry* entry =
        &g_hz5_midpagefront_map[(idx + probe) &
                                (HZ5_MIDPAGEFRONT_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->slab_base, memory_order_acquire);
    if (current == slab_base) {
      return atomic_load_explicit(&entry->page, memory_order_acquire);
    }
    if (current == 0) {
      return NULL;
    }
  }
  return NULL;
}

static int hz5_midpagefront_map_insert(uintptr_t slab_base,
                                       Hz5MidPage* page) {
  size_t idx = hz5_midpagefront_hash(slab_base);
  pthread_mutex_lock(&g_hz5_midpagefront_map_lock);
  for (size_t probe = 0; probe < HZ5_MIDPAGEFRONT_MAP_CAP; ++probe) {
    Hz5MidPageMapEntry* entry =
        &g_hz5_midpagefront_map[(idx + probe) &
                                (HZ5_MIDPAGEFRONT_MAP_CAP - 1u)];
    uintptr_t current =
        atomic_load_explicit(&entry->slab_base, memory_order_acquire);
    if (current == slab_base) {
      atomic_store_explicit(&entry->page, page, memory_order_release);
      pthread_mutex_unlock(&g_hz5_midpagefront_map_lock);
      return 1;
    }
    if (current == 0) {
      atomic_store_explicit(&entry->page, page, memory_order_relaxed);
      atomic_store_explicit(&entry->slab_base, slab_base,
                            memory_order_release);
      pthread_mutex_unlock(&g_hz5_midpagefront_map_lock);
      return 1;
    }
  }
  pthread_mutex_unlock(&g_hz5_midpagefront_map_lock);
  return 0;
}
#endif

static Hz5MidPage* hz5_midpagefront_page_for_ptr(void* ptr) {
  if (!ptr) {
    return NULL;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t slab_base =
      p & ~(uintptr_t)(HZ5_MIDPAGEFRONT_SLAB_BYTES - 1u);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
  uintptr_t region_base =
      p & ~(uintptr_t)(HZ5_MIDPAGEFRONT_REGION_BYTES - 1u);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_REGION_CACHE
  Hz5MidPageRegion* region =
      hz5_midpagefront_lookup_region_tls_cached(region_base);
#else
  Hz5MidPageRegion* region =
      hz5_midpagefront_lookup_region(region_base);
#endif
  if (!region || region->base != region_base || !region->pages) {
    return NULL;
  }
  uintptr_t slab_index =
      (slab_base - region_base) / HZ5_MIDPAGEFRONT_SLAB_BYTES;
  if (slab_index >= region->slab_count) {
    return NULL;
  }
  Hz5MidPage* page = &region->pages[slab_index];
#else
  Hz5MidPage* page = hz5_midpagefront_lookup_slab(slab_base);
#endif
  if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC ||
      (uintptr_t)page->slab_base != slab_base) {
    return NULL;
  }
  return page;
}

static int hz5_midpagefront_slot_index(Hz5MidPage* page,
                                       void* ptr,
                                       uint32_t* slot_out) {
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)page->slab_base;
  if (p < base || p >= base + HZ5_MIDPAGEFRONT_SLAB_BYTES) {
    return 0;
  }
  size_t offset = (size_t)(p - base);
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_SLOT_SWITCH
  uint32_t slot = 0;
  switch (page->class_index) {
    case 0:
      if (page->class_size != 3072u || offset % 3072u != 0) {
        return 0;
      }
      slot = (uint32_t)(offset / 3072u);
      break;
    case 1:
      if (page->class_size != 4096u || (offset & 4095u) != 0) {
        return 0;
      }
      slot = (uint32_t)(offset >> 12);
      break;
    case 2:
      if (page->class_size != 8192u || (offset & 8191u) != 0) {
        return 0;
      }
      slot = (uint32_t)(offset >> 13);
      break;
    case 3:
      if (page->class_size != 16384u || (offset & 16383u) != 0) {
        return 0;
      }
      slot = (uint32_t)(offset >> 14);
      break;
    case 4:
      if (page->class_size != 32768u || (offset & 32767u) != 0) {
        return 0;
      }
      slot = (uint32_t)(offset >> 15);
      break;
    default:
      return 0;
  }
#else
  if (page->class_size == 0 || offset % page->class_size != 0) {
    return 0;
  }
  uint32_t slot = (uint32_t)(offset / page->class_size);
#endif
  if (slot >= page->slot_count || slot >= 64u) {
    return 0;
  }
  *slot_out = slot;
  return 1;
}

static uint64_t hz5_midpagefront_slot_mask(uint32_t slot) {
  return UINT64_C(1) << slot;
}

static uint64_t hz5_midpagefront_slot_count_mask(uint32_t slot_count) {
  if (slot_count >= 64u) {
    return UINT64_MAX;
  }
  if (slot_count == 0u) {
    return 0;
  }
  return (UINT64_C(1) << slot_count) - UINT64_C(1);
}

