  if (addr && bytes) {
    munmap(addr, bytes);
  }
#endif
}

static int hz5_lowpage64_p43_segment_contains(Hz5Lowpage64Segment* seg,
                                              uintptr_t p,
                                              uint32_t* slot_out) {
  if (!seg) {
    return 0;
  }
  uintptr_t base = (uintptr_t)seg->base;
  uintptr_t end = base + HZ5_LOWPAGE64_P43_SEGMENT_SIZE;
  if (p < base || p >= end) {
    return 0;
  }
  uint32_t slot =
      (uint32_t)((p - base) / HZ5_LOWPAGE64_P43_SLOT_SIZE);
  if (slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return 0;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return 1;
}

static uint32_t hz5_lowpage64_p43_mask_load(_Atomic uint32_t* mask) {
  return atomic_load_explicit(mask, memory_order_acquire);
}

static void hz5_lowpage64_p43_mask_store(_Atomic uint32_t* mask,
                                          uint32_t value) {
  atomic_store_explicit(mask, value, memory_order_release);
}

/* The P43i2 writer-store variant is only valid at call sites that already
 * hold the P43 segment lock. Lockless readers still observe these masks with
 * acquire loads, so keep all mask updates behind these helpers. */
static void hz5_lowpage64_p43_mask_or(_Atomic uint32_t* mask,
                                      uint32_t value) {
#if HZ5_LOWPAGE64_P43_LOCKED_WRITER_MASKS
  uint32_t current = atomic_load_explicit(mask, memory_order_relaxed);
  atomic_store_explicit(mask, current | value, memory_order_release);
#else
  atomic_fetch_or_explicit(mask, value, memory_order_acq_rel);
#endif
}

static void hz5_lowpage64_p43_mask_and(_Atomic uint32_t* mask,
                                       uint32_t value) {
#if HZ5_LOWPAGE64_P43_LOCKED_WRITER_MASKS
  uint32_t current = atomic_load_explicit(mask, memory_order_relaxed);
  atomic_store_explicit(mask, current & value, memory_order_release);
#else
  atomic_fetch_and_explicit(mask, value, memory_order_acq_rel);
#endif
}

static size_t hz5_lowpage64_p43_popcount32(uint32_t value) {
  size_t count = 0;
  while (value) {
    count += (size_t)(value & 1u);
    value >>= 1;
  }
  return count;
}

static uint32_t hz5_lowpage64_p43_slot_bit(uint32_t slot) {
  return (uint32_t)1u << slot;
}

static uint32_t hz5_lowpage64_p43_slot_full_mask(void) {
  return hz5_lowpage64_p43_slot_bit(
             (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) -
         1u;
}

static void* hz5_lowpage64_p43_slot_base(Hz5Lowpage64Segment* seg,
                                         uint32_t slot) {
  return (void*)((uintptr_t)seg->base +
                 (uintptr_t)slot * HZ5_LOWPAGE64_P43_SLOT_SIZE);
}

static int hz5_lowpage64_p43_slot_lookup_result(
    Hz5Lowpage64Segment* seg,
    uint32_t slot) {
  if (!seg || slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return HZ5_LOWPAGE64_LOOKUP_MISS;
  }

  uint32_t bit = hz5_lowpage64_p43_slot_bit(slot);
  uint32_t allocated =
      hz5_lowpage64_p43_mask_load(&seg->allocated_mask);
  uint32_t committed =
      hz5_lowpage64_p43_mask_load(&seg->committed_mask);
  uint32_t cold = hz5_lowpage64_p43_mask_load(&seg->cold_mask);
#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u
  uint32_t pending =
      hz5_lowpage64_p43_mask_load(&seg->pending_free_mask);
#else
  uint32_t pending = 0u;
#endif
  int active = ((allocated & bit) != 0u) &&
               ((committed & bit) != 0u) && ((cold & bit) == 0u) &&
               ((pending & bit) == 0u);
  return active ? HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE
                : HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE;
}

static void hz5_lowpage64_p43_count_lookup_result(int result,
                                                  size_t scanned) {
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_segments_scanned_total,
                    scanned);
  HZ5_P43_COUNT_MAX(g_hz5_lowpage64_p43_lookup_segments_scanned_max,
                    scanned);
  if (result == HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE) {
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_active, 1);
  } else if (result == HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE) {
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_nonactive, 1);
  } else {
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_miss, 1);
  }
  (void)scanned;
}

