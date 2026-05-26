static int hz5_policy_local2p_direct_decode(void* ptr,
                                            Hz5WrapperHdr** header_out) {
  if (!ptr) {
    return 0;
  }
  uintptr_t aligned = (uintptr_t)ptr;
  if (aligned < sizeof(Hz5WrapperHdr)) {
    return 0;
  }

  Hz5WrapperHdr* header =
      (Hz5WrapperHdr*)(aligned - sizeof(Hz5WrapperHdr));
  if (header->magic != HZ5_WRAPPER_HDR_MAGIC ||
      header->layout_version != HZ5_WRAPPER_LAYOUT_VERSION ||
      header->layout_size != sizeof(Hz5WrapperHdr) ||
      header->source != HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P) {
    return 0;
  }
  uintptr_t raw = header->raw;
#if BENCHLAB_HZ5_LINUX_LOCAL2P_ROUTE_COOKIE
  if (raw == 0) {
    return 0;
  }
#else
  if (raw == 0 || header->cookie != hz5_wrapper_cookie(raw, aligned)) {
    return 0;
  }
#endif
  if (header_out) {
    *header_out = header;
  }
  return 1;
}

static inline int
hz5_policy_local2p_try_free_direct(void* ptr, Hz5FreeResult* result_out) {
  Hz5WrapperHdr* local2p_wrapped = NULL;
  if (!hz5_policy_local2p_direct_decode(ptr, &local2p_wrapped)) {
    return 0;
  }
  hz5_trace_inc(HZ5_TRACE_WRAPPER_DECODE_OK);
  if (result_out) {
    *result_out = hz5_policy_local2p_free(local2p_wrapped, (uintptr_t)ptr);
  }
  return 1;
}

static int hz5_policy_local2p_metadata_present(const Hz5WrapperHdr* header) {
  return hz5_policy_local2p_common_metadata_present(header);
}
