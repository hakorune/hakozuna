void free(void* ptr) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_calls);
  if (!ptr) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_null);
    return;
  }
  if (g_hz6_preload_reentry) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_reentry_real);
    hz6_preload_real_free(ptr);
    return;
  }
#if HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1 || \
    HZ6_PRELOAD_CALLOC_REAL_FREE_SKIP_L1
  if (atomic_load_explicit(&g_hz6_preload_real_aligned_count,
                           memory_order_relaxed) != 0) {
    unsigned char real_ptr_kind = hz6_preload_real_pointer_forget(ptr);
    if (real_ptr_kind != 0) {
      if (real_ptr_kind == HZ6_PRELOAD_REAL_PTR_KIND_CALLOC) {
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.calloc_real_free_skip_hit);
      } else {
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.real_aligned_free_skip_hit);
      }
      hz6_preload_real_free(ptr);
      return;
    }
#if HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.real_aligned_free_skip_miss);
#endif
#if HZ6_PRELOAD_CALLOC_REAL_FREE_SKIP_L1
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.calloc_real_free_skip_miss);
#endif
  }
#endif
  g_hz6_preload_reentry = 1;
  Hz6Allocator* allocator = hz6_preload_allocator();
  uint16_t page_kind = hz6_preload_page_kind_selector_probe(allocator, ptr);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
  int midpage_hint_would_first = hz6_preload_midpage_hint_maybe(ptr);
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_midpage_hint_probe);
  if (midpage_hint_would_first) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_hint_would_first);
  }
#endif
#if HZ6_PRELOAD_FOREIGN_ROUTE_BEFORE_MAPS_L1
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_route_before_maps_attempt);
  Hz6PreloadRoute before_maps_route = hz6_preload_route(allocator, ptr);
  if (before_maps_route.resolve_kind == HZ6_FREE_ROUTE_FOREIGN_VALID &&
      before_maps_route.route.kind == HZ6_ROUTE_VALID &&
      before_maps_route.visible_hit) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_route_before_maps_foreign_dispatch);
    hz6_free_with_resolved_route_after_maps(allocator, ptr,
                                            before_maps_route.route,
                                            before_maps_route.visible_hit);
    g_hz6_preload_reentry = 0;
    return;
  }
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_route_before_maps_fallback);
#endif
#if HZ6_PRELOAD_FREE_TOY_PAGE_SKIP_MIDPAGE_L1
  if (page_kind == HZ6_PAGE_KIND_TOY) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
    goto hz6_preload_after_active_maps;
  }
#endif
#if HZ6_PAGE_KIND_FREE_SELECTOR_FIRST_L1
  if (page_kind == HZ6_PAGE_KIND_MIDPAGE) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
  } else if (page_kind == HZ6_PAGE_KIND_TOY) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  } else if (HZ6_PRELOAD_UNLIKELY(
                 hz6_preload_midpage_current_bias_first(allocator))) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  }
#elif HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1
  if (midpage_hint_would_first) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_true_midpage);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_false_positive);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_missed_midpage);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  }
#elif HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
  if (hz6_midpage_active_map_try_free(allocator, ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
    hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
    g_hz6_preload_reentry = 0;
    return;
  }
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
  if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_hit);
    hz6_preload_page_kind_selector_note_toy_hit(page_kind);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
    if (midpage_hint_would_first) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_false_positive);
    }
#endif
    g_hz6_preload_reentry = 0;
    return;
  }
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_toy_active_map_miss);
#elif HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1
  if (hz6_midpage_active_map_aligned(ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  }
#elif HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1
  if (HZ6_PRELOAD_UNLIKELY(
          hz6_preload_midpage_current_bias_first(allocator))) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
    if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_toy_active_map_hit);
      hz6_preload_page_kind_selector_note_toy_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_miss);
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
    if (hz6_midpage_active_map_try_free(allocator, ptr)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
      hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
  }
#else
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_toy_active_map_attempt);
  if (hz6_toy_small_active_map_try_free(allocator, ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_toy_active_map_hit);
    hz6_preload_page_kind_selector_note_toy_hit(page_kind);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
    if (midpage_hint_would_first) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_false_positive);
    }
