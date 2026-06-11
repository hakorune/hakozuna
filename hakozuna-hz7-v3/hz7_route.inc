static size_t h7_route_hash(uintptr_t base) {
  return (size_t)((base >> 16u) & (H7_ROUTE_CAPACITY - 1u));
}

static int h7_direct_retain_bucket_for_size(size_t size) {
  size_t i;
  if (h7_size_is_small(size)) {
    return 0;
  }
  for (i = 0; i < H7_DIRECT_RETAIN_BUCKET_COUNT; ++i) {
    if (size <= g_h7_direct_retain[i].max_size) {
      return (int)i;
    }
  }
  return -1;
}

static H7Direct* h7_direct_retain_pop(size_t size) {
  int bucket_index = h7_direct_retain_bucket_for_size(size);
  H7DirectRetainBucket* retain;
  H7Direct* direct;
  if (bucket_index < 0) {
    return 0;
  }
  retain = &g_h7_direct_retain[bucket_index];
  if (retain->count == 0) {
    return 0;
  }
  --retain->count;
  direct = retain->items[retain->count];
  retain->items[retain->count] = 0;
  return direct;
}

static int h7_direct_retain_push(H7Direct* direct) {
  int bucket_index;
  H7DirectRetainBucket* retain;
  if (!direct) {
    return 0;
  }
  bucket_index = h7_direct_retain_bucket_for_size(direct->requested_size);
  if (bucket_index < 0) {
    return 0;
  }
  retain = &g_h7_direct_retain[bucket_index];
  if (retain->count >= H7_DIRECT_RETAIN_CAP) {
    return 0;
  }
  retain->items[retain->count] = direct;
  ++retain->count;
  return 1;
}

static int h7_route_register(void* base, size_t size, H7RegionKind kind) {
  size_t i;
  uintptr_t key = (uintptr_t)base;
  H7RegionHeader* region = (H7RegionHeader*)base;
  size_t start = h7_route_hash(key);
  for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
    size_t slot = (start + i) & (H7_ROUTE_CAPACITY - 1u);
    if (!g_h7_routes[slot].active) {
      g_h7_routes[slot].base = key;
      g_h7_routes[slot].size = size;
      g_h7_routes[slot].kind = kind;
      g_h7_routes[slot].active = 1u;
      if (region) {
        region->reserved = (uint32_t)slot;
      }
      ++g_h7_stats.route_count;
      return 1;
    }
  }
  ++g_h7_stats.route_register_fail;
  return 0;
}

static void h7_route_clear_slot(size_t slot, H7RegionHeader* region) {
  g_h7_routes[slot].active = 0u;
  g_h7_routes[slot].base = 0u;
  g_h7_routes[slot].size = 0u;
  g_h7_routes[slot].kind = 0;
  if (region) {
    region->reserved = H7_ROUTE_SLOT_NONE;
  }
  --g_h7_stats.route_count;
}

static void h7_route_unregister(void* base) {
  size_t i;
  uintptr_t key = (uintptr_t)base;
  H7RegionHeader* region = (H7RegionHeader*)base;
  if (region && region->reserved < H7_ROUTE_CAPACITY) {
    size_t slot = (size_t)region->reserved;
    if (g_h7_routes[slot].active && g_h7_routes[slot].base == key) {
      h7_route_clear_slot(slot, region);
      return;
    }
  }
  {
    size_t start = h7_route_hash(key);
    for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
      size_t slot = (start + i) & (H7_ROUTE_CAPACITY - 1u);
      if (g_h7_routes[slot].active && g_h7_routes[slot].base == key) {
        h7_route_clear_slot(slot, region);
        return;
      }
    }
  }
}

static int h7_region_matches_route_entry(const H7RegionHeader* region,
                                         const H7RouteEntry* entry) {
  return region->magic == H7_MAGIC &&
         region->cookie == H7_COOKIE &&
         region->kind == entry->kind;
}

static int h7_region_is_active(const H7RegionHeader* region) {
  return (region->flags & H7_REGION_ACTIVE) != 0;
}

static void h7_region_mark_active(H7RegionHeader* region) {
  region->flags = H7_REGION_ACTIVE;
}

static void h7_region_mark_retained(H7RegionHeader* region) {
  region->flags = H7_REGION_RETAINED;
}

static void h7_region_mark_released(H7RegionHeader* region) {
  region->flags = 0;
}

static H7RouteResult h7_route_result_for_entry(const H7RouteEntry* entry) {
  H7RouteResult result;
  H7RegionHeader* region = (H7RegionHeader*)entry->base;
  result.kind = H7_ROUTE_VALID;
  result.region = region;
  if (!h7_region_matches_route_entry(region, entry)) {
    result.kind = H7_ROUTE_INVALID;
  } else if (!h7_region_is_active(region)) {
    result.kind = H7_ROUTE_INVALID;
  }
  return result;
}

static H7RouteResult h7_route_miss(void) {
  H7RouteResult result;
  result.kind = H7_ROUTE_MISS;
  result.region = 0;
  return result;
}

static H7RouteResult h7_route_lookup_exact_base(uintptr_t region_base) {
  size_t i;
  size_t start = h7_route_hash(region_base);
  for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
    size_t slot = (start + i) & (H7_ROUTE_CAPACITY - 1u);
    if (!g_h7_routes[slot].active) {
      continue;
    }
    if (g_h7_routes[slot].base == region_base) {
      return h7_route_result_for_entry(&g_h7_routes[slot]);
    }
  }
  return h7_route_miss();
}

static H7RouteResult h7_route_lookup_range(uintptr_t addr) {
  size_t i;
  for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
    uintptr_t base;
    uintptr_t end;
    if (!g_h7_routes[i].active) {
      continue;
    }
    base = g_h7_routes[i].base;
    end = base + g_h7_routes[i].size;
    if (addr >= base && addr < end) {
      return h7_route_result_for_entry(&g_h7_routes[i]);
    }
  }
  return h7_route_miss();
}

static H7RouteResult h7_route_lookup_raw(const void* ptr) {
  H7RouteResult result;
  uintptr_t region_base;
  uintptr_t addr = (uintptr_t)ptr;
  if (!ptr) {
    return h7_route_miss();
  }
  region_base = h7_region_base_from_ptr(ptr);
  result = h7_route_lookup_exact_base(region_base);
  if (result.kind != H7_ROUTE_MISS) {
    return result;
  }

  /* Direct regions can span multiple 64KiB chunks; keep INVALID semantics for
     interior pointers by falling back to a bounded range scan on miss. */
  return h7_route_lookup_range(addr);
}

static H7RouteResult h7_route_lookup_user_ptr(const void* ptr) {
  H7RouteResult route = h7_route_lookup_raw(ptr);
  if (route.kind == H7_ROUTE_MISS) {
    return route;
  }
  if (h7_route_kind_for_user_ptr(route, (void*)ptr) != H7_ROUTE_VALID) {
    route.kind = H7_ROUTE_INVALID;
  }
  return route;
}
