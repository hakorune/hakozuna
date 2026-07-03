static uint64_t h9_lsp_pending_load(const H9LspSegment* segment) {
  return atomic_load_explicit(&segment->pending_bits, memory_order_acquire);
}

static void h9_lsp_pending_clear(H9LspSegment* segment) {
  atomic_store_explicit(&segment->pending_bits, 0u, memory_order_release);
}

static uint64_t h9_lsp_pending_exchange_clear(H9LspSegment* segment) {
  return atomic_exchange_explicit(&segment->pending_bits, 0u,
                                  memory_order_acq_rel);
}

static bool h9_lsp_exact_slot_for_pending(void* ptr, H9LspSegment** segment_out,
                                          uint32_t* slot_out) {
  if (!ptr) {
    return false;
  }
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t base = addr & ~(uintptr_t)(H9_LSP_SEGMENT_BYTES - 1u);
  H9LspSegment* segment = h9_lsp_hash_find(base);
  if (!segment) {
    return false;
  }
  uintptr_t payload = base + H9_LSP_PAYLOAD_OFFSET;
  uintptr_t offset = addr - payload;
  size_t used = (size_t)segment->slot_size * (size_t)segment->slot_count;
  if (addr < payload || offset >= used ||
      (offset % (uintptr_t)segment->slot_size) != 0u) {
    h9_lsp_stats.remote_pending_invalid++;
    if (segment_out) {
      *segment_out = segment;
    }
    return false;
  }
  if (segment_out) {
    *segment_out = segment;
  }
  if (slot_out) {
    *slot_out = (uint32_t)(offset / (uintptr_t)segment->slot_size);
  }
  return true;
}

static bool h9_lsp_remote_pending_claim(void* ptr, bool* owned_out) {
  H9LspSegment* segment = NULL;
  uint32_t slot = 0u;
  bool exact = h9_lsp_exact_slot_for_pending(ptr, &segment, &slot);
  if (owned_out) {
    *owned_out = segment != NULL;
  }
  if (!exact) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  uint64_t old =
      atomic_fetch_or_explicit(&segment->pending_bits, bit, memory_order_acq_rel);
  if ((old & bit) != 0u) {
    h9_lsp_stats.remote_pending_duplicate++;
    return false;
  }
  h9_lsp_stats.remote_pending_claim++;
  return true;
}