#endif
    g_hz6_preload_reentry = 0;
    return;
  }
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_toy_active_map_miss);
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_midpage_active_map_attempt);
  if (hz6_midpage_active_map_try_free(allocator, ptr)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_midpage_active_map_hit);
    hz6_preload_page_kind_selector_note_midpage_hit(page_kind);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
    if (midpage_hint_would_first) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_true_midpage);
    } else {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_midpage_hint_missed_midpage);
    }
#endif
    g_hz6_preload_reentry = 0;
    return;
  }
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_midpage_active_map_miss);
#endif
#if HZ6_PRELOAD_FREE_TOY_PAGE_SKIP_MIDPAGE_L1
hz6_preload_after_active_maps:
#endif
#if HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1 || \
    HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1
  int source_run_route_prechecked = 0;
  Hz6PreloadRoute source_run_preload_route = {0};
  source_run_preload_route.route = hz6_route_miss();
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_source_run_route_attempt);
  source_run_preload_route.route =
      hz6_allocator_source_block_route_lookup(allocator, ptr);
  if (source_run_preload_route.route.kind == HZ6_ROUTE_VALID &&
      source_run_preload_route.route.route_allocator == allocator &&
      source_run_preload_route.route.front_id == HZ6_FRONT_MIDPAGE &&
      (source_run_preload_route.route.class_id == HZ6_MIDPAGE_8K_CLASS_ID ||
       source_run_preload_route.route.class_id == HZ6_MIDPAGE_32K_CLASS_ID)) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_source_run_route_hit);
    if (source_run_preload_route.route.class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_source_run_route_midpage8_hit);
    } else {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_source_run_route_midpage32_hit);
    }
#if HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1
    source_run_route_prechecked = 1;
#endif
  } else if (source_run_preload_route.route.kind == HZ6_ROUTE_INVALID) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_source_run_route_invalid);
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.free_source_run_route_fallback);
  }
#endif
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.free_route_lookup_after_maps);
#if HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1 || \
    HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1
  Hz6PreloadRoute preload_route =
      source_run_route_prechecked ? source_run_preload_route
                                  : hz6_preload_route(allocator, ptr);
#else
  Hz6PreloadRoute preload_route = hz6_preload_route(allocator, ptr);
#endif
  if (preload_route.route.kind == HZ6_ROUTE_VALID ||
      preload_route.route.kind == HZ6_ROUTE_INVALID) {
#if HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1
    int midpage_fast_free = 0;
#endif
    if (preload_route.route.kind == HZ6_ROUTE_VALID) {
      if (preload_route.visible_hit) {
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_visible_route_hit);
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_route_valid_foreign_visible);
      } else {
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_local_route_valid);
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_route_valid_owned);
        if (preload_route.route.route_allocator == allocator) {
          hz6_preload_phase_count(
              &g_hz6_preload_phase_stats.free_prechecked_candidate);
#if HZ6_DIAGNOSTIC_PROBES
          if (preload_route.route.front_id == HZ6_FRONT_MIDPAGE) {
            if (preload_route.route.class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
              ++allocator->stats.midpage_8k_preload_local_route_valid;
            } else if (preload_route.route.class_id ==
                       HZ6_MIDPAGE_32K_CLASS_ID) {
              ++allocator->stats.midpage_32k_preload_local_route_valid;
            }
          }
#endif
#if HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1
          int midpage_fast_free_class_allowed =
              preload_route.route.class_id <=
              HZ6_PRELOAD_MIDPAGE_FAST_FREE_MAX_CLASS;
#if HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS > 0
          midpage_fast_free_class_allowed =
              midpage_fast_free_class_allowed &&
              preload_route.route.class_id >=
                  HZ6_PRELOAD_MIDPAGE_FAST_FREE_MIN_CLASS;
#endif
          midpage_fast_free =
              midpage_fast_free_class_allowed &&
              hz6_midpage_active_map_eligible(preload_route.route.front_id,
                                              preload_route.route.class_id);
#endif
#if HZ6_PRELOAD_MIDPAGE_ROUTE_REARM_L1
          hz6_midpage_active_map_register(
              allocator, preload_route.route.front_id,
              preload_route.route.class_id, ptr,
              (Hz6ObjectDescriptor*)preload_route.route.descriptor);
#endif
        }
      }
    } else {
      hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_route_invalid);
    }
    int preload_route_dispatched = 0;
