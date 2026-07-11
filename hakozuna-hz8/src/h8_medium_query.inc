bool h8_medium_free_inner(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  H8ThreadCtx* ctx = h8_tls_ctx;
#if defined(H8_MEDIUM_ENABLE_LOCAL_FREE_CACHE)
  if (h8_medium_try_cached_local_free(ctx, ptr, owned_out)) {
    return true;
  }
#else
  (void)ctx;
#endif

  H8_DEBUG_INC(medium_free_lookup_count);
  H8MediumRun* run = h8_medium_directory_find(ptr);
#if defined(H8_MEDIUM_PAGE_SUBSTRATE_SHADOW_L0)
  h8_medium_page_shadow_compare(ptr, run);
#endif
  if (run) {
    uint64_t owner_word =
        atomic_load_explicit(&run->owner_word, memory_order_acquire);
    bool same_owner = h8_medium_owner_word_matches_ctx(owner_word, ctx);
    if (!same_owner && owner_word != 0) {
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
      if (owned_out) { *owned_out = true; }
      for (;;) {
        H8PublishResult res = h8_medium_remote_publish(run, ptr);
        if (res == H8_PUBLISH_OK) {
          return true;
        }
        if (res != H8_PUBLISH_OWNER_TRANSITION) {
          return false;
        }
        h8_platform_yield();
      }
    }
    h8_medium_debug_lock_elide_candidate_known(same_owner, true);
    if (same_owner) {
      H8_DEBUG_INC(medium_local_free_owner_match);
      h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_FREE);
      if (owned_out) {
        *owned_out = true;
      }
      bool keep_empty_live =
          ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
          ctx->active_medium_runs[run->class_id] == run;
      bool ok = h8_medium_run_free_local_scaffold(run, ptr, keep_empty_live);
      if (!ok) {
        H8_DEBUG_INC(medium_invalid_owned_count);
      }
      if (ok) {
        h8_medium_debug_class_inc(run->class_id,
                                  &h8g.medium_local_free_class_8k,
                                  &h8g.medium_local_free_class_16k,
                                  &h8g.medium_local_free_class_32k,
                                  &h8g.medium_local_free_class_64k);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
        h8_medium_debug_v12_class_inc(run->class_id,
                                      &h8g.medium_local_free_class_24k,
                                      &h8g.medium_local_free_class_48k);
#endif
      }
      if (ok && ctx && run->class_id < H8_MEDIUM_CLASS_COUNT) {
        h8_medium_set_active_run(ctx, run->class_id, run);
      }
      h8_medium_debug_writer_exit(run);
      return ok;
    }
    H8_DEBUG_INC(medium_remote_free_owner_mismatch);
    h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                 H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE);
    ctx = NULL;
    h8_medium_lock_run(run);
  } else {
    h8_medium_lock_global();
    run = h8_medium_find_run_locked(ptr, false);
    if (!run) {
      h8_medium_unlock_global();
      return false;
    }
    uint64_t owner_word =
        atomic_load_explicit(&run->owner_word, memory_order_acquire);
    bool same_owner = h8_medium_owner_word_matches_ctx(owner_word, ctx);
    if (!same_owner && owner_word != 0) {
      h8_medium_unlock_global();
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
      if (owned_out) { *owned_out = true; }
      for (;;) {
        H8PublishResult res = h8_medium_remote_publish(run, ptr);
        if (res == H8_PUBLISH_OK) {
          return true;
        }
        if (res != H8_PUBLISH_OWNER_TRANSITION) {
          return false;
        }
        h8_platform_yield();
      }
    }
    if (same_owner) {
      H8_DEBUG_INC(medium_local_free_owner_match);
    } else {
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
    }
    h8_medium_debug_writer_enter(
        run, ctx ? ctx->owner : NULL,
        same_owner ? H8_MEDIUM_WRITER_OWNER_LOCAL_FREE
                   : H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE);
    h8_medium_lock_run(run);
    h8_medium_unlock_global();
    if (!same_owner) {
      ctx = NULL;
    }
  }
  if (owned_out) {
    *owned_out = true;
  }
  bool keep_empty_live =
      ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
      ctx->active_medium_runs[run->class_id] == run;
  bool ok = h8_medium_run_free_local_scaffold(run, ptr, keep_empty_live);
  if (!ok) {
    H8_DEBUG_INC(medium_invalid_owned_count);
  }
  if (ok) {
    if (ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
        ctx->active_medium_runs[run->class_id] == run) {
      h8_medium_debug_class_inc(run->class_id,
                                &h8g.medium_local_free_class_8k,
                                &h8g.medium_local_free_class_16k,
                                &h8g.medium_local_free_class_32k,
                                &h8g.medium_local_free_class_64k);
#if defined(H8_MEDIUM_V12_48K2_CLASS)
      h8_medium_debug_v12_class_inc(run->class_id,
                                    &h8g.medium_local_free_class_24k,
                                    &h8g.medium_local_free_class_48k);
#endif
      h8_medium_set_active_run(ctx, run->class_id, run);
    }
  }
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
  return ok;
}

