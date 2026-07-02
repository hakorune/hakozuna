typedef struct H9LspLoopResult {
  uint64_t ok;
  uintptr_t sink;
  double elapsed;
  uint64_t fast_hits;
  uint64_t fallback_hits;
  uint64_t state_mismatch;
  int rc;
} H9LspLoopResult;

static inline bool h9_lsp_bench_fastleaf_free(H9LspIntegratedEntry* entry,
                                              void* ptr) {
  if (entry->last_live && entry->last_ptr == ptr &&
      entry->last_generation == entry->page.generation &&
      entry->last_slot < entry->page.slot_count) {
    uint32_t slot = entry->last_slot;
    uint64_t bit = UINT64_C(1) << slot;
    if ((entry->page.alloc_bits & bit) != 0u &&
        (entry->page.free_bits & bit) == 0u) {
      entry->page.alloc_bits &= ~bit;
      entry->page.free_bits |= bit;
      entry->last_live = 0u;
      entry->fast_hits++;
      return true;
    }
    entry->last_live = 0u;
    entry->state_mismatch++;
  }
  return false;
}

static H9_BENCH_NOINLINE H9LspLoopResult h9_lsp_run_integrated_worker(
    uint32_t class_id, uint32_t slot_size, uint64_t iters, bool touch) {
  H9LspLoopResult result = {0};
  size_t payload_bytes = (size_t)slot_size * 64u;
  void* payload = malloc(payload_bytes);
  if (!payload) {
    result.rc = 2;
    return result;
  }
  H9LspIntegratedEntry entry;
  h9_lsp_integrated_init(&entry, (uintptr_t)payload, slot_size, 64u, class_id);
  double start = now_seconds();
  for (uint64_t i = 0; i < iters; ++i) {
    void* ptr = NULL;
    bool success = h9_lsp_integrated_alloc(&entry, &ptr);
    if (success && touch) {
      volatile unsigned char* p = (volatile unsigned char*)ptr;
      p[0] = (unsigned char)i;
      p[slot_size - 1u] = (unsigned char)(i >> 8);
    }
    success = success && h9_lsp_integrated_free(&entry, ptr);
    if (!success) {
      result.rc = 3;
      free(payload);
      return result;
    }
    result.sink ^= (uintptr_t)ptr;
    ++result.ok;
  }
  result.elapsed = now_seconds() - start;
  result.fast_hits = entry.fast_hits;
  result.fallback_hits = entry.fallback_hits;
  result.state_mismatch = entry.state_mismatch;
  free(payload);
  return result;
}

static H9_BENCH_NOINLINE H9LspLoopResult h9_lsp_run_fastleaf_worker(
    uint32_t class_id, uint32_t slot_size, uint64_t iters, bool touch) {
  H9LspLoopResult result = {0};
  size_t payload_bytes = (size_t)slot_size * 64u;
  void* payload = malloc(payload_bytes);
  if (!payload) {
    result.rc = 2;
    return result;
  }
  H9LspIntegratedEntry entry;
  h9_lsp_integrated_init(&entry, (uintptr_t)payload, slot_size, 64u, class_id);
  double start = now_seconds();
  for (uint64_t i = 0; i < iters; ++i) {
    void* ptr = NULL;
    bool success = h9_lsp_integrated_alloc(&entry, &ptr);
    if (success && touch) {
      volatile unsigned char* p = (volatile unsigned char*)ptr;
      p[0] = (unsigned char)i;
      p[slot_size - 1u] = (unsigned char)(i >> 8);
    }
    success = success && h9_lsp_bench_fastleaf_free(&entry, ptr);
    if (!success) {
      result.rc = 3;
      free(payload);
      return result;
    }
    result.sink ^= (uintptr_t)ptr;
    ++result.ok;
  }
  result.elapsed = now_seconds() - start;
  result.fast_hits = entry.fast_hits;
  result.fallback_hits = entry.fallback_hits;
  result.state_mismatch = entry.state_mismatch;
  free(payload);
  return result;
}