#if HZ6_PRELOAD_FOREIGN_RESOLVED_DISPATCH_L1
    if (preload_route.resolve_kind == HZ6_FREE_ROUTE_FOREIGN_VALID &&
        preload_route.route.kind == HZ6_ROUTE_VALID &&
        preload_route.visible_hit) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_resolved_foreign_direct_dispatch);
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats
               .free_resolved_duplicate_route_lookup_avoided);
      hz6_free_with_resolved_route_after_maps(allocator, ptr,
                                              preload_route.route,
                                              preload_route.visible_hit);
      preload_route_dispatched = 1;
    }
#endif
    if (!preload_route_dispatched) {
#if HZ6_PRELOAD_FAST_FREE_L1
      hz6_free_with_route_prechecked(allocator, ptr, preload_route.route,
                                     preload_route.visible_hit);
#elif HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1 || \
    HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1
      if (source_run_route_prechecked) {
        hz6_preload_phase_count(
            &g_hz6_preload_phase_stats.free_source_run_route_prechecked);
        hz6_free_with_route_prechecked(allocator, ptr, preload_route.route, 0);
      } else {
        hz6_free(allocator, ptr);
      }
#elif HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1
      if (midpage_fast_free) {
        hz6_free_with_route_prechecked(allocator, ptr, preload_route.route,
                                       preload_route.visible_hit);
      } else {
        hz6_free(allocator, ptr);
      }
#else
      hz6_free(allocator, ptr);
#endif
    }
  } else {
    hz6_preload_route_fail_fast_if_integrity(preload_route);
    if (!hz6_preload_route_real_fallback_allowed(preload_route)) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.free_route_real_free_unproven);
      g_hz6_preload_reentry = 0;
      return;
    }
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.free_route_miss_real);
    hz6_preload_real_free(ptr);
  }
  g_hz6_preload_reentry = 0;
}

void* calloc(size_t nmemb, size_t size) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.calloc_calls);
  if (size != 0 && nmemb > ((size_t)-1) / size) {
    errno = ENOMEM;
    return NULL;
  }
  size_t bytes = nmemb * size;
  (void)bytes;
  hz6_preload_phase_add(&g_hz6_preload_phase_stats.calloc_zero_bytes,
                        bytes);
  hz6_preload_phase_count_size_bucket(
      bytes,
      &g_hz6_preload_phase_stats.calloc_size_zero,
      &g_hz6_preload_phase_stats.calloc_size_le1024,
      &g_hz6_preload_phase_stats.calloc_size_1025_4096,
      &g_hz6_preload_phase_stats.calloc_size_4097_16384,
      &g_hz6_preload_phase_stats.calloc_size_gt16384);
  if (g_hz6_preload_reentry) {
    return hz6_preload_real_calloc(nmemb, size);
  }
#if HZ6_PRELOAD_CALLOC_REAL_FALLBACK_L1
#if HZ6_PRELOAD_CALLOC_REAL_MIN_BYTES > 0
  if (bytes < (size_t)HZ6_PRELOAD_CALLOC_REAL_MIN_BYTES) {
    void* ptr = hz6_preload_calloc_malloc_bytes(bytes);
    if (ptr) {
      memset(ptr, 0, bytes);
    }
    return ptr;
  }
#endif
  void* ptr = hz6_preload_real_calloc(nmemb, size);
#if HZ6_PRELOAD_CALLOC_REAL_FREE_SKIP_L1
  (void)hz6_preload_real_pointer_record(ptr, HZ6_PRELOAD_REAL_PTR_KIND_CALLOC);
#endif
  return ptr;
#else
  void* ptr = hz6_preload_calloc_malloc_bytes(bytes);
  if (ptr) {
    memset(ptr, 0, bytes);
  }
  return ptr;
