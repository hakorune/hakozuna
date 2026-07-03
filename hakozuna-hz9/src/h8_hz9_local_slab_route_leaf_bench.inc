typedef struct H9LspRouteLeafEntry {
  H9LspSegment* segment;
  void* last_ptr;
  uint32_t last_slot;
  uint32_t last_generation;
  uint8_t last_live;
  uint64_t fast_hits;
  uint64_t fallback_hits;
  uint64_t state_mismatch;
} H9LspRouteLeafEntry;

static inline bool h9_lsp_routeleaf_alloc(H9LspRouteLeafEntry* entry,
                                          void** ptr_out) {
  H9LspSegment* segment = entry->segment;
  if (!segment || segment->free_bits == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(segment->free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  segment->free_bits &= ~bit;
  segment->alloc_bits |= bit;
  void* ptr = h9_lsp_slot_ptr(segment, slot);
  entry->last_ptr = ptr;
  entry->last_slot = slot;
  entry->last_generation = segment->generation;
  entry->last_live = 1u;
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return true;
}

static inline bool h9_lsp_routeleaf_fast_free(H9LspRouteLeafEntry* entry,
                                              void* ptr) {
  H9LspSegment* segment = entry->segment;
  if (entry->last_live && entry->last_ptr == ptr && segment &&
      entry->last_generation == segment->generation &&
      entry->last_slot < segment->slot_count) {
    uint64_t bit = UINT64_C(1) << entry->last_slot;
    if ((segment->alloc_bits & bit) != 0u &&
        (segment->free_bits & bit) == 0u &&
        (segment->cache_bits & bit) == 0u &&
        (segment->pending_bits & bit) == 0u) {
      segment->alloc_bits &= ~bit;
      segment->free_bits |= bit;
      entry->last_live = 0u;
      entry->fast_hits++;
      return true;
    }
    entry->last_live = 0u;
    entry->state_mismatch++;
  }
  return false;
}

static H9_LSP_COLD_NOINLINE bool h9_lsp_routeleaf_cold_free(
    H9LspRouteLeafEntry* entry, void* ptr) {
  H9LspRouteResult route = h9_lsp_debug_route_direct_owned(ptr);
  if (route.kind != H8_ROUTE_VALID) {
    return false;
  }
  H9LspSegment* segment = (H9LspSegment*)route.segment;
  uint64_t bit = UINT64_C(1) << route.slot;
  segment->alloc_bits &= ~bit;
  segment->free_bits |= bit;
  entry->fallback_hits++;
  return true;
}

static inline bool h9_lsp_routeleaf_free(H9LspRouteLeafEntry* entry,
                                         void* ptr) {
  if (h9_lsp_routeleaf_fast_free(entry, ptr)) {
    return true;
  }
  return h9_lsp_routeleaf_cold_free(entry, ptr);
}

bool h9_lsp_debug_routeleaf_bench(uint32_t class_id, uint64_t iters,
                                  bool touch, bool non_lifo,
                                  H9LspRouteLeafBenchResult* result_out) {
  h9_lsp_debug_reset();
  H9LspSegment* segment = h9_lsp_create_segment(class_id);
  if (!segment) {
    return false;
  }
  H9LspRouteLeafEntry entry = {
      .segment = segment,
  };
  H9LspRouteLeafBenchResult result = {0};
  for (uint64_t i = 0; i < iters; ++i) {
    void* a = NULL;
    if (!h9_lsp_routeleaf_alloc(&entry, &a)) {
      return false;
    }
    if (touch) {
      volatile unsigned char* p = (volatile unsigned char*)a;
      p[0] = (unsigned char)i;
      p[segment->slot_size - 1u] = (unsigned char)(i >> 8);
    }
    if (non_lifo) {
      void* b = NULL;
      if (!h9_lsp_routeleaf_alloc(&entry, &b)) {
        return false;
      }
      if (!h9_lsp_routeleaf_free(&entry, a) ||
          !h9_lsp_routeleaf_free(&entry, b)) {
        return false;
      }
      result.sink ^= (uintptr_t)a ^ (uintptr_t)b;
      result.ok += 2u;
    } else {
      if (!h9_lsp_routeleaf_free(&entry, a)) {
        return false;
      }
      result.sink ^= (uintptr_t)a;
      result.ok++;
    }
  }
  result.fast_hits = entry.fast_hits;
  result.fallback_hits = entry.fallback_hits;
  result.state_mismatch = entry.state_mismatch;
  if (result_out) {
    *result_out = result;
  }
  return true;
}

typedef struct H9LspCompactRouteLeafEntry {
  H9LspInlinePage page;
  void* last_ptr;
  uint32_t last_slot;
  uint32_t last_generation;
  uint8_t last_live;
  uint64_t fast_hits;
  uint64_t fallback_hits;
  uint64_t state_mismatch;
  H9LspSegment* authority_segment;
} H9LspCompactRouteLeafEntry;

static inline bool h9_lsp_compact_routeleaf_alloc(
    H9LspCompactRouteLeafEntry* entry, void** ptr_out) {
  uint32_t slot = UINT32_MAX;
  void* ptr = NULL;
  if (!h9_lsp_inline_alloc_slot(&entry->page, &slot, &ptr)) {
    return false;
  }
  entry->last_ptr = ptr;
  entry->last_slot = slot;
  entry->last_generation = entry->page.generation;
  entry->last_live = 1u;
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return true;
}

static inline bool h9_lsp_compact_routeleaf_fast_free(
    H9LspCompactRouteLeafEntry* entry, void* ptr) {
  if (entry->last_live && entry->last_ptr == ptr &&
      entry->last_generation == entry->page.generation &&
      entry->last_slot < entry->page.slot_count) {
    if (h9_lsp_inline_free_slot(&entry->page, entry->last_slot)) {
      entry->last_live = 0u;
      entry->fast_hits++;
      return true;
    }
    entry->last_live = 0u;
    entry->state_mismatch++;
  }
  return false;
}

static H9_LSP_COLD_NOINLINE bool h9_lsp_compact_routeleaf_cold_free(
    H9LspCompactRouteLeafEntry* entry, void* ptr) {
  H9LspSegment* segment = entry->authority_segment;
  if (!segment) {
    return false;
  }
  segment->generation = entry->page.generation;
  segment->free_bits = entry->page.free_bits;
  segment->alloc_bits = entry->page.alloc_bits;
  segment->cache_bits = 0u;
  segment->pending_bits = 0u;

  H9LspRouteResult route = h9_lsp_debug_route_direct_owned(ptr);
  if (route.kind != H8_ROUTE_VALID) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << route.slot;
  segment->alloc_bits &= ~bit;
  segment->free_bits |= bit;
  entry->page.alloc_bits &= ~bit;
  entry->page.free_bits |= bit;
  entry->fallback_hits++;
  return true;
}

static inline bool h9_lsp_compact_routeleaf_free(
    H9LspCompactRouteLeafEntry* entry, void* ptr) {
  if (h9_lsp_compact_routeleaf_fast_free(entry, ptr)) {
    return true;
  }
  return h9_lsp_compact_routeleaf_cold_free(entry, ptr);
}

bool h9_lsp_debug_routeleaf_compact_bench(
    uint32_t class_id, uint64_t iters, bool touch, bool non_lifo,
    H9LspRouteLeafBenchResult* result_out) {
  h9_lsp_debug_reset();
  H9LspSegment* segment = h9_lsp_create_segment(class_id);
  if (!segment) {
    return false;
  }
  H9LspCompactRouteLeafEntry entry = {
      .authority_segment = segment,
  };
  h9_lsp_inline_page_init(&entry.page, (uintptr_t)h9_lsp_slot_ptr(segment, 0u),
                          segment->slot_size, segment->slot_count);
  entry.page.class_id = segment->class_id;
  entry.page.generation = segment->generation;

  H9LspRouteLeafBenchResult result = {0};
  for (uint64_t i = 0; i < iters; ++i) {
    void* a = NULL;
    if (!h9_lsp_compact_routeleaf_alloc(&entry, &a)) {
      return false;
    }
    if (touch) {
      volatile unsigned char* p = (volatile unsigned char*)a;
      p[0] = (unsigned char)i;
      p[entry.page.slot_size - 1u] = (unsigned char)(i >> 8);
    }
    if (non_lifo) {
      void* b = NULL;
      if (!h9_lsp_compact_routeleaf_alloc(&entry, &b)) {
        return false;
      }
      if (!h9_lsp_compact_routeleaf_free(&entry, a) ||
          !h9_lsp_compact_routeleaf_free(&entry, b)) {
        return false;
      }
      result.sink ^= (uintptr_t)a ^ (uintptr_t)b;
      result.ok += 2u;
    } else {
      if (!h9_lsp_compact_routeleaf_free(&entry, a)) {
        return false;
      }
      result.sink ^= (uintptr_t)a;
      result.ok++;
    }
  }
  result.fast_hits = entry.fast_hits;
  result.fallback_hits = entry.fallback_hits;
  result.state_mismatch = entry.state_mismatch;
  if (result_out) {
    *result_out = result;
  }
  return true;
}

typedef struct H9LspTrimRouteLeafEntry {
  H9LspInlinePage page;
  void* last_ptr;
  uint32_t last_slot;
  uint32_t last_generation;
  uint8_t last_live;
  uint64_t fallback_hits;
  uint64_t state_mismatch;
  H9LspSegment* authority_segment;
} H9LspTrimRouteLeafEntry;

static inline bool h9_lsp_trim_routeleaf_alloc(
    H9LspTrimRouteLeafEntry* entry, void** ptr_out) {
  uint32_t slot = UINT32_MAX;
  void* ptr = NULL;
  if (!h9_lsp_inline_alloc_slot(&entry->page, &slot, &ptr)) {
    return false;
  }
  entry->last_ptr = ptr;
  entry->last_slot = slot;
  entry->last_generation = entry->page.generation;
  entry->last_live = 1u;
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return true;
}

static inline bool h9_lsp_trim_routeleaf_fast_free(
    H9LspTrimRouteLeafEntry* entry, void* ptr) {
  if (entry->last_live && entry->last_ptr == ptr &&
      entry->last_generation == entry->page.generation &&
      entry->last_slot < entry->page.slot_count) {
    if (h9_lsp_inline_free_slot(&entry->page, entry->last_slot)) {
      entry->last_live = 0u;
      return true;
    }
    entry->last_live = 0u;
    entry->state_mismatch++;
  }
  return false;
}

static H9_LSP_COLD_NOINLINE bool h9_lsp_trim_routeleaf_cold_free(
    H9LspTrimRouteLeafEntry* entry, void* ptr) {
  H9LspSegment* segment = entry->authority_segment;
  if (!segment) {
    return false;
  }
  segment->generation = entry->page.generation;
  segment->free_bits = entry->page.free_bits;
  segment->alloc_bits = entry->page.alloc_bits;
  segment->cache_bits = 0u;
  segment->pending_bits = 0u;

  H9LspRouteResult route = h9_lsp_debug_route_direct_owned(ptr);
  if (route.kind != H8_ROUTE_VALID) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << route.slot;
  segment->alloc_bits &= ~bit;
  segment->free_bits |= bit;
  entry->page.alloc_bits &= ~bit;
  entry->page.free_bits |= bit;
  entry->fallback_hits++;
  return true;
}

static inline bool h9_lsp_trim_routeleaf_free(H9LspTrimRouteLeafEntry* entry,
                                              void* ptr) {
  if (h9_lsp_trim_routeleaf_fast_free(entry, ptr)) {
    return true;
  }
  return h9_lsp_trim_routeleaf_cold_free(entry, ptr);
}

bool h9_lsp_debug_routeleaf_trim_bench(
    uint32_t class_id, uint64_t iters, bool touch, bool non_lifo,
    H9LspRouteLeafBenchResult* result_out) {
  h9_lsp_debug_reset();
  H9LspSegment* segment = h9_lsp_create_segment(class_id);
  if (!segment) {
    return false;
  }
  H9LspTrimRouteLeafEntry entry = {
      .authority_segment = segment,
  };
  h9_lsp_inline_page_init(&entry.page, (uintptr_t)h9_lsp_slot_ptr(segment, 0u),
                          segment->slot_size, segment->slot_count);
  entry.page.class_id = segment->class_id;
  entry.page.generation = segment->generation;

  H9LspRouteLeafBenchResult result = {0};
  for (uint64_t i = 0; i < iters; ++i) {
    void* a = NULL;
    if (!h9_lsp_trim_routeleaf_alloc(&entry, &a)) {
      return false;
    }
    if (touch) {
      volatile unsigned char* p = (volatile unsigned char*)a;
      p[0] = (unsigned char)i;
      p[entry.page.slot_size - 1u] = (unsigned char)(i >> 8);
    }
    if (non_lifo) {
      void* b = NULL;
      if (!h9_lsp_trim_routeleaf_alloc(&entry, &b)) {
        return false;
      }
      if (!h9_lsp_trim_routeleaf_free(&entry, a) ||
          !h9_lsp_trim_routeleaf_free(&entry, b)) {
        return false;
      }
      result.sink ^= (uintptr_t)a ^ (uintptr_t)b;
      result.ok += 2u;
    } else {
      if (!h9_lsp_trim_routeleaf_free(&entry, a)) {
        return false;
      }
      result.sink ^= (uintptr_t)a;
      result.ok++;
    }
  }
  result.fallback_hits = entry.fallback_hits;
  result.fast_hits = result.ok - result.fallback_hits;
  result.state_mismatch = entry.state_mismatch;
  if (result_out) {
    *result_out = result;
  }
  return true;
}

static inline bool h9_lsp_tight_routeleaf_fast_free(
    H9LspTrimRouteLeafEntry* entry, void* ptr) {
  if (entry->last_live && entry->last_ptr == ptr &&
      entry->last_generation == entry->page.generation &&
      entry->last_slot < entry->page.slot_count) {
    uint64_t bit = UINT64_C(1) << entry->last_slot;
    uint64_t alloc = entry->page.alloc_bits;
    uint64_t free = entry->page.free_bits;
    if ((alloc & bit) != 0u && (free & bit) == 0u) {
      entry->page.alloc_bits = alloc & ~bit;
      entry->page.free_bits = free | bit;
      entry->last_live = 0u;
      return true;
    }
    entry->last_live = 0u;
    entry->state_mismatch++;
  }
  return false;
}

static inline bool h9_lsp_tight_routeleaf_free(H9LspTrimRouteLeafEntry* entry,
                                               void* ptr) {
  if (h9_lsp_tight_routeleaf_fast_free(entry, ptr)) {
    return true;
  }
  return h9_lsp_trim_routeleaf_cold_free(entry, ptr);
}

bool h9_lsp_debug_routeleaf_tight_bench(
    uint32_t class_id, uint64_t iters, bool touch, bool non_lifo,
    H9LspRouteLeafBenchResult* result_out) {
  h9_lsp_debug_reset();
  H9LspSegment* segment = h9_lsp_create_segment(class_id);
  if (!segment) {
    return false;
  }
  H9LspTrimRouteLeafEntry entry = {
      .authority_segment = segment,
  };
  h9_lsp_inline_page_init(&entry.page, (uintptr_t)h9_lsp_slot_ptr(segment, 0u),
                          segment->slot_size, segment->slot_count);
  entry.page.class_id = segment->class_id;
  entry.page.generation = segment->generation;

  H9LspRouteLeafBenchResult result = {0};
  for (uint64_t i = 0; i < iters; ++i) {
    void* a = NULL;
    if (!h9_lsp_trim_routeleaf_alloc(&entry, &a)) {
      return false;
    }
    if (touch) {
      volatile unsigned char* p = (volatile unsigned char*)a;
      p[0] = (unsigned char)i;
      p[entry.page.slot_size - 1u] = (unsigned char)(i >> 8);
    }
    if (non_lifo) {
      void* b = NULL;
      if (!h9_lsp_trim_routeleaf_alloc(&entry, &b)) {
        return false;
      }
      if (!h9_lsp_tight_routeleaf_free(&entry, a) ||
          !h9_lsp_tight_routeleaf_free(&entry, b)) {
        return false;
      }
      result.sink ^= (uintptr_t)a ^ (uintptr_t)b;
      result.ok += 2u;
    } else {
      if (!h9_lsp_tight_routeleaf_free(&entry, a)) {
        return false;
      }
      result.sink ^= (uintptr_t)a;
      result.ok++;
    }
  }
  result.fallback_hits = entry.fallback_hits;
  result.fast_hits = result.ok - result.fallback_hits;
  result.state_mismatch = entry.state_mismatch;
  if (result_out) {
    *result_out = result;
  }
  return true;
}

static _Thread_local H9LspCompactRouteLeafEntry
    h9_lsp_public_entries[H8_MEDIUM_CLASS_COUNT];
static _Thread_local uint32_t h9_lsp_public_last_class = UINT32_MAX;
static _Thread_local H9LspCompactRouteLeafEntry* h9_lsp_public_current_entry;

static bool h9_lsp_public_entry_init(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9LspSegment* segment = h9_lsp_create_segment(class_id);
  if (!segment) {
    return false;
  }
  H9LspCompactRouteLeafEntry* entry = &h9_lsp_public_entries[class_id];
  *entry = (H9LspCompactRouteLeafEntry){
      .authority_segment = segment,
  };
  h9_lsp_inline_page_init(&entry->page, (uintptr_t)h9_lsp_slot_ptr(segment, 0u),
                          segment->slot_size, segment->slot_count);
  entry->page.class_id = segment->class_id;
  entry->page.generation = segment->generation;
  return true;
}

static bool h9_lsp_public_entry_ready(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  return h9_lsp_public_entries[class_id].authority_segment != NULL ||
         h9_lsp_public_entry_init(class_id);
}

static void h9_lsp_public_sync_entry(H9LspCompactRouteLeafEntry* entry) {
  H9LspSegment* segment = entry ? entry->authority_segment : NULL;
  if (!segment) {
    return;
  }
  segment->generation = entry->page.generation;
  segment->free_bits = entry->page.free_bits;
  segment->alloc_bits = entry->page.alloc_bits;
  segment->cache_bits = 0u;
  segment->pending_bits = 0u;
}

static void h9_lsp_public_sync_all(void) {
  for (uint32_t i = 0; i < H8_MEDIUM_CLASS_COUNT; ++i) {
    h9_lsp_public_sync_entry(&h9_lsp_public_entries[i]);
  }
}

void h9_lsp_debug_public_entry_reset(void) {
  h9_lsp_debug_reset();
  memset(h9_lsp_public_entries, 0, sizeof(h9_lsp_public_entries));
  h9_lsp_public_last_class = UINT32_MAX;
  h9_lsp_public_current_entry = NULL;
}

void* h9_lsp_debug_public_malloc(size_t size) {
  if (!h8_medium_size_supported_fast(size)) {
    return NULL;
  }
  uint32_t class_id = h8_medium_class_for_size_fast(size);
  if (!h9_lsp_public_entry_ready(class_id)) {
    return NULL;
  }
  H9LspCompactRouteLeafEntry* entry = &h9_lsp_public_entries[class_id];
  void* ptr = NULL;
  if (!h9_lsp_compact_routeleaf_alloc(entry, &ptr)) {
    if (!h9_lsp_public_entry_init(class_id)) {
      return NULL;
    }
    entry = &h9_lsp_public_entries[class_id];
    if (!h9_lsp_compact_routeleaf_alloc(entry, &ptr)) {
      return NULL;
    }
  }
  h9_lsp_public_sync_entry(entry);
  h9_lsp_public_last_class = class_id;
  h9_lsp_public_current_entry = entry;
  return ptr;
}

bool h9_lsp_debug_public_free(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  uint32_t class_id = h9_lsp_public_last_class;
  if (class_id < H8_MEDIUM_CLASS_COUNT &&
      h9_lsp_compact_routeleaf_fast_free(&h9_lsp_public_entries[class_id],
                                         ptr)) {
    h9_lsp_public_sync_entry(&h9_lsp_public_entries[class_id]);
    if (owned_out) {
      *owned_out = true;
    }
    return true;
  }

  h9_lsp_public_sync_all();
  H9LspRouteResult route = h9_lsp_debug_route_direct_owned(ptr);
  if (route.kind == H8_ROUTE_MISS) {
    return false;
  }
  if (owned_out) {
    *owned_out = true;
  }
  if (route.kind != H8_ROUTE_VALID) {
    return false;
  }
  H9LspSegment* segment = (H9LspSegment*)route.segment;
  uint64_t bit = UINT64_C(1) << route.slot;
  segment->alloc_bits &= ~bit;
  segment->free_bits |= bit;
  if (route.class_id < H8_MEDIUM_CLASS_COUNT &&
      h9_lsp_public_entries[route.class_id].authority_segment == segment) {
    h9_lsp_public_entries[route.class_id].page.alloc_bits &= ~bit;
    h9_lsp_public_entries[route.class_id].page.free_bits |= bit;
  }
  return true;
}

void* h9_lsp_debug_public_nosync_malloc(size_t size) {
  if (!h8_medium_size_supported_fast(size)) {
    return NULL;
  }
  uint32_t class_id = h8_medium_class_for_size_fast(size);
  if (!h9_lsp_public_entry_ready(class_id)) {
    return NULL;
  }
  H9LspCompactRouteLeafEntry* entry = &h9_lsp_public_entries[class_id];
  void* ptr = NULL;
  if (!h9_lsp_compact_routeleaf_alloc(entry, &ptr)) {
    if (!h9_lsp_public_entry_init(class_id)) {
      return NULL;
    }
    entry = &h9_lsp_public_entries[class_id];
    if (!h9_lsp_compact_routeleaf_alloc(entry, &ptr)) {
      return NULL;
    }
  }
  h9_lsp_public_last_class = class_id;
  h9_lsp_public_current_entry = entry;
  return ptr;
}

bool h9_lsp_debug_public_current_free(void* ptr) {
  H9LspCompactRouteLeafEntry* entry = h9_lsp_public_current_entry;
  if (entry && h9_lsp_compact_routeleaf_fast_free(entry, ptr)) {
    return true;
  }

  h9_lsp_public_sync_all();
  H9LspRouteResult route = h9_lsp_debug_route_direct_owned(ptr);
  if (route.kind != H8_ROUTE_VALID) {
    return false;
  }
  H9LspSegment* segment = (H9LspSegment*)route.segment;
  uint64_t bit = UINT64_C(1) << route.slot;
  segment->alloc_bits &= ~bit;
  segment->free_bits |= bit;
  if (route.class_id < H8_MEDIUM_CLASS_COUNT &&
      h9_lsp_public_entries[route.class_id].authority_segment == segment) {
    h9_lsp_public_entries[route.class_id].page.alloc_bits &= ~bit;
    h9_lsp_public_entries[route.class_id].page.free_bits |= bit;
  }
  return true;
}

bool h9_lsp_debug_public_nosync_free(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  uint32_t class_id = h9_lsp_public_last_class;
  if (class_id < H8_MEDIUM_CLASS_COUNT &&
      h9_lsp_compact_routeleaf_fast_free(&h9_lsp_public_entries[class_id],
                                         ptr)) {
    if (owned_out) {
      *owned_out = true;
    }
    return true;
  }

  h9_lsp_public_sync_all();
  H9LspRouteResult route = h9_lsp_debug_route_direct_owned(ptr);
  if (route.kind == H8_ROUTE_MISS) {
    return false;
  }
  if (owned_out) {
    *owned_out = true;
  }
  if (route.kind != H8_ROUTE_VALID) {
    return false;
  }
  H9LspSegment* segment = (H9LspSegment*)route.segment;
  uint64_t bit = UINT64_C(1) << route.slot;
  segment->alloc_bits &= ~bit;
  segment->free_bits |= bit;
  if (route.class_id < H8_MEDIUM_CLASS_COUNT &&
      h9_lsp_public_entries[route.class_id].authority_segment == segment) {
    h9_lsp_public_entries[route.class_id].page.alloc_bits &= ~bit;
    h9_lsp_public_entries[route.class_id].page.free_bits |= bit;
  }
  return true;
}

static bool h9_lsp_debug_publicentry_bench_impl(
    size_t size, uint64_t iters, bool touch, bool non_lifo, bool nosync,
    H9LspRouteLeafBenchResult* result_out) {
  h9_lsp_debug_public_entry_reset();
  H9LspRouteLeafBenchResult result = {0};
  for (uint64_t i = 0; i < iters; ++i) {
    void* a = nosync ? h9_lsp_debug_public_nosync_malloc(size)
                     : h9_lsp_debug_public_malloc(size);
    if (!a) {
      return false;
    }
    if (touch) {
      volatile unsigned char* p = (volatile unsigned char*)a;
      p[0] = (unsigned char)i;
      p[size - 1u] = (unsigned char)(i >> 8);
    }
    if (non_lifo) {
      void* b = nosync ? h9_lsp_debug_public_nosync_malloc(size)
                       : h9_lsp_debug_public_malloc(size);
      bool owned_a = false;
      bool owned_b = false;
      bool ok_a = nosync ? h9_lsp_debug_public_nosync_free(a, &owned_a)
                         : h9_lsp_debug_public_free(a, &owned_a);
      bool ok_b = nosync ? h9_lsp_debug_public_nosync_free(b, &owned_b)
                         : h9_lsp_debug_public_free(b, &owned_b);
      if (!b || !ok_a || !ok_b || !owned_a || !owned_b) {
        return false;
      }
      result.sink ^= (uintptr_t)a ^ (uintptr_t)b;
      result.ok += 2u;
    } else {
      bool owned = false;
      bool ok = nosync ? h9_lsp_debug_public_nosync_free(a, &owned)
                       : h9_lsp_debug_public_free(a, &owned);
      if (!ok || !owned) {
        return false;
      }
      result.sink ^= (uintptr_t)a;
      result.ok++;
    }
  }
  if (result_out) {
    *result_out = result;
  }
  return true;
}

bool h9_lsp_debug_publicentry_bench(size_t size, uint64_t iters, bool touch,
                                    bool non_lifo,
                                    H9LspRouteLeafBenchResult* result_out) {
  return h9_lsp_debug_publicentry_bench_impl(size, iters, touch, non_lifo,
                                             false, result_out);
}

bool h9_lsp_debug_publicentry_nosync_bench(
    size_t size, uint64_t iters, bool touch, bool non_lifo,
    H9LspRouteLeafBenchResult* result_out) {
  return h9_lsp_debug_publicentry_bench_impl(size, iters, touch, non_lifo,
                                             true, result_out);
}
