  for (size_t i = 0; i < 6; ++i) {
    out->h9_local_arena_page_create_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_page_create_class[i], memory_order_acquire);
    out->h9_local_arena_alloc_hit_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_alloc_hit_class[i], memory_order_acquire);
    out->h9_local_arena_free_local_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_free_local_class[i], memory_order_acquire);
    out->h9_local_arena_free_remote_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_free_remote_class[i], memory_order_acquire);
    out->h9_local_arena_remote_first_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_remote_first_class[i], memory_order_acquire);
    out->h9_local_arena_remote_disable_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_remote_disable_class[i], memory_order_acquire);
    out->h9_local_arena_remote_retire_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_remote_retire_class[i], memory_order_acquire);
    out->h9_local_arena_remote_safe_page_create_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_remote_safe_page_create_class[i],
            memory_order_acquire);
    out->h9_local_arena_remote_safe_alloc_hit_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_remote_safe_alloc_hit_class[i],
        memory_order_acquire);
    out->h9_local_arena_remote_safe_free_local_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_remote_safe_free_local_class[i],
            memory_order_acquire);
    out->h9_local_arena_remote_safe_free_remote_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_remote_safe_free_remote_class[i],
            memory_order_acquire);
    out->h9_local_arena_ownerfast_alloc_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_ownerfast_alloc_class[i], memory_order_acquire);
    out->h9_local_arena_ownerfast_free_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_ownerfast_free_class[i], memory_order_acquire);
    out->h9_local_arena_admit_create_before_remote_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_admit_create_before_remote_class[i],
            memory_order_acquire);
    out->h9_local_arena_admit_create_after_remote_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_admit_create_after_remote_class[i],
            memory_order_acquire);
    out->h9_local_arena_admit_remote_class_first[i] = atomic_load_explicit(
        &h8g.h9_local_arena_admit_remote_class_first[i], memory_order_acquire);
    out->h9_local_arena_remote_admit_block_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_remote_admit_block_class[i], memory_order_acquire);
    out->h9_local_arena_admission_attempt_class[i] = atomic_load_explicit(
        &h8g.h9_local_arena_admission_attempt_class[i], memory_order_acquire);
    out->h9_local_arena_admission_remote_seen_class[i] =
        atomic_load_explicit(&h8g.h9_local_arena_admission_remote_seen_class[i],
                             memory_order_acquire);
    out->h9_local_arena_admission_active_owner_class[i] =
        atomic_load_explicit(&h8g.h9_local_arena_admission_active_owner_class[i],
                             memory_order_acquire);
    out->h9_local_arena_admission_active_nonfull_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_admission_active_nonfull_class[i],
            memory_order_acquire);
    out->h9_local_arena_admission_remote_seen_no_capacity_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_admission_remote_seen_no_capacity_class[i],
            memory_order_acquire);
    out->h9_local_arena_admission_remote_seen_active_nonfull_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_admission_remote_seen_active_nonfull_class[i],
            memory_order_acquire);
    out->h9_local_arena_remote_probation_block_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_remote_probation_block_class[i],
            memory_order_acquire);
    out->h9_local_arena_remote_probation_allow_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_remote_probation_allow_class[i],
            memory_order_acquire);
    out->h9_local_arena_remote_free_after_local_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_remote_free_after_local_class[i],
            memory_order_acquire);
    out->h9_local_arena_local_free_after_remote_class[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_local_free_after_remote_class[i],
            memory_order_acquire);
  }
  for (size_t i = 0; i < 5; ++i) {
    out->h9_local_arena_remote_first_alloc_age[i] = atomic_load_explicit(
        &h8g.h9_local_arena_remote_first_alloc_age[i], memory_order_acquire);
    out->h9_local_arena_create_thread_page_seq[i] = atomic_load_explicit(
        &h8g.h9_local_arena_create_thread_page_seq[i], memory_order_acquire);
    out->h9_local_arena_remote_first_thread_page_seq[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_remote_first_thread_page_seq[i],
            memory_order_acquire);
    out->h9_local_arena_create_thread_alloc_history[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_create_thread_alloc_history[i],
            memory_order_acquire);
    out->h9_local_arena_remote_first_thread_alloc_history[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_remote_first_thread_alloc_history[i],
            memory_order_acquire);
    out->h9_local_arena_remote_first_local_free_history[i] =
        atomic_load_explicit(
            &h8g.h9_local_arena_remote_first_local_free_history[i],
            memory_order_acquire);
  }
  out->h9_slab_adaptive_remote_mark = atomic_load_explicit(
      &h8g.h9_slab_adaptive_remote_mark, memory_order_acquire);
  out->h9_slab_adaptive_remote_mark_first = atomic_load_explicit(
      &h8g.h9_slab_adaptive_remote_mark_first, memory_order_acquire);
  out->h9_slab_adaptive_alloc_blocked = atomic_load_explicit(
      &h8g.h9_slab_adaptive_alloc_blocked, memory_order_acquire);
  out->h9_slab_adaptive_alloc_enabled = atomic_load_explicit(
      &h8g.h9_slab_adaptive_alloc_enabled, memory_order_acquire);
