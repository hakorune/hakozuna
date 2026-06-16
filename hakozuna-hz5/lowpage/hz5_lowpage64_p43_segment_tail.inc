// LowPage P43 segment stats snapshot tail.

void hz5_lowpage64_p43_stats_snapshot(
    Hz5Lowpage64P43StatsSnapshot* snapshot) {
  if (!snapshot) {
    return;
  }
#if HZ5_LOWPAGE64_STATS
  snapshot->segments_reserved = atomic_load_explicit(
      &g_hz5_lowpage64_p43_segments_reserved, memory_order_relaxed);
  snapshot->segments_released = atomic_load_explicit(
      &g_hz5_lowpage64_p43_segments_released, memory_order_relaxed);
  snapshot->slot_commits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_slot_commits, memory_order_relaxed);
  snapshot->slot_commit_failures = atomic_load_explicit(
      &g_hz5_lowpage64_p43_slot_commit_failures, memory_order_relaxed);
  snapshot->slot_decommits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_slot_decommits, memory_order_relaxed);
  snapshot->slot_decommit_failures = atomic_load_explicit(
      &g_hz5_lowpage64_p43_slot_decommit_failures, memory_order_relaxed);
  snapshot->tls_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_tls_hits, memory_order_relaxed);
  snapshot->tls_pushes = atomic_load_explicit(
      &g_hz5_lowpage64_p43_tls_pushes, memory_order_relaxed);
  snapshot->tls_overflow_pushes = atomic_load_explicit(
      &g_hz5_lowpage64_p43_tls_overflow_pushes, memory_order_relaxed);
  snapshot->global_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_global_hits, memory_order_relaxed);
  snapshot->global_pushes = atomic_load_explicit(
      &g_hz5_lowpage64_p43_global_pushes, memory_order_relaxed);
  snapshot->cold_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_cold_hits, memory_order_relaxed);
  snapshot->free_slot_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_free_slot_hits, memory_order_relaxed);
  snapshot->source_alloc_calls = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_alloc_calls, memory_order_relaxed);
  snapshot->source_cold_segment_scans_total = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_cold_segment_scans_total,
      memory_order_relaxed);
  snapshot->source_cold_segment_scans_max = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_cold_segment_scans_max,
      memory_order_relaxed);
  snapshot->source_free_segment_scans_total = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_free_segment_scans_total,
      memory_order_relaxed);
  snapshot->source_free_segment_scans_max = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_free_segment_scans_max,
      memory_order_relaxed);
  snapshot->source_new_segment_scan_total = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_new_segment_scan_total,
      memory_order_relaxed);
  snapshot->source_new_segment_scan_max = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_new_segment_scan_max,
      memory_order_relaxed);
  snapshot->lookup_calls = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_calls, memory_order_relaxed);
  snapshot->lookup_active = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_active, memory_order_relaxed);
  snapshot->lookup_nonactive = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_nonactive, memory_order_relaxed);
  snapshot->lookup_miss = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_miss, memory_order_relaxed);
  snapshot->lookup_segments_scanned_total = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_segments_scanned_total,
      memory_order_relaxed);
  snapshot->lookup_segments_scanned_max = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_segments_scanned_max,
      memory_order_relaxed);
  snapshot->lookup_fast_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_fast_hits, memory_order_relaxed);
  snapshot->lookup_fast_misses = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_fast_misses, memory_order_relaxed);
  snapshot->lookup_lockless_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_lockless_hits, memory_order_relaxed);
  snapshot->lookup_lockless_misses = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_lockless_misses, memory_order_relaxed);
  snapshot->find_fast_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_find_fast_hits, memory_order_relaxed);
  snapshot->find_segments_scanned_total = atomic_load_explicit(
      &g_hz5_lowpage64_p43_find_segments_scanned_total,
      memory_order_relaxed);
  snapshot->find_segments_scanned_max = atomic_load_explicit(
      &g_hz5_lowpage64_p43_find_segments_scanned_max, memory_order_relaxed);
  snapshot->va_allocs = atomic_load_explicit(
      &g_hz5_lowpage64_p43_va_allocs, memory_order_relaxed);
  snapshot->va_alloc_failures = atomic_load_explicit(
      &g_hz5_lowpage64_p43_va_alloc_failures, memory_order_relaxed);
  snapshot->va_releases = atomic_load_explicit(
      &g_hz5_lowpage64_p43_va_releases, memory_order_relaxed);
