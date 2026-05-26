#if BENCHLAB_HZ5_WIN_LOCAL2P_ROUTE_HEADER_GUARD
static int hz5_policy_win_local2p_route_header_readable(uintptr_t aligned) {
  if (aligned < sizeof(Hz5WrapperHdr)) {
    return 0;
  }

  /*
   * HZ4's Windows wrapper guard only probes page-boundary cases so the hot
   * exact route does not pay a VirtualQuery tax on every free. This optional
   * guard gives Local2P the same protection when ptr-header would cross into
   * the previous page before route-cookie direct decode touches the header.
   */
  if ((aligned & ((uintptr_t)HZ5_POLICY_PAGE_SIZE - 1u)) >=
      sizeof(Hz5WrapperHdr)) {
    return 1;
  }

  uintptr_t hdr_addr = aligned - sizeof(Hz5WrapperHdr);
  MEMORY_BASIC_INFORMATION mbi;
  if (VirtualQuery((const void*)hdr_addr, &mbi, sizeof(mbi)) == 0) {
    return 0;
  }
  if (mbi.State != MEM_COMMIT) {
    return 0;
  }
  if ((mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
    return 0;
  }

  uintptr_t region_start = (uintptr_t)mbi.BaseAddress;
  uintptr_t region_end = region_start + (uintptr_t)mbi.RegionSize;
  uintptr_t hdr_end = hdr_addr + sizeof(Hz5WrapperHdr);
  if (region_end < region_start || hdr_end < hdr_addr) {
    return 0;
  }
  return hdr_addr >= region_start && hdr_end <= region_end;
}
#endif

static int hz5_policy_win_local2p_direct_decode(void* ptr,
                                                Hz5WrapperHdr** header_out) {
#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ROUTE
#if !BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_FIRST
  (void)hz5_policy_win_local2p_sidecar_probe((uintptr_t)ptr, NULL);
#endif
#endif
#if BENCHLAB_HZ5_WIN_LOCAL2P_ROUTE_COOKIE
  if (!ptr) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_MISS);
    return 0;
  }
  uintptr_t aligned = (uintptr_t)ptr;
  if (aligned < sizeof(Hz5WrapperHdr)) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_MISS);
    return 0;
  }
#if BENCHLAB_HZ5_WIN_LOCAL2P_ROUTE_HEADER_GUARD
  if (!hz5_policy_win_local2p_route_header_readable(aligned)) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_MISS);
    return 0;
  }
#endif
  Hz5WrapperHdr* header =
      (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
  if (header->magic != HZ5_WRAPPER_HDR_MAGIC ||
      header->layout_version != HZ5_WRAPPER_LAYOUT_VERSION ||
      header->layout_size != sizeof(Hz5WrapperHdr) ||
      header->source != HZ5_WRAPPER_SOURCE_WIN_LOCAL2P ||
      header->raw == 0) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_MISS);
    return 0;
  }
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_OK);
  if (header_out) {
    *header_out = header;
  }
  return 1;
#else
  Hz5WrapperHdr* header = NULL;
  if (!hz5_wrapper_decode(ptr, &header)) {
    return 0;
  }
  if (header->source != HZ5_WRAPPER_SOURCE_WIN_LOCAL2P) {
    return 0;
  }
  if (header_out) {
    *header_out = header;
  }
  return 1;
#endif
}

static int
hz5_policy_win_local2p_try_free_direct(void* ptr,
                                       Hz5FreeResult* result_out) {
  Hz5WrapperHdr* wrapped = NULL;
#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ROUTE && \
    BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_FIRST
  int sidecar_kind =
      hz5_policy_win_local2p_sidecar_probe((uintptr_t)ptr, &wrapped);
  if (sidecar_kind == HZ5_WIN_LOCAL2P_SIDECAR_VALID) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_FIRST_OK);
    if (result_out) {
      *result_out = hz5_policy_win_local2p_free(wrapped, (uintptr_t)ptr);
    }
    return 1;
  }
  if (sidecar_kind == HZ5_WIN_LOCAL2P_SIDECAR_INVALID) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_FIRST_INVALID);
    if (result_out) {
      *result_out = HZ5_FREE_INVALID;
    }
    return 1;
  }
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_FIRST_MISS);
#endif
  if (!hz5_policy_win_local2p_direct_decode(ptr, &wrapped)) {
    return 0;
  }
  if (result_out) {
    *result_out = hz5_policy_win_local2p_free(wrapped, (uintptr_t)ptr);
  }
  return 1;
}

static int hz5_policy_win_local2p_metadata_present(
    const Hz5WrapperHdr* header) {
  return hz5_policy_local2p_common_metadata_present(header);
}
