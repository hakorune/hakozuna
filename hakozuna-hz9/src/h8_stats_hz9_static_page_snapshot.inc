  H8_DEBUG_LOAD(h9_static_page_shadow_alloc_seen);
  H8_DEBUG_LOAD(h9_static_page_shadow_alloc_hit_possible);
  H8_DEBUG_LOAD(h9_static_page_shadow_local_free_seen);
  H8_DEBUG_LOAD(h9_static_page_shadow_local_free_store_possible);
  H8_DEBUG_LOAD(h9_static_page_shadow_local_free_not_active);
  H8_DEBUG_LOAD(h9_static_page_shadow_local_free_store_blocked);
  H8_DEBUG_LOAD(h9_static_page_shadow_remote_seen);
  H8_DEBUG_LOAD(h9_static_page_shadow_remote_after_local);
  H8_DEBUG_LOAD(h9_static_page_shadow_remote_class_disable);
  H8_DEBUG_LOAD(h9_static_page_shadow_alloc_disabled);
  H8_DEBUG_LOAD(h9_static_page_shadow_local_free_disabled);
  H8_DEBUG_LOAD(h9_static_page_shadow_phase_open);
  H8_DEBUG_LOAD(h9_static_page_shadow_phase_remote_close);
  H8_DEBUG_LOAD(h9_static_page_shadow_phase_alloc_closed);
  H8_DEBUG_LOAD(h9_static_page_shadow_phase_free_closed);
  H8_DEBUG_LOAD(h9_static_page_shadow_phase_hit_possible);
  H8_DEBUG_LOAD(h9_static_page_shadow_phase_store_possible);
  for (size_t i = 0; i < 6u; ++i) {
    out->h9_static_page_shadow_alloc_class[i] = atomic_load_explicit(
        &h8g.h9_static_page_shadow_alloc_class[i], memory_order_acquire);
    out->h9_static_page_shadow_hit_class[i] = atomic_load_explicit(
        &h8g.h9_static_page_shadow_hit_class[i], memory_order_acquire);
    out->h9_static_page_shadow_free_class[i] = atomic_load_explicit(
        &h8g.h9_static_page_shadow_free_class[i], memory_order_acquire);
    out->h9_static_page_shadow_remote_class[i] = atomic_load_explicit(
        &h8g.h9_static_page_shadow_remote_class[i], memory_order_acquire);
  }