static void hz5_lowpage64_p43_fill_free_ctx(Hz5Lowpage64FreeCtx* ctx,
                                            int result,
                                            Hz5Lowpage64Segment* seg,
                                            uint32_t slot) {
  if (!ctx) {
    return;
  }
  ctx->lookup_kind = result;
  ctx->segment_token = NULL;
  ctx->slot_base = NULL;
  ctx->slot_size = 0;
  ctx->slot_index = 0;
  ctx->flags = 0;

  if (result == HZ5_LOWPAGE64_LOOKUP_MISS || !seg ||
      slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return;
  }

  ctx->segment_token = seg;
  ctx->slot_base = hz5_lowpage64_p43_slot_base(seg, slot);
  ctx->slot_size = HZ5_LOWPAGE64_P43_SLOT_SIZE;
  ctx->slot_index = slot;
}

#if HZ5_LOWPAGE64_P43_FAST_LOOKUP
static uint32_t hz5_lowpage64_p43_fast_lookup_index(uintptr_t p) {
  return (uint32_t)((p >> 16) &
                    ((uintptr_t)HZ5_LOWPAGE64_P43_FAST_LOOKUP_CAP - 1u));
}

static void hz5_lowpage64_p43_publish_fast_lookup_locked(
    Hz5Lowpage64Segment* seg) {
  uintptr_t base = (uintptr_t)seg->base;
  for (uint32_t i = 0; i < HZ5_LOWPAGE64_P43_SEGMENT_SIZE / 65536u; i++) {
    uint32_t idx =
        hz5_lowpage64_p43_fast_lookup_index(base + (uintptr_t)i * 65536u);
    atomic_store_explicit(&g_hz5_lowpage64_p43_fast_lookup[idx], seg,
                          memory_order_release);
  }
}

static void hz5_lowpage64_p43_unpublish_fast_lookup_locked(
    Hz5Lowpage64Segment* seg) {
  uintptr_t base = (uintptr_t)seg->base;
  for (uint32_t i = 0; i < HZ5_LOWPAGE64_P43_SEGMENT_SIZE / 65536u; i++) {
    uint32_t idx =
        hz5_lowpage64_p43_fast_lookup_index(base + (uintptr_t)i * 65536u);
    Hz5Lowpage64Segment* cur = atomic_load_explicit(
        &g_hz5_lowpage64_p43_fast_lookup[idx], memory_order_acquire);
    if (cur == seg) {
      atomic_store_explicit(&g_hz5_lowpage64_p43_fast_lookup[idx], NULL,
                            memory_order_release);
    }
  }
}

#if HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP
static int hz5_lowpage64_p43_lookup_fast_lockless(
    uintptr_t p,
    size_t* scanned_out,
    Hz5Lowpage64Segment** seg_out,
    uint32_t* slot_out) {
  Hz5Lowpage64Segment* hint =
      atomic_load_explicit(
          &g_hz5_lowpage64_p43_fast_lookup[
              hz5_lowpage64_p43_fast_lookup_index(p)],
          memory_order_acquire);
  uint32_t slot = 0;
  if (!hz5_lowpage64_p43_segment_contains(hint, p, &slot)) {
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_lockless_misses, 1);
    return HZ5_LOWPAGE64_LOOKUP_MISS;
  }

  if (scanned_out) {
    *scanned_out = 1;
  }
  if (seg_out) {
    *seg_out = hint;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_lockless_hits, 1);
#if HZ5_LOWPAGE64_P43_TRUST_FAST_LOOKUP
  return HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE;
#else
  return hz5_lowpage64_p43_slot_lookup_result(hint, slot);
#endif
}
#endif
#endif

static Hz5Lowpage64Segment* hz5_lowpage64_p43_find_segment_locked(
    void* raw,
    uint32_t* slot_out) {
  uintptr_t p = (uintptr_t)raw;
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP
  {
    Hz5Lowpage64Segment* hint =
        atomic_load_explicit(
            &g_hz5_lowpage64_p43_fast_lookup[
                hz5_lowpage64_p43_fast_lookup_index(p)],
            memory_order_acquire);
    if (hz5_lowpage64_p43_segment_contains(hint, p, slot_out)) {
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_find_fast_hits, 1);
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_find_segments_scanned_total, 1);
      HZ5_P43_COUNT_MAX(g_hz5_lowpage64_p43_find_segments_scanned_max, 1);
      return hint;
    }
  }
