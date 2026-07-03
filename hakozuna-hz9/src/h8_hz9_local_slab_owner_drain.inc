static bool h9_lsp_public_drain_pending(H9LspCompactRouteLeafEntry* entry) {
  H9LspSegment* segment = entry ? entry->authority_segment : NULL;
  if (!segment) return false;
  uint64_t claimed = h9_lsp_pending_exchange_clear(segment);
  if (claimed == 0u) return false;
  uint64_t mask = entry->page.slot_count == 64u
                      ? UINT64_MAX
                      : ((UINT64_C(1) << entry->page.slot_count) - 1u);
  claimed &= mask;
  uint64_t valid = claimed & entry->page.alloc_bits;
  uint64_t invalid = claimed & ~entry->page.alloc_bits;
  entry->page.alloc_bits &= ~valid;
  entry->page.free_bits |= valid;
  h9_lsp_stats.remote_pending_drain++;
  h9_lsp_stats.remote_pending_drain_slots +=
      (size_t)__builtin_popcountll(valid);
  h9_lsp_stats.remote_pending_drain_invalid +=
      (size_t)__builtin_popcountll(invalid);
  return valid != 0u;
}
