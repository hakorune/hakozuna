static bool h9_owner_page_local_take(H9OwnerLocalPage* page,
                                     uint64_t* bit_out) {
  uint64_t bits =
      atomic_load_explicit(&page->local_free_bits, memory_order_acquire);
  if (bits == 0) {
    H8_DEBUG_INC(h9_owner_page_api_alloc_empty);
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(bits);
  uint64_t bit = UINT64_C(1) << slot;
#if defined(H9_OWNER_LOCAL_PAGE_POOL_OWNER_FAST_BITS_L1)
  if (page->class_id <= H9_OWNER_LOCAL_PAGE_POOL_OWNER_FAST_BITS_MAX_CLASS_ID) {
    atomic_store_explicit(&page->local_free_bits, bits & ~bit,
                          memory_order_release);
  } else
#endif
  {
    while (!atomic_compare_exchange_weak_explicit(
        &page->local_free_bits, &bits, bits & ~bit, memory_order_acq_rel,
        memory_order_acquire)) {
      if (bits == 0) {
        H8_DEBUG_INC(h9_owner_page_api_alloc_empty);
        return false;
      }
      slot = (uint32_t)__builtin_ctzll(bits);
      bit = UINT64_C(1) << slot;
    }
  }
  if (bit_out) {
    *bit_out = bit;
  }
  return true;
}

static bool h9_owner_page_local_put(H9OwnerLocalPage* page, uint64_t bit) {
#if defined(H9_OWNER_LOCAL_PAGE_POOL_OWNER_FAST_BITS_L1)
  if (page->class_id > H9_OWNER_LOCAL_PAGE_POOL_OWNER_FAST_BITS_MAX_CLASS_ID ||
      atomic_load_explicit(&page->mode, memory_order_acquire) !=
          H9_OWNER_PAGE_PURE_LOCAL) {
    uint64_t old = atomic_fetch_or_explicit(&page->local_free_bits, bit,
                                            memory_order_acq_rel);
    return (old & bit) == 0;
  }
  uint64_t old =
      atomic_load_explicit(&page->local_free_bits, memory_order_acquire);
  if ((old & bit) != 0) {
    return false;
  }
  atomic_store_explicit(&page->local_free_bits, old | bit,
                        memory_order_release);
  return true;
#else
  uint64_t old = atomic_fetch_or_explicit(&page->local_free_bits, bit,
                                          memory_order_acq_rel);
  return (old & bit) == 0;
#endif
}