#endif
  size_t scanned = 0;
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
    scanned++;
    if (!hz5_lowpage64_p43_segment_contains(seg, p, slot_out)) {
      continue;
    }
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_find_segments_scanned_total,
                      scanned);
    HZ5_P43_COUNT_MAX(g_hz5_lowpage64_p43_find_segments_scanned_max,
                      scanned);
    return seg;
  }
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_find_segments_scanned_total,
                    scanned);
  HZ5_P43_COUNT_MAX(g_hz5_lowpage64_p43_find_segments_scanned_max,
                    scanned);
  (void)scanned;
  return NULL;
}

static void hz5_lowpage64_p43_unlink_segment_locked(
    Hz5Lowpage64Segment* target) {
  Hz5Lowpage64Segment** cur = &g_hz5_lowpage64_p43_segments;
  while (*cur) {
    if (*cur == target) {
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP
      hz5_lowpage64_p43_unpublish_fast_lookup_locked(target);
#endif
      *cur = target->next;
      return;
    }
    cur = &(*cur)->next;
  }
}

#if (HZ5_LOWPAGE64_P43_SLOT_DECOMMIT || \
     HZ5_LOWPAGE64_P43_PAGE_NOACCESS) && \
    HZ5_LOWPAGE64_P43_REWARM_BATCH > 1u
static uint32_t hz5_lowpage64_p43_first_set_bit(uint32_t mask) {
  uint32_t slot = 0;
  while (((mask >> slot) & 1u) == 0u) {
    slot++;
  }
  return slot;
}

static int hz5_lowpage64_p43_rewarm_slot(Hz5Lowpage64Segment* seg,
                                         uint32_t slot) {
  uint32_t bit = hz5_lowpage64_p43_slot_bit(slot);
  void* raw = hz5_lowpage64_p43_slot_base(seg, slot);
  hz5_lowpage64_p43_mask_or(&seg->allocated_mask, bit);
  hz5_lowpage64_p43_lock_leave();

#if HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  DWORD old_protect = 0;
  int ok = VirtualProtect(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE, PAGE_READWRITE,
                          &old_protect) != 0;
#else
  int ok = hz5_p43_os_commit(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE);
#endif

  hz5_lowpage64_p43_lock_enter();
  if (!ok) {
    hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commit_failures, 1);
    return 0;
  }

  hz5_lowpage64_p43_mask_and(&seg->cold_mask, ~bit);
  hz5_lowpage64_p43_mask_or(&seg->committed_mask, bit);
  hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commits, 1);
  return 1;
}

static void hz5_lowpage64_p43_rewarm_segment_neighbors(
    Hz5Lowpage64Segment* seg,
    uint32_t already_allocated_slot) {
  size_t warmed = 1u;
  while (warmed < (size_t)HZ5_LOWPAGE64_P43_REWARM_BATCH) {
    uint32_t candidates =
        hz5_lowpage64_p43_mask_load(&seg->cold_mask) &
        ~hz5_lowpage64_p43_mask_load(&seg->allocated_mask);
    candidates &= ~hz5_lowpage64_p43_slot_bit(already_allocated_slot);
    if (!candidates) {
      return;
    }

    uint32_t slot = hz5_lowpage64_p43_first_set_bit(candidates);
    if (!hz5_lowpage64_p43_rewarm_slot(seg, slot)) {
      return;
    }
    warmed++;
  }
}
#endif

#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS
static void hz5_lowpage64_p43_ref_clear(Hz5Lowpage64P43SlotRef* ref) {
  ref->seg = NULL;
  ref->slot = 0u;
}

#if !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u
static int hz5_lowpage64_p43_tls_push(Hz5Lowpage64Segment* seg,
                                      uint32_t slot);
#endif

static int hz5_lowpage64_p43_ref_valid(Hz5Lowpage64P43SlotRef ref) {
  return ref.seg != NULL;
}