#endif
}

void* realloc(void* ptr, size_t size) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.realloc_calls);
  hz6_preload_phase_count_size_bucket(
      size,
      &g_hz6_preload_phase_stats.realloc_request_zero,
      &g_hz6_preload_phase_stats.realloc_request_le1024,
      &g_hz6_preload_phase_stats.realloc_request_1025_4096,
      &g_hz6_preload_phase_stats.realloc_request_4097_16384,
      &g_hz6_preload_phase_stats.realloc_request_gt16384);
  if (!ptr) {
    return malloc(size);
  }
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  if (g_hz6_preload_reentry) {
    return hz6_preload_real_realloc(ptr, size);
  }

  g_hz6_preload_reentry = 1;
  Hz6Allocator* allocator = hz6_preload_allocator();
  Hz6PreloadRoute preload_route = hz6_preload_route(allocator, ptr);
  size_t old_size = 0;
  if (preload_route.route.kind == HZ6_ROUTE_VALID &&
      preload_route.route.descriptor) {
    const Hz6ObjectDescriptor* descriptor =
        (const Hz6ObjectDescriptor*)preload_route.route.descriptor;
    old_size = descriptor->bytes;
  }
  g_hz6_preload_reentry = 0;
  if (old_size == 0) {
    hz6_preload_route_fail_fast_if_integrity(preload_route);
    if (!hz6_preload_route_real_fallback_allowed(preload_route)) {
      return NULL;
    }
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.realloc_real_fallback);
    return hz6_preload_real_realloc(ptr, size);
  }
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.realloc_owned);
  hz6_preload_phase_count_size_bucket(
      old_size,
      &g_hz6_preload_phase_stats.realloc_owned_old_zero,
      &g_hz6_preload_phase_stats.realloc_owned_old_le1024,
      &g_hz6_preload_phase_stats.realloc_owned_old_1025_4096,
      &g_hz6_preload_phase_stats.realloc_owned_old_4097_16384,
      &g_hz6_preload_phase_stats.realloc_owned_old_gt16384);
#if HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_4K_L1
  if (old_size == 4096u && size > 4096u && size <= HZ6_MIDPAGE_8K_BYTES) {
    g_hz6_preload_realloc_boundary_adapt_4k = 1u;
  }
#endif
#if HZ6_PRELOAD_REALLOC_BOUNDARY_ADAPTIVE_8K_L1
  if (old_size == HZ6_MIDPAGE_8K_BYTES &&
      size > HZ6_MIDPAGE_8K_BYTES && size <= HZ6_MIDPAGE_32K_BYTES) {
    g_hz6_preload_realloc_boundary_adapt_8k = 1u;
  }
#endif
#if HZ6_PRELOAD_REALLOC_IN_PLACE_L1
  if (size <= old_size) {
    hz6_preload_phase_count(&g_hz6_preload_phase_stats.realloc_in_place);
    return ptr;
  }
#endif

  void* next = malloc(size);
  if (!next) {
    return NULL;
  }
  size_t copy_bytes = old_size < size ? old_size : size;
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.realloc_copy_calls);
  hz6_preload_phase_add(&g_hz6_preload_phase_stats.realloc_copy_bytes,
                        copy_bytes);
  hz6_preload_note_realloc_copy_class(preload_route, size);
  memcpy(next, ptr, copy_bytes);
  free(ptr);
  return next;
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.posix_memalign_calls);
  hz6_preload_phase_count_alignment_bucket(
      alignment,
      &g_hz6_preload_phase_stats.posix_memalign_align_le16,
      &g_hz6_preload_phase_stats.posix_memalign_align_17_64,
      &g_hz6_preload_phase_stats.posix_memalign_align_65_4096,
      &g_hz6_preload_phase_stats.posix_memalign_align_gt4096);
  hz6_preload_phase_count_size_bucket(
      size,
      &g_hz6_preload_phase_stats.posix_memalign_size_zero,
      &g_hz6_preload_phase_stats.posix_memalign_size_le1024,
      &g_hz6_preload_phase_stats.posix_memalign_size_1025_4096,
      &g_hz6_preload_phase_stats.posix_memalign_size_4097_16384,
      &g_hz6_preload_phase_stats.posix_memalign_size_gt16384);
  if ((alignment & (alignment - 1u)) != 0 || alignment < sizeof(void*)) {
    return EINVAL;
  }
  if (alignment <= 16u) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.posix_memalign_hz6_path);
    void* ptr = malloc(size);
    if (!ptr) {
      return ENOMEM;
    }
    *memptr = ptr;
    return 0;
  }
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.posix_memalign_real_fallback);
  int rc = hz6_preload_real_posix_memalign(memptr, alignment, size);
