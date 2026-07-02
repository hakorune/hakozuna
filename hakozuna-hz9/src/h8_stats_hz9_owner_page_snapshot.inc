  out->h9_owner_page_shadow_alloc_seen = atomic_load_explicit(
      &h8g.h9_owner_page_shadow_alloc_seen, memory_order_acquire);
  out->h9_owner_page_shadow_alloc_active_owner = atomic_load_explicit(
      &h8g.h9_owner_page_shadow_alloc_active_owner, memory_order_acquire);
  out->h9_owner_page_shadow_alloc_active_nonfull = atomic_load_explicit(
      &h8g.h9_owner_page_shadow_alloc_active_nonfull, memory_order_acquire);
  out->h9_owner_page_shadow_local_free_seen = atomic_load_explicit(
      &h8g.h9_owner_page_shadow_local_free_seen, memory_order_acquire);
  out->h9_owner_page_shadow_remote_free_seen = atomic_load_explicit(
      &h8g.h9_owner_page_shadow_remote_free_seen, memory_order_acquire);
  out->h9_owner_page_shadow_pure_local_candidate = atomic_load_explicit(
      &h8g.h9_owner_page_shadow_pure_local_candidate, memory_order_acquire);
  for (size_t i = 0; i < 6; ++i) {
    out->h9_owner_page_shadow_alloc_class[i] = atomic_load_explicit(
        &h8g.h9_owner_page_shadow_alloc_class[i], memory_order_acquire);
    out->h9_owner_page_shadow_local_free_class[i] = atomic_load_explicit(
        &h8g.h9_owner_page_shadow_local_free_class[i],
        memory_order_acquire);
    out->h9_owner_page_shadow_remote_free_class[i] = atomic_load_explicit(
        &h8g.h9_owner_page_shadow_remote_free_class[i],
        memory_order_acquire);
  }
  out->h9_owner_page_api_alloc_call = atomic_load_explicit(
      &h8g.h9_owner_page_api_alloc_call, memory_order_acquire);
  out->h9_owner_page_api_free_call = atomic_load_explicit(
      &h8g.h9_owner_page_api_free_call, memory_order_acquire);
  out->h9_owner_page_api_route_attempt = atomic_load_explicit(
      &h8g.h9_owner_page_api_route_attempt, memory_order_acquire);
  out->h9_owner_page_api_route_hit = atomic_load_explicit(
      &h8g.h9_owner_page_api_route_hit, memory_order_acquire);
  out->h9_owner_page_api_route_miss = atomic_load_explicit(
      &h8g.h9_owner_page_api_route_miss, memory_order_acquire);
  out->h9_owner_page_api_route_invalid = atomic_load_explicit(
      &h8g.h9_owner_page_api_route_invalid, memory_order_acquire);
  out->h9_owner_page_api_alloc_hit = atomic_load_explicit(
      &h8g.h9_owner_page_api_alloc_hit, memory_order_acquire);
  out->h9_owner_page_api_alloc_empty = atomic_load_explicit(
      &h8g.h9_owner_page_api_alloc_empty, memory_order_acquire);
  out->h9_owner_page_api_alloc_mode_block = atomic_load_explicit(
      &h8g.h9_owner_page_api_alloc_mode_block, memory_order_acquire);
  out->h9_owner_page_api_free_push = atomic_load_explicit(
      &h8g.h9_owner_page_api_free_push, memory_order_acquire);
  out->h9_owner_page_api_free_double = atomic_load_explicit(
      &h8g.h9_owner_page_api_free_double, memory_order_acquire);
  out->h9_owner_page_api_remote_pending_first = atomic_load_explicit(
      &h8g.h9_owner_page_api_remote_pending_first, memory_order_acquire);
  out->h9_owner_page_api_remote_pending_repeat = atomic_load_explicit(
      &h8g.h9_owner_page_api_remote_pending_repeat, memory_order_acquire);
  out->h9_owner_page_api_remote_note_call = atomic_load_explicit(
      &h8g.h9_owner_page_api_remote_note_call, memory_order_acquire);
  out->h9_owner_page_api_remote_fallback = atomic_load_explicit(
      &h8g.h9_owner_page_api_remote_fallback, memory_order_acquire);
  out->h9_owner_page_api_remote_mark = atomic_load_explicit(
      &h8g.h9_owner_page_api_remote_mark, memory_order_acquire);
  out->h9_owner_page_api_remote_mark_first = atomic_load_explicit(
      &h8g.h9_owner_page_api_remote_mark_first, memory_order_acquire);
  out->h9_owner_page_api_remote_mark_repeat = atomic_load_explicit(
      &h8g.h9_owner_page_api_remote_mark_repeat, memory_order_acquire);
  out->h9_owner_page_api_drain_start = atomic_load_explicit(
      &h8g.h9_owner_page_api_drain_start, memory_order_acquire);
  out->h9_owner_page_api_detach = atomic_load_explicit(
      &h8g.h9_owner_page_api_detach, memory_order_acquire);
  out->h9_owner_page_api_state_ensure = atomic_load_explicit(
      &h8g.h9_owner_page_api_state_ensure, memory_order_acquire);
  out->h9_owner_page_api_state_create = atomic_load_explicit(
      &h8g.h9_owner_page_api_state_create, memory_order_acquire);
  out->h9_owner_page_api_state_oom = atomic_load_explicit(
      &h8g.h9_owner_page_api_state_oom, memory_order_acquire);
  out->h9_owner_page_api_state_free = atomic_load_explicit(
      &h8g.h9_owner_page_api_state_free, memory_order_acquire);
  out->h9_owner_page_api_page_create = atomic_load_explicit(
      &h8g.h9_owner_page_api_page_create, memory_order_acquire);
  out->h9_owner_page_api_page_create_fail = atomic_load_explicit(
      &h8g.h9_owner_page_api_page_create_fail, memory_order_acquire);
  out->h9_owner_page_api_page_release = atomic_load_explicit(
      &h8g.h9_owner_page_api_page_release, memory_order_acquire);
  out->h9_owner_page_api_page_active_install = atomic_load_explicit(
      &h8g.h9_owner_page_api_page_active_install, memory_order_acquire);
  out->h9_owner_page_api_page_bytes_create = atomic_load_explicit(
      &h8g.h9_owner_page_api_page_bytes_create, memory_order_acquire);
  out->h9_owner_page_api_page_bytes_release = atomic_load_explicit(
      &h8g.h9_owner_page_api_page_bytes_release, memory_order_acquire);
  out->h9_owner_page_api_page_release_defer = atomic_load_explicit(
      &h8g.h9_owner_page_api_page_release_defer, memory_order_acquire);
  out->h9_owner_page_api_page_pending_collect = atomic_load_explicit(
      &h8g.h9_owner_page_api_page_pending_collect, memory_order_acquire);
  out->h9_owner_page_api_flush_thread = atomic_load_explicit(
      &h8g.h9_owner_page_api_flush_thread, memory_order_acquire);
  out->h9_owner_page_api_flush_owner = atomic_load_explicit(
      &h8g.h9_owner_page_api_flush_owner, memory_order_acquire);
  out->h9_owner_page_api_local_bits_mutation = atomic_load_explicit(
      &h8g.h9_owner_page_api_local_bits_mutation, memory_order_acquire);
  for (size_t i = 0; i < 6; ++i) {
    out->h9_owner_page_api_remote_class[i] = atomic_load_explicit(
        &h8g.h9_owner_page_api_remote_class[i], memory_order_acquire);
  }