static Hz5Lowpage64P43SlotRef hz5_lowpage64_p43_ref_next_locked(
    Hz5Lowpage64P43SlotRef ref) {
  Hz5Lowpage64P43SlotRef next = {0};
  if (!ref.seg || ref.slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return next;
  }
  return ref.seg->free_next[ref.slot];
}

static void hz5_lowpage64_p43_committed_push_locked(Hz5Lowpage64Segment* seg,
                                                    uint32_t slot) {
  if (!seg || slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return;
  }
  seg->free_next[slot] = g_hz5_lowpage64_p43_committed_head;
  g_hz5_lowpage64_p43_committed_head.seg = seg;
  g_hz5_lowpage64_p43_committed_head.slot = slot;
  g_hz5_lowpage64_p43_committed_count++;
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_global_pushes, 1);
}

#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
static void hz5_lowpage64_p43_release_ref_locked(
    Hz5Lowpage64P43SlotRef ref) {
  if (!ref.seg || ref.slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return;
  }
  uint32_t bit = hz5_lowpage64_p43_slot_bit(ref.slot);
#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u
  if ((hz5_lowpage64_p43_mask_load(&ref.seg->pending_free_mask) & bit) ==
      0u) {
    return;
  }
  hz5_lowpage64_p43_mask_and(&ref.seg->pending_free_mask, ~bit);
#endif
  if ((hz5_lowpage64_p43_mask_load(&ref.seg->allocated_mask) & bit) == 0u) {
    return;
  }
  hz5_lowpage64_p43_mask_and(&ref.seg->allocated_mask, ~bit);
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  if ((hz5_lowpage64_p43_mask_load(&ref.seg->committed_mask) & bit) != 0u &&
      (hz5_lowpage64_p43_mask_load(&ref.seg->cold_mask) & bit) == 0u &&
      hz5_lowpage64_p43_tls_push(ref.seg, ref.slot)) {
    return;
  }
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_tls_overflow_pushes, 1);
#endif
  if ((hz5_lowpage64_p43_mask_load(&ref.seg->committed_mask) & bit) != 0u &&
      (hz5_lowpage64_p43_mask_load(&ref.seg->cold_mask) & bit) == 0u) {
    hz5_lowpage64_p43_committed_push_locked(ref.seg, ref.slot);
  }
}
#endif

#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
static void hz5_lowpage64_p43_release_buffer_flush_locked(void) {
  uint32_t count = g_hz5_lowpage64_p43_release_buffer.count;
  for (uint32_t i = 0u; i < count; i++) {
    hz5_lowpage64_p43_release_ref_locked(
        g_hz5_lowpage64_p43_release_buffer.refs[i]);
    hz5_lowpage64_p43_ref_clear(
        &g_hz5_lowpage64_p43_release_buffer.refs[i]);
  }
  g_hz5_lowpage64_p43_release_buffer.count = 0u;
}

static int hz5_lowpage64_p43_release_buffer_push_locked(
    Hz5Lowpage64Segment* seg,
    uint32_t slot) {
  if (!seg || slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return 0;
  }
  if (g_hz5_lowpage64_p43_release_buffer.count >=
      (uint32_t)HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP) {
    hz5_lowpage64_p43_release_buffer_flush_locked();
  }
  if (g_hz5_lowpage64_p43_release_buffer.count >=
      (uint32_t)HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP) {
    return 0;
  }
  Hz5Lowpage64P43SlotRef* ref =
      &g_hz5_lowpage64_p43_release_buffer
           .refs[g_hz5_lowpage64_p43_release_buffer.count++];
  ref->seg = seg;
  ref->slot = slot;
  return 1;
}
#endif