#else
  snapshot->segments_reserved = 0;
  snapshot->segments_released = 0;
  snapshot->slot_commits = 0;
  snapshot->slot_commit_failures = 0;
  snapshot->slot_decommits = 0;
  snapshot->slot_decommit_failures = 0;
  snapshot->tls_hits = 0;
  snapshot->tls_pushes = 0;
  snapshot->tls_overflow_pushes = 0;
  snapshot->global_hits = 0;
  snapshot->global_pushes = 0;
  snapshot->cold_hits = 0;
  snapshot->free_slot_hits = 0;
  snapshot->source_alloc_calls = 0;
  snapshot->source_cold_segment_scans_total = 0;
  snapshot->source_cold_segment_scans_max = 0;
  snapshot->source_free_segment_scans_total = 0;
  snapshot->source_free_segment_scans_max = 0;
  snapshot->source_new_segment_scan_total = 0;
  snapshot->source_new_segment_scan_max = 0;
  snapshot->lookup_calls = 0;
  snapshot->lookup_active = 0;
  snapshot->lookup_nonactive = 0;
  snapshot->lookup_miss = 0;
  snapshot->lookup_segments_scanned_total = 0;
  snapshot->lookup_segments_scanned_max = 0;
  snapshot->lookup_fast_hits = 0;
  snapshot->lookup_fast_misses = 0;
  snapshot->lookup_lockless_hits = 0;
  snapshot->lookup_lockless_misses = 0;
  snapshot->find_fast_hits = 0;
  snapshot->find_segments_scanned_total = 0;
  snapshot->find_segments_scanned_max = 0;
  snapshot->va_allocs = 0;
  snapshot->va_alloc_failures = 0;
  snapshot->va_releases = 0;
#endif
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  snapshot->contract_lockless_enabled =
      (uint32_t)HZ5_LOWPAGE64_P43_LOCKLESS_CONTRACT;
  snapshot->lockless_lookup_enabled =
      (uint32_t)HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP;
  snapshot->slot_decommit_enabled =
      (uint32_t)HZ5_LOWPAGE64_P43_SLOT_DECOMMIT;
  snapshot->page_noaccess_enabled =
      (uint32_t)HZ5_LOWPAGE64_P43_PAGE_NOACCESS;
  snapshot->runtime_segment_release_enabled = 0;
  hz5_lowpage64_p43_lock_enter();
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
    uint32_t allocated = hz5_lowpage64_p43_mask_load(&seg->allocated_mask);
    uint32_t committed = hz5_lowpage64_p43_mask_load(&seg->committed_mask);
    uint32_t cold = hz5_lowpage64_p43_mask_load(&seg->cold_mask);
    uint32_t active = allocated & committed & ~cold;
    uint32_t committed_free = committed & ~allocated & ~cold;

    snapshot->segments_current++;
    snapshot->slots_total_current += HZ5_LOWPAGE64_P43_SLOT_COUNT;
    snapshot->slots_active_current +=
        hz5_lowpage64_p43_popcount32(active);
    snapshot->slots_committed_current +=
        hz5_lowpage64_p43_popcount32(committed);
    snapshot->slots_committed_free_current +=
        hz5_lowpage64_p43_popcount32(committed_free);
    snapshot->slots_cold_current += hz5_lowpage64_p43_popcount32(cold);
    if (HZ5_LOWPAGE64_P43_SLOT_COUNT >=
        hz5_lowpage64_p43_popcount32(committed)) {
      snapshot->slots_uncommitted_current +=
          HZ5_LOWPAGE64_P43_SLOT_COUNT -
          hz5_lowpage64_p43_popcount32(committed);
    }
  }
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS && !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  snapshot->slots_global_retained_current =
      g_hz5_lowpage64_p43_committed_count;
#endif
  hz5_lowpage64_p43_lock_leave();
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  snapshot->slots_tls_retained_current =
      g_hz5_lowpage64_p43_tls_cache.count;
#endif
#else
  snapshot->segments_current = 0;
  snapshot->slots_total_current = 0;
  snapshot->slots_active_current = 0;
  snapshot->slots_committed_current = 0;
  snapshot->slots_committed_free_current = 0;
  snapshot->slots_global_retained_current = 0;
  snapshot->slots_tls_retained_current = 0;
  snapshot->slots_cold_current = 0;
  snapshot->slots_uncommitted_current = 0;
  snapshot->contract_lockless_enabled = 0;
  snapshot->lockless_lookup_enabled = 0;
  snapshot->slot_decommit_enabled = 0;
  snapshot->page_noaccess_enabled = 0;
  snapshot->runtime_segment_release_enabled = 0;
#endif
}

#endif  /* HZ5_LOWPAGE64_P43_SEGMENT_SLOTS */

#if !HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
void hz5_lowpage64_p43_stats_snapshot(
    Hz5Lowpage64P43StatsSnapshot* snapshot) {
  if (!snapshot) {
    return;
  }
  *snapshot = (Hz5Lowpage64P43StatsSnapshot){0};
}
#endif
