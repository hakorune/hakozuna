// MidPageFront public API tail helpers.

static void* hz5_midpagefront_alloc_class(uint32_t ci) {
  if (!hz5_midpagefront_class_valid(ci)) {
    return NULL;
  }
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
  return hz5_midpagefront_pagerun_alloc_class(ci);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
  return hz5_midpagefront_m4_alloc_class(ci);
#elif BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  return hz5_midpagefront_nodeless_alloc_class(ci);
#else
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  Hz5MidPage* page = NULL;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT
  void* ptr = hz5_midpagefront_hot_pop(tls, ci, &page);
#else
  void* ptr = hz5_midpagefront_local_pop(tls, ci, &page);
#endif
  if (!ptr) {
    (void)hz5_midpagefront_drain_remote_class(tls, ci);
    ptr = hz5_midpagefront_local_pop(tls, ci, &page);
  }
  if (!ptr) {
    if (!hz5_midpagefront_new_page(tls, ci)) {
      return NULL;
    }
    ptr = hz5_midpagefront_local_pop(tls, ci, &page);
  }
  if (!ptr) {
    return NULL;
  }
  if (!page || page->magic != HZ5_MIDPAGEFRONT_MAGIC) {
    page = hz5_midpagefront_page_for_ptr(ptr);
  }
  uint32_t slot = 0;
  if (!page || !hz5_midpagefront_slot_index(page, ptr, &slot) ||
      !hz5_midpagefront_mark_active_local(page, slot)) {
    return NULL;
  }
  return ptr;
#endif
}

Hz5MidPageFrontAllocResult hz5_midpagefront_try_alloc(size_t size,
                                                      size_t align,
                                                      void** ptr_out) {
  if (ptr_out) {
    *ptr_out = NULL;
  }
  if (align > 16u) {
    return HZ5_MIDPAGEFRONT_ALLOC_UNSUPPORTED;
  }
  int class_index = hz5_midpagefront_class_index(size);
  if (class_index < 0) {
    return HZ5_MIDPAGEFRONT_ALLOC_UNSUPPORTED;
  }
  void* ptr = hz5_midpagefront_alloc_class((uint32_t)class_index);
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return ptr ? HZ5_MIDPAGEFRONT_ALLOC_OK : HZ5_MIDPAGEFRONT_ALLOC_OOM;
}

void* hz5_midpagefront_alloc(size_t size, size_t align) {
  void* ptr = NULL;
  (void)hz5_midpagefront_try_alloc(size, align, &ptr);
  return ptr;
}

static Hz5MidPageFrontFreeResult hz5_midpagefront_free_page(
    void* ptr,
    Hz5MidPage* page) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  hz5_midpagefront_m6_defer_free(tls, ptr);
  return HZ5_MIDPAGEFRONT_FREE_OK;
#else
  if (!page) {
    return HZ5_MIDPAGEFRONT_FREE_NOT_OWNED;
  }
  if (page->magic != HZ5_MIDPAGEFRONT_MAGIC) {
    return HZ5_MIDPAGEFRONT_FREE_NOT_OWNED;
  }
  uint32_t slot = 0;
  if (!hz5_midpagefront_slot_index(page, ptr, &slot)) {
    return HZ5_MIDPAGEFRONT_FREE_NOT_OWNED;
  }
  if (!hz5_midpagefront_mark_free_local(page, slot)) {
    return HZ5_MIDPAGEFRONT_FREE_NOT_OWNED;
  }
  return HZ5_MIDPAGEFRONT_FREE_OK;
#endif
}

Hz5MidPageFrontFreeResult hz5_midpagefront_free(void* ptr) {
  return hz5_midpagefront_free_page(ptr, hz5_midpagefront_page_for_ptr(ptr));
}

Hz5MidPageFrontTag hz5_midpagefront_tag(void* ptr) {
  Hz5MidPageFrontTag tag;
  tag.page = hz5_midpagefront_page_for_ptr(ptr);
  return tag;
}

Hz5MidPageFrontFreeResult hz5_midpagefront_free_tagged(
    void* ptr,
    Hz5MidPageFrontTag tag) {
  return hz5_midpagefront_free_page(ptr, (Hz5MidPage*)tag.page);
}

int hz5_midpagefront_can_handle(size_t size, size_t align) {
  return align <= 16u && hz5_midpagefront_class_index(size) >= 0;
}

int hz5_midpagefront_owns(void* ptr) {
  return hz5_midpagefront_page_for_ptr(ptr) != NULL;
}

size_t hz5_midpagefront_usable_size(void* ptr) {
  Hz5MidPage* page = hz5_midpagefront_page_for_ptr(ptr);
  if (!page) {
    return 0;
  }
  uint32_t slot = 0;
  if (!hz5_midpagefront_slot_index(page, ptr, &slot)) {
    return 0;
  }
  return page->class_size;
}

void hz5_midpagefront_owner_drain_some(unsigned budget) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
  if (budget == 0u) {
    return;
  }
  Hz5MidPageTls* tls = hz5_midpagefront_tls();
  uint32_t pending = hz5_midpagefront_owner_pending_load(tls->owner);
  if (pending == 0) {
    return;
  }
  unsigned drained_classes = 0;
  for (uint32_t ci = 0; ci < HZ5_MIDPAGEFRONT_CLASS_COUNT; ++ci) {
    if ((pending & (UINT32_C(1) << ci)) == 0) {
      continue;
    }
    (void)hz5_midpagefront_drain_remote_class(tls, ci);
    if (++drained_classes >= budget) {
      break;
    }
  }
#else
  (void)budget;
#endif
}

size_t hz5_midpagefront_release_retired(void) {
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
  return hz5_midpagefront_m4_release_checkpoint(hz5_midpagefront_tls());
#else
  return 0;
#endif
}

#else

void* hz5_midpagefront_alloc(size_t size, size_t align) {
  (void)size;
  (void)align;
  return NULL;
}

Hz5MidPageFrontAllocResult hz5_midpagefront_try_alloc(size_t size,
                                                      size_t align,
                                                      void** ptr_out) {
  (void)size;
  (void)align;
  if (ptr_out) {
    *ptr_out = NULL;
  }
  return HZ5_MIDPAGEFRONT_ALLOC_UNSUPPORTED;
}

Hz5MidPageFrontFreeResult hz5_midpagefront_free(void* ptr) {
  (void)ptr;
  return HZ5_MIDPAGEFRONT_FREE_NOT_OWNED;
}

int hz5_midpagefront_can_handle(size_t size, size_t align) {
  (void)size;
  (void)align;
  return 0;
}

int hz5_midpagefront_owns(void* ptr) {
  (void)ptr;
  return 0;
}

size_t hz5_midpagefront_usable_size(void* ptr) {
  (void)ptr;
  return 0;
}

void hz5_midpagefront_owner_drain_some(unsigned budget) {
  (void)budget;
}

size_t hz5_midpagefront_release_retired(void) {
  return 0;
}

#endif