static int hz5_lowpage64_p43_committed_pop_locked(
    Hz5Lowpage64P43SlotRef* out) {
  while (hz5_lowpage64_p43_ref_valid(g_hz5_lowpage64_p43_committed_head)) {
    Hz5Lowpage64P43SlotRef ref = g_hz5_lowpage64_p43_committed_head;
    g_hz5_lowpage64_p43_committed_head =
        hz5_lowpage64_p43_ref_next_locked(ref);
    if (g_hz5_lowpage64_p43_committed_count > 0u) {
      g_hz5_lowpage64_p43_committed_count--;
    }

    uint32_t bit = hz5_lowpage64_p43_slot_bit(ref.slot);
    if (!ref.seg || ref.slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
      continue;
    }
    if ((hz5_lowpage64_p43_mask_load(&ref.seg->allocated_mask) & bit) !=
        0u) {
      continue;
    }
    if ((hz5_lowpage64_p43_mask_load(&ref.seg->committed_mask) & bit) ==
        0u) {
      continue;
    }
    if ((hz5_lowpage64_p43_mask_load(&ref.seg->cold_mask) & bit) != 0u) {
      continue;
    }
    hz5_lowpage64_p43_mask_or(&ref.seg->allocated_mask, bit);
    hz5_lowpage64_p43_ref_clear(&ref.seg->free_next[ref.slot]);
    *out = ref;
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_global_hits, 1);
    return 1;
  }
  hz5_lowpage64_p43_ref_clear(&g_hz5_lowpage64_p43_committed_head);
  g_hz5_lowpage64_p43_committed_count = 0u;
  return 0;
}
#endif
#endif

#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
static int hz5_lowpage64_p43_tls_push(Hz5Lowpage64Segment* seg,
                                      uint32_t slot) {
  if (!seg || slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return 0;
  }
  if (g_hz5_lowpage64_p43_tls_cache.count >=
      (uint32_t)HZ5_LOWPAGE64_P43_TLS_CACHE_CAP) {
    return 0;
  }
  Hz5Lowpage64P43SlotRef* ref =
      &g_hz5_lowpage64_p43_tls_cache
           .refs[g_hz5_lowpage64_p43_tls_cache.count++];
  ref->seg = seg;
  ref->slot = slot;
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_tls_pushes, 1);
  return 1;
}

static int hz5_lowpage64_p43_tls_pop_locked(Hz5Lowpage64P43SlotRef* out) {
  while (g_hz5_lowpage64_p43_tls_cache.count > 0u) {
    Hz5Lowpage64P43SlotRef ref =
        g_hz5_lowpage64_p43_tls_cache
            .refs[--g_hz5_lowpage64_p43_tls_cache.count];
    if (!ref.seg || ref.slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
      continue;
    }
    uint32_t bit = hz5_lowpage64_p43_slot_bit(ref.slot);
    if ((hz5_lowpage64_p43_mask_load(&ref.seg->allocated_mask) & bit) !=
        0u) {
      continue;
    }
    if ((hz5_lowpage64_p43_mask_load(&ref.seg->committed_mask) & bit) ==
        0u) {
      continue;
    }
    if ((hz5_lowpage64_p43_mask_load(&ref.seg->cold_mask) & bit) != 0u) {
      continue;
    }
    hz5_lowpage64_p43_mask_or(&ref.seg->allocated_mask, bit);
    *out = ref;
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_tls_hits, 1);
    return 1;
  }
  return 0;
}
#endif
#endif

int hz5_lowpage64_p43_may_own(void* ptr) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  return hz5_lowpage64_p43_lookup(ptr) != HZ5_LOWPAGE64_LOOKUP_MISS;
#else
  (void)ptr;
  return 0;
#endif
}

int hz5_lowpage64_p43_active_owns(void* ptr) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  return hz5_lowpage64_p43_lookup(ptr) ==
         HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE;
#else
  (void)ptr;
  return 0;
#endif
}

static int hz5_lowpage64_p43_lookup_impl(void* ptr,
                                         Hz5Lowpage64FreeCtx* ctx) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  int result = HZ5_LOWPAGE64_LOOKUP_MISS;
  uintptr_t p = (uintptr_t)ptr;
  size_t scanned = 0;
  Hz5Lowpage64Segment* found_seg = NULL;
  uint32_t found_slot = 0;
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_calls, 1);
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP && HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP
  result = hz5_lowpage64_p43_lookup_fast_lockless(p, &scanned, &found_seg,
                                                  &found_slot);
  if (result != HZ5_LOWPAGE64_LOOKUP_MISS) {
    goto hz5_p43_lookup_count_result;
  }