bool h8_medium_usable_size_inner(void* ptr, size_t* usable_out,
                                 bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  H8MediumRun* run = h8_medium_directory_find(ptr);
  if (run) {
    h8_medium_lock_run(run);
  } else {
    h8_medium_lock_global();
    run = h8_medium_find_run_locked(ptr, true);
    if (!run) {
      h8_medium_unlock_global();
      return false;
    }
    h8_medium_lock_run(run);
    h8_medium_unlock_global();
  }
  if (owned_out) {
    *owned_out = true;
  }
  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked(run, ptr, &slot)) {
    h8_medium_unlock_run(run);
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  bool pending = (atomic_load_explicit(&run->pending_bits[0],
                                       memory_order_acquire) &
                  bit) != 0;
  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
  bool valid =
      h8_slot_state_tag(state) == (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT) &&
      !pending;
  if (valid && usable_out) {
    *usable_out = run->slot_size;
  }
  h8_medium_unlock_run(run);
  return valid;
}

H8RouteKind h8_medium_route_inner(void* ptr) {
  H8_DEBUG_INC(medium_route_lookup_count);
  H8MediumRun* run = h8_medium_directory_find(ptr);
  if (run) {
    h8_medium_lock_run(run);
  } else {
    h8_medium_lock_global();
    run = h8_medium_find_run_locked(ptr, true);
    if (!run) {
      h8_medium_unlock_global();
      return H8_ROUTE_MISS;
    }
    h8_medium_lock_run(run);
    h8_medium_unlock_global();
  }
  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked(run, ptr, &slot)) {
    h8_medium_unlock_run(run);
    return H8_ROUTE_INVALID;
  }
  uint64_t bit = UINT64_C(1) << slot;
  bool pending = (atomic_load_explicit(&run->pending_bits[0],
                                       memory_order_acquire) &
                  bit) != 0;
  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
  bool slot_authority_valid =
      h8_slot_state_tag(state) == (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT) &&
      !pending;
#if defined(H8_ENABLE_DEBUG_STATS)
  bool mask_authority_valid =
      (run->allocated_mask & bit) != 0 && (run->free_mask & bit) == 0 &&
      !pending;
  if (slot_authority_valid != mask_authority_valid) {
    H8_DEBUG_INC(medium_route_authority_mismatch);
  }
#endif
  H8RouteKind route = slot_authority_valid ? H8_ROUTE_VALID : H8_ROUTE_INVALID;
  h8_medium_unlock_run(run);
  return route;
}

void h8_medium_owner_detach_all(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  h8_medium_collect_owner_pending(owner);
#if defined(H8_HZ9_MEDIUM_LOCAL_MAG_SHADOW)
  h8_hz9_local_mag_shadow_flush_owner(h8_tls_ctx, true);
#endif
  h8_medium_lock_global();
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    H8MediumRun* run = owner->medium_by_class[c];
    owner->medium_by_class[c] = NULL;
    while (run) {
      H8MediumRun* next = run->next_owner;
      h8_medium_debug_writer_enter(run, owner, H8_MEDIUM_WRITER_OWNER_DETACH);
      h8_medium_lock_run(run);
#if defined(H8_MEDIUM_ENABLE_LOCAL_FAST_TIER)
      h8_medium_local_fast_flush_locked(run);
#endif
      h8_medium_debug_note_local_fast_flush(run, true);
      if (run->allocated_mask == 0 &&
          run->payload_state != H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED) {
        H8_DEBUG_INC(medium_owner_exit_drain_count);
        h8_medium_decommit_empty_owner_exit_locked(run);
        if (run->active_live_empty_charge) {
          H8_DEBUG_INC(medium_owner_exit_active_live_remaining);
        }
      }
      if (run->lazy_purge_charge) {
        H8_DEBUG_INC(medium_lazy_drop_detach);
      }
      h8_medium_debug_discard_collect_credit(run);
      h8_medium_available_shadow_remove(owner, run);
      h8_medium_lazy_purge_shadow_drop(run);
      run->owner_attached = false;
      atomic_store_explicit(&run->owner_word, 0, memory_order_release);
      h8_medium_unlock_run(run);
      h8_medium_debug_writer_exit(run);
      run->next_owner = NULL;
      h8_medium_detached_add_locked(run);
      run = next;
    }
  }
  h8_medium_unlock_global();
}