#if HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1
  if (rc == 0 && *memptr) {
    (void)hz6_preload_real_pointer_record(
        *memptr, HZ6_PRELOAD_REAL_PTR_KIND_ALIGNED);
  }
#endif
  return rc;
}

void* aligned_alloc(size_t alignment, size_t size) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.aligned_alloc_calls);
  hz6_preload_phase_count_alignment_bucket(
      alignment,
      &g_hz6_preload_phase_stats.aligned_alloc_align_le16,
      &g_hz6_preload_phase_stats.aligned_alloc_align_17_64,
      &g_hz6_preload_phase_stats.aligned_alloc_align_65_4096,
      &g_hz6_preload_phase_stats.aligned_alloc_align_gt4096);
  hz6_preload_phase_count_size_bucket(
      size,
      &g_hz6_preload_phase_stats.aligned_alloc_size_zero,
      &g_hz6_preload_phase_stats.aligned_alloc_size_le1024,
      &g_hz6_preload_phase_stats.aligned_alloc_size_1025_4096,
      &g_hz6_preload_phase_stats.aligned_alloc_size_4097_16384,
      &g_hz6_preload_phase_stats.aligned_alloc_size_gt16384);
  if (alignment <= 16u) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.aligned_alloc_hz6_path);
    return malloc(size);
  }
  if (g_hz6_preload_reentry) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.aligned_alloc_real_fallback);
    return hz6_preload_real_aligned_alloc(alignment, size);
  }
  void* ptr = NULL;
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.aligned_alloc_real_fallback);
  if (hz6_preload_real_posix_memalign(&ptr, alignment, size) != 0) {
    return NULL;
  }
#if HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1
  (void)hz6_preload_real_pointer_record(
      ptr, HZ6_PRELOAD_REAL_PTR_KIND_ALIGNED);
#endif
  return ptr;
}

size_t malloc_usable_size(void* ptr) {
  hz6_preload_phase_count(&g_hz6_preload_phase_stats.malloc_usable_size_calls);
  if (!ptr) {
    return 0;
  }
  if (!g_hz6_preload_reentry) {
    g_hz6_preload_reentry = 1;
    Hz6Allocator* allocator = hz6_preload_allocator();
    Hz6PreloadRoute preload_route = hz6_preload_route(allocator, ptr);
    size_t bytes = hz6_preload_usable_size_from_route(preload_route);
    g_hz6_preload_reentry = 0;
    if (bytes != 0) {
      hz6_preload_phase_count(
          &g_hz6_preload_phase_stats.malloc_usable_size_owned);
      return bytes;
    }
    hz6_preload_route_fail_fast_if_integrity(preload_route);
    if (!hz6_preload_route_real_fallback_allowed(preload_route)) {
      return 0;
    }
  }
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.malloc_usable_size_real_fallback);
  return hz6_preload_real_malloc_usable_size(ptr);
}

int malloc_trim(size_t pad) {
  if (g_hz6_preload_reentry) {
    return hz6_preload_real_malloc_trim(pad);
  }

  int hz6_released = hz6_preload_quiescent_release(0);
  int saved_reentry = g_hz6_preload_reentry;
  g_hz6_preload_reentry = 1;
  int real_released = hz6_preload_real_malloc_trim(pad);
  g_hz6_preload_reentry = saved_reentry;
  return hz6_released || real_released != 0;
}
