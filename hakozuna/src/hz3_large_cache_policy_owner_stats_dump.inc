static void hz3_large_s240_dump_final(void) {
    fprintf(stderr,
            "[HZ3_LARGE_S240_META] activate=%zu activate_cache=%zu "
            "activate_mmap=%zu cache_seed=%zu cache_store=%zu "
            "munmap_store=%zu free_seen=%zu free_active=%zu "
            "state_mismatch=%zu sc_mismatch=%zu owner_unknown_activate=%zu "
            "owner_unknown_free=%zu owner_same_free=%zu owner_remote_free=%zu\n",
            atomic_load_explicit(&g_s240_activate, memory_order_relaxed),
            atomic_load_explicit(&g_s240_activate_cache, memory_order_relaxed),
            atomic_load_explicit(&g_s240_activate_mmap, memory_order_relaxed),
            atomic_load_explicit(&g_s240_cache_seed, memory_order_relaxed),
            atomic_load_explicit(&g_s240_cache_store, memory_order_relaxed),
            atomic_load_explicit(&g_s240_munmap_store, memory_order_relaxed),
            atomic_load_explicit(&g_s240_free_seen, memory_order_relaxed),
            atomic_load_explicit(&g_s240_free_active, memory_order_relaxed),
            atomic_load_explicit(&g_s240_state_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_s240_sc_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_s240_owner_unknown_activate, memory_order_relaxed),
            atomic_load_explicit(&g_s240_owner_unknown_free, memory_order_relaxed),
            atomic_load_explicit(&g_s240_owner_same_free, memory_order_relaxed),
            atomic_load_explicit(&g_s240_owner_remote_free, memory_order_relaxed));
#if HZ3_S240_LARGE_FRONT_CACHE
    fprintf(stderr,
            "[HZ3_LARGE_S240_FRONT] push=%zu pop_hit=%zu pop_miss=%zu "
            "reject_limit=%zu flush_calls=%zu flush_objs=%zu flush_bytes=%zu "
            "evict_munmap=%zu nodes_current=%zu nodes_peak=%zu "
            "bytes_current=%zu bytes_peak=%zu\n",
            atomic_load_explicit(&g_s240_front_push, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_pop_hit, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_pop_miss, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_reject_limit, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_flush_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_flush_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_flush_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_evict_munmap, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_nodes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_nodes_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_bytes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_bytes_peak, memory_order_relaxed));
#endif
#if HZ3_S240_LARGE_OWNER_INBOX
    fprintf(stderr,
            "[HZ3_LARGE_S240_INBOX] remote_push=%zu reject_owner_dead=%zu "
            "reject_owner_exiting=%zu reject_shard_collision=%zu "
            "reject_budget=%zu drain_calls=%zu drain_nonempty=%zu "
            "drain_objs=%zu drain_front_reject=%zu flush_calls=%zu "
            "flush_objs=%zu flush_bytes=%zu cas_retries=%zu "
            "nodes_current=%zu nodes_peak=%zu bytes_current=%zu bytes_peak=%zu "
            "pushers_peak=%zu flush_wait_spins=%zu\n",
            atomic_load_explicit(&g_s240_inbox_remote_push, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_reject_owner_dead, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_reject_owner_exiting, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_reject_shard_collision, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_reject_budget, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_drain_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_drain_nonempty, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_drain_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_drain_front_reject, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_flush_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_flush_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_flush_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_cas_retries, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_nodes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_nodes_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_bytes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_bytes_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_pushers_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_flush_wait_spins, memory_order_relaxed));
#if HZ3_S245_LARGE_INBOX_BATCH_POP || HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH || \
    HZ3_S274_LARGE_INBOX_SIZE_DRAIN
    fprintf(stderr,
            "[HZ3_LARGE_S245_INBOX_BATCH] pop_calls=%zu pop_nonempty=%zu "
            "pop_objs=%zu pop_cas_retries=%zu pop_max_objs=%zu\n",
            atomic_load_explicit(&g_s245_batch_pop_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s245_batch_pop_nonempty, memory_order_relaxed),
            atomic_load_explicit(&g_s245_batch_pop_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s245_batch_pop_cas_retries, memory_order_relaxed),
            atomic_load_explicit(&g_s245_batch_pop_max_objs, memory_order_relaxed));
#endif
#if HZ3_S246_LARGE_INBOX_TAKE_FIRST
    fprintf(stderr,
            "[HZ3_LARGE_S246_INBOX_TAKE_FIRST] calls=%zu hits=%zu "
            "misses=%zu direct=%zu extra_fronted=%zu extra_munmap=%zu\n",
            atomic_load_explicit(&g_s246_take_first_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s246_take_first_hits, memory_order_relaxed),
            atomic_load_explicit(&g_s246_take_first_misses, memory_order_relaxed),
            atomic_load_explicit(&g_s246_take_first_direct, memory_order_relaxed),
            atomic_load_explicit(&g_s246_take_first_extra_fronted, memory_order_relaxed),
            atomic_load_explicit(&g_s246_take_first_extra_munmap, memory_order_relaxed));
#endif
#if HZ3_S282_LARGE_OWNER_INBOX_OBS
    fprintf(stderr,
            "[HZ3_S282_OWNER_INBOX] push_attempt=%zu push_ok=%zu "
            "push_reject_invalid=%zu push_reject_owner_dead=%zu "
            "push_reject_owner_exiting=%zu push_reject_shard_collision=%zu "
            "push_reject_budget=%zu push_cas_attempts=%zu "
            "push_cas_failures=%zu push_cas_failure_max_per_push=%zu "
            "push_depth_0=%zu push_depth_1=%zu push_depth_2_3=%zu "
            "push_depth_4_7=%zu push_depth_8_15=%zu "
            "push_depth_16_plus=%zu pop_cas_attempts=%zu "
            "pop_cas_failures=%zu pop_cas_failure_max_per_pop=%zu "
            "pop_success=%zu pop_empty=%zu pop_race_empty=%zu "
            "take_first_calls=%zu take_first_hit=%zu "
            "take_first_miss_empty=%zu take_first_miss_race=%zu "
            "take_depth_0=%zu take_depth_1=%zu take_depth_2_3=%zu "
            "take_depth_4_7=%zu take_depth_8_15=%zu "
            "take_depth_16_plus=%zu drain_nodes=%zu "
            "drain_return_first_nodes=%zu drain_to_front_nodes=%zu "
            "drain_to_munmap_nodes=%zu\n",
            atomic_load_explicit(&g_s282_push_attempt, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_ok, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_reject_invalid, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_reject_owner_dead, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_reject_owner_exiting, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_reject_shard_collision,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_reject_budget, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_cas_attempts, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_cas_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_cas_failure_max_per_push,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_0, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_1, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_2_3, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_4_7, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_8_15, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_16_plus, memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_cas_attempts, memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_cas_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_cas_failure_max_per_pop,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_success, memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_empty, memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_race_empty, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_first_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_first_hit, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_first_miss_empty, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_first_miss_race, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_0, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_1, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_2_3, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_4_7, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_8_15, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_16_plus, memory_order_relaxed),
            atomic_load_explicit(&g_s282_drain_nodes, memory_order_relaxed),
            atomic_load_explicit(&g_s282_drain_return_first_nodes,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s282_drain_to_front_nodes, memory_order_relaxed),
            atomic_load_explicit(&g_s282_drain_to_munmap_nodes, memory_order_relaxed));
#endif
#endif
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
    fprintf(stderr,
            "[HZ3_LARGE_S251_SAFETY] free_take_to_freeing=%zu "
            "free_take_state_fail=%zu front_pop_to_freeing=%zu "
            "front_pop_state_fail=%zu inbox_pop_to_freeing=%zu "
            "inbox_pop_state_fail=%zu global_pop_to_freeing=%zu "
            "global_pop_state_fail=%zu front_push_state_fail=%zu "
            "inbox_push_state_fail=%zu failfast=%d\n",
            atomic_load_explicit(&g_s251_free_take_to_freeing, memory_order_relaxed),
            atomic_load_explicit(&g_s251_free_take_state_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s251_front_pop_to_freeing, memory_order_relaxed),
            atomic_load_explicit(&g_s251_front_pop_state_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s251_inbox_pop_to_freeing, memory_order_relaxed),
            atomic_load_explicit(&g_s251_inbox_pop_state_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s251_global_pop_to_freeing, memory_order_relaxed),
            atomic_load_explicit(&g_s251_global_pop_state_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s251_front_push_state_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s251_inbox_push_state_fail, memory_order_relaxed),
            HZ3_S251_LARGE_OWNER_FRONT_FAILFAST);
#endif
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    fprintf(stderr,
            "[HZ3_LARGE_S276_DIRECT_RETAIN] front_take_try=%zu "
            "front_take_hit=%zu front_take_not_owner=%zu "
            "front_take_not_eligible=%zu front_take_nonactive=%zu "
            "front_take_state_cas_fail=%zu front_push_retained=%zu "
            "front_push_reject_clear=%zu front_activate_fast=%zu "
            "front_activate_rekey=%zu inbox_take_hit=%zu "
            "inbox_push_retained=%zu inbox_push_reject_clear=%zu "
            "inbox_activate_fast=%zu direct_clear=%zu\n",
            atomic_load_explicit(&g_s276_front_take_try, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_take_hit, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_take_not_owner, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_take_not_eligible, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_take_nonactive, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_take_state_cas_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_push_retained, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_push_reject_clear, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_activate_fast, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_activate_rekey, memory_order_relaxed),
            atomic_load_explicit(&g_s276_inbox_take_hit, memory_order_relaxed),
            atomic_load_explicit(&g_s276_inbox_push_retained, memory_order_relaxed),
            atomic_load_explicit(&g_s276_inbox_push_reject_clear, memory_order_relaxed),
            atomic_load_explicit(&g_s276_inbox_activate_fast, memory_order_relaxed),
            atomic_load_explicit(&g_s276_direct_clear, memory_order_relaxed));
#endif
#if HZ3_S292_HIGHBAND_RETENTION_OBS
    fprintf(stderr,
            "[HZ3_S292_HIGHBAND_ROUTE] alloc_highband=%zu alloc_65537=%zu "
            "alloc_lte128k=%zu alloc_gt256k=%zu free_highband=%zu "
            "free_65537=%zu global_fallback=%zu global_cache_store=%zu "
            "global_munmap=%zu\n",
            atomic_load_explicit(&g_s292_alloc_highband, memory_order_relaxed),
            atomic_load_explicit(&g_s292_alloc_65537, memory_order_relaxed),
            atomic_load_explicit(&g_s292_alloc_lte128k, memory_order_relaxed),
            atomic_load_explicit(&g_s292_alloc_gt256k, memory_order_relaxed),
            atomic_load_explicit(&g_s292_free_highband, memory_order_relaxed),
            atomic_load_explicit(&g_s292_free_65537, memory_order_relaxed),
            atomic_load_explicit(&g_s292_global_fallback, memory_order_relaxed),
            atomic_load_explicit(&g_s292_global_cache_store, memory_order_relaxed),
            atomic_load_explicit(&g_s292_global_munmap, memory_order_relaxed));
    fprintf(stderr,
            "[HZ3_S292_HIGHBAND_FRONT] pop_calls=%zu pop_hit=%zu "
            "pop_miss=%zu push=%zu reject=%zu nodes_current=%zu "
            "nodes_peak=%zu bytes_current=%zu bytes_peak=%zu\n",
            atomic_load_explicit(&g_s292_front_pop_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s292_front_pop_hit, memory_order_relaxed),
            atomic_load_explicit(&g_s292_front_pop_miss, memory_order_relaxed),
            atomic_load_explicit(&g_s292_front_push, memory_order_relaxed),
            atomic_load_explicit(&g_s292_front_reject, memory_order_relaxed),
            atomic_load_explicit(&g_s292_front_nodes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s292_front_nodes_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s292_front_bytes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s292_front_bytes_peak, memory_order_relaxed));
    fprintf(stderr,
            "[HZ3_S292_HIGHBAND_INBOX] push_attempt=%zu push_ok=%zu "
            "reject_invalid=%zu reject_owner=%zu reject_budget=%zu pop=%zu "
            "flush=%zu nodes_current=%zu nodes_peak=%zu bytes_current=%zu "
            "bytes_peak=%zu\n",
            atomic_load_explicit(&g_s292_inbox_push_attempt, memory_order_relaxed),
            atomic_load_explicit(&g_s292_inbox_push_ok, memory_order_relaxed),
            atomic_load_explicit(&g_s292_inbox_reject_invalid, memory_order_relaxed),
            atomic_load_explicit(&g_s292_inbox_reject_owner, memory_order_relaxed),
            atomic_load_explicit(&g_s292_inbox_reject_budget, memory_order_relaxed),
            atomic_load_explicit(&g_s292_inbox_pop, memory_order_relaxed),
            atomic_load_explicit(&g_s292_inbox_flush, memory_order_relaxed),
            atomic_load_explicit(&g_s292_inbox_nodes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s292_inbox_nodes_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s292_inbox_bytes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s292_inbox_bytes_peak, memory_order_relaxed));
#endif
#if HZ3_S293_HIGHBAND_GLOBAL_SCACHE_CAP
    fprintf(stderr,
            "[HZ3_S293_HIGHBAND_GLOBAL_SCACHE_CAP] max_nodes_per_sc=%u "
            "admit_try=%zu admit_ok=%zu reject_cap=%zu "
            "reject_nonwindow=%zu\n",
            (unsigned)HZ3_S293_HIGHBAND_GLOBAL_MAX_NODES_PER_SC,
            atomic_load_explicit(&g_s293_global_admit_try, memory_order_relaxed),
            atomic_load_explicit(&g_s293_global_admit_ok, memory_order_relaxed),
            atomic_load_explicit(&g_s293_global_reject_cap, memory_order_relaxed),
            atomic_load_explicit(&g_s293_global_reject_nonwindow, memory_order_relaxed));
#endif
#if HZ3_S293D_HIGHBAND_GLOBAL_SOFT_CAP
    fprintf(stderr,
            "[HZ3_S293D_HIGHBAND_GLOBAL_SOFT_CAP] soft_nodes_per_sc=%u "
            "hard_nodes_per_sc=%u admit_stride=%u admit_try=%zu admit_ok=%zu "
            "admit_soft=%zu reject_soft=%zu reject_hard=%zu "
            "reject_nonwindow=%zu\n",
            (unsigned)HZ3_S293D_HIGHBAND_GLOBAL_SOFT_NODES_PER_SC,
            (unsigned)HZ3_S293D_HIGHBAND_GLOBAL_HARD_NODES_PER_SC,
            (unsigned)HZ3_S293D_HIGHBAND_GLOBAL_ADMIT_STRIDE,
            atomic_load_explicit(&g_s293d_global_admit_try, memory_order_relaxed),
            atomic_load_explicit(&g_s293d_global_admit_ok, memory_order_relaxed),
            atomic_load_explicit(&g_s293d_global_admit_soft, memory_order_relaxed),
            atomic_load_explicit(&g_s293d_global_reject_soft, memory_order_relaxed),
            atomic_load_explicit(&g_s293d_global_reject_hard, memory_order_relaxed),
            atomic_load_explicit(&g_s293d_global_reject_nonwindow,
                                 memory_order_relaxed));
#endif
#if HZ3_S285_LARGE_FIXED_COST_OBS
    fprintf(stderr,
            "[HZ3_S285_FIXED_COST_DIRECT] way_loads=%zu key_busy=%zu "
            "key_miss=%zu key_match=%zu invalid_hdr=%zu state_nonactive=%zu "
            "inuse_bad=%zu not_retainable_same_owner=%zu "
            "not_retainable_remote=%zu cas_attempt=%zu cas_fail=%zu "
            "hit_front=%zu hit_inbox=%zu\n",
            atomic_load_explicit(&g_s285_direct_way_loads, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_key_busy, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_key_miss, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_key_match, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_invalid_hdr, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_state_nonactive, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_inuse_bad, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_not_retainable_same_owner,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_not_retainable_remote,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_cas_attempt, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_cas_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_hit_front, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_hit_inbox, memory_order_relaxed));
    fprintf(stderr,
            "[HZ3_S285_FIXED_COST_ACTIVATE] checks=%zu fail_header=%zu "
            "fail_slot=%zu fail_flag=%zu fail_sc=%zu fail_alignment=%zu "
            "fail_user_alignment=%zu fail_range=%zu fail_size=%zu "
            "success_front=%zu success_inbox=%zu\n",
            atomic_load_explicit(&g_s285_activate_check, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_header, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_slot, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_flag, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_sc, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_alignment, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_user_alignment,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_range, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_size, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_success_front, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_success_inbox, memory_order_relaxed));
    fprintf(stderr,
            "[HZ3_S285_FIXED_COST_DRAIN] return_first=%zu "
            "extra_pop_attempt=%zu extra_pop_hit=%zu extra_pop_empty=%zu "
            "extra_front_ok=%zu extra_front_reject=%zu drain_nodes_1=%zu "
            "drain_nodes_2_3=%zu drain_nodes_4_7=%zu drain_nodes_8_plus=%zu\n",
            atomic_load_explicit(&g_s285_take_first_return_first,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_extra_pop_attempt,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_extra_pop_hit,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_extra_pop_empty,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_extra_front_ok,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_extra_front_reject,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_drain_nodes_1,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_drain_nodes_2_3,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_drain_nodes_4_7,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_drain_nodes_8_plus,
                                 memory_order_relaxed));
#endif
#if HZ3_S240_LARGE_FLUSH_VALIDATE
    fprintf(stderr,
            "[HZ3_LARGE_S240_FLUSH_VALIDATE] enabled=1 failfast=%d "
            "calls=%zu failures=%zu front_node_fail=%zu inbox_node_fail=%zu "
            "drain_node_fail=%zu null_node_fail=%zu bad_state=%zu "
            "bad_header=%zu bad_link=%zu sc_mismatch=%zu "
            "inbox_count_underflow=%zu inbox_bytes_underflow=%zu\n",
            HZ3_S240_LARGE_FLUSH_FAILFAST,
            atomic_load_explicit(&g_s240_flush_validate_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_validate_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_front_node_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_inbox_node_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_drain_node_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_null_node_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_bad_state, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_bad_header, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_bad_link, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_sc_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_inbox_count_underflow,
                                  memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_inbox_bytes_underflow,
                                  memory_order_relaxed));
#endif
#if HZ3_S240_LARGE_FLUSH_QUARANTINE
    fprintf(stderr,
            "[HZ3_LARGE_S240_FLUSH_QUARANTINE] enabled=1 objs=%zu bytes=%zu\n",
            atomic_load_explicit(&g_s240_flush_quarantine_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_quarantine_bytes, memory_order_relaxed));
#endif
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
    size_t global_cached_bytes = 0;
    size_t global_cached_peak = 0;
    size_t global_cached_nodes = 0;
    int nonempty_classes = 0;
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    global_cached_bytes = hz3_large_total_cached_load();
    global_cached_peak = atomic_load_explicit(&g_s253_global_cached_bytes_peak,
                                              memory_order_relaxed);
    for (int i = 0; i < HZ3_LARGE_SC_COUNT; i++) {
        global_cached_nodes += (size_t)g_sc_nodes[i];
        if (g_sc_nodes[i] != 0 || g_sc_bytes[i] != 0) {
            nonempty_classes++;
        }
    }
#endif
    size_t front_bytes_current =
#if HZ3_S240_LARGE_FRONT_CACHE
        atomic_load_explicit(&g_s240_front_bytes_current, memory_order_relaxed);
#else
        0;
#endif
    size_t front_bytes_peak =
#if HZ3_S240_LARGE_FRONT_CACHE
        atomic_load_explicit(&g_s240_front_bytes_peak, memory_order_relaxed);
#else
        0;
#endif
    size_t inbox_bytes_current =
#if HZ3_S240_LARGE_OWNER_INBOX
        atomic_load_explicit(&g_s240_inbox_bytes_current, memory_order_relaxed);
#else
        0;
#endif
    size_t inbox_bytes_peak =
#if HZ3_S240_LARGE_OWNER_INBOX
        atomic_load_explicit(&g_s240_inbox_bytes_peak, memory_order_relaxed);
#else
        0;
#endif
    fprintf(stderr,
            "[HZ3_LARGE_S253_RSS] global_cached_bytes=%zu "
            "global_cached_peak=%zu global_cached_nodes=%zu "
            "global_nonempty_classes=%d front_bytes_current=%zu "
            "front_bytes_peak=%zu inbox_bytes_current=%zu inbox_bytes_peak=%zu "
            "front_max_class_bytes=%zu front_max_nodes_per_sc=%u "
            "front_max_bytes_per_thread=%zu inbox_max_nodes_per_sc=%u "
            "inbox_max_bytes_per_sc=%zu direct_mapless_max_class_bytes=%zu\n",
            global_cached_bytes,
            global_cached_peak,
            global_cached_nodes,
            nonempty_classes,
            front_bytes_current,
            front_bytes_peak,
            inbox_bytes_current,
            inbox_bytes_peak,
            (size_t)HZ3_S240_LARGE_FRONT_MAX_CLASS_BYTES,
            (unsigned)HZ3_S240_LARGE_FRONT_MAX_NODES_PER_SC,
            (size_t)HZ3_S240_LARGE_FRONT_MAX_BYTES_PER_THREAD,
            (unsigned)HZ3_S240_LARGE_INBOX_MAX_NODES_PER_SC,
            (size_t)HZ3_S240_LARGE_INBOX_MAX_BYTES_PER_SC,
            (size_t)HZ3_S242_DIRECT_MAPLESS_MAX_CLASS_BYTES);
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    fprintf(stderr, "[HZ3_LARGE_S253_SCACHE]");
    for (int i = 0; i < HZ3_LARGE_SC_COUNT; i++) {
        if (g_sc_nodes[i] != 0 || g_sc_bytes[i] != 0) {
            fprintf(stderr,
                    " sc%d_nodes=%u sc%d_bytes=%zu",
                    i,
                    g_sc_nodes[i],
                    i,
                    g_sc_bytes[i]);
        }
    }
    if (nonempty_classes == 0) {
        fprintf(stderr, " empty=1");
    }
    fprintf(stderr, "\n");
#endif
#endif
}

static inline void hz3_large_s240_register_once(void) {
    static _Atomic int registered = 0;
    if (atomic_exchange_explicit(&registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_large_s240_dump_final);
    }
}

static inline uint8_t hz3_large_s240_current_owner(void) {
    hz3_tcache_ensure_init();
    if (!t_hz3_cache.initialized) {
        return UINT8_MAX;
    }
    return (uint8_t)t_hz3_cache.my_shard;
}

static inline uint8_t hz3_large_s240_normalize_sc(int sc) {
    return (sc >= 0 && sc < HZ3_LARGE_SC_COUNT) ? (uint8_t)sc : UINT8_MAX;
}