#endif
  hz5_lowpage64_p43_lock_enter();
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP
  {
    uint32_t slot = 0;
    Hz5Lowpage64Segment* hint =
        atomic_load_explicit(
            &g_hz5_lowpage64_p43_fast_lookup[
                hz5_lowpage64_p43_fast_lookup_index(p)],
            memory_order_acquire);
    if (hz5_lowpage64_p43_segment_contains(hint, p, &slot)) {
      result = hz5_lowpage64_p43_slot_lookup_result(hint, slot);
      found_seg = hint;
      found_slot = slot;
      scanned = 1;
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_fast_hits, 1);
      goto hz5_p43_lookup_done;
    }
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_fast_misses, 1);
  }
#endif
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
    scanned++;
    uint32_t slot = 0;
    if (!hz5_lowpage64_p43_segment_contains(seg, p, &slot)) {
      continue;
    }
    result = hz5_lowpage64_p43_slot_lookup_result(seg, slot);
    found_seg = seg;
    found_slot = slot;
    break;
  }
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP
hz5_p43_lookup_done:
#endif
  hz5_lowpage64_p43_lock_leave();
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP && HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP
hz5_p43_lookup_count_result:
#endif
  hz5_lowpage64_p43_fill_free_ctx(ctx, result, found_seg, found_slot);
  hz5_lowpage64_p43_count_lookup_result(result, scanned);
  return result;
#else
  (void)ptr;
  if (ctx) {
    ctx->lookup_kind = HZ5_LOWPAGE64_LOOKUP_MISS;
  }
  return HZ5_LOWPAGE64_LOOKUP_MISS;
#endif
}

static int hz5_lowpage64_p43_lookup_raw_fast_impl(
    void* raw,
    Hz5Lowpage64FreeCtx* ctx) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS && HZ5_LOWPAGE64_P43_FAST_LOOKUP
  int result = HZ5_LOWPAGE64_LOOKUP_MISS;
  uintptr_t p = (uintptr_t)raw;
  size_t scanned = 0;
  Hz5Lowpage64Segment* found_seg = NULL;
  uint32_t found_slot = 0;
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_calls, 1);
  Hz5Lowpage64Segment* hint =
      atomic_load_explicit(
          &g_hz5_lowpage64_p43_fast_lookup[
              hz5_lowpage64_p43_fast_lookup_index(p)],
          memory_order_acquire);
  uint32_t slot = 0;
  if (hz5_lowpage64_p43_segment_contains(hint, p, &slot) &&
      hz5_lowpage64_p43_slot_base(hint, slot) == raw) {
#if HZ5_LOWPAGE64_P43_RAW_ALLOCATED_LOOKUP_ONLY
    uint32_t bit = hz5_lowpage64_p43_slot_bit(slot);
    result = (hz5_lowpage64_p43_mask_load(&hint->allocated_mask) & bit)
                 ? HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE
                 : HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE;
#else
    result = hz5_lowpage64_p43_slot_lookup_result(hint, slot);
#endif
    found_seg = hint;
    found_slot = slot;
    scanned = 1;
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_fast_hits, 1);
  } else {
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_fast_misses, 1);
  }
  hz5_lowpage64_p43_fill_free_ctx(ctx, result, found_seg, found_slot);
  hz5_lowpage64_p43_count_lookup_result(result, scanned);
  return result;
#else
  return hz5_lowpage64_p43_lookup_impl(raw, ctx);
#endif
}

int hz5_lowpage64_p43_lookup(void* ptr) {
  return hz5_lowpage64_p43_lookup_impl(ptr, NULL);
}

int hz5_lowpage64_p43_prepare_free_user(void* ptr,
                                        Hz5Lowpage64FreeCtx* ctx) {
  return hz5_lowpage64_p43_lookup_impl(ptr, ctx);
}

int hz5_lowpage64_p43_prepare_free_raw(void* raw,
                                       Hz5Lowpage64FreeCtx* ctx) {
#if HZ5_LOWPAGE64_P43_RAW_FAST_LOOKUP_ONLY
  return hz5_lowpage64_p43_lookup_raw_fast_impl(raw, ctx);
#else
  return hz5_lowpage64_p43_lookup_impl(raw, ctx);
#endif
}

int hz5_lowpage64_p43_release_token(void* segment_token,
                                    uint32_t slot_index,
                                    void* raw) {
  Hz5Lowpage64FreeCtx ctx;
  ctx.lookup_kind = HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE;
  ctx.segment_token = segment_token;
  ctx.slot_base = raw;
