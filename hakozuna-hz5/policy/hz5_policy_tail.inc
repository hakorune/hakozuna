// HZ5 policy public API tail helpers.

void* hz5_policy_alloc_aligned(size_t size, size_t align,
                               const Hz5PolicyHooks* hooks) {
  hz5_policy_register_observers_once();
#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  hz5_hz3_fallback_register_once();
#endif

#if BENCHLAB_HZ5_P12_ROUTE_SHADOW
  hz5_route_p12_on_alloc(size, align);
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_SMALLFRONT_S1
  if (hz5_smallfront_can_handle(size, align)) {
    void* small = hz5_smallfront_alloc(size, align);
    if (small) {
      atomic_store_explicit(&g_hz5_policy_seen_allocation, 1,
                            memory_order_relaxed);
    }
    return small;
  }
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P && \
    BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_ROUTE
  if (hz5_policy_local2p_exact(size, align)) {
    void* wrapped = hz5_policy_local2p_alloc(size, align);
    if (wrapped) {
      return wrapped;
    }
  }
#endif
#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P && \
    BENCHLAB_HZ5_WIN_LOCAL2P_DIRECT_ROUTE
  if (hz5_policy_win_local2p_exact(size, align)) {
    hz5_trace_inc(HZ5_TRACE_ALLOC_LOCAL2P_EXACT_HIT);
    void* wrapped = hz5_policy_win_local2p_alloc(size, align);
    if (wrapped) {
      return wrapped;
    }
  } else {
    hz5_policy_win_local2p_note_escape(size, align);
  }
#endif

  if (hz5_route_exact_a8192(size, align)) {
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P && \
    !BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_ROUTE
    if (hz5_policy_local2p_exact(size, align)) {
      void* wrapped = hz5_policy_local2p_alloc(size, align);
      if (wrapped) {
        return wrapped;
      }
    }
#endif
#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P && \
    !BENCHLAB_HZ5_WIN_LOCAL2P_DIRECT_ROUTE
    if (hz5_policy_win_local2p_exact(size, align)) {
      void* wrapped = hz5_policy_win_local2p_alloc(size, align);
      if (wrapped) {
        return wrapped;
      }
    }
#endif
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
    if (size == 65536u && align == (HZ5_POLICY_PAGE_SIZE * (size_t)2)) {
      void* wrapped = hz5_policy_p25_wrapper_alloc(size, align);
      if (wrapped) {
        return wrapped;
      }
    }
#endif
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || BENCHLAB_HZ5_P24_RAW64K_A8192
    if (size == 65536u && align == (HZ5_POLICY_PAGE_SIZE * (size_t)2) &&
        hooks && hooks->p17_wrapper_alloc) {
      void* wrapped = hooks->p17_wrapper_alloc(size, align);
      if (wrapped) {
        return wrapped;
      }
    }
#endif
#if BENCHLAB_HZ5_P16_WRAPPER_64K_A8192
    if (size == 65536u && align == (HZ5_POLICY_PAGE_SIZE * (size_t)2)) {
      void* wrapped = hz5_policy_wrapper_alloc_with_hz3(size, align);
      if (wrapped) {
        return wrapped;
      }
    }
#endif
    void* ptr = hz5_p2_alloc_aligned(size, align);
    if (ptr) {
      hz5_policy_register_stats_once();
      atomic_store_explicit(&g_hz5_policy_seen_allocation, 1,
                            memory_order_relaxed);
      return ptr;
    }
  }

#if BENCHLAB_HZ5_STANDALONE_EXACT_ONLY
  return NULL;
#endif

  if (align <= HZ5_POLICY_MIN_ALIGN ||
      hz5_route_hz3_medium_can_satisfy(size, align)) {
#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
    return hz5_policy_crt_wrapped_alloc(size, align);
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
    if (size <= 16u && align <= HZ5_POLICY_MIN_ALIGN) {
      void* bypass = hz5_policy_crt_wrapped_alloc(size, align);
      if (bypass) {
        return bypass;
      }
    }
    return hz5_hz3_fallback_alloc(size, align);
#else
    return hz3_malloc(size);
#endif
  }

#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
  return hz5_policy_crt_wrapped_alloc(size, align);
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  return hz5_hz3_fallback_alloc(size, align);
#else
  return hz3_large_aligned_alloc(align, size);
#endif
}

void* hz5_policy_alloc_local2p_64k_a8192(void) {
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P
  hz5_policy_register_observers_once();
  return hz5_policy_local2p_alloc(65536u,
                                  BENCHLAB_HZ5_LINUX_LOCAL2P_ALIGN);
#elif defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
  hz5_policy_register_observers_once();
  return hz5_policy_win_local2p_alloc(65536u,
                                      BENCHLAB_HZ5_WIN_LOCAL2P_ALIGN);
#else
  return NULL;
#endif
}

