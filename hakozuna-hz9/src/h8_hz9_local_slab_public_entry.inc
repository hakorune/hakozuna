bool h9_lsp_debug_public_usable_size(void* ptr, size_t* usable_out,
                                     bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  H9LspSegment* segment_hint = h9_lsp_public_segment_for_ptr(ptr);
  if (!segment_hint) {
    return false;
  }
  if (!h9_lsp_public_sync_segment(segment_hint)) {
    if (owned_out) {
      *owned_out = true;
    }
    return false;
  }
  H9LspRouteResult route = h9_lsp_debug_route(ptr);
  if (route.kind == H8_ROUTE_MISS) {
    return false;
  }
  if (owned_out) {
    *owned_out = true;
  }
  if (route.kind != H8_ROUTE_VALID) {
    return false;
  }
  if (usable_out) {
    *usable_out = route.slot_size;
  }
  h9_lsp_stats.usable_route_valid++;
  return true;
}
