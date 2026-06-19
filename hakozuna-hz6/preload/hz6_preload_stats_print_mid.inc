  fprintf(stderr,
          "[HZ6_PRELOAD_STATS] allocators=%zu route_valid=%zu "
          "route_invalid=%zu route_miss=%zu transfer_push=%zu "
          "transfer_pop=%zu source_alloc=%zu toy_source_alloc=%zu "
          "midpage_source_alloc=%zu large_source_alloc=%zu "
          "local2p_source_alloc=%zu alloc_fail=%zu "
          "descriptor_exhausted=%zu "
          "elastic_descriptor_overflow_alloc=%zu "
          "elastic_descriptor_overflow_reset=%zu "
          "elastic_descriptor_overflow_exhausted=%zu "
          "route_register_fail=%zu "
          "source_block_exhausted=%zu source_prefill_attempt=%zu "
          "source_prefill_filled=%zu source_prefill_fallback=%zu "
          "front_source_prefill_alloc=%zu toy_source_prefill_call=%zu "
          "route_lookup_probe_total=%zu "
          "route_lookup_probe_max=%zu route_register_probe_total=%zu "
          "route_register_probe_max=%zu route_unregister_probe_total=%zu "
          "route_unregister_probe_max=%zu "
          "retain_reserve_calls=%zu retain_reserve_64k_calls=%zu "
          "retain_64k_take_hit=%zu retain_64k_take_miss=%zu "
          "retain_generic_take_hit=%zu retain_generic_take_miss=%zu "
          "retain_mmap_fallback=%zu retain_release_calls=%zu "
          "retain_release_64k_calls=%zu retain_64k_put_hit=%zu "
          "retain_64k_put_full=%zu retain_generic_put_hit=%zu "
          "retain_generic_put_full=%zu retain_munmap_fallback=%zu "
          "retain_retained_bytes=%zu retain_retained_64k_count=%zu\n",
          allocator_count, route_valid, route_invalid, route_miss,
          transfer_push, transfer_pop, source_alloc, toy_source_alloc,
          midpage_source_alloc, large_source_alloc, local2p_source_alloc,
          alloc_fail, descriptor_exhausted,
          elastic_descriptor_overflow_alloc,
          elastic_descriptor_overflow_reset,
          elastic_descriptor_overflow_exhausted,
          route_register_fail,
          source_block_exhausted, source_prefill_attempt,
          source_prefill_filled, source_prefill_fallback,
          front_source_prefill_alloc, toy_source_prefill_call,
          route_lookup_probe_total,
          route_lookup_probe_max, route_register_probe_total,
          route_register_probe_max, route_unregister_probe_total,
          route_unregister_probe_max, retain_stats.reserve_calls,
          retain_stats.reserve_64k_calls, retain_stats.retain_64k_take_hit,
          retain_stats.retain_64k_take_miss,
          retain_stats.retain_generic_take_hit,
          retain_stats.retain_generic_take_miss, retain_stats.mmap_fallback,
          retain_stats.release_calls, retain_stats.release_64k_calls,
          retain_stats.retain_64k_put_hit, retain_stats.retain_64k_put_full,
          retain_stats.retain_generic_put_hit,
          retain_stats.retain_generic_put_full, retain_stats.munmap_fallback,
          retain_stats.retained_bytes, retain_stats.retained_64k_count);

  fprintf(stderr,
          "[HZ6_PRELOAD_ROUTE_DETAIL] "
          "route_register_reason_unknown=%zu "
          "route_register_reason_source_run_slot=%zu "
          "route_register_reason_direct_source=%zu "
          "route_register_reason_materialize=%zu "
          "route_register_reason_rehome=%zu "
          "route_unregister_reason_unknown=%zu "
          "route_unregister_reason_frontcache_overflow=%zu "
          "route_unregister_reason_cap_release=%zu "
          "route_unregister_reason_descriptorless_detach=%zu "
          "route_unregister_reason_source_slot_release=%zu "
          "route_unregister_reason_rehome=%zu "
          "route_rehome_attempt=%zu "
          "route_rehome_success=%zu "
          "route_rehome_fail=%zu "
          "route_rehome_expected_owner_mismatch=%zu "
          "route_rehome_destination_route_fail=%zu "
          "route_rehome_directory_transfer_fail=%zu "
          "route_rehome_directory_transfer_probe_total=%zu "
          "route_rehome_directory_transfer_probe_max=%zu "
          "route_rehome_origin_lookup_probe_total=%zu "
          "route_rehome_origin_lookup_probe_max=%zu "
          "route_rehome_destination_register_probe_total=%zu "
          "route_rehome_destination_register_probe_max=%zu "
          "route_rehome_origin_unregister_probe_total=%zu "
          "route_rehome_origin_unregister_probe_max=%zu "
          "route_rehome_rollback_success=%zu "
          "route_rehome_rollback_fail=%zu "
          "remote_free_foreign_candidate=%zu "
          "remote_free_status_committed=%zu "
          "remote_free_status_backpressure=%zu "
          "remote_free_status_stale=%zu "
          "remote_free_status_integrity_failure=%zu "
          "remote_free_status_dispatch_transfer=%zu "
          "transfer_reserve_attempt=%zu "
          "transfer_reserve_success=%zu "
          "transfer_reserve_full=%zu "
          "transfer_reserve_full_after_state_mutation=%zu "
          "route_rehome_commit_enter=%zu "
          "route_rehome_commit_success=%zu "
          "remote_free_backpressure=%zu "
          "remote_free_returned_uncommitted=%zu "
          "remote_free_returned_backpressure=%zu "
          "remote_free_returned_stale=%zu "
          "remote_free_returned_integrity_failure=%zu "
          "preload_malloc_transfer_retry_eligible=%zu "
          "preload_malloc_transfer_retry_stride_skip=%zu "
          "preload_malloc_transfer_retry_attempt=%zu "
          "preload_malloc_transfer_retry_hit=%zu "
          "remote_pending_enqueue_attempt=%zu "
          "remote_pending_enqueue_success=%zu "
          "remote_pending_enqueue_full=%zu "
          "remote_pending_duplicate_claim=%zu "
          "remote_pending_owner_dead=%zu "
          "remote_pending_publish_fail=%zu "
          "remote_pending_current=%zu "
          "remote_pending_queued_current=%zu "
          "remote_pending_claimed_current=%zu "
          "remote_pending_total_current=%zu "
          "remote_pending_high_water=%zu "
          "remote_pending_maintenance_check=%zu "
          "remote_pending_maintenance_armed=%zu "
          "remote_pending_maintenance_noop=%zu "
          "remote_pending_maintenance_key_race=%zu "
          "remote_pending_maintenance_external_miss=%zu "
          "remote_pending_maintenance_inline_empty=%zu "
          "remote_pending_maintenance_frontcache_full_stop=%zu "
          "remote_pending_maintenance_entry_gate_miss=%zu "
          "remote_pending_maintenance_inline_gate_hit=%zu "
          "remote_pending_maintenance_external_gate_hit=%zu "
          "remote_pending_maintenance_external_attempt=%zu "
          "remote_pending_maintenance_external_success=%zu "
          "remote_pending_maintenance_external_only_call=%zu "
          "remote_pending_maintenance_inline_deferred=%zu "
          "remote_pending_maintenance_inline_pop_attempt=%zu "
          "remote_pending_maintenance_inline_pop_success=%zu "
          "remote_pending_maintenance_route_validate_inline=%zu "
          "remote_pending_maintenance_route_validate_external=%zu "
          "remote_pending_maintenance_frontcache_push_attempt_inline=%zu "
          "remote_pending_maintenance_frontcache_push_attempt_external=%zu "
          "remote_pending_maintenance_frontcache_push_success_inline=%zu "
          "remote_pending_maintenance_frontcache_push_success_external=%zu "
          "remote_pending_maintenance_drained_inline=%zu "
          "remote_pending_maintenance_drained_external=%zu "
          "remote_pending_external_key_probe=%zu "
          "remote_pending_external_key_hit=%zu "
          "remote_pending_batch_call=%zu "
          "remote_pending_batch_items=%zu "
          "remote_pending_frontcache_push=%zu "
          "remote_pending_frontcache_full=%zu "
          "remote_pending_generation_mismatch=%zu "
          "remote_pending_owner_mismatch=%zu "
          "remote_pending_route_mismatch=%zu "
          "remote_pending_state_mismatch=%zu "
          "remote_free_origin_pending_commit=%zu "
          "remote_free_pending_no_rehome=%zu "
          "remote_free_pending_publish_fail=%zu "
          "remote_pending_owner_inbox_storage_ineligible=%zu "
          "remote_pending_owner_inbox_descriptor_mismatch=%zu "
          "remote_pending_owner_inbox_owner_dead=%zu "
          "remote_pending_owner_inbox_owner_mismatch=%zu "
          "remote_pending_owner_inbox_enqueue_fail=%zu "
          "remote_pending_external_ticket_attempt=%zu "
          "remote_pending_external_ticket_success=%zu "
          "remote_pending_external_ticket_full=%zu "
          "remote_pending_external_ticket_duplicate=%zu "
          "remote_pending_external_ticket_duplicate_probe_total=%zu "
          "remote_pending_external_ticket_duplicate_probe_max=%zu "
          "remote_pending_external_ticket_duplicate_index_stale=%zu "
          "remote_pending_external_ticket_duplicate_scan_skip=%zu "
          "remote_pending_external_ticket_locked_revalidate_fail=%zu "
          "remote_pending_external_ticket_consume=%zu "
          "remote_pending_external_ticket_current=%zu "
          "remote_pending_external_ticket_high_water=%zu "
          "remote_pending_external_ticket_consume_empty=%zu "
          "remote_pending_external_ticket_frontcache_full=%zu "
          "remote_pending_external_ticket_route_mismatch=%zu "
          "remote_pending_external_ticket_owner_mismatch=%zu "
          "remote_pending_external_ticket_state_mismatch=%zu "
          "remote_pending_external_ticket_storage_mismatch=%zu "
          "remote_pending_external_ticket_integrity_abort=%zu "
          "remote_pending_inline_accounting_mismatch=%zu "
          "remote_pending_external_accounting_mismatch=%zu "
          "remote_pending_total_state_count_mismatch=%zu "
          "remote_pending_external_free_list_corruption=%zu "
          "remote_pending_external_list_cycle=%zu "
          "remote_pending_external_ticket_multiple_list_membership=%zu "
          "remote_pending_claimed_current_at_quiescence=%zu "
          "remote_pending_external_claimed_at_quiescence=%zu "
          "remote_pending_destroy_inline_closeout=%zu "
          "remote_pending_destroy_external_closeout=%zu "
          "remote_pending_destroy_external_release_fail=%zu "
          "remote_pending_enqueue_after_owner_dying=%zu "
          "remote_pending_ticket_to_dead_owner=%zu "
          "remote_pending_ticket_to_dead_storage_owner=%zu "
          "remote_pending_route_removed_while_pending=%zu "
          "remote_pending_after_allocator_destroy=%zu "
          "remote_pending_key_nonempty_load=%zu "
          "remote_pending_key_nonempty_hit=%zu "
          "pending_same_key_on_frontcache_miss=%zu "
          "pending_same_key_on_front_dispatch=%zu "
          "pending_same_key_on_source_alloc=%zu "
          "source_alloc_with_matching_pending=%zu "
          "pending_same_key_before_maintenance=%zu "
          "pending_same_key_after_maintenance=%zu "
          "pending_maintenance_immediate_reuse_success=%zu "
          "pending_maintenance_batch_surplus=%zu "
          "pending_same_key_on_prefill_attempt=%zu "
          "prefill_commit_with_matching_pending=%zu "
          "prefill_commit_with_inline_pending=%zu "
          "prefill_commit_with_external_pending=%zu "
          "source_block_commit_with_matching_pending=%zu "
          "source_block_commit_with_inline_pending=%zu "
          "source_block_commit_with_external_pending=%zu "
          "direct_source_attempt_with_matching_pending=%zu "
          "direct_source_commit_with_matching_pending=%zu "
          "remote_pending_direct_gate_load=%zu "
          "remote_pending_direct_gate_hit=%zu "
          "remote_pending_direct_claim_attempt=%zu "
          "remote_pending_direct_claim_success=%zu "
          "remote_pending_direct_claim_busy=%zu "
          "remote_pending_direct_claim_empty_after_hint=%zu "
          "remote_pending_direct_activate_success=%zu "
          "remote_pending_direct_integrity_failure=%zu "
          "remote_pending_direct_source_boundary_attempt=%zu "
          "remote_pending_direct_source_boundary_gate_hit=%zu "
          "remote_pending_direct_source_boundary_claim_success=%zu "
          "remote_pending_direct_prefill_avoided=%zu "
          "remote_pending_direct_source_alloc_avoided=%zu "
          "remote_pending_direct_claim_before_existing_reuse=%zu "
          "remote_pending_direct_claim_while_frontcache_nonempty=%zu "
          "remote_pending_direct_claim_while_transfer_nonempty=%zu "
          "remote_pending_direct_claim_success_toy=%zu "
          "remote_pending_direct_claim_success_midpage=%zu "
          "remote_pending_direct_claim_success_transfer_nonempty=%zu "
          "remote_pending_direct_claim_success_transfer_toy=%zu "
          "remote_pending_direct_claim_success_transfer_midpage=%zu "
          "remote_free_backpressure_origin_transfer_success=%zu "
          "remote_free_backpressure_origin_transfer_fail=%zu "
          "remote_free_backpressure_origin_transfer_stride_skip=%zu "
          "remote_free_backpressure_origin_transfer_validation_fail=%zu "
          "remote_free_backpressure_origin_transfer_full=%zu "
          "remote_free_backpressure_origin_full_transfer_count_total=%zu "
          "remote_free_backpressure_origin_full_class_count_total=%zu "
          "remote_free_backpressure_origin_full_class_count_max=%zu "
          "remote_free_backpressure_origin_drain_attempt=%zu "
          "remote_free_backpressure_origin_drain_success=%zu "
          "remote_free_backpressure_origin_drain_retry_success=%zu "
          "remote_free_backpressure_drain_attempt=%zu "
          "remote_free_backpressure_drain_success=%zu "
          "remote_free_backpressure_retry_success=%zu "
          "remote_free_backpressure_requeue_fail=%zu "
          "transfer_reserve_full_transfer_count_total=%zu "
          "transfer_reserve_full_class_count_total=%zu "
          "transfer_reserve_full_class_count_max=%zu "
          "remote_free_overflow_reserve_success=%zu "
          "remote_free_overflow_reserve_full=%zu "
          "remote_free_overflow_pop=%zu "
          "transfer_reuse_rehome_attempt=%zu "
          "transfer_reuse_rehome_success=%zu "
          "transfer_reuse_rehome_fail=%zu "
          "route_active_current=%zu route_active_max=%zu "
          "route_tombstone_current=%zu route_tombstone_max=%zu "
          "route_register_used_tombstone=%zu "
          "route_register_full_probe_with_tombstone=%zu "
          "route_tombstone_compact_attempt=%zu "
          "route_tombstone_compact_success=%zu "
          "route_tombstone_compact_fail_alloc=%zu "
          "route_tombstone_compact_moved=%zu "
          "route_compact_deferred=%zu "
          "route_compact_owner_maintenance=%zu "
          "route_compact_remote_path_attempt=%zu "
          "route_lock_read_contended=%zu "
          "route_lock_write_contended=%zu "
          "route_lock_max_wait=%zu "
          "route_tombstone_cond_probe=%zu "
          "route_tombstone_cond_would_compact=%zu "
          "route_tombstone_cond_ratio25=%zu "
          "route_tombstone_cond_occupancy75=%zu "
          "route_tombstone_cond_cooldown_blocked=%zu "
          "route_tombstone_cond_highwater=%zu "
          "shared_dir_tombstone_current=%zu "
          "shared_dir_tombstone_max=%zu "
          "shared_dir_register_used_tombstone=%zu "
          "shared_dir_maintenance_attempt=%zu "
          "shared_dir_maintenance_success=%zu "
          "shared_dir_maintenance_cleared=%zu\n",
          route_register_reason_unknown,
          route_register_reason_source_run_slot,
          route_register_reason_direct_source,
          route_register_reason_materialize, route_register_reason_rehome,
          route_unregister_reason_unknown,
          route_unregister_reason_frontcache_overflow,
          route_unregister_reason_cap_release,
          route_unregister_reason_descriptorless_detach,
          route_unregister_reason_source_slot_release,
          route_unregister_reason_rehome, route_rehome_attempt,
          route_rehome_success, route_rehome_fail,
          route_rehome_expected_owner_mismatch,
          route_rehome_destination_route_fail,
          route_rehome_directory_transfer_fail,
          route_rehome_directory_transfer_probe_total,
          route_rehome_directory_transfer_probe_max,
          route_rehome_origin_lookup_probe_total,
          route_rehome_origin_lookup_probe_max,
          route_rehome_destination_register_probe_total,
          route_rehome_destination_register_probe_max,
          route_rehome_origin_unregister_probe_total,
          route_rehome_origin_unregister_probe_max,
          route_rehome_rollback_success, route_rehome_rollback_fail,
          remote_free_foreign_candidate, remote_free_status_committed,
          remote_free_status_backpressure, remote_free_status_stale,
          remote_free_status_integrity_failure,
          remote_free_status_dispatch_transfer, transfer_reserve_attempt,
          transfer_reserve_success, transfer_reserve_full,
          transfer_reserve_full_after_state_mutation,
          route_rehome_commit_enter, route_rehome_commit_success,
          remote_free_backpressure, remote_free_returned_uncommitted,
          remote_free_returned_backpressure, remote_free_returned_stale,
          remote_free_returned_integrity_failure,
          preload_malloc_transfer_retry_eligible,
          preload_malloc_transfer_retry_stride_skip,
          preload_malloc_transfer_retry_attempt,
          preload_malloc_transfer_retry_hit,
          remote_pending_enqueue_attempt,
          remote_pending_enqueue_success,
          remote_pending_enqueue_full,
          remote_pending_duplicate_claim,
          remote_pending_owner_dead,
          remote_pending_publish_fail,
          remote_pending_current,
          remote_pending_queued_current,
          remote_pending_claimed_current,
          remote_pending_total_current,
          remote_pending_high_water,
          remote_pending_maintenance_check,
          remote_pending_maintenance_armed,
          remote_pending_maintenance_noop,
          remote_pending_maintenance_key_race,
          remote_pending_maintenance_external_miss,
          remote_pending_maintenance_inline_empty,
          remote_pending_maintenance_frontcache_full_stop,
          remote_pending_maintenance_entry_gate_miss,
          remote_pending_maintenance_inline_gate_hit,
          remote_pending_maintenance_external_gate_hit,
          remote_pending_maintenance_external_attempt,
          remote_pending_maintenance_external_success,
          remote_pending_maintenance_external_only_call,
          remote_pending_maintenance_inline_deferred,
          remote_pending_maintenance_inline_pop_attempt,
          remote_pending_maintenance_inline_pop_success,
          remote_pending_maintenance_route_validate_inline,
          remote_pending_maintenance_route_validate_external,
          remote_pending_maintenance_frontcache_push_attempt_inline,
          remote_pending_maintenance_frontcache_push_attempt_external,
          remote_pending_maintenance_frontcache_push_success_inline,
          remote_pending_maintenance_frontcache_push_success_external,
          remote_pending_maintenance_drained_inline,
          remote_pending_maintenance_drained_external,
          remote_pending_external_key_probe,
          remote_pending_external_key_hit,
          remote_pending_batch_call,
          remote_pending_batch_items,
          remote_pending_frontcache_push,
          remote_pending_frontcache_full,
          remote_pending_generation_mismatch,
          remote_pending_owner_mismatch,
          remote_pending_route_mismatch,
          remote_pending_state_mismatch,
          remote_free_origin_pending_commit,
          remote_free_pending_no_rehome,
          remote_free_pending_publish_fail,
          remote_pending_owner_inbox_storage_ineligible,
          remote_pending_owner_inbox_descriptor_mismatch,
          remote_pending_owner_inbox_owner_dead,
          remote_pending_owner_inbox_owner_mismatch,
          remote_pending_owner_inbox_enqueue_fail,
          remote_pending_external_ticket_attempt,
          remote_pending_external_ticket_success,
          remote_pending_external_ticket_full,
          remote_pending_external_ticket_duplicate,
          remote_pending_external_ticket_duplicate_probe_total,
          remote_pending_external_ticket_duplicate_probe_max,
          remote_pending_external_ticket_duplicate_index_stale,
          remote_pending_external_ticket_duplicate_scan_skip,
          remote_pending_external_ticket_locked_revalidate_fail,
          remote_pending_external_ticket_consume,
          remote_pending_external_ticket_current,
          remote_pending_external_ticket_high_water,
          remote_pending_external_ticket_consume_empty,
          remote_pending_external_ticket_frontcache_full,
          remote_pending_external_ticket_route_mismatch,
          remote_pending_external_ticket_owner_mismatch,
          remote_pending_external_ticket_state_mismatch,
          remote_pending_external_ticket_storage_mismatch,
          remote_pending_external_ticket_integrity_abort,
          remote_pending_inline_accounting_mismatch,
          remote_pending_external_accounting_mismatch,
          remote_pending_total_state_count_mismatch,
          remote_pending_external_free_list_corruption,
          remote_pending_external_list_cycle,
          remote_pending_external_ticket_multiple_list_membership,
          remote_pending_claimed_current_at_quiescence,
          remote_pending_external_claimed_at_quiescence,
          remote_pending_destroy_inline_closeout,
          remote_pending_destroy_external_closeout,
          remote_pending_destroy_external_release_fail,
          remote_pending_enqueue_after_owner_dying,
          remote_pending_ticket_to_dead_owner,
          remote_pending_ticket_to_dead_storage_owner,
          remote_pending_route_removed_while_pending,
          remote_pending_after_allocator_destroy,
          remote_pending_key_nonempty_load,
          remote_pending_key_nonempty_hit,
          pending_same_key_on_frontcache_miss,
          pending_same_key_on_front_dispatch,
          pending_same_key_on_source_alloc,
          source_alloc_with_matching_pending,
          pending_same_key_before_maintenance,
          pending_same_key_after_maintenance,
          pending_maintenance_immediate_reuse_success,
          pending_maintenance_batch_surplus,
          pending_same_key_on_prefill_attempt,
          prefill_commit_with_matching_pending,
          prefill_commit_with_inline_pending,
          prefill_commit_with_external_pending,
          source_block_commit_with_matching_pending,
          source_block_commit_with_inline_pending,
          source_block_commit_with_external_pending,
          direct_source_attempt_with_matching_pending,
          direct_source_commit_with_matching_pending,
          remote_pending_direct_gate_load,
          remote_pending_direct_gate_hit,
          remote_pending_direct_claim_attempt,
          remote_pending_direct_claim_success,
          remote_pending_direct_claim_busy,
          remote_pending_direct_claim_empty_after_hint,
          remote_pending_direct_activate_success,
          remote_pending_direct_integrity_failure,
          remote_pending_direct_source_boundary_attempt,
          remote_pending_direct_source_boundary_gate_hit,
          remote_pending_direct_source_boundary_claim_success,
          remote_pending_direct_prefill_avoided,
          remote_pending_direct_source_alloc_avoided,
          remote_pending_direct_claim_before_existing_reuse,
          remote_pending_direct_claim_while_frontcache_nonempty,
          remote_pending_direct_claim_while_transfer_nonempty,
          remote_pending_direct_claim_success_toy,
          remote_pending_direct_claim_success_midpage,
          remote_pending_direct_claim_success_while_transfer_nonempty,
          remote_pending_direct_claim_success_transfer_toy,
          remote_pending_direct_claim_success_transfer_midpage,
          remote_free_backpressure_origin_transfer_success,
          remote_free_backpressure_origin_transfer_fail,
          remote_free_backpressure_origin_transfer_stride_skip,
          remote_free_backpressure_origin_transfer_validation_fail,
          remote_free_backpressure_origin_transfer_full,
          remote_free_backpressure_origin_full_transfer_count_total,
          remote_free_backpressure_origin_full_class_count_total,
          remote_free_backpressure_origin_full_class_count_max,
          remote_free_backpressure_origin_drain_attempt,
          remote_free_backpressure_origin_drain_success,
          remote_free_backpressure_origin_drain_retry_success,
          remote_free_backpressure_drain_attempt,
          remote_free_backpressure_drain_success,
          remote_free_backpressure_retry_success,
          remote_free_backpressure_requeue_fail,
          transfer_reserve_full_transfer_count_total,
          transfer_reserve_full_class_count_total,
          transfer_reserve_full_class_count_max,
          remote_free_overflow_reserve_success,
          remote_free_overflow_reserve_full,
          remote_free_overflow_pop,
          transfer_reuse_rehome_attempt,
          transfer_reuse_rehome_success,
          transfer_reuse_rehome_fail,
          route_active_current,
          route_active_max, route_tombstone_current, route_tombstone_max,
          route_register_used_tombstone,
          route_register_full_probe_with_tombstone,
          route_tombstone_compact_attempt, route_tombstone_compact_success,
          route_tombstone_compact_fail_alloc,
          route_tombstone_compact_moved, route_compact_deferred,
          route_compact_owner_maintenance, route_compact_remote_path_attempt,
          route_lock_read_contended, route_lock_write_contended,
          route_lock_max_wait,
          route_tombstone_cond_probe,
          route_tombstone_cond_would_compact,
          route_tombstone_cond_ratio25, route_tombstone_cond_occupancy75,
          route_tombstone_cond_cooldown_blocked,
          route_tombstone_cond_highwater,
          shared_dir_tombstone_current, shared_dir_tombstone_max,
          shared_dir_register_used_tombstone,
          shared_dir_maintenance_attempt, shared_dir_maintenance_success,
          shared_dir_maintenance_cleared);

  fprintf(stderr,
          "[HZ6_PRELOAD_RESOLVE_DETAIL] "
          "free_resolve_attempt=%zu "
          "free_resolve_shared_probe=%zu "
          "free_resolve_shared_valid=%zu "
          "free_resolve_shared_miss=%zu "
          "free_resolve_shared_retry=%zu "
          "free_resolve_shared_stale=%zu "
          "free_resolve_local_lookup=%zu "
          "free_resolve_visible_shadow=%zu "
          "free_resolve_visible_hit=%zu "
          "free_resolve_result_local_valid=%zu "
          "free_resolve_result_foreign_valid=%zu "
          "free_resolve_result_owned_invalid=%zu "
          "free_resolve_result_proven_external=%zu "
          "free_resolve_result_retry=%zu "
          "free_resolve_result_unresolved_integrity=%zu\n",
          free_resolve_attempt, free_resolve_shared_probe,
          free_resolve_shared_valid, free_resolve_shared_miss,
          free_resolve_shared_retry, free_resolve_shared_stale,
          free_resolve_local_lookup, free_resolve_visible_shadow,
          free_resolve_visible_hit, free_resolve_result_local_valid,
          free_resolve_result_foreign_valid,
          free_resolve_result_owned_invalid,
          free_resolve_result_proven_external, free_resolve_result_retry,
          free_resolve_result_unresolved_integrity);

  fprintf(stderr,
          "[HZ6_PRELOAD_FRONT_DETAIL] descriptor_live_max=%zu "
          "source_block_active_max=%zu frontcache_total_max=%zu "
          "frontcache_reuse_hit=%zu frontcache_reuse_invalid=%zu "
          "frontcache_spill_success=%zu "
          "frontcache_spill_retry_success=%zu "
          "frontcache_borrow_success=%zu "
          "toy_small_malloc_frontcache_pop=%zu "
          "toy_small_malloc_activate_success=%zu "
          "toy_class4_malloc_fast_attempt=%zu "
          "toy_class4_malloc_fast_hit=%zu "
          "toy_class4_malloc_front_dispatch=%zu "
          "toy_class4_malloc_frontcache_pop=%zu "
          "toy_class4_malloc_activate_success=%zu "
          "toy_small_free_cache_attempt=%zu "
          "toy_small_free_cache_success=%zu "
          "toy_class4_free_cache_attempt=%zu "
          "toy_class4_free_cache_success=%zu "
          "toy_small_active_map_register=%zu "
          "toy_small_active_map_register_collision=%zu "
          "toy_class4_active_map_register=%zu "
          "toy_class4_active_map_register_collision=%zu "
          "toy_small_active_map_free_attempt=%zu "
          "toy_small_active_map_free_hit=%zu "
          "toy_small_active_map_free_hit_base_slot=%zu "
          "toy_small_active_map_free_hit_probe_total=%zu "
          "toy_small_active_map_free_hit_probe_max=%zu "
          "toy_small_active_map_free_miss=%zu "
          "toy_small_active_map_free_stale=%zu "
          "toy_small_active_map_free_cache_fail=%zu "
          "toy_small_active_map_route_bypass=%zu "
          "toy_class4_active_map_free_hit=%zu "
          "toy_class4_active_map_free_hit_base_slot=%zu "
          "toy_class4_active_map_free_hit_probe_total=%zu "
          "toy_class4_active_map_free_hit_probe_max=%zu "
          "midpage_active_map_register=%zu "
          "midpage_active_map_register_direct=%zu "
          "midpage_active_map_register_front_alloc=%zu "
          "midpage_active_map_register_route_fallback=%zu "
          "midpage_active_map_register_collision=%zu "
          "midpage_active_map_register_empty_slot=%zu "
          "midpage_active_map_register_same_ptr=%zu "
          "midpage_active_map_register_base_slot=%zu "
          "midpage_active_map_register_probe_total=%zu "
          "midpage_active_map_register_probe_max=%zu "
          "midpage_active_map_register_overwrite=%zu "
          "midpage_8k_active_map_register_overwrite=%zu "
          "midpage_32k_active_map_register_overwrite=%zu "
          "midpage_active_map_register_overwrite_same_class_alt=%zu "
          "midpage_active_map_register_overwrite_stale_alt=%zu "
          "midpage_active_map_free_attempt=%zu "
          "midpage_active_map_free_hit=%zu "
          "midpage_active_map_free_hit_base_slot=%zu "
          "midpage_active_map_free_hit_probe_total=%zu "
          "midpage_active_map_free_hit_probe_max=%zu "
          "midpage_active_map_free_miss=%zu "
          "midpage_active_map_free_miss_probe_empty=%zu "
          "midpage_active_map_free_miss_probe_occupied=%zu "
          "midpage_active_map_free_miss_found_elsewhere=%zu "
          "midpage_8k_active_map_free_miss_found_elsewhere=%zu "
          "midpage_32k_active_map_free_miss_found_elsewhere=%zu "
          "midpage_active_map_free_stale=%zu "
          "midpage_active_map_free_cache_attempt=%zu "
          "midpage_active_map_free_cache_success=%zu "
          "midpage_active_map_free_cache_fail=%zu "
          "midpage_active_map_alignment_skip=%zu "
          "midpage_active_map_addr_envelope_skip=%zu "
          "midpage_active_map_route_bypass=%zu\n",
          descriptor_live_max, source_block_active_max,
          frontcache_total_max, frontcache_reuse_hit,
          frontcache_reuse_invalid, frontcache_spill_success,
          frontcache_spill_retry_success, frontcache_borrow_success,
          toy_small_malloc_frontcache_pop,
          toy_small_malloc_activate_success,
          toy_class4_malloc_fast_attempt,
          toy_class4_malloc_fast_hit,
          toy_class4_malloc_front_dispatch,
          toy_class4_malloc_frontcache_pop,
          toy_class4_malloc_activate_success,
          toy_small_free_cache_attempt, toy_small_free_cache_success,
          toy_class4_free_cache_attempt, toy_class4_free_cache_success,
          toy_small_active_map_register,
          toy_small_active_map_register_collision,
          toy_class4_active_map_register,
          toy_class4_active_map_register_collision,
          toy_small_active_map_free_attempt,
          toy_small_active_map_free_hit,
          toy_small_active_map_free_hit_base_slot,
          toy_small_active_map_free_hit_probe_total,
          toy_small_active_map_free_hit_probe_max,
          toy_small_active_map_free_miss,
          toy_small_active_map_free_stale,
          toy_small_active_map_free_cache_fail,
          toy_small_active_map_route_bypass,
          toy_class4_active_map_free_hit,
          toy_class4_active_map_free_hit_base_slot,
          toy_class4_active_map_free_hit_probe_total,
          toy_class4_active_map_free_hit_probe_max,
          midpage_active_map_register,
          midpage_active_map_register_direct,
          midpage_active_map_register_front_alloc,
          midpage_active_map_register_route_fallback,
          midpage_active_map_register_collision,
          midpage_active_map_register_empty_slot,
          midpage_active_map_register_same_ptr,
          midpage_active_map_register_base_slot,
          midpage_active_map_register_probe_total,
          midpage_active_map_register_probe_max,
          midpage_active_map_register_overwrite,
          midpage_8k_active_map_register_overwrite,
          midpage_32k_active_map_register_overwrite,
          midpage_active_map_register_overwrite_same_class_alt,
          midpage_active_map_register_overwrite_stale_alt,
          midpage_active_map_free_attempt,
          midpage_active_map_free_hit,
          midpage_active_map_free_hit_base_slot,
          midpage_active_map_free_hit_probe_total,
          midpage_active_map_free_hit_probe_max,
          midpage_active_map_free_miss,
          midpage_active_map_free_miss_probe_empty,
          midpage_active_map_free_miss_probe_occupied,
          midpage_active_map_free_miss_found_elsewhere,
          midpage_8k_active_map_free_miss_found_elsewhere,
          midpage_32k_active_map_free_miss_found_elsewhere,
          midpage_active_map_free_stale,
          midpage_active_map_free_cache_attempt,
          midpage_active_map_free_cache_success,
          midpage_active_map_free_cache_fail,
          midpage_active_map_alignment_skip,
          midpage_active_map_addr_envelope_skip,
          midpage_active_map_route_bypass);

  fprintf(stderr,
          "[HZ6_PRELOAD_PAGE_KIND_DETAIL] "
          "free_page_kind_selector_probe=%zu "
          "free_page_kind_selector_unknown=%zu "
          "free_page_kind_selector_toy=%zu "
          "free_page_kind_selector_midpage=%zu "
          "free_page_kind_selector_mixed=%zu "
          "free_page_kind_selector_toy_hit=%zu "
          "free_page_kind_selector_midpage_hit=%zu "
          "free_page_kind_selector_wrong_toy_page_mid_hit=%zu "
          "free_page_kind_selector_wrong_midpage_page_toy_hit=%zu\n",
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_probe),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_unknown),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_toy),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_midpage),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_mixed),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_toy_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(free_page_kind_selector_midpage_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_page_kind_selector_wrong_toy_page_mid_hit),
          HZ6_PRELOAD_PHASE_LOAD_FIELD(
              free_page_kind_selector_wrong_midpage_page_toy_hit));

  fprintf(stderr, "[HZ6_PRELOAD_FRONTCACHE_CLASS_DETAIL]");
  for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
    fprintf(stderr,
            " c%zu_push=%zu c%zu_empty=%zu c%zu_max=%zu",
            class_id,
            frontcache_push_by_class[class_id],
            class_id,
            frontcache_pop_empty_by_class[class_id],
            class_id,
            frontcache_bin_max_by_class[class_id]);
  }
  fprintf(stderr, "\n");

  fprintf(stderr, "[HZ6_PRELOAD_DIRECT_PENDING_CLASS_DETAIL]");
  for (size_t class_id = 0; class_id < HZ6_STATS_CLASS_COUNT; ++class_id) {
    fprintf(stderr,
            " c%zu_claim=%zu c%zu_transfer=%zu",
            class_id,
            remote_pending_direct_claim_success_by_class[class_id],
            class_id,
            remote_pending_direct_claim_success_transfer_by_class[class_id]);
  }
  fprintf(stderr, "\n");

  fprintf(stderr,
          "[HZ6_PRELOAD_MIDPAGE_CLASS_DETAIL] "
          "midpage_8k_alloc_call=%zu midpage_32k_alloc_call=%zu "
          "midpage_8k_prefill_run_call=%zu "
          "midpage_32k_prefill_run_call=%zu "
          "midpage_8k_prefill_run_filled=%zu "
          "midpage_32k_prefill_run_filled=%zu "
          "midpage_8k_active_map_register=%zu "
          "midpage_32k_active_map_register=%zu "
          "midpage_8k_active_map_free_hit=%zu "
          "midpage_32k_active_map_free_hit=%zu "
          "midpage_8k_frontcache_push=%zu "
          "midpage_32k_frontcache_push=%zu "
          "midpage_8k_frontcache_pop_empty=%zu "
          "midpage_32k_frontcache_pop_empty=%zu "
          "midpage_direct_transfer_probe_attempt=%zu "
          "midpage_direct_transfer_probe_hit=%zu "
          "midpage_direct_transfer_probe_empty=%zu "
          "midpage_8k_direct_transfer_probe_attempt=%zu "
          "midpage_32k_direct_transfer_probe_attempt=%zu "
          "midpage_8k_preload_local_route_valid=%zu "
          "midpage_32k_preload_local_route_valid=%zu "
          "midpage_8k_source_run_slot_route_register=%zu "
          "midpage_32k_source_run_slot_route_register=%zu "
          "midpage_32k_cold_retire_attempt=%zu "
          "midpage_32k_cold_retire_scan_blocks=%zu "
          "midpage_32k_cold_retire_candidate_blocks=%zu "
          "midpage_32k_cold_retire_retired_blocks=%zu "
          "midpage_32k_cold_retire_retired_descriptors=%zu "
          "midpage_32k_cold_retire_retired_bytes=%zu "
          "midpage_32k_cold_retire_frontcache_remove_fail=%zu "
          "midpage_32k_cold_retire_blocked=%zu "
          "midpage_32k_cold_purge_attempt=%zu "
          "midpage_32k_cold_purge_scan_blocks=%zu "
          "midpage_32k_cold_purge_candidate_blocks=%zu "
          "midpage_32k_cold_purge_purged_blocks=%zu "
          "midpage_32k_cold_purge_purged_bytes=%zu "
          "midpage_32k_cold_purge_fail=%zu "
          "midpage_32k_cold_purge_blocked=%zu "
          "midpage_32k_cold_purge_recommit=%zu "
          "midpage_32k_cold_purge_recommit_fail=%zu\n",
          midpage_8k_alloc_call, midpage_32k_alloc_call,
          midpage_8k_prefill_run_call, midpage_32k_prefill_run_call,
          midpage_8k_prefill_run_filled, midpage_32k_prefill_run_filled,
          midpage_8k_active_map_register, midpage_32k_active_map_register,
          midpage_8k_active_map_free_hit, midpage_32k_active_map_free_hit,
          midpage_8k_frontcache_push, midpage_32k_frontcache_push,
          midpage_8k_frontcache_pop_empty, midpage_32k_frontcache_pop_empty,
          midpage_direct_transfer_probe_attempt,
          midpage_direct_transfer_probe_hit,
          midpage_direct_transfer_probe_empty,
          midpage_8k_direct_transfer_probe_attempt,
          midpage_32k_direct_transfer_probe_attempt,
          midpage_8k_preload_local_route_valid,
          midpage_32k_preload_local_route_valid,
          midpage_8k_source_run_slot_route_register,
          midpage_32k_source_run_slot_route_register,
          midpage_32k_cold_retire_attempt,
          midpage_32k_cold_retire_scan_blocks,
          midpage_32k_cold_retire_candidate_blocks,
          midpage_32k_cold_retire_retired_blocks,
          midpage_32k_cold_retire_retired_descriptors,
          midpage_32k_cold_retire_retired_bytes,
          midpage_32k_cold_retire_frontcache_remove_fail,
          midpage_32k_cold_retire_blocked,
          midpage_32k_cold_purge_attempt,
          midpage_32k_cold_purge_scan_blocks,
          midpage_32k_cold_purge_candidate_blocks,
          midpage_32k_cold_purge_purged_blocks,
          midpage_32k_cold_purge_purged_bytes,
          midpage_32k_cold_purge_fail,
          midpage_32k_cold_purge_blocked,
          midpage_32k_cold_purge_recommit,
          midpage_32k_cold_purge_recommit_fail);

  fprintf(stderr,
          "[HZ6_PRELOAD_RUNMETA_DETAIL] "
          "smallrun_route_attempt=%zu "
          "smallrun_range_hit=%zu "
          "smallrun_active_slot_hit=%zu "
          "smallrun_descriptor_match=%zu "
          "smallrun_generation_match=%zu "
          "smallrun_would_valid=%zu "
          "smallrun_would_invalid=%zu "
          "smallrun_exact_fallback_needed=%zu "
          "smallrun_false_positive=%zu "
          "source_block_route_range_index_lookup=%zu "
          "source_block_route_range_index_hit=%zu "
          "source_block_route_range_index_miss=%zu "
          "source_block_route_range_index_stale=%zu "
          "source_block_route_range_index_probe_total=%zu "
          "source_block_route_range_index_probe_max=%zu "
          "source_block_route_behavior_attempt=%zu "
          "source_block_route_behavior_valid=%zu "
          "source_block_route_behavior_fallback=%zu "
          "source_block_route_behavior_invalid_front=%zu "
          "source_block_route_behavior_invalid_descriptor=%zu\n",
          smallrun_route_attempt, smallrun_range_hit,
          smallrun_active_slot_hit, smallrun_descriptor_match,
          smallrun_generation_match, smallrun_would_valid,
          smallrun_would_invalid, smallrun_exact_fallback_needed,
          smallrun_false_positive, source_block_route_range_index_lookup,
          source_block_route_range_index_hit,
          source_block_route_range_index_miss,
          source_block_route_range_index_stale,
          source_block_route_range_index_probe_total,
          source_block_route_range_index_probe_max,
          source_block_route_behavior_attempt,
          source_block_route_behavior_valid,
          source_block_route_behavior_fallback,
          source_block_route_behavior_invalid_front,
          source_block_route_behavior_invalid_descriptor);