Hz5FreeResult hz5_policy_free_local2p_64k_a8192(void* ptr) {
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P
  if (!ptr) {
    return HZ5_FREE_OK_HZ5;
  }
  Hz5FreeResult result = HZ5_FREE_INVALID;
  if (hz5_policy_local2p_try_free_direct(ptr, &result)) {
    return result;
  }
  return HZ5_FREE_INVALID;
#elif defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
  if (!ptr) {
    return HZ5_FREE_OK_HZ5;
  }
  Hz5FreeResult result = HZ5_FREE_INVALID;
  if (hz5_policy_win_local2p_try_free_direct(ptr, &result)) {
    return result;
  }
  return HZ5_FREE_INVALID;
#else
  (void)ptr;
  return HZ5_FREE_INVALID;
#endif
}

Hz5FreeResult hz5_policy_free(void* ptr, const Hz5PolicyHooks* hooks) {
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_SMALLFRONT_S1
  Hz5SmallFrontFreeResult small_result = hz5_smallfront_free(ptr);
  if (small_result == HZ5_SMALLFRONT_FREE_OK) {
    return HZ5_FREE_OK_HZ5;
  }
  if (small_result == HZ5_SMALLFRONT_FREE_INVALID) {
    return HZ5_FREE_INVALID;
  }
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P && \
    BENCHLAB_HZ5_LINUX_LOCAL2P_FREE_FIRST
  Hz5FreeResult local2p_result = HZ5_FREE_INVALID;
  if (hz5_policy_local2p_try_free_direct(ptr, &local2p_result)) {
    return local2p_result;
  }
#endif
#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
  Hz5FreeResult win_local2p_result = HZ5_FREE_INVALID;
  if (hz5_policy_win_local2p_try_free_direct(ptr, &win_local2p_result)) {
    return win_local2p_result;
  }
#endif

  if (atomic_load_explicit(&g_hz5_policy_seen_allocation,
                           memory_order_relaxed) &&
      hz5_p1_owns(ptr)) {
    hz5_p2_free(ptr);
    return HZ5_FREE_OK_HZ5;
  }

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P && \
    !BENCHLAB_HZ5_LINUX_LOCAL2P_FREE_FIRST
  Hz5FreeResult local2p_result = HZ5_FREE_INVALID;
  if (hz5_policy_local2p_try_free_direct(ptr, &local2p_result)) {
    return local2p_result;
  }
#endif

#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK && !BENCHLAB_HZ5_P16_WRAPPER_64K_A8192 && \
    !BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 && !BENCHLAB_HZ5_P24_RAW64K_A8192 && \
    !BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 && \
    !BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
  uint32_t p20_fb_state = hz5_hz3_fallback_state();
  if (p20_fb_state == HZ5_HZ3_FALLBACK_READY ||
      p20_fb_state == HZ5_HZ3_FALLBACK_LOADING) {
    hz5_hz3_fallback_free(ptr);
    return HZ5_FREE_OK_FALLBACK;
  }
#endif

#if (BENCHLAB_HZ5_LAZY_HZ3_FALLBACK || HZ5_POLICY_P43_PREPARED_ANY) && \
    (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192)
  int p25_lowpage_lookup = HZ5_LOWPAGE64_LOOKUP_MISS;
  Hz5Lowpage64FreeCtx p25_lowpage_ctx = {0};
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR || \
    BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE || \
    BENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP || \
    BENCHLAB_HZ5_P43_WRAPPER_TOKEN
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
  (void)p25_lowpage_lookup;
  (void)p25_lowpage_ctx;
#elif BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE
  (void)p25_lowpage_lookup;
  (void)p25_lowpage_ctx;
#elif BENCHLAB_HZ5_P43_WRAPPER_TOKEN
  (void)p25_lowpage_lookup;
  (void)p25_lowpage_ctx;
#endif
#else
  if (hz5_policy_prepare_p25_lowpage_free(ptr, &p25_lowpage_lookup,
                                          &p25_lowpage_ctx)) {
    return HZ5_FREE_OK_FALLBACK;
  }
#endif
#else
  int p25_lowpage_lookup = HZ5_LOWPAGE64_LOOKUP_MISS;
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
  Hz5Lowpage64FreeCtx p25_lowpage_ctx = {0};
#else
  (void)p25_lowpage_lookup;
#endif
#endif

#if BENCHLAB_HZ5_P16_WRAPPER_64K_A8192 || BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || \
    BENCHLAB_HZ5_P24_RAW64K_A8192 || BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192 || \
    (defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P) || \
    BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
    BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
  if (p25_lowpage_lookup == HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE) {
    /* HZ5 owns this range, but it is not an active allocation. Do not let a
       stale/nonactive HZ5 pointer fall through to HZ3 fallback free. */
    return HZ5_FREE_INVALID;
  }
#endif
  Hz5WrapperHdr* wrapped = NULL;
  if (hz5_wrapper_decode(ptr, &wrapped)) {
    hz5_trace_inc(HZ5_TRACE_WRAPPER_DECODE_OK);
#if HZ5_POLICY_P43_PREPARED_ANY && \
    BENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP && \
    (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192)
    if (wrapped->source == HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE) {
      p25_lowpage_lookup =
          hz5_lowpage64_prepare_free_raw((void*)wrapped->raw,
                                         &p25_lowpage_ctx);
    }
#endif
#if HZ5_POLICY_P43_PREPARED_ANY && \
    (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192)
#if BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE
    hz5_lowpage64_p43g_note_wrapper(
        wrapped->source == HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE, 1);
#else
    hz5_lowpage64_p43g_note_wrapper(
        wrapped->source == HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE,
        hz5_policy_p25_raw_matches_lowpage_ctx(&p25_lowpage_ctx,
                                               wrapped->raw));
#endif
#endif
#if defined(__linux__) && BENCHLAB_HZ5_LINUX_LOCAL2P
    if (wrapped->source == HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P) {
      return hz5_policy_local2p_free(wrapped, (uintptr_t)ptr);
    }
    if (hz5_policy_local2p_metadata_present(wrapped)) {
      hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE);
      return HZ5_FREE_INVALID;
    }
#endif
#if defined(_WIN32) && BENCHLAB_HZ5_WIN_LOCAL2P
    if (wrapped->source == HZ5_WRAPPER_SOURCE_WIN_LOCAL2P) {
      return hz5_policy_win_local2p_free(wrapped, (uintptr_t)ptr);
    }
    if (hz5_policy_win_local2p_metadata_present(wrapped)) {
      hz5_trace_inc(HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE);
      return HZ5_FREE_INVALID;
    }
#endif
#if BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
    if (wrapped->source == HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE) {
#if BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR
      if (hz5_policy_bridge_attr_verify_and_mark_free(wrapped,
                                                      (uintptr_t)ptr)) {
        hz5_trace_inc(HZ5_TRACE_FREE_P25_BRIDGE_ATTR);
        hz5_lowpage64_release((void*)wrapped->raw);
        return HZ5_FREE_OK_HZ5;
      } else {
        hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
        return HZ5_FREE_INVALID;
      }
#elif BENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE
      hz5_trace_inc(HZ5_TRACE_FREE_TRUSTWRAP);
      hz5_lowpage64_release((void*)wrapped->raw);
      return HZ5_FREE_OK_HZ5;
#elif BENCHLAB_HZ5_P43_WRAPPER_TOKEN
      if (hz5_policy_p43_release_wrapper_token(wrapped, (uintptr_t)ptr) !=
          HZ5_FREE_OK_HZ5) {
        hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
        return HZ5_FREE_INVALID;
      }
      return HZ5_FREE_OK_HZ5;
#elif BENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP
      hz5_trace_inc(HZ5_TRACE_FREE_RAWLOOKUP);
      if (p25_lowpage_lookup == HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE) {
        return hz5_policy_free_p25_lowpage_raw(&p25_lowpage_ctx, wrapped->raw);
      }
      hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
      return HZ5_FREE_INVALID;
#else
      return hz5_policy_free_p25_lowpage_raw(&p25_lowpage_ctx, wrapped->raw);
#endif
    }
#endif
#if BENCHLAB_HZ5_P17_PAGEBIN_64K_A8192 || BENCHLAB_HZ5_P24_RAW64K_A8192
    if ((wrapped->source == HZ5_WRAPPER_SOURCE_P17_PAGEBIN ||
         wrapped->source == HZ5_WRAPPER_SOURCE_P24_RAW64K) &&
        hooks && hooks->p17_raw_release) {
      hooks->p17_raw_release((void*)wrapped->raw);
      return HZ5_FREE_OK_HZ5;
    }
#endif
    if (wrapped->source == HZ5_WRAPPER_SOURCE_P20_CRT_BYPASS) {
#if BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
      hz5_hz3_fallback_note_crt_bypass_free();
#endif
      hz5_trace_inc(HZ5_TRACE_P20_CRT_BYPASS_FREE);
      hz5_trace_add(HZ5_TRACE_P20_CRT_BYPASS_FREE_BYTES,
                    (uint64_t)wrapped->raw_bytes);
      _aligned_free((void*)wrapped->raw);
      return HZ5_FREE_OK_FALLBACK;
    }
    return hz5_policy_free_decoded_fallback_raw((void*)wrapped->raw);
  }
  hz5_trace_inc(HZ5_TRACE_WRAPPER_DECODE_MISS);
#if (BENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192 || \
     BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192) && BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  if (p25_lowpage_lookup != HZ5_LOWPAGE64_LOOKUP_MISS) {
    return HZ5_FREE_INVALID;
  }
#endif
#endif

#if BENCHLAB_HZ5_NO_HZ3_FALLBACK
  /* No-HZ3 control fallback allocations are wrapper-tagged above. Reaching
     here means the pointer was not produced by this allocator. */
  (void)ptr;
  hz5_trace_inc(HZ5_TRACE_FREE_FALLBACK_OR_INVALID);
  return HZ5_FREE_INVALID;
#elif BENCHLAB_HZ5_STANDALONE_EXACT_ONLY
  (void)ptr;
  return HZ5_FREE_INVALID;
#elif BENCHLAB_HZ5_LAZY_HZ3_FALLBACK
  hz5_hz3_fallback_free(ptr);
  return HZ5_FREE_OK_FALLBACK;
#else
  hz3_free(ptr);
  return HZ5_FREE_OK_FALLBACK;
#endif
}
