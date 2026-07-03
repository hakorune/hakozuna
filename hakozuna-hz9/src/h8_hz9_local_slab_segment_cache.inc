#define H9_LSP_SEGMENT_CACHE_CAP 48u

static H9LspSegment* h9_lsp_segment_cache[H9_LSP_SEGMENT_CACHE_CAP];
static size_t h9_lsp_segment_cache_count;

static uint64_t h9_lsp_full_bits(uint32_t slot_count) {
  return slot_count == 64u ? UINT64_MAX
                           : ((UINT64_C(1) << slot_count) - 1u);
}

static H9LspSegment* h9_lsp_cache_pop(uint32_t class_id, uint32_t slot_size,
                                      uint32_t slot_count) {
  h8_platform_mutex_lock(&h9_lsp_lock);
  for (size_t i = h9_lsp_segment_cache_count; i > 0u; --i) {
    H9LspSegment* segment = h9_lsp_segment_cache[i - 1u];
    if (!segment || segment->class_id != class_id) {
      continue;
    }
    h9_lsp_segment_cache[i - 1u] =
        h9_lsp_segment_cache[--h9_lsp_segment_cache_count];
    segment->generation = h9_lsp_generation++;
    segment->slot_size = slot_size;
    segment->slot_count = (uint16_t)slot_count;
    segment->free_bits = h9_lsp_full_bits(slot_count);
    segment->alloc_bits = 0u;
    segment->cache_bits = 0u;
    h9_lsp_pending_clear(segment);
    h9_lsp_stats.segment_cache_hit++;
    h9_lsp_stats.segment_cache_live--;
    h9_lsp_stats.segment_cache_bytes -= H9_LSP_SEGMENT_BYTES;
    h8_platform_mutex_unlock(&h9_lsp_lock);
    return segment;
  }
  h8_platform_mutex_unlock(&h9_lsp_lock);
  return NULL;
}

static bool h9_lsp_cache_store_locked(H9LspSegment* segment) {
  if (!segment || segment->alloc_bits || segment->cache_bits ||
      h9_lsp_pending_load(segment) ||
      h9_lsp_segment_cache_count >= H9_LSP_SEGMENT_CACHE_CAP) {
    h9_lsp_stats.segment_cache_drop++;
    return false;
  }
  segment->free_bits = h9_lsp_full_bits(segment->slot_count);
  h9_lsp_segment_cache[h9_lsp_segment_cache_count++] = segment;
  h9_lsp_stats.segment_cache_store++;
  h9_lsp_stats.segment_cache_live++;
  h9_lsp_stats.segment_cache_bytes += H9_LSP_SEGMENT_BYTES;
  if (h9_lsp_stats.segment_cache_bytes >
      h9_lsp_stats.segment_cache_peak_bytes)
    h9_lsp_stats.segment_cache_peak_bytes = h9_lsp_stats.segment_cache_bytes;
  return true;
}

static void h9_lsp_release_segment(H9LspSegment* segment) {
  if (!segment) {
    return;
  }
  void* raw = NULL;
  size_t raw_bytes = 0u;
  h8_platform_mutex_lock(&h9_lsp_lock);
  for (size_t i = 0; i < H9_LSP_MAX_SEGMENTS; ++i) {
    if (h9_lsp_segments[i] == segment) {
      if (h9_lsp_cache_store_locked(segment)) {
        h8_platform_mutex_unlock(&h9_lsp_lock);
        return;
      }
      raw = segment->raw_base, raw_bytes = segment->raw_bytes;
      segment->magic = H9_LSP_SEGMENT_DEAD;
      h9_lsp_segments[i] = NULL;
      h9_lsp_stats.segment_release++;
      h9_lsp_stats.segment_live -= h9_lsp_stats.segment_live ? 1u : 0u;
      size_t cb = h9_lsp_stats.segment_committed_bytes;
      h9_lsp_stats.segment_committed_bytes =
          cb >= H9_LSP_SEGMENT_BYTES ? cb - H9_LSP_SEGMENT_BYTES : 0u;
      size_t rb = h9_lsp_stats.segment_reserved_bytes;
      h9_lsp_stats.segment_reserved_bytes =
          rb >= raw_bytes ? rb - raw_bytes : 0u;
      break;
    }
  }
  h8_platform_mutex_unlock(&h9_lsp_lock);
  if (raw)
    h8_platform_release(raw, raw_bytes);
}

static void h9_lsp_cache_clear(void) {
  memset(h9_lsp_segment_cache, 0, sizeof(h9_lsp_segment_cache));
  h9_lsp_segment_cache_count = 0u;
}
